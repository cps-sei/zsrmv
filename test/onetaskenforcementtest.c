#include <stdio.h>
#define __USE_GNU
#include <sched.h>
#undef __USE_GNU
#include <time.h>
#include <sys/types.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h> 
#include <errno.h>
#include <sys/sem.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include <zsrmvapi.h>

struct trace_rec_t trace[1000];

#define TS_BUFFER_SIZE 100000

unsigned long long timestamp1[TS_BUFFER_SIZE];
long tsindex1=0;

unsigned long long start_timestamp_ns;

int enforced=0;

void enforcement_handler(void* p, int rid)
{
  enforced = 1;
  printf("enforcement\n");
}

int main(int argc, char *argv[])
{
  int rid;
  int i;
  unsigned long long wcet;
  long l;
  int schedfd;
  int enffd;
  int r,cnt;

  if ((schedfd = zsv_open_scheduler())<0){
    printf("Error opening the scheduler");
    return -1;
  }

  if ((rid = zsv_create_reserve(schedfd,
				1, // period_secs
				0, // period_nsecs
				1, // zsinstant_sec -- same as period = disabled
				0, // zsinstant_nsec -- same as period = disabled
				0, // exectime _secs
				300000000, // exectime_nsecs
				0, // nominal_exectime_sec -- same as overloaded
				300000000, // nominal_exectime_nsec -- same as overloaded
				10, // priority
				1  // criticality
				) 
       )<0){
    printf("error calling create reserve\n");
    return -1;
  }

  if ((enffd = zsv_fork_enforcement_handler(schedfd, rid, enforcement_handler, NULL)) <0 ){
    printf("Could not setup enforcement handler\n");
    return -1;
  }
  
  start_timestamp_ns = get_now_ns();
  
  if (zsv_attach_reserve(schedfd, getpid(), rid)<0){
    printf("error calling attach reserve\n");
    return -1;
  }
  
  printf("task attached and ready\n");

  for (i=0;i<10;i++){
    if (i == 5){
      busy_timestamped(700, timestamp1, TS_BUFFER_SIZE, &tsindex1);
    } else if (i == 7) {
      busy_timestamped(3000, timestamp1, TS_BUFFER_SIZE, &tsindex1);
    } else {
      busy_timestamped(150, timestamp1, TS_BUFFER_SIZE, &tsindex1);
    }
    if (enforced){
      printf("USER: enforced -- should not complete?\n");
      enforced = 0;
    }
    printf("wfnp\n");
    zsv_wait_period(schedfd,rid);
  }

  zsv_get_wcet_ns(schedfd,rid, &wcet);
  printf("WCET: %llu ns \n",wcet);

  //if (zsv_stop_enforcement_handler(enffd)<0){
  if (zsv_stop_enforcement_handler(rid)<0){
    printf("error stopping enforcement handler\n");
  }

  zsv_delete_reserve(schedfd,rid);

  r = read(schedfd,trace,1000);

  FILE *fid = fopen("plot.csv","w+");
  if (fid == NULL){
    printf("could not open plot file\n");
  } else {  
    if (r<0){
      printf("error\n");
    } else {
      cnt = r / sizeof(struct trace_rec_t);
      for (i=0;i<cnt;i++){
	fprintf(fid,"%d, %lld, %d\n",
		trace[i].rid,
		trace[i].timestamp_ns,
		trace[i].event_type);
      }
    }
    fclose(fid);
  }

  
  zsv_close_scheduler(schedfd);

  FILE *plotfd = fopen("ts-task.txt","w+");
  if (plotfd  == NULL){
    printf("error opening ts-task.txt\n");
    return -1;
  }

  for (l=0; l < tsindex1; l++){
    fprintf(plotfd,"%llu 1\n",(timestamp1[l]-start_timestamp_ns)/1000000);
  }

  fclose(plotfd);
  
  printf("DONE\n");

  return 0;
}
