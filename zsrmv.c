#include <linux/cdev.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/syscalls.h>
#include <linux/signal.h>

#include <linux/clocksource.h>
#include <linux/timekeeping.h>

#include "zsrmv.h"

#include "zsrmvapi.h"

/*********************************************************************/
//-- variables to model time
/*********************************************************************/

//-- ghost variable: the current time
unsigned long long stac_now = 0;


/**
 * Variables to measure overhead
 */

unsigned long long context_switch_start_timestamp_ticks=0L;
unsigned long long context_switch_end_timestamp_ticks=0L;
unsigned long long cumm_context_switch_ticks=0L;
unsigned long long num_context_switches = 0L;

unsigned long long enforcement_start_timestamp_ticks=0L;
unsigned long long enforcement_end_timestamp_ticks=0L;
unsigned long long cumm_enforcement_ticks = 0L;
unsigned long long num_enforcements = 0L;

unsigned long long zs_enforcement_start_timestamp_ticks=0L;
unsigned long long zs_enforcement_end_timestamp_ticks=0L;
unsigned long long cumm_zs_enforcement_ticks =0L;
unsigned long long num_zs_enforcements=0L;

unsigned long long arrival_start_timestamp_ticks=0L;
unsigned long long arrival_end_timestamp_ticks=0L;
unsigned long long cumm_arrival_ticks = 0L;
unsigned long long num_arrivals = 0L;

unsigned long long cumm_blocked_arrival_ticks=0L;
unsigned long long num_blocked_arrivals=0L;

unsigned long long departure_start_timestamp_ticks=0L;
unsigned long long departure_end_timestamp_ticks=0L;
unsigned long long cumm_departure_ticks=0L;
unsigned long long num_departures = 0L;

/**
 * Trace structures
 */
#define TRACE_BUFFER_SIZE 100000
struct trace_rec_t trace_table[TRACE_BUFFER_SIZE];
int trace_index=0;



/*********************************************************************/
//-- variables and functions to model preemption
/*********************************************************************/

/*********************************************************************/
//-- ZSRM specific data structures
/*********************************************************************/

/**
 * DEBUG configurations
 */

//#define __ZS_DEBUG__ 1

// DEBUG VARIABLE

int calling_start_from=0;
#define NAME_START_FROM(n) (n == 1 ? "start_of_period" : \
			     n == 2 ? "attach_reserve " : \
			     n == 3 ? "wait_period" : \
			     n == 4 ? "scheduler_task" : \
			     "unknown")
int prev_calling_stop_from=0;
int calling_stop_from=0;
#define NAME_STOP_FROM(n) ( n == 1 ? "start_of_period" : \
			    n == 2 ? "delete_reserve"  : \
			    n == 3 ? "wait_period"     : \
			    n == 4 ? "scheduler_task"  : \
			    "unknown")


#define TIMER_ZS  1
#define TIMER_ENF 2
#define TIMER_ZS_ENF 3
#define TIMER_PERIOD 4

// OTHER LOCKING SITUATIONS
#define SCHED_TASK 5
#define ZSV_CALL   6

#define STRING_TIMER_TYPE(t) ( t == TIMER_ENF ? "timer_enf" :\
			       t == TIMER_PERIOD ? "timer_period" :\
			       t == TIMER_ZS_ENF ? "timer_zs_enf" : \
			       "unknown")

int prevlocker=0;
int zsrmcall=-1;

#define STRING_LOCKER(t) ( t == TIMER_ENF ? "timer_enf" :\
			   t == TIMER_PERIOD ? "timer_period" :	    \
			   t == TIMER_ZS_ENF ? "timer_zs_enf" :	    \
			   t == SCHED_TASK   ? "sched_task" : \
			   t == ZSV_CALL     ? "zsv_call"  :\
			   t == 0            ? "none" : \
			   "unknown")


#define MAX_RESERVES 100

#define MIN_PRIORITY 50

#define DAEMON_PRIORITY (MIN_PRIORITY + 40)

struct task_struct *sched_task;
struct task_struct *active_task;

/**
 *   MOVED TO zsrmv.h to improve modularity
 */

/* struct zs_timer{ */
/*   timer_t tid; */
/*   int rid; */
/*   int timer_type; */
/*   struct timespec expiration; */
/*   unsigned long long absolute_expiration_ns; */
/*   struct hrtimer kernel_timer; */
/*   struct zs_timer *next; */

/* #ifdef STAC_FRAMAC_STUBS */
/*   //-- ghost variable to indicate whether the timer is armed and the */
/*   //-- time (relative to point of arming) when it will go off */
/*   int stac_armed; */
/*   unsigned long long stac_expiration_ns; */
/*   enum hrtimer_restart (*stac_handler)(struct hrtimer *timer); */
/* #endif */
/* }; */

/* struct reserve { */
/*   struct pid_namespace *task_namespace; */
/*   pid_t  pid; */
/*   int rid; */
/*   unsigned long long start_ns; */
/*   unsigned long long stop_ns; */
/*   unsigned long long current_exectime_ns; */
/*   unsigned long long exectime_ns; */
/*   unsigned long long nominal_exectime_ns; */

/*   unsigned long long start_ticks; */
/*   unsigned long long stop_ticks; */
/*   unsigned long long current_exectime_ticks; */
/*   unsigned long long exectime_ticks; */
/*   unsigned long long nominal_exectime_ticks; */
/*   unsigned long long worst_exectime_ticks; */
/*   unsigned long long avg_exectime_ticks; */
/*   unsigned long avg_exectime_ticks_measurements; */

/*   unsigned int num_period_wakeups; */

/*   unsigned long num_enforcements; */
/*   unsigned long num_wfnp; */
/*   unsigned long num_wait_release; */
/*   unsigned int non_periodic_wait; */
/*   unsigned int end_of_period_marked; */

/* #ifdef STAC_FRAMAC_STUBS */
/*   //-- ghost variables we use to model the real execution time of a */
/*   //-- job and start time of last job fragment */
/*   unsigned long long real_exectime_ns; */
/*   unsigned long long real_start_ns; */
/* #endif */

/*   int enforcement_type; */
/*   struct timespec period; */
/*   struct timespec zsinstant; */
/*   unsigned long long period_ns; */
/*   unsigned long long period_ticks; */
/*   struct timespec execution_time; */
/*   struct timespec nominal_execution_time; */
/*   int priority; */
/*   int criticality; */
/*   int in_critical_mode; */
/*   int enforced; */
/*   int bound_to_cpu; */
/*   struct zs_timer period_timer; */
/*   struct zs_timer enforcement_timer; */
/*   struct zs_timer zero_slack_timer; */
/*   struct reserve *next; */
/*   struct reserve *rm_next; */
/*   struct reserve *crit_next; */
/*   struct reserve *crit_block_next; */
/*   int request_stop; */
/*   int enforcement_signal_captured; */
/*   int enforcement_signal_receiver_pid; */
/*   int enforcement_signo; */
/*   int attached; */

/*   // Some debugging variables */
/*   int start_period; */
/* } reserve_table[MAX_RESERVES]; */

struct reserve reserve_table[MAX_RESERVES];

// only tasks with higher or equal criticality than sys_criticality are allowed
// to run
int sys_criticality = 0;

struct reserve *critical_reservesq=NULL;
struct reserve *crit_blockq=NULL;
struct reserve *readyq=NULL;
struct reserve *rm_head=NULL;
int rm_queue_size=0;

int sync_start_semid;
int ready_semid;

/*********************************************************************/
// -----FUNCTION SIGNATURES
/*********************************************************************/

unsigned long long get_now_ns(void);
unsigned long long get_now_ticks(void);
int getreserve(void);
void budget_enforcement(int rid, int request_stop);
void start_of_period(int rid);
void timer_handler(struct zs_timer *timer);
int add_timerq(struct zs_timer *t);
int attach_reserve(int rid, int pid);
int start_enforcement_timer(struct reserve *rsvp);
void start_stac(int rid);
void start(int rid);
void stop_stac(int rid);
void stop(int rid);
int wait_for_next_period(int rid, int nowait);
int calculate_rm_priorities(void);
int set_rm_priorities(void);
struct task_struct *gettask(int pid, struct pid_namespace *ns);
float compute_total_utilization(void);
int push_to_reschedule(int i);
int pop_to_reschedule(void);
void init(void);
enum hrtimer_restart kernel_timer_handler(struct hrtimer *timer);
unsigned long long ticks2ns(unsigned long long ticks);
unsigned long long ns2ticks(unsigned long long ns);
void init_reserve(int rid);
int get_acet_ns(int rid, unsigned long long *avet);
int get_wcet_ns(int rid, unsigned long long *wcet);
void reset_exectime_counters(int rid);
int delete_reserve(int rid);
int add_trace_record(int rid, unsigned long long ts, int event_type);
int in_readyq(int rid);
int push_to_activate(int i);
int pop_to_activate(void);
int end_of_period(int rid);
int wait_for_next_release(int rid);
/*********************************************************************/
// ---- END OF FUNCTION SIGNATURES
/*********************************************************************/

//@logic int maxReserves = (int)100;

/*@predicate elem(struct reserve *p) = \exists int i; (0 <= i < maxReserves && p == &(reserve_table[i]));*/
/*@predicate elemNull(struct reserve *p) = (p == \null) || elem(p);*/
/*@predicate disc(struct reserve *p) = \forall int i; 0 <= i < maxReserves ==> reserve_table[i].next != p;*/

/*@predicate elemt(struct zs_timer *p) = \exists int i; (0 <= i < maxReserves && 
  (p == &(reserve_table[i].period_timer) || p == &(reserve_table[i].enforcement_timer)));*/
/*@predicate elemtNull(struct zs_timer *p) = (p == \null) || elemt(p);*/
/*@predicate disct(struct zs_timer *p) = \forall int i; 0 <= i < maxReserves ==>
  (reserve_table[i].period_timer.next != p &&
  reserve_table[i].enforcement_timer.next != p);*/

/*@predicate fp11 = elemNull(readyq);*/
/*@predicate fp12 = elemNull(rm_head);*/
/*@predicate fp13 = \forall int i; 0 <= i < maxReserves ==>
  reserve_table[i].rid == i;*/ 
/*@predicate fp1 = fp11 && fp12 && fp13;*/

/*@predicate fp21 = \forall struct reserve *p; elem(p) ==> elemNull(p->next);*/
/*@predicate fp22 = \forall struct reserve *p; elem(p) ==> elemNull(p->rm_next);*/
/*@predicate fp23 = \forall struct zs_timer *p; elemt(p) ==> elemtNull(p->next);*/
/*@predicate fp2 = fp21 && fp22 && fp23;*/

/*@predicate fp31 = \forall struct reserve *p, *q; elem(p) ==> elem(q) ==> 
  (p->next == q) ==> (p->priority >= q->priority);*/
/*@predicate fp32 = \forall struct reserve *p, *q; elem(p) ==> elem(q) ==> 
  (p->rm_next == q) ==> (p->period_ns <= q->period_ns);*/
/*@predicate fp3 = fp31 && fp32;*/

/*@predicate fp = fp1 && fp2 && fp3;*/

///*@global invariant fpi : fp11 && fp12 && fp13 && fp21 && fp22 && fp23 && fp31 && fp32;*/

//-- ZSRM invariants

/*@predicate zsrm_lem11 = \forall int i; 0 <= i < maxReserves ==> 
  ((readyq != &(reserve_table[i]) ==> \separated(readyq,&(reserve_table[i]))) &&
  (\forall int j; 0 <= j < maxReserves ==> reserve_table[j].next != &(reserve_table[i]) ==> \separated(reserve_table[j].next,&(reserve_table[i]))));*/

/*@predicate zsrm_lem12 = \forall struct reserve *p, struct zs_timer *q; \separated(p,q);*/

/*@predicate zsrm_lem1 = zsrm_lem11 && zsrm_lem12;*/

//-- real_exec_time <= current_exec_time && start_ns <= real_start_ns
/*@predicate zsrm1 = \forall int i; 0 <= i < maxReserves ==>
  (reserve_table[i].real_exectime_ns <= reserve_table[i].current_exectime_ns && 
  reserve_table[i].start_ns <= reserve_table[i].real_start_ns);*/

//-- real_exec_time <= current_exec_time && start_ns <= real_start_ns
/*@predicate zsrm1_stop1 = \forall int i; 0 <= i < maxReserves ==>
  ((readyq != &(reserve_table[i]) ==> reserve_table[i].real_exectime_ns <= reserve_table[i].current_exectime_ns) && 
  (readyq == &(reserve_table[i]) ==> reserve_table[i].real_exectime_ns <= reserve_table[i].current_exectime_ns + stac_now - readyq->real_start_ns) && 
  reserve_table[i].start_ns <= reserve_table[i].real_start_ns);*///-- real_exec_time <= current_exec_time && start_ns <= real_start_ns

/*@predicate zsrm1_stop2 = \forall int i; 0 <= i < maxReserves ==>
  (reserve_table[i].real_exectime_ns <= reserve_table[i].current_exectime_ns && 
  (readyq != &(reserve_table[i]) ==> reserve_table[i].start_ns <= reserve_table[i].real_start_ns) &&
  (readyq == &(reserve_table[i]) ==> reserve_table[i].start_ns <= stac_now));*/

//-- real_exec_time <= current_exec_time && start_ns <= real_start_ns
/*@predicate zsrm1_start1 = \forall int i; 0 <= i < maxReserves ==>
  ((readyq != &(reserve_table[i]) ==> reserve_table[i].real_exectime_ns <= reserve_table[i].current_exectime_ns) && 
  (readyq == &(reserve_table[i]) ==> reserve_table[i].real_exectime_ns <= reserve_table[i].current_exectime_ns + stac_now - readyq->real_start_ns) && 
  reserve_table[i].start_ns <= reserve_table[i].real_start_ns);*///-- real_exec_time <= current_exec_time && start_ns <= real_start_ns

/*@predicate zsrm1_start2 = \forall int i; 0 <= i < maxReserves ==>
  (reserve_table[i].real_exectime_ns <= reserve_table[i].current_exectime_ns && 
  (readyq != &(reserve_table[i]) ==> reserve_table[i].start_ns <= reserve_table[i].real_start_ns) &&
  (readyq == &(reserve_table[i]) ==> reserve_table[i].start_ns <= stac_now));*/

//-- the active job must have a timer set to enforce its execution time
/*@predicate zsrm2 = elem(readyq) ==>
  (((readyq->enforcement_timer.expiration.tv_sec * 1000000000L +
  readyq->enforcement_timer.expiration.tv_nsec) <= 
  (readyq->exectime_ns - readyq->current_exectime_ns)) &&
  ((readyq->enforcement_timer.expiration.tv_sec * 1000000000L +
  readyq->enforcement_timer.expiration.tv_nsec) <= 
  (readyq->exectime_ns - readyq->real_exectime_ns)));*/

//-- current_exec_time <= exec_time
/*@predicate zsrm3 = \forall int i; 0 <= i < maxReserves ==>
  reserve_table[i].current_exectime_ns <= reserve_table[i].exectime_ns;*/ 

//-- the active job must have a timer set to enforce its execution time
/*@predicate zsrm4 = \forall int i; 0 <= i < maxReserves ==>
  (reserve_table[i].enforcement_timer.expiration.tv_sec * 1000000000L +
  reserve_table[i].enforcement_timer.expiration.tv_nsec) <= 
  reserve_table[i].exectime_ns;*/

//-- the active job must have a timer set to enforce its execution time
/*@predicate zsrm5 = elem(readyq) ==>
  ((readyq->enforcement_timer.absolute_expiration_ns <= 
  (readyq->exectime_ns - readyq->current_exectime_ns)) &&
  (readyq->enforcement_timer.absolute_expiration_ns <= 
  (readyq->exectime_ns - readyq->real_exectime_ns)));*/

