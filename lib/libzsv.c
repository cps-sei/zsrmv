#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "../zsrmvapi.h"

#include <unistd.h>
#include <sys/syscall.h>

pthread_mutex_t handler_table_lock = PTHREAD_MUTEX_INITIALIZER;
int handler_table_initialized = 1;

struct threaded_signal_handler_table_t *tsignal_handler_head = NULL;

struct threaded_signal_handler_table_t *get_threaded_signal_handler_record(int rid)
{
  struct threaded_signal_handler_table_t *tmp=tsignal_handler_head;
  
  if (!handler_table_initialized){
    handler_table_initialized=1;
    if (pthread_mutex_init(&handler_table_lock,NULL)){
      printf("could not initialized the mutex\n");
      return NULL;
    }
  }
  
  if (tmp == NULL)
    return NULL;
  
  if (pthread_mutex_lock(&handler_table_lock)){
    printf("error locking mutex\n");
  }

  while( tmp->next != tsignal_handler_head && tmp->rid != rid){
    tmp = tmp->next;
  }

  if (tmp->rid == rid){
    if (pthread_mutex_unlock(&handler_table_lock)){
      printf("error unlocking mutex\n");
    }
    return tmp;
  } else {
    if (pthread_mutex_unlock(&handler_table_lock)){
      printf("error unlocking mutex\n");
    }
    return NULL;
  }
}

int add_threaded_signal_handler_record(struct threaded_signal_handler_table_t *rec)
{
  if (!handler_table_initialized){
    handler_table_initialized=1;
    if (pthread_mutex_init(&handler_table_lock,NULL)){
      printf("could not initialized the mutex\n");
      return -1;
    }
  }

  if (pthread_mutex_lock(&handler_table_lock)){
    printf("error locking mutex\n");
  }
    
  if (tsignal_handler_head == NULL){
    tsignal_handler_head = rec;
    rec->next = rec;
    rec->prev = rec;
  } else {
    rec->next = tsignal_handler_head->next;
    tsignal_handler_head->next->prev = rec;
    rec->prev = tsignal_handler_head;
    tsignal_handler_head->next = rec;
    tsignal_handler_head = rec;
  }

  if (pthread_mutex_unlock(&handler_table_lock)){
    printf("error unlocking mutex\n");
  }
  
  return 0;
}

int del_thraded_signal_handler_record(struct threaded_signal_handler_table_t *rec)
{
  if (!handler_table_initialized){
    handler_table_initialized=1;
    if (pthread_mutex_init(&handler_table_lock,NULL)){
      printf("could not initialized the mutex\n");
      return -1;
    }
  }

  if (pthread_mutex_lock(&handler_table_lock)){
    printf("error locking mutex\n");
  }

  if (rec->next == rec){
    tsignal_handler_head = NULL;
  } else {
    if (tsignal_handler_head == rec){
      tsignal_handler_head = tsignal_handler_head->next;
    }
    rec->next->prev = rec->prev;
    rec->prev->next = rec->next;
  }

  rec->next = NULL;
  rec->prev = NULL;

  if (pthread_mutex_unlock(&handler_table_lock)){
    printf("error unlocking mutex\n");
  }

  return 0;
}


// at frequency: 1500000 
// #define IN_LOOP_ONE_MS 213788
// Surface Book i7 at 2.1 GHz
#define IN_LOOP_ONE_MS 289451
#define NUM_IN_LOOPS_ONE_MS 1
#define NUM_SILENT_ITERATIONS 10000
//#define NUM_SILENT_ITERATIONS 100

unsigned long long get_now_ns(void)
{
  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC,&now);
  return ((now.tv_sec * 1000000000L)+now.tv_nsec);
}

/*********************************************************************/
/*@requires fp1 && fp2 && fp31 && fp32 && zsrm1 && zsrm2 && zsrm3 && zsrm4;
  @assigns \nothing;
  @ensures fp1;
  @ensures fp2;
  @ensures fp31 && fp32 && zsrm1 && zsrm2 && zsrm3 && zsrm4;
*/
//SC: TBD
/*********************************************************************/
void busy_timestamped(long millis, unsigned long long tsbuffer[], 
                      long bufsize, long *bufidx)
{
  //#if 0 // DD: commented out
  long i,j,k,s;  
  s=0;
  /*@loop assigns i,j,k,s,bufidx[0];*/
  for (k=0;k<millis;k++)
    /*@loop assigns i,j,s,bufidx[0];*/
    for (i=0;i<NUM_IN_LOOPS_ONE_MS;i++)
      /*@loop assigns j,s,bufidx[0];*/
      for (j=0;j<IN_LOOP_ONE_MS;j++){
        s++;
	if (s >= NUM_SILENT_ITERATIONS){
	  s=0;
	  if (*bufidx < bufsize){
	    tsbuffer[(*bufidx)++] = get_now_ns(); // DD: was stac_time();
	  }
	}
      }
  //#endif // DD: Commented out
}

