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
#include <pthread.h>

#include <zsrmvapi.h>

pthread_cond_t condA  = PTHREAD_COND_INITIALIZER;
struct trace_rec_t trace[1000];

#define TS_BUFFER_SIZE 100000

unsigned long long timestamp1[TS_BUFFER_SIZE];
long tsindex1=0;

unsigned long long timestamp2[TS_BUFFER_SIZE];
long tsindex2=0;

unsigned long long start_timestamp_ns;

int enforced1=0;

void enforcement_handler1(void* p, int rid)
{
  enforced1 = 1;
  printf("enforcement 1 rid(%d) tid(%ld)\n",rid,gettid());
}

int enforced2=0;

void enforcement_handler2(void* p, int rid)
{
  enforced2 = 1;
  printf("enforcement 2 rid(%d) tid(%ld)\n",rid,gettid());
}


void *thread1_main( void *ptr )
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
    return NULL;
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
    return NULL;
  }

  if (zsv_fork_enforcement_handler(schedfd, rid, enforcement_handler1, NULL) <0 ){
    printf("Could not setup enforcement handler\n");
    return NULL;
  }
  
  if (zsv_attach_reserve(schedfd, gettid(), rid)<0){
    printf("error calling attach reserve\n");
    return NULL;
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
    if (enforced1){
      printf("thread1: enforced -- should not complete?\n");
      enforced1 = 0;
    }
    printf("wfnp 1\n");
    zsv_wait_period(schedfd,rid);
  }

  sleep(30);
  
  zsv_get_wcet_ns(schedfd,rid, &wcet);
  printf("thread1 WCET: %llu ns \n",wcet);

  //if (zsv_stop_enforcement_handler(enffd)<0){
  if (zsv_stop_enforcement_handler(rid)<0){
    printf("error stopping enforcement handler\n");
  }

  zsv_delete_reserve(schedfd,rid);
  zsv_close_scheduler(schedfd);

  r = read(schedfd,trace,1000);

  if (r<0){
    printf("error\n");
  } else {
    cnt = r / sizeof(struct trace_rec_t);
    for (i=0;i<cnt;i++){
      printf("rid(%d), time ns(%lld), type(%d)\n",
	     trace[i].rid,
	     trace[i].timestamp_ns,
	     trace[i].event_type);
    }
  }

  
  FILE *fid = fopen("ts-task1.txt","w+");
  if (fid == NULL){
    printf("error opening ts-task1.txt\n");
    return NULL;
  }

  for (l=0; l < tsindex1; l++){
    fprintf(fid,"%llu 1\n",(timestamp1[l]-start_timestamp_ns)/1000000);
  }

  fclose(fid);
}

void *thread2_main( void *ptr )
{
  int rid;
  int i;
  unsigned long long wcet;
  long l;
  int schedfd;
  int enffd;

  if ((schedfd = zsv_open_scheduler())<0){
    printf("Error opening the scheduler");
    return NULL;
  }

  if ((rid = zsv_create_reserve(schedfd,
				2, // period_secs
				0, // period_nsecs
				2, // zsinstant_sec -- same as period = disabled
				0, // zsinstant_nsec -- same as period = disabled
				0, // exectime _secs
				300000000, // exectime_nsecs
				0, // nominal_exectime_sec -- same as overloaded
				300000000, // nominal_exectime_nsec -- same as overloaded
				9, // priority
				1  // criticality
				) 
       )<0){
    printf("error calling create reserve\n");
    return NULL;
  }

  if (zsv_fork_enforcement_handler(schedfd, rid, enforcement_handler2, NULL) <0 ){
    printf("Could not setup enforcement handler\n");
    return NULL;
  }
  
  if (zsv_attach_reserve(schedfd, gettid(), rid)<0){
    printf("error calling attach reserve\n");
    return NULL;
  }
  
  printf("task attached and ready\n");

  for (i=0;i<10;i++){
    if (i == 5){
      busy_timestamped(700, timestamp2, TS_BUFFER_SIZE, &tsindex2);
    } else if (i == 7) {
      busy_timestamped(3000, timestamp2, TS_BUFFER_SIZE, &tsindex2);
    } else {
      busy_timestamped(150, timestamp2, TS_BUFFER_SIZE, &tsindex2);
    }
    if (enforced2){
      printf("thread2: enforced -- should not complete?\n");
      enforced2 = 0;
    }
    printf("wfnp 2\n");
    zsv_wait_period(schedfd,rid);
  }

  zsv_get_wcet_ns(schedfd,rid, &wcet);
  printf("thread2 WCET: %llu ns \n",wcet);

  //if (zsv_stop_enforcement_handler(enffd)<0){
  if (zsv_stop_enforcement_handler(rid)<0){
    printf("error stopping enforcement handler\n");
  }

  zsv_delete_reserve(schedfd,rid);
  zsv_close_scheduler(schedfd);

  FILE *fid = fopen("ts-task2.txt","w+");
  if (fid == NULL){
    printf("error opening ts-task2.txt\n");
    return NULL;
  }

  for (l=0; l < tsindex2; l++){
    fprintf(fid,"%llu 1\n",(timestamp2[l]-start_timestamp_ns)/1000000);
  }

  fclose(fid);

}

int main(int argc, char *argv[])
{
  pthread_t thread1, thread2;
  int schedfd;
  int r,i,cnt;

  if ((schedfd = zsv_open_scheduler())<0){
    printf("Error opening the scheduler");
    return -1;
  }


  start_timestamp_ns = get_now_ns();

  if (pthread_create( &thread1, NULL, thread1_main, NULL)){
    printf("Could not create thread1\n");
    exit(1);
  }

  if (pthread_create( &thread2, NULL, thread2_main, NULL)){
    printf("Could not create thread1\n");
    exit(1);
  }

  pthread_join(thread1,NULL);
  pthread_join(thread2,NULL);


  
  zsv_close_scheduler(schedfd);
  
  printf("DONE\n");

  return 0;
}