//-- the active job must have a timer set to enforce its execution time
/*@predicate zsrm6 = elem(readyq) ==>
  ((readyq->enforcement_timer.stac_expiration_ns <= 
  (readyq->exectime_ns - readyq->current_exectime_ns)) &&
  (readyq->enforcement_timer.stac_armed == 1) &&
  (readyq->enforcement_timer.stac_handler == kernel_timer_handler));*/

//-- periods must equal expiration time of period timer, and timer
//-- rids must equal the reservation ids
/*@predicate zsrm7 = \forall int i; 0 <= i < maxReserves ==>
  (reserve_table[i].period_timer.expiration.tv_sec == reserve_table[i].period.tv_sec &&
  reserve_table[i].period_timer.expiration.tv_nsec == reserve_table[i].period.tv_nsec &&
  reserve_table[i].period_timer.rid == i &&
  reserve_table[i].enforcement_timer.rid == i);*/

//-- period timer must be armed properly
/*@predicate zsrm8 (int i) =
  (reserve_table[i].period_timer.stac_armed == 1 &&
  reserve_table[i].period_timer.stac_expiration_ns == reserve_table[i].period.tv_sec * 1000000000L + reserve_table[i].period.tv_nsec &&
  reserve_table[i].period_timer.stac_handler == kernel_timer_handler);*/

/*********************************************************************/
//-- functions to model time
/*********************************************************************/

//-- exit the program
/*@requires \true;
  @assigns \nothing;
  @ensures \false;
*/
void stac_exit(void)
{
#ifdef STAC_FRAMAC_STUBS
  /*@loop invariant \true;
    @loop assigns \nothing;
  */
  while(1) {}
#endif
}

/*********************************************************************/
//-- system call stubs
/*********************************************************************/

#ifdef STAC_FRAMAC_STUBS

/*********************************************************************/
/*@requires fp11 && fp12 && fp13;
  @requires fp21 && fp22 && fp23;
  @requires fp31 && fp32;
  @requires zsrm1 && zsrm2 && zsrm3 && zsrm4 && zsrm7;
  @assigns \nothing;
  @ensures fp11 && fp12 && fp13;
  @ensures fp21 && fp22 && fp23;
  @ensures fp31 && fp32;
  @ensures zsrm1 && zsrm2 && zsrm3 && zsrm4 && zsrm7;
*/
/*********************************************************************/
int sched_setscheduler(struct task_struct *, int,
                       const struct sched_param *);

/*********************************************************************/
/*@requires fp11 && fp12 && fp13;
  @requires fp21 && fp22 && fp23;
  @requires fp31 && fp32;
  @requires zsrm1 && zsrm2 && zsrm3 && zsrm4 && zsrm7;
  @assigns \nothing;
  @ensures fp11 && fp12 && fp13;
  @ensures fp21 && fp22 && fp23;
  @ensures fp31 && fp32;
  @ensures zsrm1 && zsrm2 && zsrm3 && zsrm4 && zsrm7;
*/
/*********************************************************************/
ktime_t ktime_set(const s64 secs, const unsigned long nsecs);

/*********************************************************************/
/*@requires fp11 && fp12 && fp13;
  @requires fp21 && fp22 && fp23;
  @requires fp31 && fp32;
  @requires zsrm1 && zsrm2 && zsrm3 && zsrm4 && zsrm7;
  @assigns \nothing;
  @ensures fp11 && fp12 && fp13;
  @ensures fp21 && fp22 && fp23;
  @ensures fp31 && fp32;
  @ensures zsrm1 && zsrm2 && zsrm3 && zsrm4 && zsrm7;
*/
/*********************************************************************/
int printk(const char *fmt, ...);

/*********************************************************************/
/*@requires fp11 && fp12 && fp13;
  @requires fp21 && fp22 && fp23;
  @requires fp31 && fp32;
  @requires zsrm1 && zsrm2 && zsrm3 && zsrm4 && zsrm7;
  @assigns \nothing;
  @ensures fp11 && fp12 && fp13;
  @ensures fp21 && fp22 && fp23;
  @ensures fp31 && fp32;
  @ensures zsrm1 && zsrm2 && zsrm3 && zsrm4 && zsrm7;
*/
/*********************************************************************/
void hrtimer_init(struct hrtimer *timer, clockid_t which_clock,
                  enum hrtimer_mode mode);

/*********************************************************************/
/*@requires fp11 && fp12 && fp13;
  @requires fp21 && fp22 && fp23;
  @requires fp31 && fp32;
  @requires zsrm1 && zsrm2 && zsrm3 && zsrm4 && zsrm7;
  @assigns \nothing;
  @ensures fp11 && fp12 && fp13;
  @ensures fp21 && fp22 && fp23;
  @ensures fp31 && fp32;
  @ensures zsrm1 && zsrm2 && zsrm3 && zsrm4 && zsrm7;
*/
/*********************************************************************/
int hrtimer_start(struct hrtimer *timer, ktime_t tim,
                  const enum hrtimer_mode mode);

/*********************************************************************/
/*@requires fp11 && fp12 && fp13;
  @requires fp21 && fp22 && fp23;
  @requires fp31 && fp32;
  @requires zsrm_lem1 && zsrm1 && zsrm3 && zsrm4 && zsrm7;
  @assigns \nothing;
  @ensures fp11 && fp12 && fp13;
  @ensures fp21 && fp22 && fp23;
  @ensures fp31 && fp32;
  @ensures zsrm_lem1 && zsrm1 && zsrm3 && zsrm4 && zsrm7;
  @behavior b1: @assumes \true; @ensures \true;
  @behavior b2: @assumes zsrm2; @ensures zsrm2;
*/
/*********************************************************************/
int hrtimer_cancel(struct hrtimer *timer);

/*********************************************************************/
/*@requires fp11 && fp12 && fp13;
  @requires fp21 && fp22 && fp23;
  @requires fp31 && fp32;
  @requires zsrm1 && zsrm2 && zsrm3 && zsrm4 && zsrm7;
  @assigns \nothing;
  @ensures fp11 && fp12 && fp13;
  @ensures fp21 && fp22 && fp23;
  @ensures fp31 && fp32;
  @ensures zsrm1 && zsrm2 && zsrm3 && zsrm4 && zsrm7;
*/
/*********************************************************************/
struct task_struct *gettask(int pid,struct pid_namespace *ns);

/*********************************************************************/
/*@requires fp11 && fp12 && fp13;
  @requires fp21 && fp22 && fp23;
  @requires fp31 && fp32;
  @requires zsrm1 && zsrm2 && zsrm3 && zsrm4 && zsrm7;
  @assigns \nothing;
  @ensures fp11 && fp12 && fp13;
  @ensures fp21 && fp22 && fp23;
  @ensures fp31 && fp32;
  @ensures zsrm1 && zsrm2 && zsrm3 && zsrm4 && zsrm7;
*/
/*********************************************************************/
int push_to_reschedule(int i);

/*********************************************************************/
/*@requires fp11 && fp12 && fp13;
  @requires fp21 && fp22 && fp23;
  @requires fp31 && fp32;
  @requires zsrm1 && zsrm2 && zsrm3 && zsrm4 && zsrm7;
  @assigns \nothing;
  @ensures fp11 && fp12 && fp13;
  @ensures fp21 && fp22 && fp23;
  @ensures fp31 && fp32;
  @ensures zsrm1 && zsrm2 && zsrm3 && zsrm4 && zsrm7;
*/
/*********************************************************************/
int wake_up_process(struct task_struct *tsk);

/*********************************************************************/
/*@requires fp11 && fp12 && fp13;
  @requires fp21 && fp22 && fp23;
  @requires fp31 && fp32;
  @requires zsrm1 && zsrm2 && zsrm3 && zsrm4 && zsrm7;
  @assigns \nothing;
  @ensures fp11 && fp12 && fp13;
  @ensures fp21 && fp22 && fp23;
  @ensures fp31 && fp32;
  @ensures zsrm1 && zsrm2 && zsrm3 && zsrm4 && zsrm7;
*/
/*********************************************************************/
void set_tsk_need_resched(struct task_struct *tsk);

/*********************************************************************/
/*@requires fp11 && fp12 && fp13;
  @requires fp21 && fp22 && fp23;
  @requires fp31 && fp32;
  @requires zsrm1 && zsrm2 && zsrm3 && zsrm4 && zsrm7;
  @assigns \nothing;
  @ensures fp11 && fp12 && fp13;
  @ensures fp21 && fp22 && fp23;
  @ensures fp31 && fp32;
  @ensures zsrm1 && zsrm2 && zsrm3 && zsrm4 && zsrm7;
*/
/*********************************************************************/
int set_cpus_allowed_ptr(struct task_struct *p,
                         const struct cpumask *new_mask);

/*********************************************************************/
/*@requires fp11 && fp12 && fp13;
  @requires fp21 && fp22 && fp23;
  @requires fp31 && fp32;
  @requires zsrm1 && zsrm2 && zsrm3 && zsrm4 && zsrm7;
  @assigns \nothing;
  @ensures fp11 && fp12 && fp13;
  @ensures fp21 && fp22 && fp23;
  @ensures fp31 && fp32;
  @ensures zsrm1 && zsrm2 && zsrm3 && zsrm4 && zsrm7;
*/
/*********************************************************************/
void __xchg_wrong_size(void);

#endif //STAC_FRAMAC_STUBS


/*********************************************************************/
//-- ZSRM functions
/*********************************************************************/

int add_trace_record(int rid, unsigned long long ts, int event_type)
{
  if (trace_index >= TRACE_BUFFER_SIZE)
    return -1;

  trace_table[trace_index].timestamp_ns = ts;
  trace_table[trace_index].event_type = event_type;
  trace_table[trace_index].rid = rid;
  trace_index++;
  return 0;
}

/*********************************************************************/
/*@requires fp11 && fp12 && fp13;
  @requires fp21 && fp22 && fp23;
  @requires fp31 && fp32;
  @requires zsrm1 && zsrm2 && zsrm3 && zsrm4 && zsrm7;
  @requires elemt(timer);
  @requires timer->absolute_expiration_ns == timer->expiration.tv_sec * 1000000000L + timer->expiration.tv_nsec;
  @assigns *timer;
  @ensures fp11 && fp12 && fp13;
  @ensures fp21 && fp22 && fp23;
  @ensures fp31 && fp32;
  @ensures zsrm1 && zsrm2 && zsrm3 && zsrm4 && zsrm7;
  @behavior b1:
  @assumes timer == &(readyq->enforcement_timer) && zsrm5;
  @ensures zsrm5 && zsrm6;
  @behavior b2:
  @assumes \exists int i; 0 <= i < maxReserves && timer == &(reserve_table[i].period_timer);
  @ensures timer->stac_expiration_ns == timer->expiration.tv_sec * 1000000000L + timer->expiration.tv_nsec;
  @ensures timer->stac_armed == 1 && timer->stac_handler == kernel_timer_handler;
  @ensures timer->stac_expiration_ns == reserve_table[timer->rid].period.tv_sec * 1000000000L + reserve_table[timer->rid].period.tv_nsec;
*/
/*********************************************************************/
int arm_relative_timer(struct zs_timer *timer)
{
  ktime_t ktime;

  ktime = ktime_set(timer->absolute_expiration_ns / 1000000000,
                    timer->absolute_expiration_ns % 1000000000);

#ifdef __ZS_DEBUG__
  printk("ZSRMV: arm_timer (%llu secs, %llu ns ) type(%s)\n", (timer->absolute_expiration_ns  / 1000000000),
	 (timer->absolute_expiration_ns % 1000000000),
	  timer->timer_type == TIMER_ENF ? "TIMER_ENF" : timer->timer_type == TIMER_PERIOD ? "TIMER_PERIOD" : "TIMER_ZS_ENF");
#endif
  /* hrtimer_init(&(timer->kernel_timer), CLOCK_MONOTONIC, HRTIMER_MODE_REL); */
  timer->kernel_timer.function= kernel_timer_handler;
  hrtimer_start(&(timer->kernel_timer), ktime, HRTIMER_MODE_REL);

  //-- update ghost variables
#ifdef STAC_FRAMAC_STUBS
  timer->stac_armed = 1;
  timer->stac_expiration_ns = timer->absolute_expiration_ns;
  timer->stac_handler = timer->kernel_timer.function;
#endif  

  return 0;
}

/*********************************************************************/
/*@requires fp11 && fp12 && fp13;
  @requires fp21 && fp22 && fp23;
  @requires fp31 && fp32;
  @requires zsrm_lem1 && zsrm1 && zsrm2 && zsrm3 && zsrm4 && zsrm7;
  @requires elemt(t);
  @assigns reserve_table[0..(maxReserves-1)],stac_now;
  @ensures stac_now >= \old(stac_now);
  @ensures fp11 && fp12 && fp13;
  @ensures fp21 && fp22 && fp23;
  @ensures fp31 && fp32;
  @ensures zsrm_lem1 && zsrm1 && zsrm2 && zsrm3 && zsrm4 && zsrm7;
  @behavior b1:
  @assumes t == &(readyq->enforcement_timer); @ensures zsrm5 && zsrm6;
  @behavior b2:
  @assumes \exists int i; 0 <= i < maxReserves && t == &(reserve_table[i].period_timer); 
  @ensures t->stac_armed == 1 && t->stac_handler == kernel_timer_handler;
  @ensures t->stac_expiration_ns == reserve_table[t->rid].period.tv_sec * 1000000000L + reserve_table[t->rid].period.tv_nsec;
*/
/*********************************************************************/
int add_timerq(struct zs_timer *t)
{
  unsigned long long expiration_ns;
  expiration_ns = t->expiration.tv_sec * 1000000000L + t->expiration.tv_nsec;
  t->absolute_expiration_ns = expiration_ns;
  return arm_relative_timer(t);
}

int add_crit_blocked(struct reserve *r){
  r->crit_block_next = NULL;
  if (crit_blockq == NULL){
    crit_blockq = r;
  } else if (crit_blockq->criticality < r->criticality){
    r->crit_block_next = crit_blockq;
    crit_blockq = r;
  } else {
    struct reserve *t=crit_blockq;
    int rsv_visited=0;
    while(rsv_visited <= MAX_RESERVES && t->crit_block_next != NULL && t->crit_block_next->criticality > r->criticality && r->rid != t->rid){
      rsv_visited++;
      t=t->crit_block_next;
    }
    if (rsv_visited > MAX_RESERVES &&  t->crit_block_next != NULL){
      printk("ZSRMMV.add_crit_blocked(): ERROR corrupted crit_block_queue\n");
      return 0;
    }
    if (r->rid == t->rid){
      // trying to add rid that is already addded
      printk("ZSRMMV.add_crit_blocked(): WARNING tied to add rid(%d) already in queue\n",r->rid);
      return 0;
    }
    if (t->crit_block_next == NULL){
      t->crit_block_next = r;
      r->crit_block_next = NULL;
    } else { // t->crit_block_next->criticality <= r->criticality
      r->crit_block_next = t->crit_block_next;
      t->crit_block_next = r;
    }
  }
  return 1;
}

