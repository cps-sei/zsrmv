/*
Copyright (c) 2014 Carnegie Mellon University.

All Rights Reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the 
following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following 
acknowledgments and disclaimers.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following 
acknowledgments and disclaimers in the documentation and/or other materials provided with the distribution.
3. Products derived from this software may not include “Carnegie Mellon University,” "SEI” and/or “Software 
Engineering Institute" in the name of such derived product, nor shall “Carnegie Mellon University,” "SEI” and/or 
“Software Engineering Institute" be used to endorse or promote products derived from this software without prior 
written permission. For written permission, please contact permission@sei.cmu.edu.

ACKNOWLEDMENTS AND DISCLAIMERS:
Copyright 2014 Carnegie Mellon University
This material is based upon work funded and supported by the Department of Defense under Contract No. FA8721-
05-C-0003 with Carnegie Mellon University for the operation of the Software Engineering Institute, a federally 
funded research and development center.

Any opinions, findings and conclusions or recommendations expressed in this material are those of the author(s) and 
do not necessarily reflect the views of the United States Department of Defense.

NO WARRANTY. 
THIS CARNEGIE MELLON UNIVERSITY AND SOFTWARE ENGINEERING INSTITUTE 
MATERIAL IS FURNISHED ON AN “AS-IS” BASIS. CARNEGIE MELLON UNIVERSITY MAKES NO 
WARRANTIES OF ANY KIND, EITHER EXPRESSED OR IMPLIED, AS TO ANY MATTER INCLUDING, 
BUT NOT LIMITED TO, WARRANTY OF FITNESS FOR PURPOSE OR MERCHANTABILITY, 
EXCLUSIVITY, OR RESULTS OBTAINED FROM USE OF THE MATERIAL. CARNEGIE MELLON 
UNIVERSITY DOES NOT MAKE ANY WARRANTY OF ANY KIND WITH RESPECT TO FREEDOM FROM 
PATENT, TRADEMARK, OR COPYRIGHT INFRINGEMENT.

This material has been approved for public release and unlimited distribution.
Carnegie Mellon® is registered in the U.S. Patent and Trademark Office by Carnegie Mellon University.

DM-0000891
*/

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
  int pid;

  if ((schedfd = zsv_open_scheduler())<0){
    printf("Error opening the scheduler");
    return NULL;
  }

  pid = gettid();
  
  printf("main1.pid(%d) before create reserve\n",pid);
  if ((rid = zsv_create_reserve(schedfd,
				1, // period_secs
				0, // period_nsecs
				1, // zsinstant_sec -- same as period = disabled
				0, // zsinstant_nsec -- same as period = disabled
				0, // exectime _secs
				990000000, // exectime_nsecs
				0, // nominal_exectime_sec -- same as overloaded
				990000000, // nominal_exectime_nsec -- same as overloaded
				10, // priority
				1  // criticality
				) 
       )<0){
    printf("error calling create reserve\n");
    return NULL;
  }
  printf("main1.pid(%d) after create rid(%d). about to fork handler \n",pid,rid);

  if (zsv_fork_enforcement_handler(schedfd, rid, enforcement_handler1, NULL) <0 ){
    printf("Could not setup enforcement handler\n");
    return NULL;
  }

  printf("main1.pid(%d) after fork about to attach rid(%d)\n",pid,rid);
  
  if (zsv_attach_reserve(schedfd, gettid(), rid)<0){
    printf("***** ERROR calling attach reserve\n");
    return NULL;
  }
  
  printf("main1.pid(%d) rid(%d) attached and ready\n",pid,rid);

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
    printf("wfnp 1 pid(%d) rid(%d)\n",pid,rid);
    zsv_wait_period(schedfd,rid);
  }

  sleep(30);
  
  zsv_get_wcet_ns(schedfd,rid, &wcet);
  printf("main1.pid(%d) rid(%d)  WCET: %llu ns \n",pid,rid,wcet);

  if (zsv_stop_enforcement_handler(rid)<0){
    printf("error stopping enforcement handler\n");
  }

  zsv_delete_reserve(schedfd,rid);
  zsv_close_scheduler(schedfd);

  
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
  int pid;

  pid = gettid();


  if ((schedfd = zsv_open_scheduler())<0){
    printf("Error opening the scheduler");
    return NULL;
  }

  printf("main2.pid(%d) before create \n",pid);

  if ((rid = zsv_create_reserve(schedfd,
				2, // period_secs
				0, // period_nsecs
				2, // zsinstant_sec -- same as period = disabled
				0, // zsinstant_nsec -- same as period = disabled
				1, // exectime _secs
				700000000, // exectime_nsecs
				1, // nominal_exectime_sec -- same as overloaded
				700000000, // nominal_exectime_nsec -- same as overloaded
				9, // priority
				1  // criticality
				) 
       )<0){
    printf("error calling create reserve\n");
    return NULL;
  }

  printf("main2.pid(%d) rid(%d) after create before fork \n",pid,rid);

  if (zsv_fork_enforcement_handler(schedfd, rid, enforcement_handler2, NULL) <0 ){
    printf("Could not setup enforcement handler\n");
    return NULL;
  }

  printf("main2.pid(%d) rid(%d) after fork before attach \n",pid,rid);

  if (zsv_attach_reserve(schedfd, gettid(), rid)<0){
    printf("**** ERROR calling attach reserve\n");
    return NULL;
  }
  
  printf("main2.pid(%d) rid(%d)  attached and ready\n",pid,rid);

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
    printf("wfnp 2 pid(%d) rid(%d) \n",pid,rid);
    zsv_wait_period(schedfd,rid);
  }

  zsv_get_wcet_ns(schedfd,rid, &wcet);
  printf("thread2 pid(%d) rid(%d) WCET: %llu ns \n",pid,rid, wcet);

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
  int parent;

  if ((schedfd = zsv_open_scheduler())<0){
    printf("Error opening the scheduler");
    return -1;
  }


  start_timestamp_ns = get_now_ns();

  parent = fork();

  if (parent <0){
    printf("could not fork\n");
    return -1;
  }

  if (parent){
    thread1_main(NULL);
    wait(&r);
  } else {
    parent = fork();
    if (parent){
      thread2_main(NULL);
      wait(&r);
    } else {
      parent = fork();
      if (parent){
	thread2_main(NULL);
	wait(&r);
      } else{
	parent = fork();
	if (parent){
	  thread2_main(NULL);
	  wait(&r);
	}else{
	  thread2_main(NULL);
	  return 0;
	}
	return 0;
      }
      return 0;
    }
    return 0;
  }

  /* if (pthread_create( &thread1, NULL, thread1_main, NULL)){ */
  /*   printf("Could not create thread1\n"); */
  /*   exit(1); */
  /* } */

  /* if (pthread_create( &thread2, NULL, thread2_main, NULL)){ */
  /*   printf("Could not create thread1\n"); */
  /*   exit(1); */
  /* } */

  /* pthread_join(thread1,NULL); */
  /* pthread_join(thread2,NULL); */
  
  r = read(schedfd,trace,1000*sizeof(struct trace_rec_t));

  if (r<0){
    printf("error\n");
  } else {
    FILE *ofid = fopen("five-plot.csv","w+");
    if (ofid == NULL){
      printf("Error opening output file\n");
      return -1;
    }
    cnt = r / sizeof(struct trace_rec_t);
    for (i=0;i<cnt;i++){
      fprintf(ofid, "%d %lld %d\n",
  	     trace[i].rid,
  	     trace[i].timestamp_ns,
  	     trace[i].event_type);
    }
    fclose(ofid);
  }

  /* FILE *ofid = fopen("two-plot.csv","w+"); */
  /* if (ofid == NULL){ */
  /*   printf("Error opening output file\n"); */
  /*   return -1; */
  /* } */

  /* zsv_write_trace(schedfd,ofid); */
  
  /* fclose(ofid); */
  
  zsv_close_scheduler(schedfd);
  
  printf("DONE\n");

  return 0;
}
