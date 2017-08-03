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

struct rsvparam_t {
  long psec; // period_secs
  long pnsec;// period_nsecs
  long zsec; // zsinstant_sec -- same as period = disabled
  long znsec; // zsinstant_nsec -- same as period = disabled
  long oesec; // exectime _secs
  long oensec; // exectime_nsecs
  long nesec; // nominal_exectime_sec -- same as overloaded
  long nensec; // nominal_exectime_nsec -- same as overloaded
  int prio; // priority
  int crit; // criticality
  int rid;
  unsigned long long *tsbuf;
  long *tsidx;
};

#define TS_BUFFER_SIZE 100000
unsigned long long timestamp1[TS_BUFFER_SIZE];
long tsindex1=0;
unsigned long long timestamp2[TS_BUFFER_SIZE];
long tsindex2=0;
unsigned long long timestamp3[TS_BUFFER_SIZE];
long tsindex3=0;
unsigned long long timestamp4[TS_BUFFER_SIZE];
long tsindex4=0;
unsigned long long timestamp5[TS_BUFFER_SIZE];
long tsindex5=0;

#define TRACE_BUF_SIZE 100000
struct trace_rec_t trace[TRACE_BUF_SIZE];

unsigned long long start_timestamp_ns;

int venforced[10];

void enforcement_handler(void* p, int rid)
{
  venforced[rid]=1;
  printf("enforcement 1 rid(%d) tid(%ld)\n",rid,gettid());
}

void *thread_main( void *ptr )
{
  int rid;
  int i;
  unsigned long long wcet;
  long l;
  int schedfd;
  int enffd;
  int r,cnt;
  int pid;
  unsigned long wait_usleep;

  struct rsvparam_t *rsvp = (struct rsvparam_t *) ptr;

  wait_usleep = rsvp->pnsec / 1000;
  
  if ((schedfd = zsv_open_scheduler())<0){
    printf("Error opening the scheduler");
    return NULL;
  }

  pid = gettid();
  
  printf("main.pid(%d) before create reserve\n",pid);
  if ((rid = zsv_create_reserve(schedfd,
				rsvp->psec, // period_secs
				rsvp->pnsec, // period_nsecs
				rsvp->zsec, // zsinstant_sec -- same as period = disabled
				rsvp->znsec, // zsinstant_nsec -- same as period = disabled
				rsvp->oesec, // exectime _secs
				rsvp->oensec, // exectime_nsecs
				rsvp->nesec, // nominal_exectime_sec -- same as overloaded
				rsvp->nensec, // nominal_exectime_nsec -- same as overloaded
				rsvp->prio, // priority
				rsvp->crit  // criticality
				) 
       )<0){
    printf("error calling create reserve\n");
    return NULL;
  }
  rsvp->rid = rid;
  printf("main.pid(%d) after create rid(%d). about to fork handler \n",pid,rid);

  if (zsv_fork_enforcement_handler(schedfd, rid, enforcement_handler, NULL) <0 ){
    printf("Could not setup enforcement handler\n");
    return NULL;
  }

  printf("main.pid(%d) after fork about to attach rid(%d)\n",pid,rid);

  venforced[rid] = 0;
  
  if (zsv_attach_reserve(schedfd, gettid(), rid)<0){
    printf("***** ERROR calling attach reserve\n");
    return NULL;
  }
  
  printf("main.pid(%d) rid(%d) attached and ready\n",pid,rid);

  for (i=0;i<10;i++){
    if (i == 5){
      busy_timestamped(700, rsvp->tsbuf, TS_BUFFER_SIZE, rsvp->tsidx);
    } else if (i == 7) {
      busy_timestamped(3000, rsvp->tsbuf, TS_BUFFER_SIZE, rsvp->tsidx);
    } else {
      busy_timestamped(150, rsvp->tsbuf, TS_BUFFER_SIZE, rsvp->tsidx);
    }
    if (venforced[rid]){
      printf("thread: enforced -- should not complete?\n");
      venforced[rid] = 0;
    }
    printf("wfnp pid(%d) rid(%d)\n",pid,rid);
    //zsv_wait_period(schedfd,rid);
    zsv_end_period(schedfd,rid);
    // simulate a wait for message
    usleep(wait_usleep);
    zsv_wait_release(schedfd,rid);
  }

  zsv_get_wcet_ns(schedfd,rid, &wcet);
  printf("main.pid(%d) rid(%d)  WCET: %llu ns \n",pid,rid,wcet);

  /* if (zsv_stop_enforcement_handler(rid)<0){ */
  /*   printf("pid(%d) rid(%d) error stopping enforcement handler\n",pid,rid); */
  /* } */

  zsv_delete_reserve(schedfd,rid);
  zsv_close_scheduler(schedfd);
  
}

void *thread1_main( void *ptr )
{
  return thread_main(ptr);
}

void *thread2_main( void *ptr )
{
  return thread_main(ptr);
}