int add_crit_stack(struct reserve *r)
{
  r->crit_next = NULL;
  if (critical_reservesq == NULL){
    critical_reservesq = r;
  } else if (critical_reservesq->criticality < r->criticality){
    r->crit_next = critical_reservesq;
    critical_reservesq = r;
  } else {
    struct reserve *t=critical_reservesq;
    int rsv_visited = 0;
    while(rsv_visited <= MAX_RESERVES && t->crit_next != NULL && t->crit_next->criticality > r->criticality && r->rid != t->rid){
      rsv_visited ++;
      t=t->crit_next;
    }
    if (rsv_visited > MAX_RESERVES && t->crit_next != NULL){
      printk("ZSRMMV.add_crit_stack() ERROR corrupted crit_stack\n");
      return 0;
    }
    if (r->rid == t->rid){
      printk("ZSRMMV.add_crit_stack() WARNING tried to add rid(%d) that was already there\n",r->rid);
      return 0;
    }
    if (t->crit_next == NULL){
      t->crit_next = r;
      r->crit_next = NULL;
    } else { // t->crit_next->criticality <= r->criticality
      r->crit_next = t->crit_next;
      t->crit_next = r;
    }
  }
  return 1;
}

/*********************************************************************/
/*@requires fp11 && fp12 && fp13;
  @requires fp21 && fp22 && fp23;
  @requires fp31 && fp32 && zsrm_lem1 && zsrm1 && zsrm2 && zsrm3 && zsrm4 && zsrm7;
  @requires elem(r);
  @assigns reserve_table[0..(maxReserves-1)],rm_head,rm_queue_size;
  @ensures fp1;
  @ensures fp2;
  @ensures fp31 && fp32 && zsrm_lem1 && zsrm1 && zsrm2 && zsrm3 && zsrm4 && zsrm7;
*/
/*********************************************************************/
int add_rm_queue(struct reserve *r)
{
  // making sure we start with a null next
  r->rm_next = NULL;
  
  if (rm_head == NULL){
    rm_head = r;
    r->rm_next = NULL;
  } else if (rm_head->period_ns > r->period_ns){
    r->rm_next = rm_head;
    rm_head = r;
  } else {
    struct reserve *t=rm_head;
    int rsv_visited=0;
    /*@loop invariant elem(t) && t->period_ns <= r->period_ns;
      @loop assigns t;*/
    rsv_visited=0;
    while(rsv_visited <= MAX_RESERVES && t->rm_next != NULL && t->rm_next->period_ns <= r->period_ns && t->rid != r->rid){
      rsv_visited++;
      t=t->rm_next;
    }
    if (t->rid == r->rid){
      // reserve already in
      printk("ZSRMMV.add_rm_queue(): WARNING tried to add reserve rid(%d) already in\n",r->rid);
      return 0;
    }

    if (rsv_visited > MAX_RESERVES && t->rm_next != NULL){
      printk("ZSRMMV.add_rm_queue() ERROR rm_queue corrupted\n");
      return 0;
    }
    
    r->rm_next = t->rm_next;
    t->rm_next = r;
    
    /* if (t->rm_next == NULL){ */
    /*   t->rm_next = r; */
    /*   r->rm_next = NULL; */
    /* } else { // t->next_prio->priority < r->priority */
    /*   r->rm_next = t->rm_next; */
    /*   t->rm_next = r; */
    /* } */
  }
  rm_queue_size++;
  return 1;
}

/*********************************************************************/
/*@requires fp1 && fp2 && fp31 && fp32 && zsrm1 && zsrm2 && zsrm3 && zsrm4 && zsrm7;
  @requires elem(r);
  @assigns reserve_table[0..(maxReserves-1)],rm_head,rm_queue_size;
  @ensures fp1;
  @ensures fp2;
  @ensures fp31 && fp32 && zsrm1 && zsrm2 && zsrm3 && zsrm4 && zsrm7;
*/
/*********************************************************************/
void del_rm_queue(struct reserve *r)
{
  int rsv_visited=0;
  
  if (rm_head == NULL) return;
  if (rm_head == r){
    rm_head = r->rm_next;
    rm_queue_size--;
  } else {
    struct reserve *t = rm_head;
    /*@loop invariant elem(t);
      @loop assigns t;*/
    rsv_visited=0;
    while (rsv_visited <= MAX_RESERVES && t->rm_next != NULL && t->rm_next != r){
      t=t->rm_next;
      rsv_visited ++;
    }
    
    if (rsv_visited > MAX_RESERVES && t->rm_next != NULL && t->rm_next != r){
      printk("ZSRMMV ERROR: del_rm_queue(rid=%d) rsv_visited > MAX_RESERVES\n",r->rid);
    }
    
    if (t->rm_next == r){
      t->rm_next = r->rm_next;
      r->rm_next=NULL;
      rm_queue_size--;
    }
  }
}

/*********************************************************************/
/*@requires fp1 && fp2 && fp31 && fp32 && zsrm1 && zsrm2 && zsrm3 && zsrm4 && zsrm7;
  @assigns reserve_table[0..(maxReserves-1)];
  @ensures fp1;
  @ensures fp2;
  @ensures fp31 && fp32 && zsrm1 && zsrm2 && zsrm3 && zsrm4 && zsrm7;
*/
//SC: TBD. this is challenging because the priorities are changed
//globally.
/*********************************************************************/
int calculate_rm_priorities(void)
{
#ifndef STAC_FRAMAC_STUBS
  struct reserve *t=rm_head;
  int topprio=MIN_PRIORITY + rm_queue_size;
  int rsv_visited=0;

  if (topprio >= DAEMON_PRIORITY){
    printk("ZSRMMV.calculate_rm_priorities(): WARNING assigning task priority higher than scheduler task priority. Timing cannot be guaranteed\n");
  }
  
  /*@loop invariant fp1 && fp2 && fp31 && fp32 && zsrm1 && zsrm2 && zsrm3 && zsrm4 && zsrm7;
    @loop assigns reserve_table[0..(maxReserves-1)];
  */
  rsv_visited=0;
  while(rsv_visited <= MAX_RESERVES && t!= NULL){
    rsv_visited++;
    t->priority = topprio--;
    if (t->priority <=0){
      printk("ZSRMMV.calculate_rm_priority(): ERROR assigned an invalid priority(%d) to rid(%d)\n",t->priority,t->rid);
    }
#ifdef __ZS_DEBUG__    
    printk("ZSRMMV: calculate_rm_priorities(): assigning priority(%d) to rid(%d)\n",t->priority,t->rid);
#endif
    t=t->rm_next;
  }

  if (rsv_visited > MAX_RESERVES && t != NULL){
    printk("ZSRMMV.calculate_rm_priorities() WARNING: corrupted rmqueue... tried to visit more than MAX_RESERVES\n");
  }
#endif
  return 0;
}

/*********************************************************************/
/*@requires fp1 && fp2 && fp31 && fp32 && zsrm1 && zsrm2 && zsrm3 && zsrm4 && zsrm7;
  @assigns reserve_table[0..(maxReserves-1)];
  @ensures fp1;
  @ensures fp2;
  @ensures fp31 && fp32 && zsrm1 && zsrm2 && zsrm3 && zsrm4 && zsrm7;
*/
/*********************************************************************/
int set_rm_priorities(void)
{
  struct reserve *t=rm_head;
  int rsv_visited=0;
 
  /*@loop invariant fp1 && fp2 && fp31 && fp32 && elemNull(t);
    @loop assigns task,t,p;*/
  rsv_visited=0;
  while(rsv_visited <= MAX_RESERVES && t!= NULL){
    rsv_visited++;
    if (t->pid >0){
      push_to_activate(t->rid);      
    } else {
      // pid == 0 means that it has not been attached
      if (t->pid != 0){
	printk("ZSRMMV.set_rm_priorities() ERROR pid(%d) invalid\n",t->pid);
      } else {
#ifdef __ZS_DEBUG__	
	printk("ZSRMMV.set_rm_priorities() reserve rid(%d) not yet attached\n",t->rid);
#endif	
      }
    }
    t=t->rm_next;
  }

  if (rsv_visited > MAX_RESERVES && t != NULL){
    printk("ZSRMMV.set_rm_priorities() WARNING: tried to visite more reserves than MAX_RESERVES\n");
  }

  // Activate activator task
  wake_up_process(active_task);
  
  return 0;
}

// Dio: cannot use floating point in kernel -- disable for now
#if 0
/*********************************************************************/
/*@requires fp1 && fp2 && fp31 && fp32 && zsrm1 && zsrm2 && zsrm3 && zsrm4 && zsrm7;
  @assigns \nothing;
  @ensures fp1;
  @ensures fp2;
  @ensures fp31 && fp32 && zsrm1 && zsrm2 && zsrm3 && zsrm4 && zsrm7;
*/
/*********************************************************************/
float compute_total_utilization(void)
{
  float u=0.0f;
  struct reserve *t = rm_head;
  int rsv_visited=0;

  /*@loop assigns t,u;*/
  rsv_visited=0;
  while(rsv_visited <= MAX_RESERVES && t!=NULL){
    rsv_visited++;
    u = u + ((t->execution_time.tv_sec*1000000000L+t->execution_time.tv_nsec) / (t->period_ns *1.0f));
    t=t->rm_next;
  }
  if (rsv_visited >MAX_RESERVES && t != NULL){
    printk("ZSRMMV.compute_total_utilization() ERROR rm_queue corrupted\n");
  }
  return u;
}
#endif

/*********************************************************************/
/*@requires fp1 && fp2 && fp31 && fp32 && zsrm1 && zsrm2 && zsrm3 && zsrm4 && zsrm7;
  @requires elem(r);
  @assigns \nothing;
  @ensures fp1;
  @ensures fp2;
  @ensures fp31 && fp32 && zsrm1 && zsrm2 && zsrm3 && zsrm4 && zsrm7;
*/
/*********************************************************************/
/* int admit(struct reserve *r) */
/* { */
/*   //Dio: cannot use floating point in kernel -- disable for now */

/*   /\* */
/*   float u = ((r->execution_time.tv_sec*1000000000L+r->execution_time.tv_nsec) / (r->period_ns *1.0f)); */
/*   float ut =  compute_total_utilization(); */

/*   if (u+ut<=0.69f){ */
/*     printk("admit():total utilization:%f\n",(u+ut)); */
/*     // schedulable */
/*     return 1; */
/*   } */
/*   // not schedulable */
/*   return 0; */
/*   *\/ */

/*   return 1; */
/* } */

/*********************************************************************/
/*@requires fp11 && fp12 && fp13;
  @requires fp21 && fp22 && fp23;
  @requires fp31 && fp32;
  @requires zsrm_lem1 && zsrm3 && zsrm4 && zsrm7;
  @assigns stac_now;
  @ensures stac_now > \old(stac_now);
  @ensures \result == stac_now;
  @ensures fp11 && fp12 && fp13;
  @ensures fp21 && fp22 && fp23;
  @ensures fp31 && fp32;
  @ensures zsrm_lem1 && zsrm2 && zsrm3 && zsrm4 && zsrm7;
  @behavior b1: @assumes zsrm1 && zsrm2; @ensures zsrm1 && zsrm2;  
  @behavior b2: @assumes zsrm1_stop1; @ensures zsrm1_stop1;  
  @behavior b3: @assumes zsrm1_stop2 && zsrm2; @ensures zsrm1_stop2 && zsrm2;
*/
/*********************************************************************/
unsigned long long get_now_ns()
#ifdef STAC_FRAMAC_STUBS
;
#else
{
  struct timespec ts;

  getnstimeofday(&ts);

  return (((unsigned long long)ts.tv_sec) * 1000000000L) + (unsigned long long) ts.tv_nsec;
}
#endif

int capture_enforcement_signal(int rid, int pid, int signo)
{
  //struct sched_param p;
  //struct task_struct *task;

  if (rid <0 || rid >= MAX_RESERVES){
    printk("ZSRMMV: ERROR in capture_enforcement_signal rid(%d) outside valid range\n",rid);
    return -1;
  }

  /* if (reserve_table[rid].attached){ */
  /*   task = gettask(pid); */
  /*   if (task == NULL){ */
  /*     printk("ZSRMMV: ERROR in capture_enforcement_signal rid(%d) invalid pid(%d)\n",rid,pid); */
  /*     return -2; */
  /*   } */
  /*   p.sched_priority = reserve_table[rid].priority; */
  /*   if (sched_setscheduler(task,SCHED_FIFO,&p)<0){ */
  /*     printk("ZSRMMV: ERROR in capture_enforcement_signal rid(%d) could not assign priority to pid(%d)\n",rid,pid); */
  /*     return -3; */
  /*   } */
  /* } */
  
  reserve_table[rid].enforcement_signal_captured = (pid < 0)? 0 : 1;
  reserve_table[rid].enforcement_signal_receiver_pid = pid;
  reserve_table[rid].enforcement_signo = signo;
  return 0;
}

int send_budget_enforcement_signal(struct task_struct *task, int signo, int rid){
  struct siginfo info;
  
  info.si_signo = signo;
  info.si_value.sival_int = rid;
  info.si_errno = 0; // no recovery?
  info.si_code = SI_QUEUE;

  if (send_sig_info(signo, &info, task)<0){
    printk("ZSRMMV: error sending budget enforcement signal\n");
    return -1;
  }

#ifdef __ZS_DEBUG__
  printk("ZSRMMV: budget enforcement signal sent\n");
#endif
  return 0;
}


/*********************************************************************/
/*@requires fp1 && fp2 && fp31 && fp32;
  @requires zsrm_lem1 && zsrm2 && zsrm3 && zsrm4 && zsrm7;
  @requires 0 <= rid < maxReserves;
  @assigns reserve_table[0..(maxReserves-1)],readyq,stac_now;
  @ensures stac_now >= \old(stac_now);
  @ensures fp1;
  @ensures fp2;
  @ensures fp31 && fp32;
  @ensures zsrm_lem1 && zsrm2 && zsrm3 && zsrm4 && zsrm7;
  @behavior b1: @assumes zsrm1; @ensures zsrm1;
  @behavior b2: @assumes zsrm1_stop2; @ensures zsrm1_stop2;
*/
/*********************************************************************/
void budget_enforcement(int rid, int request_stop)
{
  struct task_struct *task;

  enforcement_start_timestamp_ticks = get_now_ticks();

  add_trace_record(rid, ticks2ns(enforcement_start_timestamp_ticks), TRACE_EVENT_BUDGET_ENFORCEMENT);
#ifdef __ZS_DEBUG__
  printk("ZSRMMV: budget_enforcement(rid(%d)) pid(%d)\n",rid, reserve_table[rid].pid);
#endif
  
  // cancel the zero_slack timer just in case it is still active
  hrtimer_cancel(&(reserve_table[rid].zero_slack_timer.kernel_timer));

  reserve_table[rid].num_enforcements++;

  if (reserve_table[rid].enforcement_signal_captured){
    // if the enforcement signal is capture we call the signal handler
    // and then enforce the other task. This can potentially be
    // a thread in the enforced task but this is trusted to finish on time
    // (included in the budget) given that is not supervised by the temporal enforcer

    task = gettask(reserve_table[rid].pid,reserve_table[rid].task_namespace); //enforcement_signal_receiver_pid);
    if (task != NULL){
      /* send_budget_enforcement_signal(task,reserve_table[rid].enforcement_signo, */
      /* 				     BUNDLE_RID_STOP_PERIODIC(rid,request_stop, */
      /* 							      (reserve_table[rid].non_periodic_wait ? 0 : 1) )); */
      send_budget_enforcement_signal(task,reserve_table[rid].enforcement_signo,
				     BUNDLE_RID_STOP_PERIODIC(rid,request_stop,
							      (reserve_table[rid].end_of_period_marked ? 0 : 1) ));
    }
    if (!request_stop){
      add_trace_record(rid, ticks2ns(enforcement_start_timestamp_ticks), TRACE_EVENT_DONT_WFNP);
    }
  } else {
    // ONLY ENFORCE IF THE SIGNAL IS NOT CAPTURED -- NOT SECURE MUST BE MODIFIED LATER
    // ask the scheduler_thread to stop this thread
    if (request_stop){
      reserve_table[rid].request_stop = 1;
      push_to_reschedule(rid);
      wake_up_process(sched_task);
    }
    //reserve_table[rid].start_period--;
  }
}

