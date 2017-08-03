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

#include "../zsrmv.h"
#include "../zsrmvapi.h"
#include "speed_params.h"


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
//#define IN_LOOP_ONE_MS 289451
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


int zsv_is_admissible(struct reserve_spec_t *reserves_specs_table, int tablesize)
{
  struct reserve *rtable;
  int i,j,t;
  int *dec_crit_indices;
  int admissible;
  unsigned long long Z;

  // allocate resources
  dec_crit_indices = malloc(sizeof(int)*tablesize);
  if (dec_crit_indices == NULL){
    printf("failed to allocate memory for reordering table\n");
    return -1;
  }

  rtable = malloc(sizeof(struct reserve)*tablesize);
  if (rtable == NULL){
    printf("failed to allocate temporal reserve table\n");
    return -2;
  }

  // create decreasing criticality order

  // create direct mapping first
  for (i=0;i<tablesize;i++){
    dec_crit_indices[i]=i;
  }

  // indirect buble sort
  for (i=0;i<tablesize;i++){
    for (j=i+1;j<tablesize;j++){
      if (reserves_specs_table[dec_crit_indices[i]].criticality <reserves_specs_table[dec_crit_indices[j]].criticality){
	t=dec_crit_indices[i];
	dec_crit_indices[i]=dec_crit_indices[j];
	dec_crit_indices[j]=t;
      }
    }
  }

  // copy table
  for (i=0;i<tablesize;i++){
    rtable[i].period_ns=reserves_specs_table[dec_crit_indices[i]].period_sec * 1000000000L +
      reserves_specs_table[dec_crit_indices[i]].period_nsec;
    rtable[i].exectime_ns = reserves_specs_table[dec_crit_indices[i]].exec_sec * 1000000000L +
      reserves_specs_table[dec_crit_indices[i]].exec_nsec;
    rtable[i].nominal_exectime_ns=reserves_specs_table[dec_crit_indices[i]].nominal_exec_sec * 1000000000L +
      reserves_specs_table[dec_crit_indices[i]].nominal_exec_nsec;
    rtable[i].exectime_in_rm_ns=0;
    rtable[i].pid=0;
    rtable[i].criticality=reserves_specs_table[dec_crit_indices[i]].criticality;
  }

  // try admission
  admissible=1;
  for (i=0;i<tablesize && admissible ;i++){
    admissible = admissible &&
      admit(rtable,tablesize,&rtable[i],&Z);
    if (admissible){
      reserves_specs_table[dec_crit_indices[i]].zsinstant_sec = (long)(Z / 1000000000L);
      reserves_specs_table[dec_crit_indices[i]].zsinstant_nsec = (long)(Z % 1000000000L);
    }
  }

  // free resources
  free(dec_crit_indices);
  free(rtable);
  
  return admissible;
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