/*********************************************************************/
/*@requires fp1 && fp2 && fp31 && fp32 && zsrm1 && zsrm2 && zsrm3 && zsrm4;
  @assigns reserve_table[0..9];
  @ensures fp1;
  @ensures fp2;
  @ensures fp31 && fp32 && zsrm1 && zsrm2 && zsrm3 && zsrm4;
*/
/*********************************************************************/
int zsv_create_reserve(int schedfd, long period_sec, long period_nsec,
		       long zsinstant_sec, long zsinstant_nsec,
		       long exec_sec, long exec_nsec,
		       long nominal_exec_sec, long nominal_exec_nsec,
		       int priority, int criticality)
{
  struct api_call call;
  int ret;

  call.cmd = CREATE_RSV;
  call.period_sec = period_sec;
  call.period_nsec = period_nsec;
  call.zsinstant_sec = zsinstant_sec;
  call.zsinstant_nsec = zsinstant_nsec;
  call.exec_sec = exec_sec;
  call.exec_nsec = exec_nsec;
  call.nominal_exec_sec = nominal_exec_sec;
  call.nominal_exec_nsec = nominal_exec_nsec;
  call.priority=priority; // larger number = higher priority
  call.criticality = criticality; // larger number = higher criticality
  ret = write(schedfd, &call, sizeof(call));
  return ret;
}

/*********************************************************************/
/*@requires fp1 && fp2 && fp31 && fp32 && zsrm1 && zsrm2 && zsrm3 && zsrm4;
  @assigns reserve_table[0..9];
  @ensures fp1;
  @ensures fp2;
  @ensures fp31 && fp32 && zsrm1 && zsrm2 && zsrm3 && zsrm4;
*/
/*********************************************************************/
int zsv_attach_reserve(int schedfd, int pid, int rid)
{
  struct api_call call;
  int ret;

  call.cmd = ATTACH_RSV;
  call.pid = pid;
  call.rid = rid;
  ret = write(schedfd, &call, sizeof(call));
  return ret;
}

int zsv_get_wcet_ns(int schedfd, int rid, unsigned long long *pwcet)
{
  struct api_call call;
  int ret;

  call.cmd = GET_WCET_NS;
  call.rid = rid;
  call.pwcet = pwcet;
  ret = write(schedfd, &call, sizeof(call));
  return ret;
}

int zsv_get_trace_size(int schedfd)
{
  struct api_call call;
  int ret;

  call.cmd = GET_TRACE_SIZE;
  ret = write(schedfd, &call, sizeof(call));
  return ret;
}

int zsv_get_acet_ns(int schedfd, int rid, unsigned long long *pacet)
{
  struct api_call call;
  int ret;

  call.cmd = GET_ACET_NS;
  call.rid = rid;
  call.pwcet = pacet;
  ret = write(schedfd, &call, sizeof(call));
  return ret;
}

/*********************************************************************/
/*@requires fp1 && fp2 && fp31 && fp32 && zsrm1 && zsrm2 && zsrm3 && zsrm4;
  @assigns reserve_table[0..9];
  @ensures fp1;
  @ensures fp2;
  @ensures fp31 && fp32 && zsrm1 && zsrm2 && zsrm3 && zsrm4;
*/
/*********************************************************************/
int zsv_wait_period(int schedfd, int rid)
{
  struct api_call call;
  int ret;

  call.cmd = WAIT_PERIOD;
  call.rid = rid;
  ret =  write(schedfd, &call, sizeof(call));
  return ret;
}

int zsv_wait_release(int schedfd, int rid)
{
  struct api_call call;
  int ret;

  call.cmd = WAIT_RELEASE;
  call.rid = rid;
  ret = write(schedfd, &call, sizeof(call));
  return ret;
}

int zsv_end_period(int schedfd, int rid)
{
  struct api_call call;
  int ret;

  call.cmd = END_PERIOD;
  call.rid = rid;
  ret = write(schedfd, &call, sizeof(call));
  return ret;
}

int zsv_nowait_period(int schedfd, int rid)
{
  struct api_call call;
  int ret;

  call.cmd = NOWAIT_PERIOD;
  call.rid = rid;
  ret =  write(schedfd, &call, sizeof(call));
  return ret;
}

/*********************************************************************/
/*@requires fp1 && fp2 && fp31 && fp32 && zsrm1 && zsrm2 && zsrm3 && zsrm4;
  @assigns reserve_table[0..9];
  @ensures fp1;
  @ensures fp2;
  @ensures fp31 && fp32 && zsrm1 && zsrm2 && zsrm3 && zsrm4;
*/
/*********************************************************************/
int zsv_delete_reserve(int schedfd, int rid)
{
  struct api_call call;
  int ret;

  call.cmd=DELETE_RSV;
  call.rid = rid;
  ret =  write(schedfd, &call, sizeof(call));
  return ret;
}

int zsv_open_scheduler(void){
  int fd;

  fd = open("/dev/zsrmmv0",O_RDWR);
  
  return fd;
}