void reset_exectime_counters(int rid)
{
  if (reserve_table[rid].worst_exectime_ticks < reserve_table[rid].current_exectime_ticks){
    reserve_table[rid].worst_exectime_ticks = reserve_table[rid].current_exectime_ticks;
  }
  reserve_table[rid].avg_exectime_ticks += reserve_table[rid].current_exectime_ticks;
  reserve_table[rid].avg_exectime_ticks_measurements ++;
  
  reserve_table[rid].current_exectime_ns = 0;
  reserve_table[rid].current_exectime_ticks = 0;
  reserve_table[rid].in_critical_mode=0;
}

void init_exectime_counters(int rid)
{
  reserve_table[rid].current_exectime_ns = 0;
  reserve_table[rid].current_exectime_ticks = 0;
  reserve_table[rid].in_critical_mode=0;
}

/*********************************************************************/
/*@requires fp11 && fp12 && fp13;
  @requires fp21 && fp22 && fp23;
  @requires fp31 && fp32 && zsrm_lem1 && zsrm1 && zsrm2 && zsrm3 && zsrm4 && zsrm7;
  @requires 0 <= rid < maxReserves;
  @assigns reserve_table[0..(maxReserves-1)],readyq,stac_now;
  @ensures fp1;
  @ensures fp2;
  @ensures fp31 && fp32 && zsrm_lem1 && zsrm1 && zsrm2 && zsrm3 && zsrm4 && zsrm7;
  @ensures \forall int i; 0 <= i < maxReserves ==> i != rid ==> 
  reserve_table[i].real_exectime_ns == \old(reserve_table[i].real_exectime_ns);
  @ensures reserve_table[rid].real_exectime_ns == 0;
  @ensures zsrm8(rid);
*/
/*********************************************************************/
void start_of_period(int rid)
{
  struct task_struct *task;

  // overhead measurement
  arrival_start_timestamp_ticks = get_now_ticks();
  context_switch_start_timestamp_ticks = arrival_start_timestamp_ticks;

  //add_trace_record(rid,ticks2ns(arrival_start_timestamp_ticks),TRACE_EVENT_START_PERIOD);
  
  //-- SC: reset "real execution time"
#ifdef STAC_FRAMAC_STUBS
  reserve_table[rid].real_exectime_ns = 0;
#endif

  // protect against timers with incomplete info/ dead tasks
  if (rid <0 || rid >= MAX_RESERVES){
    printk("ZSRMMV.start_of_period(): WARNING tried to start invalid reserve rid(%d)\n",rid);
    return;
  }
  
  task = gettask(reserve_table[rid].pid,reserve_table[rid].task_namespace);
  if (task == NULL){
    printk("ZSRMMV.start_of_period(): WARNING tried to start reserve rid(%d) for dead process pid(%d) -- calling delete_reserve()\n",rid,reserve_table[rid].pid);
    delete_reserve(rid);
    return;
  }

  // prevent consecutive start_of_period calls without wfnp
  reserve_table[rid].start_period++;

  if (reserve_table[rid].start_period>1){
    printk("ZSRMMV.start_of_period(): ERROR %d consecutive calls to start of period from rid(%d)\n",reserve_table[rid].start_period,rid);

    // call budget_enforcement + cancel enforcement timer
    hrtimer_cancel(&(reserve_table[rid].enforcement_timer.kernel_timer));
    budget_enforcement(rid, 0);
    //if (in_readyq(rid)){
    prev_calling_stop_from=calling_stop_from;
    calling_stop_from=1;
    stop_stac(rid);
      //}
    reserve_table[rid].start_period--;
  }

  reset_exectime_counters(rid);
  
  // start timers

#ifdef __ZS_DEBUG__
  printk("zsrm.start_of_period: start of period rid(%d)\n",rid);
#endif

  // cancel the zero_slack timer just in case it is still active
  hrtimer_cancel(&(reserve_table[rid].zero_slack_timer.kernel_timer));
  add_timerq(&reserve_table[rid].zero_slack_timer);
  add_timerq(&reserve_table[rid].period_timer);

  if (reserve_table[rid].criticality < sys_criticality){
    // should not start given that its criticality is lower
    // than current system level

    printk("zsrm.start_of_period: rid(%d) lower crit than sys-crit\n",rid);

#ifdef __ZS_DEBUG__    
    printk("zsrm.start_of_period: rid(%d) lower crit than sys-crit\n",rid);
#endif    

    arrival_end_timestamp_ticks = get_now_ticks();
    cumm_blocked_arrival_ticks += arrival_end_timestamp_ticks - arrival_start_timestamp_ticks;
    num_blocked_arrivals++;
    arrival_end_timestamp_ticks = arrival_start_timestamp_ticks=0L;
    
    return;
  }

  reserve_table[rid].enforced = 0; // outside of enforcement

  // "send" wakeup call to task just in case it has
  // not yet call wait_for_next_period()
  //   --- ASSUMING: atomic increment
  reserve_table[rid].num_period_wakeups++;

  // only accumulate one
  if (reserve_table[rid].num_period_wakeups >1)
    reserve_table[rid].num_period_wakeups = 1;

  if (reserve_table[rid].non_periodic_wait){
    if (reserve_table[rid].num_wait_release>0){
      add_trace_record(rid,ticks2ns(arrival_start_timestamp_ticks),TRACE_EVENT_START_PERIOD_NON_PERIODIC_WAIT_WAKEUP);
      wake_up_process(task);
      set_tsk_need_resched(task);
      
      calling_start_from = 1;
      start_stac(rid);
    } else {
      add_trace_record(rid,ticks2ns(arrival_start_timestamp_ticks),TRACE_EVENT_START_PERIOD_NON_PERIODIC_WAIT_NO_WAKEUP);
    }
    reserve_table[rid].num_wait_release = 0;
  } else {
    add_trace_record(rid,ticks2ns(arrival_start_timestamp_ticks),TRACE_EVENT_START_PERIOD_PERIODIC_WAIT);
      wake_up_process(task);
      set_tsk_need_resched(task);
      
      calling_start_from = 1;
      start_stac(rid);
  }

  // overhead measurement
  arrival_end_timestamp_ticks = get_now_ticks();
  context_switch_end_timestamp_ticks = arrival_end_timestamp_ticks;

  if (readyq == &reserve_table[rid]){
    // new activated task is now active
    cumm_context_switch_ticks += context_switch_end_timestamp_ticks -
      context_switch_start_timestamp_ticks;
    num_context_switches ++;
  } else {
    // arrivals without context switch 
    cumm_arrival_ticks += arrival_end_timestamp_ticks - arrival_start_timestamp_ticks;
    num_arrivals++;
  }
  arrival_end_timestamp_ticks = arrival_start_timestamp_ticks = 0L;
  context_switch_end_timestamp_ticks = context_switch_start_timestamp_ticks=0L;
}

void zs_enforcement(int rid)
{
  struct reserve *rsv = readyq;
  int call_scheduler=0;
  int rsv_visited=0;

  zs_enforcement_start_timestamp_ticks = get_now_ticks();

#ifdef __ZS_DEBUG__  
  printk("zsrmv.zs_enforcement rid(%d)\n",rid);

#endif  
  if (reserve_table[rid].enforced || reserve_table[rid].criticality < sys_criticality){
    zs_enforcement_start_timestamp_ticks = 0L;
#ifdef __ZS_DEBUG__    
    printk("zsrm.zs_enforcment rid(%d) is enforced itself\n",rid);
#endif   
    return;
  }
  //first set the new sys_criticality
  sys_criticality = reserve_table[rid].criticality;
  reserve_table[rid].in_critical_mode =1;

  add_crit_stack(&reserve_table[rid]);
  rsv_visited = 0;
  while(rsv_visited <= MAX_RESERVES && rsv != NULL){
    rsv_visited ++;
    if (rsv->criticality < sys_criticality){
      struct reserve *r = rsv;
      rsv = rsv->next;
      r->request_stop=1;
      push_to_reschedule(r->rid);
      call_scheduler=1;
      // add to blocked reserves
      add_crit_blocked(r);
#ifdef __ZS_DEBUG__      
      printk("zsrmv.zs_enforcement rid(%d) enforcing rid(%d)\n",rid, r->rid);
#endif      
    } else {
      rsv = rsv->next;
    }
  }
  if (rsv_visited > MAX_RESERVES && rsv != NULL){
    printk("ZSRMMV.zs_enforcement() ERROR reserve queue corrupted\n");
  }
  if (call_scheduler)
    wake_up_process(sched_task);
}

/*********************************************************************/
/*@requires fp1 && fp2 && fp31 && fp32 && zsrm_lem1 && 
  zsrm1 && zsrm2 && zsrm3 && zsrm4 && zsrm7;
  @requires elemt(timer);
  @assigns reserve_table[0..(maxReserves-1)],readyq,stac_now;
  @ensures fp1;
  @ensures fp2;
  @ensures fp31 && fp32 && zsrm_lem1 && zsrm1 && zsrm2 && zsrm3 && zsrm4 && zsrm7;
*/
/*********************************************************************/
void timer_handler(struct zs_timer *timer)
{
  // process the timer
#ifdef __ZS_DEBUG__  
  printk("ZSRMMV: timer handler rid(%d) timer-type(%s)\n",timer->rid,
	 (timer->timer_type == TIMER_ENF? "TIMER_ENF":
	  ( timer->timer_type == TIMER_PERIOD ? "TIMER_PERIOD" : "TIMER_ZS_ENF")));
#endif  

  if (timer->rid <0 || timer->rid >= MAX_RESERVES ||(reserve_table[timer->rid].pid == -1)){
    printk("ZSRMMV: ERROR timer with invalid reserve rid(%d) or pid\n",timer->rid);
  } else {
    struct task_struct *task;
    task = gettask(reserve_table[timer->rid].pid,reserve_table[timer->rid].task_namespace);
    if (task != NULL){
      switch(timer->timer_type){
      case TIMER_ENF:
	budget_enforcement(timer->rid, 1);
	break;
      case TIMER_PERIOD:
	start_of_period(timer->rid);
	break;
      case TIMER_ZS_ENF:
	zs_enforcement(timer->rid);
	break;
      default:
	printk("ZSRMMV: unknown type of timer\n");
	break;
      }
    } else {
      printk("ZSRMMV: timer(%s) without process(%d) -- deleting reserve rid(%d)\n",
	     STRING_TIMER_TYPE(timer->timer_type),
	     reserve_table[timer->rid].pid,
	     timer->rid);
      
      // make sure that request_stop is not set
      reserve_table[timer->rid].request_stop =0;

      //force a reschedule that will delete the reserve
      push_to_reschedule(timer->rid);
      wake_up_process(sched_task);
    }
  }
}

/*********************************************************************/
/*@requires fp11 && fp12 && fp13;
  @requires fp21 && fp22 && fp23;
  @requires fp31 && fp32;
  @requires zsrm_lem1 && zsrm1 && zsrm2 && zsrm3 && zsrm4 && zsrm7;
  @requires (0 <= rid < maxReserves);
  @assigns reserve_table[0..(maxReserves-1)],readyq,stac_now;
  @ensures fp11 && fp12 && fp13;
  @ensures fp21 && fp22 && fp23;
  @ensures fp31 && fp32;
  @ensures zsrm_lem1 && zsrm1 && zsrm2 && zsrm3 && zsrm4 && zsrm7;
*/
/*********************************************************************/
int attach_reserve(int rid, int pid)
{
  struct task_struct *task;
  struct pid_namespace *ns = task_active_pid_ns(current);
  task = gettask(pid,ns);
  if (task == NULL)
    return -1;

  // mark the starting of the first period
  reserve_table[rid].start_period = 1;
  
  reserve_table[rid].enforcement_timer.timer_type = TIMER_ENF;
  reserve_table[rid].period_timer.timer_type = TIMER_PERIOD;
  reserve_table[rid].period_timer.expiration.tv_sec = reserve_table[rid].period.tv_sec;
  reserve_table[rid].period_timer.expiration.tv_nsec = reserve_table[rid].period.tv_nsec;
  reserve_table[rid].zero_slack_timer.expiration.tv_sec = reserve_table[rid].zsinstant.tv_sec;
  reserve_table[rid].zero_slack_timer.expiration.tv_nsec = reserve_table[rid].zsinstant.tv_nsec;
  reserve_table[rid].zero_slack_timer.timer_type = TIMER_ZS_ENF;
  reserve_table[rid].next = NULL;
  reserve_table[rid].enforced=0;
  reserve_table[rid].in_critical_mode=0;
  reserve_table[rid].bound_to_cpu=0; // request activator to bound task to cpu

  init_exectime_counters(rid);
  // MOVED TO ACTIVATOR TASK
/* #if LINUX_VERSION_CODE < KERNEL_VERSION(3,19,0) */
/*   { */
/*     cpumask_t cpumask; */
/*     cpus_clear(cpumask); */
/*     cpu_set(0); */
/*     set_cpus_allowed_ptr(task,&cpumask); */
/*   } */
/* #else */
/*   set_cpus_allowed_ptr(task,cpumask_of(0)); */
/* #endif */

  reserve_table[rid].pid = pid;
  reserve_table[rid].task_namespace = ns;

  //reset_exectime_counters(rid);

  // calculate new priorities of all tasks.
  calculate_rm_priorities();

  // assign new priorities to all tasks
  set_rm_priorities();

  // mark as attached.
  reserve_table[rid].attached = 1;

#ifdef __ZS_DEBUG__  
  printk("ZSRMMV: attached rid(%d) to pid(%d)\n",rid, pid);
#endif

  // TODO: should we move this to the activator??
  add_timerq(&(reserve_table[rid].period_timer));
  add_timerq(&(reserve_table[rid].zero_slack_timer));

  add_trace_record(rid,ticks2ns(get_now_ticks()),TRACE_EVENT_START_PERIOD);

  
  calling_start_from = 2;
  start_stac(rid);
  return 0;
}