void *thread3_main( void *ptr )
{
  return thread_main(ptr);
}

void *thread4_main( void *ptr )
{
  return thread_main(ptr);
}

void *thread5_main( void *ptr )
{
  return thread_main(ptr);
}

int main(int argc, char *argv[])
{
  pthread_t thread1, thread2,thread3,thread4,thread5;
  struct rsvparam_t par1,par2,par3,par4,par5;
  int schedfd;
  int r,i,cnt;
  int parent;
  int rid;

  if ((schedfd = zsv_open_scheduler())<0){
    printf("Error opening the scheduler");
    return -1;
  }

  par1.psec = 1;
  par1.pnsec=0;
  par1.zsec = 1;
  par1.znsec=0;
  par1.oesec=0;
  par1.oensec=200000000;
  par1.nesec=0;
  par1.nensec=200000000;
  par1.prio=1;
  par1.crit=1;
  par1.tsbuf = timestamp1;
  par1.tsidx = &tsindex1;

  par2.psec = 2;
  par2.pnsec=0;
  par2.zsec = 2;
  par2.znsec=0;
  par2.oesec=0;
  par2.oensec=300000000;
  par2.nesec=0;
  par2.nensec=300000000;
  par2.prio=1;
  par2.crit=1;
  par2.tsbuf = timestamp2;
  par2.tsidx = &tsindex2;

  par3.psec = 1;
  par3.pnsec=0;
  par3.zsec = 1;
  par3.znsec=0;
  par3.oesec=0;
  par3.oensec=200000000;
  par3.nesec=0;
  par3.nensec=200000000;
  par3.prio=1;
  par3.crit=1;
  par3.tsbuf = timestamp3;
  par3.tsidx = &tsindex3;

  par4.psec = 2;
  par4.pnsec=0;
  par4.zsec = 2;
  par4.znsec=0;
  par4.oesec=0;
  par4.oensec=300000000;
  par4.nesec=0;
  par4.nensec=300000000;
  par4.prio=1;
  par4.crit=1;
  par4.tsbuf = timestamp4;
  par4.tsidx = &tsindex4;

  par5.psec = 1;
  par5.pnsec=0;
  par5.zsec = 1;
  par5.znsec=0;
  par5.oesec=0;
  par5.oensec=200000000;
  par5.nesec=0;
  par5.nensec=200000000;
  par5.prio=1;
  par5.crit=1;
  par5.tsbuf = timestamp5;
  par5.tsidx = &tsindex5;
  
  
  start_timestamp_ns = get_now_ns();

  if (pthread_create( &thread1, NULL, thread1_main, &par1)){
    printf("Could not create thread1\n");
    exit(1);
  }
  if (pthread_create( &thread2, NULL, thread2_main, &par2)){
    printf("Could not create thread2\n");
    exit(1);
  }
  if (pthread_create( &thread3, NULL, thread3_main, &par3)){
    printf("Could not create thread3\n");
    exit(1);
  }
  if (pthread_create( &thread4, NULL, thread4_main, &par4)){
    printf("Could not create thread4\n");
    exit(1);
  }
  if (pthread_create( &thread5, NULL, thread5_main, &par5)){
    printf("Could not create thread5\n");
    exit(1);
  }
  
  pthread_join(thread1,NULL);
  pthread_join(thread2,NULL);
  pthread_join(thread3,NULL);
  pthread_join(thread4,NULL);
  pthread_join(thread5,NULL);

  for (i=0;i<5;i++){
    char fname [100];
    unsigned long long *tsbuf;
    long *tsidx;

    tsbuf = (i == 0 ? timestamp1 : i == 1? timestamp2 : i == 2 ? timestamp3 :
	     i == 3 ? timestamp4 : timestamp5) ;
    tsidx = (i == 0 ? &tsindex1 : i == 1 ? &tsindex2 : i == 2 ? &tsindex3 :
	     i == 3 ? &tsindex4 : &tsindex5);

    rid = (i==0 ? par1.rid: i==1 ? par2.rid : i== 2 ? par3.rid : i == 3 ?  par4.rid : par5.rid);
    
    sprintf(fname,"ts-task%d.txt",rid);
    FILE *fid = fopen(fname,"w+");
    if (fid == NULL){
      printf("error opening %s\n",fname);
      return -1;
    }
    
    for (long l=0; l < *tsidx; l++){
      fprintf(fid,"%llu 1\n",(tsbuf[l]-start_timestamp_ns)/1000000);
    }
    
    fclose(fid);
  }

  
  r = read(schedfd,trace,TRACE_BUF_SIZE*sizeof(struct trace_rec_t));

  if (r<0){
    printf("error\n");
  } else {
    FILE *ofid = fopen("five-threaded-plot.csv","w+");
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

  zsv_close_scheduler(schedfd);
  
  printf("DONE\n");

  return 0;
}
