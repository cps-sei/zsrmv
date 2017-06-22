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

#include <zsrmvapi.h>

#define TS_BUFFER_SIZE 100000

unsigned long long timestamp1[TS_BUFFER_SIZE];
long tsindex1=0;

unsigned long long start_timestamp_ns;



int main(int argc, char *argv[])
{
  int rid;
  int i;
  unsigned long long wcet;
  long l;
  int schedfd;
  int parent;

  if ((schedfd = zsv_open_scheduler())<0){
    printf("Error opening the scheduler");
    return -1;
  }

  // Duplicate --
  parent = fork();
  
  if ((rid = zsv_create_reserve(schedfd,
				1, // period_secs
				0, // period_nsecs
				100, // zsinstant_sec -- same as period = disabled
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

  start_timestamp_ns = get_now_ns();
  
  if (zsv_attach_reserve(schedfd, getpid(), rid)<0){
    printf("error calling attach reserve\n");
    return -1;
  }
  
  printf("task attached and ready\n");

  for (i=0;i<10;i++){
    busy_timestamped(150, timestamp1, TS_BUFFER_SIZE, &tsindex1);
    if (parent){
      if (i ==5){
	printf("waiting 10 secs\n");
	sleep(10);
      }
    } else {
      if (i==7){
	printf("waiting 5 secs\n");
	sleep(5);
      }
    }
    printf("wfnp\n");
    zsv_wait_period(schedfd,rid);
  }

  zsv_get_wcet_ns(schedfd,rid, &wcet);

  printf("WCET: %llu ns \n",wcet);

  zsv_delete_reserve(schedfd,rid);
  zsv_close_scheduler(schedfd);

  FILE *fid = fopen("ts-task.txt","w+");
  if (fid == NULL){
    printf("error opening ts-task.txt\n");
    return -1;
  }

  for (l=0; l < tsindex1; l++){
    fprintf(fid,"%llu 1\n",(timestamp1[l]-start_timestamp_ns)/1000000);
  }

  fclose(fid);
  
  printf("DONE\n");

  return 0;
}