/*********************************************************************/
/*@requires fp1 && fp2 && fp31 && fp32;
  @requires zsrm_lem1 && zsrm1 && zsrm2 && zsrm3 && zsrm4 && zsrm7;
  @requires (0 <= rid < maxReserves);
  @assigns reserve_table[0..(maxReserves-1)],readyq,stac_now;
  @ensures fp1;
  @ensures fp2;
  @ensures fp31 && fp32 && zsrm_lem1 && zsrm1 && zsrm2 && zsrm3 && zsrm4 && zsrm7;
*/
/*********************************************************************/
int delete_reserve(int rid)
{
  struct task_struct *task;
  struct sched_param p;
  int need_enforcement = 0;
  int rsv_visited=0;
  unsigned long long now_ticks = get_now_ticks();

  if (rid <0 || rid >= MAX_RESERVES){
    printk("ZSRMMV: WARNING tried to delete invalid reserve");
    return -1;
  }
  
  task = gettask(reserve_table[rid].pid,reserve_table[rid].task_namespace);

  /* Special case when the task died and we are forcing
   *   the deletion of the reserve
   */
  if (task != NULL){
    prev_calling_stop_from = calling_stop_from;
    calling_stop_from = 2;
    stop_stac(rid);
  } else if (readyq != NULL) {
    if (readyq != &reserve_table[rid]){
      struct reserve *t = readyq;

      // **** DEBUG READYQ ****
      //printk("ZSRMMV.delete_reserve(%d) accessing readyq\n",rid);
      
      rsv_visited=0;
      while (rsv_visited <= MAX_RESERVES && t->next != NULL && t->next != &reserve_table[rid]){
	t=t->next;
	rsv_visited++;
      }

      if (rsv_visited > MAX_RESERVES && t->next != NULL){
	printk("ZSRMMV ERROR: delete_reserve(rid=%d) readyq corrupted rsv_visited > MAX_RESERVES\n",rid);
      }
      
      if (t->next == &reserve_table[rid]){
	t->next = reserve_table[rid].next;
	reserve_table[rid].next = NULL;
      }
    } else if (readyq == &reserve_table[rid]) {

      // *** DEBUG READYQ ***
      //printk("ZSRMMV.delete_reserve(%d) accessing readyq\n",rid);
      
      readyq = readyq->next;
      reserve_table[rid].next = NULL;
      if (readyq != NULL){
	// double check for existest on task
	task = gettask(readyq->pid,readyq->task_namespace);
	  if (task != NULL){
	    readyq->start_ticks = now_ticks;
	    need_enforcement = start_enforcement_timer(readyq);
	    if (need_enforcement){
#ifdef __ZS_DEBUG__	      
	      printk("ZSRMMV.delete_reserve(rid=%d) enforcing rid(%d) immediately after activation\n",rid,readyq->rid);
#endif	      
	      budget_enforcement(readyq->rid,1);
	    }
	  }
	  // else we will let the timer force the deletion.
	  // Note that if the reserve was not properly deleted at least the
	  // periodic timer will be active and we will take advantage of it
	  // to delete te reserve.
      }
    } else {
      printk("ZSRMMV: deleting reserve not in ready queue\n");
    }
  }
  hrtimer_cancel(&(reserve_table[rid].enforcement_timer.kernel_timer));
  hrtimer_cancel(&(reserve_table[rid].period_timer.kernel_timer));
  hrtimer_cancel(&(reserve_table[rid].zero_slack_timer.kernel_timer));

#ifdef __ZS_DEBUG__  
  printk("ZSRMMV.delete reserve(): enforcements(%ld), wfnps(%ld)\n",
	 reserve_table[rid].num_enforcements,
	 reserve_table[rid].num_wfnp);
#endif  
  
  reserve_table[rid].pid=-1;
  reserve_table[rid].next=NULL;
  if (task != NULL){
    p.sched_priority=0;
    sched_setscheduler(task,SCHED_NORMAL,&p);
  }

  del_rm_queue(&reserve_table[rid]);
  return 0;
}

int get_wcet_ns(int rid, unsigned long long *wcet)
{
  *wcet = ticks2ns(reserve_table[rid].worst_exectime_ticks);
  return 0;
}

int get_acet_ns(int rid, unsigned long long *avet)
{
  if (reserve_table[rid].avg_exectime_ticks_measurements>0){
    *avet = ticks2ns(reserve_table[rid].avg_exectime_ticks /
		     reserve_table[rid].avg_exectime_ticks_measurements);
  } else {
    *avet = 0L;
  }
  return 0;
}

/*********************************************************************/
//-- does not require zsrm2 but ensures it
/*@requires fp1 && fp2 && fp31 && fp32;
  @requires zsrm_lem1 && zsrm3 && zsrm4 && zsrm7;
  @requires elem(rsvp) && rsvp == readyq;
  @assigns rsvp->enforcement_timer,stac_now;
  @ensures stac_now >= \old(stac_now);
  @ensures fp1;
  @ensures fp2;
  @ensures fp31 && fp32;
  @ensures zsrm_lem1 && zsrm2 && zsrm3 && zsrm4 && zsrm7;
  @behavior b1: @assumes zsrm1; @ensures zsrm1 && zsrm5 && zsrm6;
  @behavior b2: @assumes zsrm1_stop2; @ensures zsrm1_stop2 && zsrm5 && zsrm6;
*/
/*********************************************************************/
int start_enforcement_timer(struct reserve *rsvp)
{
  unsigned long long rest_ticks;
  unsigned long long rest_ns;

  //if (rsvp->current_exectime_ns < rsvp->exectime_ns){
  if (rsvp->exectime_ticks > rsvp->current_exectime_ticks){
    rest_ticks = rsvp->exectime_ticks - rsvp->current_exectime_ticks;
    rest_ns = ticks2ns(rest_ticks);
    rsvp->enforcement_timer.expiration.tv_sec = (rest_ns / 1000000000L);
    rsvp->enforcement_timer.expiration.tv_nsec = (rest_ns % 1000000000L);
  } else {
    return 1;
  }

#ifdef __ZS_DEBUG__  
  printk("ZSRMMV: start_enforcement_timer rid(%d) STARTED\n",rsvp->rid);
#endif  
  add_timerq(&(rsvp->enforcement_timer));
  return 0;
  
  /* } else { */
  /*   stac_exit(); */
  /*   // already expired! */
  /*   // ask whoever is calling me to call enforcement */
  /*   printk("ZSRMMV: start_enforcement_timer FAILED -- already expired\n"); */
  /*   return 1; */
  /* } */
}

/*********************************************************************/
/*@requires fp1 && fp2 && fp31 && fp32;
  @requires zsrm_lem1 && zsrm1 && zsrm2 && zsrm3 && zsrm4 && zsrm7;
  @requires (0 <= rid < maxReserves);
  @assigns reserve_table[0..(maxReserves-1)],readyq,stac_now;
  @ensures fp1;
  @ensures fp21 && fp22 && fp23;
  @ensures fp31 && fp32 && zsrm_lem1 && zsrm1 && zsrm2 && zsrm3 && zsrm4 && zsrm7;
  @ensures readyq != \null ==> zsrm5 && zsrm6;
*/
/*********************************************************************/
void start_stac(int rid)
{
#ifdef STAC_FRAMAC_STUBS
  //-- SC: update "real execution time"
  if(readyq != NULL)
    readyq->real_exectime_ns += get_now_ns() - readyq->real_start_ns;
#endif

  start(rid);

#ifdef STAC_FRAMAC_STUBS
  //-- SC: update "real start_ns"
  if(readyq != NULL)
    readyq->real_start_ns = get_now_ns();
#endif
}

int in_readyq(int rid)
{
  int rsv_visited=0;
  int cnt=0;
  struct reserve *tmp = readyq;
  while (rsv_visited <= MAX_RESERVES && tmp != NULL){// && tmp->rid != rid){
    rsv_visited++;
    if (tmp->rid == rid)
      cnt++;
    tmp = tmp->next;
  }

  if (rsv_visited > MAX_RESERVES && tmp != NULL){
    printk("ZSRMMV.in_readyq(): ERROR: queue larger than MAX_RESERVES\n");
  }

  if (cnt > 1){
    printk("ZSRMMV.in_readyq(): ERROR rid(%d) is %d times in the queue\n",rid,cnt);
  }

  return cnt;
  /* if (tmp != NULL && tmp->rid == rid){ */
  /*   return 1; */
  /* } else { */
  /*   return 0; */
  /* } */
}



/*********************************************************************/
/*@requires fp1 && fp2 && fp31 && fp32;
  @requires zsrm_lem1 && zsrm1_start1 && zsrm3 && zsrm4 && zsrm7;
  @requires (0 <= rid < maxReserves);
  @assigns reserve_table[0..(maxReserves-1)],readyq,stac_now;
  @ensures fp1;
  @ensures fp21 && fp22 && fp23;
  @ensures fp31 && fp32 && zsrm_lem1 && zsrm1_start2 && zsrm2 && zsrm3 && zsrm4 && zsrm7;
  @ensures readyq != \null ==> readyq->start_ns <= stac_now;
  @ensures readyq != \null ==> zsrm5 && zsrm6;
*/
/*********************************************************************/
void start(int rid)
{
  int need_enforcement=0;
  unsigned long long now_ticks = get_now_ticks();
  int new_runner=0;
  int old_rid=-1;
  int cnt=0;

  if ((cnt=in_readyq(rid))){
    printk("ZSRMMV.start(): WARNING trying to add rid(%d) already in ready queue cnt(%d)-- aborting addition\n",rid,cnt);
    printk("ZSRMMV.start():     called start from: %s\n",NAME_START_FROM(calling_start_from));
    return;
  }


  // *** DEBUG READYQ ***
  //printk("ZSRMMV.start(%d) accessing readyq called from(%s)\n",rid,NAME_START_FROM(calling_start_from));
  
  if (readyq == NULL){
    readyq = &reserve_table[rid];
    new_runner=1;
  } else {
    readyq->stop_ticks = now_ticks;

    //SC: abnormal termination
    if (readyq->current_exectime_ticks + readyq->stop_ticks - readyq->start_ticks > readyq->exectime_ticks){
      stac_exit();
    }
    //SC: abnormal termination
    if (readyq->current_exectime_ticks + readyq->stop_ticks - readyq->start_ticks < readyq->current_exectime_ticks){
      stac_exit();
    }
    
    readyq->current_exectime_ticks += readyq->stop_ticks - readyq->start_ticks;

    // cancel timer
    hrtimer_cancel(&(readyq->enforcement_timer.kernel_timer));

    if (readyq->priority < reserve_table[rid].priority){
      new_runner = 1;
      old_rid = readyq->rid;
      reserve_table[rid].next = readyq;
      // switch to new task
      readyq=&reserve_table[rid];
    } else {
      int rsv_visited=0;
      struct reserve *tmp = readyq;
      /*@loop invariant elem(tmp) && (tmp->priority >= reserve_table[rid].priority);
        @loop assigns tmp;*/
      //while (tmp->next != NULL && tmp->next->priority >= reserve_table[rid].priority){
      rsv_visited = 0;
      //while (rsv_visited <= MAX_RESERVES && tmp->next != NULL && tmp->next->priority > reserve_table[rid].priority && tmp->rid != rid){
      // Make sure we respect FIFO when same priority 
      while (rsv_visited <= MAX_RESERVES && tmp->next != NULL && tmp->next->priority >= reserve_table[rid].priority && tmp->rid != rid){
	rsv_visited++;
        tmp = tmp->next;
      }

      if (tmp->rid == rid){
	// double activation+addition. Abort
	printk("ZSRMMV.start(): WARNING tried to ACTIVE an ALREADY ACTIVE task rid(%d)\n",rid);
	// break here!
	return;
      }
      
      if (rsv_visited > MAX_RESERVES && tmp->next != NULL){
	tmp = readyq;
	rsv_visited=0;
	while (rsv_visited <= MAX_RESERVES && tmp->next != NULL && tmp->next->priority > reserve_table[rid].priority && tmp->rid != rid){
	  rsv_visited++;
	  printk("ZSRMMV.start(): ERROR tried to search more thabn MAX_RESERVES. rid(%d)\n",tmp->rid);
	  tmp = tmp->next;
	}
	// break here
	return;
      }
      
      if (tmp->next != NULL){ // tmp->next->priority < reserve_table[rid].priority
        reserve_table[rid].next = tmp->next;
        tmp->next = &reserve_table[rid];
      } else {
	tmp->next = &reserve_table[rid];
	reserve_table[rid].next=NULL;
      }
    }
  }

  if (readyq != NULL) {
    // start readyq accounting
    readyq->start_ticks = now_ticks;
    // start enforcement_timer
    need_enforcement= start_enforcement_timer(readyq);
    if (need_enforcement){
      budget_enforcement(readyq->rid,1);
    } else {
      if (new_runner){
	if (old_rid >= 0){
	  add_trace_record(old_rid,ticks2ns(now_ticks),TRACE_EVENT_PREEMPTED);	
	}
	add_trace_record(readyq->rid,ticks2ns(now_ticks),TRACE_EVENT_RESUMED);
      }
    }
  }
  
  /* if (need_enforcement){ */
  /*   budget_enforcement(readyq->rid); */
  /* } */
}

/*********************************************************************/
/*@requires fp1 && fp2 && fp31 && fp32;
  @requires zsrm_lem1 && zsrm1 && zsrm2 && zsrm3 && zsrm4 && zsrm7;
  @requires (0 <= rid < maxReserves);
  @assigns reserve_table[0..(maxReserves-1)],readyq,stac_now;
  @ensures fp1;
  @ensures fp2;
  @ensures fp31 && fp32 && zsrm_lem1 && zsrm1 && zsrm2 && zsrm3 && zsrm4 && zsrm7;
  @ensures readyq != \null ==> zsrm5 && zsrm6;
*/
/*********************************************************************/
void stop_stac(int rid)
{
#ifdef STAC_FRAMAC_STUBS
  //-- SC: update "real execution time"
  if(readyq != NULL)
    readyq->real_exectime_ns += get_now_ns() - readyq->real_start_ns;
#endif

  stop(rid);

#ifdef STAC_FRAMAC_STUBS
  //-- SC: update "real start_ns"
  if(readyq != NULL)
    readyq->real_start_ns = get_now_ns();
#endif
}

int inside_stop=0;
/*********************************************************************/
/*@requires fp1 && fp2 && fp31 && fp32;
  @requires zsrm_lem1 && zsrm1_stop1 && zsrm3 && zsrm4 && zsrm7;
  @requires (0 <= rid < maxReserves);
  @assigns reserve_table[0..(maxReserves-1)],readyq,stac_now;
  @ensures fp1;
  @ensures fp2;
  @ensures fp31 && fp32 && zsrm_lem1 && zsrm1_stop2 && zsrm2 && zsrm3 && zsrm4 && zsrm7;
  @ensures readyq != \null ==> readyq->start_ns <= stac_now;
  @ensures readyq != \null ==> zsrm5 && zsrm6;
*/
/*********************************************************************/
void stop(int rid)
{
  unsigned long long now_ticks = get_now_ticks();
  int need_enforcement=0;

  if (!in_readyq(rid)){
    printk("ZSRMMV.stop() WARNING trying to stop rid(%d) not in ready queue from(%s) prev(%s)\n",rid,
	   NAME_STOP_FROM(calling_stop_from),NAME_STOP_FROM(prev_calling_stop_from));
    return;
  }

  // *** DEBUG READYQ ***
  //printk("ZSRMMV.stop(%d) modifying readyq\n",rid);
  
  inside_stop++;

  // make sure I have a correct rid
  if (rid < 0 || rid >= MAX_RESERVES){
    printk("ZSRMMV.stop(): WARNING tried to stop invalid reserve rid(%d)\n",rid);
    return;
  }

  if (readyq == &reserve_table[rid]){
    add_trace_record(rid,ticks2ns(now_ticks),TRACE_EVENT_PREEMPTED);
    if(readyq != NULL) {
      //readyq->stop_ns = now_ns;
      readyq->stop_ticks = now_ticks;

      //SC: abnormal termination
      //if(readyq->current_exectime_ns + readyq->stop_ns - readyq->start_ns > readyq->exectime_ns)
      if (readyq->current_exectime_ticks + readyq->stop_ticks - readyq->start_ticks > readyq->exectime_ticks)
	stac_exit();

      //SC: abnormal termination
      //if(readyq->current_exectime_ns + readyq->stop_ns - readyq->start_ns < readyq->current_exectime_ns)
      if (readyq->current_exectime_ticks + readyq->stop_ticks - readyq->start_ticks < readyq->current_exectime_ticks)
	stac_exit();

      //readyq->current_exectime_ns += readyq->stop_ns - readyq->start_ns;
      readyq->current_exectime_ticks += readyq->stop_ticks - readyq->start_ticks;
    }

    // cancel timer
    hrtimer_cancel(&(readyq->enforcement_timer.kernel_timer));

    if (readyq == &reserve_table[rid]){
      readyq = readyq->next;
      reserve_table[rid].next = NULL;
    }

    if (readyq != NULL) {
      //readyq->start_ns = now_ns;
      readyq->start_ticks = now_ticks;
      // start enforcement timer
      need_enforcement = start_enforcement_timer(readyq);
      if (need_enforcement){
      	budget_enforcement(readyq->rid,1);
      }
      add_trace_record(readyq->rid,ticks2ns(now_ticks),TRACE_EVENT_RESUMED);
    }

    /* if (need_enforcement){ */
    /*   budget_enforcement(readyq->rid); */
    /* } */
  } else {
    if (readyq != NULL){
      // it is not at the head (readyq)
      // only remove it from the readyq
      int rsv_visited=0;
      struct reserve *t = readyq;
      while (rsv_visited <= MAX_RESERVES && t->next != NULL && t->next != &reserve_table[rid]){
	rsv_visited++;
	t=t->next;
      }

      if (rsv_visited > MAX_RESERVES && t->next != NULL){
	printk("ZSRMMV.stop(): tried to visit more than MAX_RESERVES\n");
      }
      
      if (t->next == &reserve_table[rid]){
	t->next = reserve_table[rid].next;
	reserve_table[rid].next = NULL;
      }
    } else {
      reserve_table[rid].next = NULL;
    }
  }

  if (in_readyq(rid)){
    printk("ZSRMMV.stop(): ERROR stopped rid(%d) still in ready queue\n",rid);
  }
  inside_stop--;
}