int zsv_close_scheduler(int fd){
  return close(fd);
}

int zsv_capture_budget_enforcement_signal(int schedfd, int rid, int pid, int signo)
{
  struct api_call call;
  int ret;

  call.cmd = CAPTURE_ENFORCEMENT_SIGNAL;
  call.pid = pid;
  call.rid = rid;
  call.priority = signo;
  ret = write(schedfd, &call, sizeof(call));
  return ret;
  
}

//static void (*zsv_user_enforcement_handler)(void *p, int) = NULL;

//static void *zsv_user_enforcement_handler_param = NULL;
//static int zsv_enforcement_schedfd;

void zsv_user_enforcement_handler_wrapper(int signo, siginfo_t *siginfo, void *ptr)
{
  int bundle_rid_stop_periodic  = siginfo->si_value.sival_int;
  int rid;
  int request_stop;
  int periodic;
  struct threaded_signal_handler_table_t *rec = NULL;

  rid = EXTRACT_RID(bundle_rid_stop_periodic);
  request_stop = EXTRACT_STOP(bundle_rid_stop_periodic);
  periodic = EXTRACT_PERIODIC(bundle_rid_stop_periodic);
  
  rec = get_threaded_signal_handler_record(rid);
  
  if (rec  != NULL){
    rec->zsv_user_enforcement_handler(rec->zsv_user_enforcement_handler_param, rid);
    if (request_stop){
      if (periodic){
	zsv_wait_period(rec->zsv_enforcement_schedfd,rid);
      } else {
	zsv_wait_release(rec->zsv_enforcement_schedfd,rid);
      }
    }
  } else {
    printf("ZSRMlib.zsv_user_enforcement_handler_wrapper() ERROR no signal handler record -- NOT CALLING WFNP\n");
  }
}

int zsv_fork_enforcement_handler(int schedfd, int rid, void (*handler)(void *,int), void *param)
{
  struct threaded_signal_handler_table_t *rec = NULL;
  struct sigaction sa;
  struct zsv_enf_arg *penfarg;
  int retval;

  rec = get_threaded_signal_handler_record(rid);
  if (rec == NULL){
    rec = malloc(sizeof(struct threaded_signal_handler_table_t));
    if (rec == NULL){
      printf("error allocaing signal handler record\n");
      return -1;
    }
    rec->rid = rid;
    add_threaded_signal_handler_record(rec);
  }
  rec->zsv_user_enforcement_handler = handler;
  rec->zsv_user_enforcement_handler_param = param;
  rec->zsv_enforcement_schedfd = schedfd;
  
  /* zsv_user_enforcement_handler = handler; */
  /* zsv_user_enforcement_handler_param = param; */
  /* zsv_enforcement_schedfd = schedfd; */
  sa.sa_sigaction = zsv_user_enforcement_handler_wrapper;
  sa.sa_flags = SA_RESTART;
  sigfillset(&sa.sa_mask);
  sa.sa_flags |= SA_SIGINFO;

  if (sigaction(ZS_BUDGET_ENFORCEMENT_SIGNAL, &sa, NULL) <0){
    return -1;
  }

  if ((retval = zsv_capture_budget_enforcement_signal(schedfd, rid, gettid(),ZS_BUDGET_ENFORCEMENT_SIGNAL))<0){
    printf("Error (%d) capturing budget enforcement signal for rid(%d)\n",retval,rid);
    return -1;
  }

  return 0;
}

int zsv_stop_enforcement_handler(int rid)
{
  struct threaded_signal_handler_table_t *rec = NULL;
  struct sigaction sa;
  int retval;

  sa.sa_handler = SIG_DFL;
  sa.sa_flags = SA_RESTART;
  sigfillset(&sa.sa_mask);

  if (sigaction(ZS_BUDGET_ENFORCEMENT_SIGNAL, &sa, NULL) <0){
    return -1;
  }

  rec = get_threaded_signal_handler_record(rid);
  if (rec != NULL){
    if((retval = zsv_capture_budget_enforcement_signal(rec->zsv_enforcement_schedfd, rid, -1 ,ZS_BUDGET_ENFORCEMENT_SIGNAL))<0){
      printf("Error (%d) capturing budget enforcement signal for rid(%d)\n",retval,rid);
    }
    free(rec);
  }

  return 0;
}

int zsv_write_trace(int schedfd, FILE *fid)
{
  struct trace_rec_t *tracep;
  int size = zsv_get_trace_size(schedfd);
  int traces_read=0;
  int i;

  if (size >0){
    tracep = malloc(sizeof(struct trace_rec_t)*size);
    if (tracep == NULL){
      printf("zsv_write_trace(): could not allocate memory");
      return -1;
    }

    traces_read = read(schedfd,tracep,size);

    if (traces_read>0){
      for (i=0;i<traces_read;i++){
	fprintf(fid,"%d %llu %d\n",tracep[i].rid, tracep[i].timestamp_ns, tracep[i].event_type);
      }
    }
    
    free(tracep);
  }
}