/*********************************************************************/
/*@requires fp1 && fp2 && fp31 && fp32;
  @requires zsrm_lem1 && zsrm1 && zsrm2 && zsrm3 && zsrm4 && zsrm7;
  @requires 0 <= rid < maxReserves;
  @assigns reserve_table[0..(maxReserves-1)],readyq,stac_now;
  @ensures fp1;
  @ensures fp2;
  @ensures fp31 && fp32 && zsrm_lem1 && zsrm1 && zsrm2 && zsrm3 && zsrm4 && zsrm7;
*/
/*********************************************************************/
int wait_for_next_period(int rid, int nowait)
{
  struct task_struct *task;
  int rsv_visited = 0;

  //if (nowait)
  //  return 0;
  
  departure_start_timestamp_ticks = get_now_ticks();

  add_trace_record(rid, ticks2ns(departure_start_timestamp_ticks), TRACE_EVENT_WFNP);
  
  hrtimer_cancel(&(reserve_table[rid].zero_slack_timer.kernel_timer));

  // Mark as periodic
  reserve_table[rid].non_periodic_wait=0;

  prev_calling_stop_from=calling_stop_from;
  calling_stop_from=3;
  stop_stac(rid);

  reserve_table[rid].num_wfnp++;

  reserve_table[rid].start_period--;
  //printk("ZSRMMV.wait_for_next_period(%d): decremented start_period. Now: %d\n",rid,reserve_table[rid].start_period);
  
  if (reserve_table[rid].in_critical_mode){
    reserve_table[rid].in_critical_mode=0;
    if (critical_reservesq == &reserve_table[rid]){
      critical_reservesq = critical_reservesq->crit_next;
    }
    if (critical_reservesq != NULL){
      sys_criticality = critical_reservesq->criticality;
    } else {
      sys_criticality = 0;
    }
    // only re-enable blocked tasks if I did not exceeded my nominal execution time
    if (reserve_table[rid].current_exectime_ticks <= reserve_table[rid].nominal_exectime_ticks){
#ifdef __ZS_DEBUG__      
      printk("zsrm.wait_for_next_period(): rid(%d) did no exceed nominal -- waking up blocked reserves\n",rid);
#endif      
      // re-enable blocked reserves
      rsv_visited=0;
      while(rsv_visited <= MAX_RESERVES && crit_blockq!= NULL && crit_blockq->criticality >=sys_criticality){
	struct reserve *t = crit_blockq;
	rsv_visited ++;
	crit_blockq = crit_blockq->crit_block_next;
	task = gettask(t->pid,t->task_namespace);
	if (task != NULL){
	  wake_up_process(task);
	  set_tsk_need_resched(task);
	  calling_start_from = 3;
	  start_stac(t->rid);
	} else {
	  delete_reserve(t->rid);
	  printk("zsrmv.wait_for_next_period(): could not find criticality-blocked task pid(%d)\n",t->pid);
	}
	//start_stac(t->rid);
      }
      if (rsv_visited > MAX_RESERVES && crit_blockq != NULL){
	printk("ZSRMMV.wait_for_next_period(rid(%d)) [1] ERROR crit_blockq corrupted\n",rid);
      }
    } else {
      // if I exceeded it take them out of the crit_block queue but do not
      // wake them up -- let their period timers wake them up
#ifdef __ZS_DEBUG__      
      printk("zsrm.wait_for_next_period(): rid(%d) EXCEEDED nominal -- not waking blocked reserves\n",rid);
#endif
      rsv_visited = 0;
      while(rsv_visited <= MAX_RESERVES && crit_blockq!= NULL && crit_blockq->criticality >=sys_criticality){
	rsv_visited++;
    	crit_blockq = crit_blockq->crit_block_next;
      }
      if (rsv_visited > MAX_RESERVES && crit_blockq != NULL){
	printk("ZSRMMV.wait_for_next_period(rid(%d)) [2]  ERROR crit_blockq corrupted\n",rid);	
      }
    }
  }

  departure_end_timestamp_ticks = get_now_ticks();
  cumm_departure_ticks += departure_end_timestamp_ticks - departure_start_timestamp_ticks;
  num_departures++;
  departure_end_timestamp_ticks = departure_start_timestamp_ticks = 0L;

  // Only go to sleep if I have not received an unprocessed wakeup from
  // the period timer
  //if (reserve_table[rid].num_period_wakeups <= 0 ){ //&& !nowait){
#ifdef __ZS_DEBUG__  
  printk("ZSRMMV: wait_next_period rid(%d) pid(%d) STOP\n",rid, current->pid);
#endif  
  set_current_state(TASK_INTERRUPTIBLE);
#ifdef __ZS_DEBUG__  
  printk("ZSRMMV: wait_next_period rid(%d) pid(%d) RESUME\n",rid, current->pid);
#endif    
  /* } else { */
  /*   printk("ZSRMMV.wait_next_period(): skipping sleeping\n"); */
  /*   // reset wcet & acet counter */
  /*   //reset_exectime_counters(rid); */
  /*   // start again */
  /*   start_stac(rid); */
  /* } */

  // decrement the processed wakeup calls from the
  // periodic timer
  // --- ASSUMING atomic decrement
  reserve_table[rid].num_period_wakeups--;
  
  return 0;
}

int end_of_period(int rid)
{
  struct task_struct *task;
  int rsv_visited = 0;

  departure_start_timestamp_ticks = get_now_ticks();

  add_trace_record(rid, ticks2ns(departure_start_timestamp_ticks), TRACE_EVENT_END_PERIOD);
  
  hrtimer_cancel(&(reserve_table[rid].zero_slack_timer.kernel_timer));

  prev_calling_stop_from=calling_stop_from;
  calling_stop_from=3;
  stop_stac(rid);

  reserve_table[rid].num_wfnp++;

  // Signal scheduler that we are about to call a non-periodic wait
  reserve_table[rid].non_periodic_wait = 1;

  // mark the end of period
  reserve_table[rid].end_of_period_marked = 1;

  reserve_table[rid].start_period--;
  //printk("ZSRMMV.wait_for_next_period(%d): decremented start_period. Now: %d\n",rid,reserve_table[rid].start_period);
  
  if (reserve_table[rid].in_critical_mode){
    reserve_table[rid].in_critical_mode=0;
    if (critical_reservesq == &reserve_table[rid]){
      critical_reservesq = critical_reservesq->crit_next;
    }
    if (critical_reservesq != NULL){
      sys_criticality = critical_reservesq->criticality;
    } else {
      sys_criticality = 0;
    }
    // only re-enable blocked tasks if I did not exceeded my nominal execution time
    if (reserve_table[rid].current_exectime_ticks <= reserve_table[rid].nominal_exectime_ticks){
#ifdef __ZS_DEBUG__      
      printk("zsrm.wait_for_next_period(): rid(%d) did no exceed nominal -- waking up blocked reserves\n",rid);
#endif      
      // re-enable blocked reserves
      rsv_visited=0;
      while(rsv_visited <= MAX_RESERVES && crit_blockq!= NULL && crit_blockq->criticality >=sys_criticality){
	struct reserve *t = crit_blockq;
	rsv_visited ++;
	crit_blockq = crit_blockq->crit_block_next;
	task = gettask(t->pid,t->task_namespace);
	if (task != NULL){
	  wake_up_process(task);
	  set_tsk_need_resched(task);
	  calling_start_from = 3;
	  start_stac(t->rid);
	} else {
	  delete_reserve(t->rid);
	  printk("zsrmv.wait_for_next_period(): could not find criticality-blocked task pid(%d)\n",t->pid);
	}
	//start_stac(t->rid);
      }
      if (rsv_visited > MAX_RESERVES && crit_blockq != NULL){
	printk("ZSRMMV.wait_for_next_period(rid(%d)) [1] ERROR crit_blockq corrupted\n",rid);
      }
    } else {
      // if I exceeded it take them out of the crit_block queue but do not
      // wake them up -- let their period timers wake them up
#ifdef __ZS_DEBUG__      
      printk("zsrm.wait_for_next_period(): rid(%d) EXCEEDED nominal -- not waking blocked reserves\n",rid);
#endif
      rsv_visited = 0;
      while(rsv_visited <= MAX_RESERVES && crit_blockq!= NULL && crit_blockq->criticality >=sys_criticality){
	rsv_visited++;
    	crit_blockq = crit_blockq->crit_block_next;
      }
      if (rsv_visited > MAX_RESERVES && crit_blockq != NULL){
	printk("ZSRMMV.wait_for_next_period(rid(%d)) [2]  ERROR crit_blockq corrupted\n",rid);	
      }
    }
  }

  departure_end_timestamp_ticks = get_now_ticks();
  cumm_departure_ticks += departure_end_timestamp_ticks - departure_start_timestamp_ticks;
  num_departures++;
  departure_end_timestamp_ticks = departure_start_timestamp_ticks = 0L;

  // This is not a real wakeup yet... this will be registered in the
  // wait_for_next_release
  // reserve_table[rid].num_period_wakeups--;
  
  return 0;
}

int wait_for_next_release(int rid)
{
  // Only go to sleep if I have not received an unprocessed wakeup from
  // the period timer
  unsigned long long wait_arrival_timestamp_ticks = get_now_ticks();
  //add_trace_record(rid, ticks2ns(wait_arrival_timestamp_ticks), TRACE_EVENT_WAIT_RELEASE);

  if (reserve_table[rid].end_of_period_marked){
    if (reserve_table[rid].num_period_wakeups <= 0 ){
      add_trace_record(rid, ticks2ns(wait_arrival_timestamp_ticks), TRACE_EVENT_WAIT_RELEASE_BLOCKED);
#ifdef __ZS_DEBUG__  
      printk("ZSRMMV: wait_next_period rid(%d) pid(%d) STOP\n",rid, current->pid);
#endif  
      set_current_state(TASK_INTERRUPTIBLE);
      reserve_table[rid].num_wait_release++;
#ifdef __ZS_DEBUG__  
      printk("ZSRMMV: wait_next_period rid(%d) pid(%d) RESUME\n",rid, current->pid);
#endif
    } else {
      // Continue executing & mark new start
      add_trace_record(rid, ticks2ns(wait_arrival_timestamp_ticks), TRACE_EVENT_WAIT_RELEASE_NOT_BLOCKED);
      start_stac(rid);
    }
    
    // unmark the end of period
    reserve_table[rid].end_of_period_marked=0;

    reserve_table[rid].num_period_wakeups--;

  } else {
    // This means that the enforcer was called and it called wait_for_release() which completed the end_of_period()+wait_for_release() pair.
    // Hence, we now need to call for a full wait_for_next_period() instead.
    
    wait_for_next_period(rid,0);
  }
  
  return 0;
}

/*********************************************************************/
//-- this function initializes the scheduler. for zsrm, this means
//-- only initializing the key data structures
/*********************************************************************/
/*@requires zsrm_lem1;
  @assigns readyq, rm_head, reserve_table[0..(maxReserves-1)];
  @ensures readyq == \null && rm_head == \null;
  @ensures \forall integer i; 0 <= i < maxReserves ==> 
    (reserve_table[i].pid == -1 && 
    reserve_table[i].rid == i &&
    reserve_table[i].next == \null &&
    reserve_table[i].rm_next == \null &&
    reserve_table[i].period_timer.next == \null &&
    reserve_table[i].enforcement_timer.next == \null &&
    reserve_table[i].enforcement_timer.expiration.tv_sec == 0 &&
    reserve_table[i].enforcement_timer.expiration.tv_nsec == 0 &&
    reserve_table[i].exectime_ns == 0 &&
    reserve_table[i].current_exectime_ns == 0 &&
    reserve_table[i].real_exectime_ns == 0 &&
    reserve_table[i].start_ns == 0 &&
    reserve_table[i].real_start_ns == 0);
  @ensures fp1;
  @ensures fp2;
  @ensures fp31 && fp32 && zsrm_lem1 && zsrm1 && zsrm2 && zsrm3 && zsrm4 && zsrm7;
  @ensures readyq != \null ==> zsrm5 && zsrm6;
*/
/*********************************************************************/
void init(void)
{
  int i;
  
  readyq=NULL;
  rm_head=NULL;
  
  /*@loop invariant (\forall integer j; 
    0 <= j < i ==> (reserve_table[j].pid == -1 && 
      reserve_table[j].rid == j &&
      reserve_table[j].next == \null &&
      reserve_table[j].rm_next == \null &&
      reserve_table[j].period_timer.next == \null &&
      reserve_table[j].enforcement_timer.next == \null &&
      reserve_table[j].enforcement_timer.expiration.tv_sec == 0 &&
      reserve_table[j].enforcement_timer.expiration.tv_nsec == 0 &&
      reserve_table[j].exectime_ns == 0 &&
      reserve_table[j].current_exectime_ns == 0 &&
      reserve_table[j].real_exectime_ns == 0 &&
      reserve_table[j].start_ns == 0 &&
      reserve_table[j].real_start_ns == 0 &&
      reserve_table[j].period_timer.expiration.tv_sec == reserve_table[j].period.tv_sec &&
      reserve_table[j].period_timer.expiration.tv_nsec == reserve_table[j].period.tv_nsec &&
      reserve_table[j].period_timer.rid == j && 
      reserve_table[j].enforcement_timer.rid == j)) && zsrm_lem1;
    @loop assigns i,reserve_table[0..(maxReserves-1)];*/
  for (i=0;i<MAX_RESERVES;i++) {
    reserve_table[i].enforcement_type = ENF_NONE;
    reserve_table[i].task_namespace=NULL;
    reserve_table[i].pid=-1;
    reserve_table[i].rid=i;
    reserve_table[i].next=NULL;
    reserve_table[i].rm_next=NULL;
    reserve_table[i].crit_next=NULL;
    reserve_table[i].crit_block_next=NULL;
    reserve_table[i].period_timer.next = NULL;
    reserve_table[i].enforcement_timer.next = NULL;
    reserve_table[i].enforcement_timer.expiration.tv_sec = 0;
    reserve_table[i].enforcement_timer.expiration.tv_nsec = 0;
    reserve_table[i].exectime_ns = 0;
    reserve_table[i].exectime_ticks = 0;
    reserve_table[i].current_exectime_ns = 0;
    reserve_table[i].current_exectime_ticks = 0;
    reserve_table[i].worst_exectime_ticks = 0;
    reserve_table[i].avg_exectime_ticks=0;
    reserve_table[i].avg_exectime_ticks_measurements=0;
    reserve_table[i].start_ns = 0;
    reserve_table[i].request_stop=0;
    reserve_table[i].period_timer.expiration.tv_sec = reserve_table[i].period.tv_sec;
    reserve_table[i].period_timer.expiration.tv_nsec = reserve_table[i].period.tv_nsec;
    reserve_table[i].period_timer.rid = i;
    reserve_table[i].enforcement_timer.rid = i;
    reserve_table[i].zero_slack_timer.rid = i;
    reserve_table[i].in_critical_mode=0;
    reserve_table[i].enforced=0;
    reserve_table[i].criticality=0;
    reserve_table[i].num_enforcements=0;
    reserve_table[i].num_wfnp=0;
    reserve_table[i].non_periodic_wait = 0;
    reserve_table[i].end_of_period_marked=0;
    reserve_table[i].num_wait_release = 0;
    reserve_table[i].num_period_wakeups=0;
    reserve_table[i].enforcement_signal_captured = 0;
    reserve_table[i].attached=0;
#ifdef STAC_FRAMAC_STUBS
    reserve_table[i].real_exectime_ns = 0;
    reserve_table[i].real_start_ns = 0;
#endif
  }
}

void init_reserve(int rid)
{
  reserve_table[rid].task_namespace=NULL;
  reserve_table[rid].num_wfnp=0;
  reserve_table[rid].non_periodic_wait=0;
  reserve_table[rid].end_of_period_marked=0;
  reserve_table[rid].num_wait_release=0;
  reserve_table[rid].num_enforcements=0;
  reserve_table[rid].worst_exectime_ticks=0;
  reserve_table[rid].avg_exectime_ticks=0;
  reserve_table[rid].avg_exectime_ticks_measurements=0;
  reserve_table[rid].num_period_wakeups=0;
  reserve_table[rid].enforcement_type = ENF_NONE;
  reserve_table[rid].enforcement_signal_captured = 0;
  reserve_table[rid].attached=0;
  reserve_table[rid].next=NULL;
  reserve_table[rid].rm_next=NULL;
  reserve_table[rid].crit_next=NULL;
  reserve_table[rid].crit_block_next=NULL;
  reserve_table[rid].period_timer.next = NULL;
  reserve_table[rid].enforcement_timer.next = NULL;
  reserve_table[rid].start_period=0;
  
  // init timers to make sure we do not crash the kernel
  // if timer operations are called before we arm the timer.
  hrtimer_init(&(reserve_table[rid].period_timer.kernel_timer), CLOCK_MONOTONIC_RAW, HRTIMER_MODE_REL);
  hrtimer_init(&(reserve_table[rid].enforcement_timer.kernel_timer), CLOCK_MONOTONIC_RAW, HRTIMER_MODE_REL);
  hrtimer_init(&(reserve_table[rid].zero_slack_timer.kernel_timer), CLOCK_MONOTONIC_RAW, HRTIMER_MODE_REL);

}

/*********************************************************************/
/*@requires fp1 && fp2 && fp31 && fp32 && zsrm1 && zsrm2 && zsrm3 && zsrm4;
  @assigns reserve_table[0..(maxReserves-1)];
  @ensures fp1;
  @ensures fp2;
  @ensures fp31 && fp32 && zsrm1 && zsrm2 && zsrm3 && zsrm4;
*/
/*********************************************************************/
int getreserve()
{
  int i=0;

  /*@loop invariant (0 <= i <= maxReserves) && fp1 && fp2 && fp31 && fp32 
    && zsrm1 && zsrm2 && zsrm3 && zsrm4;
    @loop assigns i,reserve_table[0..(maxReserves-1)];*/
  for(i=0;i<MAX_RESERVES;i++){
    if(reserve_table[i].pid == -1){
      reserve_table[i].pid = 0;
      init_reserve(i);
      return i;
    }
  }

  return -1;
}

/*********************************************************************/
//-- end of VERIFIED file
/*********************************************************************/

MODULE_AUTHOR("Dionisio de Niz");
MODULE_LICENSE("GPL");

#define DEVICE_NAME "zsrmmv"

/* device number. */
static dev_t dev_id;
static unsigned int dev_major;
static struct class *dev_class = NULL;

/* char device structure. */
static struct cdev c_dev;

static DEFINE_SPINLOCK(zsrmlock);
//static DEFINE_SPINLOCK(reschedule_lock);
static DEFINE_SPINLOCK(activate_lock);
struct semaphore zsrmsem;


struct task_struct *gettask(int pid,  struct pid_namespace *task_namespace){
  struct task_struct *tsk;
  struct pid *pid_struct;

  // Search for the pid in the task namespace
  pid_struct = find_pid_ns(pid,task_namespace);
  tsk = pid_task(pid_struct, PIDTYPE_PID);
  return tsk;
}

struct zs_timer *kernel_timer2zs_timer(struct hrtimer *tmr){
  char *ztmrp = (char *) tmr;
  ztmrp = ztmrp - (((char*) &(((struct zs_timer *)ztmrp)->kernel_timer))-ztmrp);
  return (struct zs_timer *) ztmrp;
}

enum hrtimer_restart kernel_timer_handler(struct hrtimer *ktimer){
  unsigned long flags;
  struct zs_timer *zstimer;
  int tries;
  int locked=0;

  // *** DEBUGGING ONLY

  tries = 1000000;
  while(tries >0 && !(locked = spin_trylock_irqsave(&zsrmlock,flags)))
    tries--;
  
  if (!locked){
    zstimer = kernel_timer2zs_timer(ktimer);
    printk("ZSRMMV.kernel_timer_handler(type(%s)) spinlock locked by type(%s) cmd(%s): tied %d times. ABORT!\n",
	   STRING_LOCKER(zstimer->timer_type),
	   STRING_LOCKER(prevlocker),
	   STRING_ZSV_CALL(zsrmcall),
	   1000000);
    return HRTIMER_NORESTART;
  }	

  //spin_lock_irqsave(&zsrmlock,flags);

  zstimer = kernel_timer2zs_timer(ktimer);

  prevlocker = zstimer->timer_type;
  
  if (zstimer != NULL){
    timer_handler(zstimer);
  } else {
    printk("ZSRMV: kernel_timer_handler: zstimer == NULL\n");
  }
  
  spin_unlock_irqrestore(&zsrmlock,flags);
  return HRTIMER_NORESTART;
}

int reschedule_stack[MAX_RESERVES];
int top=-1;

int push_to_reschedule(int i){
  int ret;
  //unsigned long flags;
  //spin_lock_irqsave(&reschedule_lock,flags);
  if ((top+1) < MAX_RESERVES){
    top++;
    reschedule_stack[top]=i;
    ret =0;
  } else	
    ret=-1;
  
  //spin_unlock_irqrestore(&reschedule_lock,flags);
  return ret;
}

int pop_to_reschedule(void){
  int ret;
  //unsigned long flags;
  //spin_lock_irqsave(&reschedule_lock,flags);
  if (top >=0){
    ret = reschedule_stack[top];
    top--;
  } else
    ret = -1;
  //spin_unlock_irqrestore(&reschedule_lock,flags);
  return ret;
}


static void scheduler_task(void *a){
  int rid;
  struct sched_param p;
  struct task_struct *task;
  unsigned long flags;
  //int swap_task=0;
  
  while (!kthread_should_stop()) {
    // prevent concurrent execution with interrupts
    spin_lock_irqsave(&zsrmlock,flags);
    prevlocker = SCHED_TASK;
    while ((rid = pop_to_reschedule()) != -1) {
      if (reserve_table[rid].request_stop){
	reserve_table[rid].request_stop = 0;
	if (!reserve_table[rid].enforced){
	  reserve_table[rid].enforced=1;
	  task = gettask(reserve_table[rid].pid,reserve_table[rid].task_namespace);
	  if (task == NULL){
	    delete_reserve(rid);
	    continue;
	  }
#ifdef __ZS_DEBUG__	  
	  printk("ZSRMMV: sched_task: stopping rsv(%d)\n",rid);
#endif	  
	  set_task_state(task, TASK_INTERRUPTIBLE);
	  //swap_task=0;
	  set_tsk_need_resched(task);
	  // Dio: try commenting out stop_stac()
	  // for testing... this will mess up the accounting
	  // we should probably double check if this is called recursively:
	  //  - global variable (inside_stop)
	  //  - check variable
	  if (inside_stop){
	    printk("ZSRMMV.scheduler_task(): recursive call to stop() count:%d\n",inside_stop);
	  }
	  reserve_table[rid].start_period--;
	  prev_calling_stop_from=calling_stop_from;
	  calling_stop_from=4;
	  stop_stac(rid);
	} else {
	  printk("ZSRMMV.scheduler_task(): reserve_table[rid(%d)].enforced != 0\n",rid);
	}
      } else {
	p.sched_priority = reserve_table[rid].priority;
	task = gettask(reserve_table[rid].pid,reserve_table[rid].task_namespace);
	if (task == NULL){
	  delete_reserve(rid);
	  continue;
	}
#ifdef __ZS_DEBUG__	
	printk("ZSRMMV: sched_task: set prio(%d) for pid(%d)\n",p.sched_priority, reserve_table[rid].pid);
#endif 	
	if ((task->state & TASK_INTERRUPTIBLE) || (task->state & TASK_UNINTERRUPTIBLE)){
	  wake_up_process(task);
	}
	sched_setscheduler(task, SCHED_FIFO, &p);

	calling_start_from = 4;
	
	start_stac(rid);
      }
    }

    if (enforcement_start_timestamp_ticks != 0L){
      enforcement_end_timestamp_ticks = get_now_ticks();
      cumm_enforcement_ticks += enforcement_end_timestamp_ticks -
	enforcement_start_timestamp_ticks;
      num_enforcements++;
      enforcement_end_timestamp_ticks = enforcement_start_timestamp_ticks=0L;
    }

    if (zs_enforcement_start_timestamp_ticks != 0L){
      zs_enforcement_end_timestamp_ticks = get_now_ticks();
      cumm_zs_enforcement_ticks += zs_enforcement_end_timestamp_ticks -
	zs_enforcement_start_timestamp_ticks;
      num_zs_enforcements++;
      zs_enforcement_end_timestamp_ticks = zs_enforcement_start_timestamp_ticks = 0L;
    }
    
    spin_unlock_irqrestore(&zsrmlock,flags);
    
    set_current_state(TASK_INTERRUPTIBLE);
    schedule();
  }
}

int activate_stack[MAX_RESERVES];
int activate_top=-1;

int push_to_activate(int i){
  int ret;
  unsigned long flags;
  spin_lock_irqsave(&activate_lock,flags);
  if ((activate_top+1) < MAX_RESERVES){
    activate_top++;
    activate_stack[activate_top]=i;
    ret =0;
  } else	
    ret=-1;
  
  spin_unlock_irqrestore(&activate_lock,flags);
  return ret;
}

int pop_to_activate(void){
  int ret;
  unsigned long flags;
  spin_lock_irqsave(&activate_lock,flags);
  if (activate_top >=0){
    ret = activate_stack[activate_top];
    activate_top--;
  } else
    ret = -1;
  spin_unlock_irqrestore(&activate_lock,flags);
  return ret;
}

static void activator_task(void *a)
{
  int rid;
  struct sched_param p;
  struct task_struct *task;
  
  while (!kthread_should_stop()) {
    // prevent concurrent execution with interrupts
    prevlocker = SCHED_TASK;
    while ((rid = pop_to_activate()) != -1) {
      if (rid <0 || rid >= MAX_RESERVES){
	printk("ZSRMMV.activator_task(): ERROR tried to activate invalid reserve rid(%d)\n",rid);
	continue;
      }
      p.sched_priority = reserve_table[rid].priority;
      task = gettask(reserve_table[rid].pid,reserve_table[rid].task_namespace);
      if (task == NULL){
	continue;
      }

      if (!reserve_table[rid].bound_to_cpu){
	reserve_table[rid].bound_to_cpu = 1;
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,19,0)
	{
	  cpumask_t cpumask;
	  cpus_clear(cpumask);
	  cpu_set(0);
	  set_cpus_allowed_ptr(task,&cpumask);
	}
#else
	set_cpus_allowed_ptr(task,cpumask_of(0));
#endif
      }
      
      if (sched_setscheduler(task, SCHED_FIFO, &p)<0){
	printk("ZSRMMV.activator_task(): ERROR could not set priority(%d) to pid(%d)\n",p.sched_priority, reserve_table[rid].pid);
      } else {
	printk("ZSRMMV.activator_task(): set priority(%d) to pid(%d)\n",p.sched_priority, reserve_table[rid].pid);
      }
    }
    set_current_state(TASK_INTERRUPTIBLE);
    schedule();
  }
}


int long long ticksperus=0L;

void set_ticksperus(int long long ticks, int long long nanos){
  ticksperus = ticks *1000;
  ticksperus = ticksperus / nanos;
}

unsigned long long ticks2ns(unsigned long long ticks){
  unsigned long long ns;

  ns = ticks*1000;
  ns = ns / ticksperus;

  return ns;
}

unsigned long long ns2ticks(unsigned long long ns){
  unsigned long long ticks;

  ticks = ns * ticksperus;
  ticks = ticks / 1000;

  return ticks;
}

void setup_ticksclock(void)
#ifdef STAC_FRAMAC_STUBS
;
#else
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,4,6)
{}
#else
{
  struct system_time_snapshot snap,snap1;
  unsigned long long i,start;

  ktime_get_snapshot(&snap);
  i=0;
  start = get_now_ns();
  while (i<10000){
    i = get_now_ns() - start;
  }
  ktime_get_snapshot(&snap1);
  set_ticksperus((snap1.cycles-snap.cycles),(snap1.raw.tv64-snap.raw.tv64));  
}
#endif
#endif

/*********************************************************************/
/*@requires fp11 && fp12 && fp13;
  @requires fp21 && fp22 && fp23;
  @requires fp31 && fp32;
  @requires zsrm_lem1 && zsrm3 && zsrm4 && zsrm7;
  @assigns stac_now;
  @ensures stac_now > \old(stac_now);
  @ensures \result == stac_now;
  @ensures fp11 && fp12 && fp13;
  @ensures fp21 && fp22 && fp23;
  @ensures fp31 && fp32;
  @ensures zsrm_lem1 && zsrm2 && zsrm3 && zsrm4 && zsrm7;
  @behavior b1: @assumes zsrm1 && zsrm2; @ensures zsrm1 && zsrm2;  
  @behavior b2: @assumes zsrm1_stop1; @ensures zsrm1_stop1;  
  @behavior b3: @assumes zsrm1_stop2 && zsrm2; @ensures zsrm1_stop2 && zsrm2;
*/
/*********************************************************************/
unsigned long long get_now_ticks(void)
#ifdef STAC_FRAMAC_STUBS
;
#else
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,4,6)
{ return 0; }
#else
{
  struct system_time_snapshot snap;
  ktime_get_snapshot(&snap);
  return snap.cycles;
}
#endif
#endif

/* dummy function. */
static int zsrm_open(struct inode *inode, struct file *filp)
{
  return 0;
}

/* dummy function. */
static int zsrm_release(struct inode *inode, struct file *filp)
{
  return 0;
}



static ssize_t zsrm_read(struct file *filp,	/* see include/linux/fs.h   */
			   char *buffer,	/* buffer to fill with data */
			   size_t length,	/* length of the buffer     */
			   loff_t * offset)
{
  int transfer_size;

  transfer_size = (length >= (trace_index * sizeof(struct trace_rec_t))) ? (trace_index * sizeof(struct trace_rec_t)) : (length / sizeof(struct trace_rec_t)) * sizeof(struct trace_rec_t) ; 

  if (copy_to_user(buffer, trace_table, transfer_size)<0){
      printk(KERN_WARNING "ZSRMV: error copying trace_table to user space\n");
  }

  trace_index = 0;
		   
  return transfer_size;
}

int valid_rid(int rid)
{
  if (rid <0 || rid >= MAX_RESERVES)
    return 0;

  return 1;
}

int active_rid(int rid)
{
  if (valid_rid(rid))
    if (reserve_table[rid].pid != -1)
      return 1;
  return 0;
}

static ssize_t zsrm_write
(struct file *file, const char *buf, size_t count, loff_t *offset)
{
  //int err;
  int need_reschedule=0;
  int ret = 0;
  struct api_call call;
  unsigned long flags;
  unsigned long long wcet;
  unsigned long long Z;
  
  /* copy data to kernel buffer. */
  if (copy_from_user(&call, buf, count)) {
    printk(KERN_WARNING "ZSRMMV: failed to copy data.\n");
    return -EFAULT;
  }

  // try to lock semaphore to prevent concurrent syscalls
  // before disabling interrupts  
  if ((ret = down_interruptible(&zsrmsem)) < 0){
    return ret;
  } 

  // disable interrupts to avoid concurrent interrupts
  spin_lock_irqsave(&zsrmlock,flags);

  // *** DEBUG LOCKER
  prevlocker = ZSV_CALL;
  zsrmcall = call.cmd;
  
  switch (call.cmd) {
  case END_PERIOD:
    if (!active_rid(call.rid)){
      printk("ZSRMMV.write() ERROR got cmd(%s) with invalid/inactive rid(%d)\n",
	     STRING_ZSV_CALL(call.cmd),call.rid);
      ret = -1;
    } else {
      ret = end_of_period(call.rid);
      need_reschedule = 0;
    }
    break;
  case WAIT_RELEASE:
    if (!active_rid(call.rid)){
      printk("ZSRMMV.write() ERROR got cmd(%s) with invalid/inactive rid(%d)\n",
	     STRING_ZSV_CALL(call.cmd),call.rid);
      ret = -1;
    } else {
      //printk("ZSRMMV.write() calling wait_for_next_release(rid(%d))\n",call.rid);
      ret = wait_for_next_release(call.rid);
      //printk("ZSRMMV.write() returned from wait_for_next_release(rid(%d))\n",call.rid);
      need_reschedule = 1;
    }
    break;
  case GET_TRACE_SIZE:
    ret = trace_index;
    need_reschedule = 0;
    break;
  case WAIT_PERIOD:
    if (!active_rid(call.rid)){
      printk("ZSRMMV.write() ERROR got cmd(%s) with invalid/inactive rid(%d)\n",
	     STRING_ZSV_CALL(call.cmd),call.rid);
      ret = -1;
    } else {
      //printk("ZSRMMV.write() calling wait_for_next_period(rid(%d))\n",call.rid);
      wait_for_next_period(call.rid,0);
      //printk("ZSRMMV.write() returned from wait_for_next_period(rid(%d))\n",call.rid);
      ret = 0;
      need_reschedule = 1;
    }
    break;
  case NOWAIT_PERIOD:
    if (!active_rid(call.rid)){
      printk("ZSRMMV.write() ERROR got cmd(%s) with invalid/inactive rid(%d)\n",
	     STRING_ZSV_CALL(call.cmd),call.rid);
      ret = -1;
    } else {
      wait_for_next_period(call.rid,1);
      ret = 0;
      need_reschedule = 1;
    }
    break;
  case CREATE_RSV:
    ret = getreserve();
    if (ret >=0){
      reserve_table[ret].period.tv_sec = call.period_sec;
      reserve_table[ret].period.tv_nsec = call.period_nsec;
      reserve_table[ret].zsinstant.tv_sec = call.zsinstant_sec;
      reserve_table[ret].zsinstant.tv_nsec = call.zsinstant_nsec;
      reserve_table[ret].period_ns = (call.period_sec * 1000000000L) +
	call.period_nsec;
      reserve_table[ret].execution_time.tv_sec = call.exec_sec;
      reserve_table[ret].execution_time.tv_nsec = call.exec_nsec;
      reserve_table[ret].exectime_ns = (call.exec_sec * 1000000000L) +
	call.exec_nsec;
      reserve_table[ret].exectime_ticks = ns2ticks(reserve_table[ret].exectime_ns);
      reserve_table[ret].priority = call.priority;
      reserve_table[ret].criticality = call.criticality;
      reserve_table[ret].nominal_execution_time.tv_sec = call.nominal_exec_sec;
      reserve_table[ret].nominal_execution_time.tv_nsec = call.nominal_exec_nsec;
      reserve_table[ret].nominal_exectime_ns = (call.nominal_exec_sec * 1000000000L) +
	call.nominal_exec_nsec;
      reserve_table[ret].nominal_exectime_ticks = ns2ticks(reserve_table[ret].nominal_exectime_ns);

      reserve_table[ret].zsinstant_ns = (call.zsinstant_sec * 1000000000L)+call.zsinstant_nsec;
      
      // verify if zero slack instant is the same as period set it to twice its value to ensure that it does not
      // have the possibility of triggering before the end of period (effectively disabling it).
      if (reserve_table[ret].period_ns == ((call.zsinstant_sec * 1000000000L)+call.zsinstant_nsec)){
	// simple both secs and nsecs are doubled
	reserve_table[ret].zsinstant.tv_sec *= 2;
	reserve_table[ret].zsinstant.tv_nsec *= 2;
      }
      
      if (admit(reserve_table, MAX_RESERVES, &reserve_table[ret],&Z)){
	reserve_table[ret].zsinstant_ns = Z;
	// for protection
	if (reserve_table[ret].zsinstant_ns == reserve_table[ret].period_ns){
	  reserve_table[ret].zsinstant_ns *= 2;
	}

	reserve_table[ret].zsinstant.tv_sec =  reserve_table[ret].zsinstant_ns / 1000000000L;
	reserve_table[ret].zsinstant.tv_nsec = reserve_table[ret].zsinstant_ns % 1000000000L;
	
	add_rm_queue(&reserve_table[ret]);
#ifdef __ZS_DEBUG__	
	printk("zsrmv.create_rsv: rid(%d) period: sec(%ld) nsec(%ld) zs: sec(%ld), nsec(%ld) \n",
	       ret,call.period_sec, call.period_nsec,
	       call.zsinstant_sec, call.zsinstant_nsec);
#endif	
      } else {	    
	reserve_table[ret].pid=-1; // mark unused
	ret = -1;
      }
    }
    break;
  case ATTACH_RSV:
#ifdef __ZS_DEBUG__    
    printk("ZSRMMV: received attach rid(%d), pid(%d)\n",call.rid,call.pid);
#endif
    if (!active_rid(call.rid)){
      printk("ZSRMMV.write() ERROR got cmd(%s) with invalid/inactive rid(%d)\n",
	     STRING_ZSV_CALL(call.cmd),call.rid);
      ret = -1;
    } else {
      if (call.pid <=0){
	printk("ZSRMMV.write() ERROR got cmd(%s) with invalid pid(%d)\n",
	       STRING_ZSV_CALL(call.cmd),call.pid);
	ret = -1;
      } else {
	ret = attach_reserve(call.rid,call.pid);
	need_reschedule=1;
      }
    }
    break;
  case DELETE_RSV:
#ifdef __ZS_DEBUG__    
    printk("ZSRMMV: received delete rid(%d)\n",call.rid);
#endif
    if (!valid_rid(call.rid)){
      printk("ZSRMMV.write() ERROR got cmd(%s) with invalid/inactive rid(%d)\n",
	     STRING_ZSV_CALL(call.cmd),call.rid);
      ret = -1;
    } else {
      ret = delete_reserve(call.rid);
      need_reschedule=1;
    }
    break;
  case GET_WCET_NS:
    if (!valid_rid(call.rid)){
      printk("ZSRMMV.write() ERROR got cmd(%s) with invalid/inactive rid(%d)\n",
	     STRING_ZSV_CALL(call.cmd),call.rid);
    } else {
      ret = get_wcet_ns(call.rid,&wcet);
      if (copy_to_user(call.pwcet, &wcet, sizeof(wcet))<0){
	printk(KERN_WARNING "ZSRMV: error copying WCET to user space\n");
      }
      need_reschedule = 0;
    }
    break;
  case GET_ACET_NS:
    ret = get_acet_ns(call.rid,&wcet);
    if (copy_to_user(call.pwcet,&wcet, sizeof(wcet))<0){
      printk(KERN_WARNING "ZSRMV: error copying ACET to user space\n");
    }
    break;
  case CAPTURE_ENFORCEMENT_SIGNAL:
#ifdef __ZS_DEBUG__    
    printk("ZSRMMV: received CAPTURE ENFORCEMENT\n");
#endif
    if (!valid_rid(call.rid)){
      printk("ZSRMMV.write() ERROR got cmd(%s) with invalid/inactive rid(%d)\n",
	     STRING_ZSV_CALL(call.cmd),call.rid);
      ret = -1;
    } else {
      ret = capture_enforcement_signal(call.rid,call.pid,call.priority);
    }
    break;
  default:
    printk("ZSRMMV: ERROR UNKNOWN SYSCALL(%d)\n",call.cmd);
    ret=-1;
    break;
  }
  
  
  // enable interrupts
  spin_unlock_irqrestore(&zsrmlock,flags);

  // allow other syscalls
  // MOVED to after checking for need_reschedule to
  // avoid potential race condition
  // up(&zsrmsem);

  if (need_reschedule){
    // allow other syscalls
    up(&zsrmsem);
    schedule();
  } else {
    // allow other syscalls
    up(&zsrmsem);
  }
  return ret;
}

static long zsrm_ioctl(struct file *file,
		       unsigned int cmd, 
		       unsigned long arg)
{
	return 0;
}

static struct file_operations zsrm_fops;

static void
zsrm_cleanup_module(void){

  if (dev_class){
    device_destroy(dev_class, MKDEV(dev_major, 0));
  }
  /* delete the char device. */
  cdev_del(&c_dev);

  if (dev_class)
    class_destroy(dev_class);
  /* return back the device number. */
  unregister_chrdev_region(dev_id, 1);	  
}

static int __init zsrm_init(void)
{
  int ret;
  int err = 0;
  dev_t devno;
  struct device *device = NULL;
  struct sched_param p;
    
  // initialize scheduling top
  top = -1;
  
  // initialize semaphore
  sema_init(&zsrmsem,1); // binary - initially unlocked

  init();
  printk(KERN_INFO "ZSRMMV: HELLO!\n");
  
  /* get the device number of a char device. */
  ret = alloc_chrdev_region(&dev_id, 0, 1, DEVICE_NAME);
  if (ret < 0) {
    printk(KERN_WARNING "ZSRMMV: failed to allocate device.\n");
    return ret;
  }
  
  dev_major = MAJOR(dev_id);
  
  dev_class = class_create(THIS_MODULE,DEVICE_NAME);
  if (IS_ERR(dev_class)){
    printk(KERN_WARNING "ZSRMMV: failed to create device class.\n");
    err = PTR_ERR(dev_class);
    dev_class = NULL;
    zsrm_cleanup_module();
    return err;
  }
  
  devno = MKDEV(dev_major, 0);
  
  // initialize the fops 
  zsrm_fops.owner = THIS_MODULE;
  zsrm_fops.open = zsrm_open;
  zsrm_fops.release = zsrm_release;
  zsrm_fops.read = zsrm_read;
  zsrm_fops.write = zsrm_write;
  
  //#if LINUX_KERNEL_MINOR_VERSION < 37
  //zsrm_fops.ioctl = zsrm_ioctl;
  //#else
  zsrm_fops.unlocked_ioctl = zsrm_ioctl;
  //#endif
  
  /* initialize the char device. */
  cdev_init(&c_dev, &zsrm_fops);
  
  /* register the char device. */
  ret = cdev_add(&c_dev, dev_id, 1);
  if (ret < 0) {
    printk(KERN_WARNING "ZSRMMV: failed to register device.\n");
    return ret;
  }
  
  device = device_create(dev_class, NULL, devno, NULL, DEVICE_NAME "%d", 0);
  
  if (IS_ERR(device)){
    err = PTR_ERR(device);
    printk(KERN_WARNING "ZSRMMV: Error %d while trying to create dev %s%d", err, DEVICE_NAME,0);
    cdev_del(&c_dev);
    return err;
  }

  // Start scheduler task
  sched_task = kthread_create((void *)scheduler_task, NULL, "ZSRMMV scheduler thread");  
  p.sched_priority = DAEMON_PRIORITY;
  
  if (sched_setscheduler(sched_task, SCHED_FIFO, &p)<0){
    printk("ZSRMMV.init() error setting sched_task kernel thead priority\n");
  }
  
  kthread_bind(sched_task, 0);

  // Start activator task
  active_task = kthread_create((void *)activator_task, NULL, "Activator thread");  
  p.sched_priority = DAEMON_PRIORITY;
  
  if (sched_setscheduler(active_task, SCHED_FIFO, &p)<0){
    printk("ZSRMMV.init() error setting active_task kernel thead priority\n");
  }
  
  kthread_bind(active_task, 0);

  
  setup_ticksclock();
  
  printk(KERN_WARNING "ZSRMMV: ready!\n");
    
  return 0;
}

void print_overhead_stats(void)
{
  unsigned long long avg_context_switch_ns=0L;
  unsigned long long avg_enforcement_ns=0L;
  unsigned long long avg_zs_enforcement_ns=0L;
  unsigned long long avg_arrival_ns=0L;
  unsigned long long avg_blocked_arrival_ns=0L;
  unsigned long long avg_departure_ns = 0L;

  if (num_context_switches >0){
    avg_context_switch_ns = cumm_context_switch_ticks / num_context_switches;
    avg_context_switch_ns = ticks2ns(avg_context_switch_ns);
  }

  if (num_enforcements>0){
    avg_enforcement_ns = cumm_enforcement_ticks / num_enforcements;
    avg_enforcement_ns = ticks2ns(avg_enforcement_ns);
  }

  if (num_zs_enforcements>0){
    avg_zs_enforcement_ns = cumm_zs_enforcement_ticks / num_zs_enforcements;
    avg_zs_enforcement_ns = ticks2ns(avg_zs_enforcement_ns);
  }

  if (num_arrivals >0){
    avg_arrival_ns = cumm_arrival_ticks / num_arrivals;
    avg_arrival_ns = ticks2ns(avg_arrival_ns);
  }

  if (num_blocked_arrivals>0){
    avg_blocked_arrival_ns = cumm_blocked_arrival_ticks / num_blocked_arrivals;
    avg_blocked_arrival_ns = ticks2ns(avg_blocked_arrival_ns);
  }

  if (num_departures >0){
    avg_departure_ns = cumm_departure_ticks / num_departures;
    avg_departure_ns = ticks2ns(avg_departure_ns);
  }

  printk("zsrmv *** OVERHEAD STATS *** \n");
  printk("avg context switch ns: %llu \t num context switch %llu \n",
	 avg_context_switch_ns, num_context_switches);
  printk("avg enforcement ns: %llu num enforcements: %llu \n",
	 avg_enforcement_ns, num_enforcements);
  printk("avg zs enforcement ns: %llu num zs enforcements: %llu \n",
	 avg_zs_enforcement_ns, num_zs_enforcements);
  printk("avg arrival ns: %llu num arrivals: %llu \n",
	 avg_arrival_ns, num_arrivals);
  printk("avg blocked arrival ns: %llu num blocked arrivals: %llu\n",
	 avg_blocked_arrival_ns, num_blocked_arrivals);
  printk("avg departure ns: %llu num departures: %llu\n",
	 avg_departure_ns, num_departures);
  printk("zsrmv *** END OVERHEAD STATS *** \n");  
}

static void __exit zsrm_exit(void)
{
  // empty rescheduling stack
  top = -1;
  kthread_stop(sched_task);

  activate_top = -1;
  kthread_stop(active_task);

  print_overhead_stats();
  
  printk(KERN_INFO "ZSRMMV: GOODBYE!\n");
  
  zsrm_cleanup_module();
  
}

module_init(zsrm_init);
module_exit(zsrm_exit);
