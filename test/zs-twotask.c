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

#include <zsrmvapi.h>

#define DAEMON_PRIORITY 50

#define TS_BUFFER_SIZE 100000

unsigned long long timestamp1[TS_BUFFER_SIZE];
unsigned long long timestamp2[TS_BUFFER_SIZE];
long tsindex1=0;
long tsindex2=0;
int sync_start_semid;
int ready_semid;

unsigned long long *startts=NULL;
unsigned long long start_timestamp_ns;


int task1(){
  int i;
  struct sembuf sops;
  key_t semkey,semkey_ready;
  char sockname[100];
  int schedfd;
  int rid;
  int firsttime=1;

  schedfd = zsv_open_scheduler();

  if (schedfd <0){
    printf("Error opening the scheduler");
    return -1;
  }
  
  if ((semkey = ftok("/tmp", 11766)) == (key_t) -1) {
    perror("IPC error: ftok"); exit(1);
  }

  // create sync start semaphore
  if ((sync_start_semid = semget(semkey, 1, 0)) <0){
    perror("creating the semaphone\n");
    return -1;
  }

  /**********
   * ready semaphore setup
   *********/
  if ((semkey_ready = ftok("/tmp", 11767)) == (key_t) -1) {
    perror("IPC error: ftok"); exit(1);
  }

  // create sync start semaphore
  if ((ready_semid = semget(semkey_ready, 1, 0)) <0){
    perror("creating the semaphone\n");
    return -1;
  }

  //req_create_reserve(sock, period_sec,period_nsec, exec_sec,exec_nsec,priority)
  if ((rid = zsv_create_reserve(schedfd,
				1, // period_secs
				0, // period_nsecs
				1, // zsinstant_sec -- larger than period = disabled
				0, // zsinstant_nsec -- same as period = disabled
				0, // exectime _secs
				500000000, // exectime_nsecs
				0, // nominal_exectime_sec -- same as overloaded
				500000000, // nominal_exectime_nsec -- same as overloaded
				10, // priority
				1  // criticality
				) 
       )<0){
    printf("error calling create reserve\n");
    return -1;
  }
  if (zsv_attach_reserve(schedfd, getpid(), rid)<0){
    printf("error calling attach reserve\n");
    return -1;
  }

  printf("task 1 attached and ready\n");

  // signal parent that I am ready
  sops.sem_num=0;
  sops.sem_op = 1; // up
  sops.sem_flg = 0;
  if (semop(ready_semid,&sops,1)<0){
    printf("error on ready sem up\n");
  }


  // wait for parent to signal go
  sops.sem_num=0;
  sops.sem_op = -1; // down
  sops.sem_flg = 0;
  if (semop(sync_start_semid,&sops,1)<0){
    printf("error on sync star sem down\n");
  }


  for (i=0;i<100;i++){
    printf("task1 rid(%d) iter(%d)\n",rid,i);
    if (firsttime){
      firsttime=0;
      busy_timestamped(550, timestamp1, TS_BUFFER_SIZE, &tsindex1);
      //busy_timestamped(150, timestamp1, TS_BUFFER_SIZE, &tsindex1);
    } else {
      busy_timestamped(550, timestamp1, TS_BUFFER_SIZE, &tsindex1);
    }
    zsv_wait_period(schedfd,rid);
  }
  printf("task 1 DONE!\n");

  zsv_delete_reserve(schedfd,rid);

  zsv_close_scheduler(schedfd);

}

int task2(){
  int i;
  struct sembuf sops;
  key_t semkey, semkey_ready;
  char sockname[100];
  int schedfd,rid;
  int firsttime=0;

  schedfd = zsv_open_scheduler();

  if (schedfd <0){
    printf("error opening the scheduler\n");
    return -1;
  }

  if ((semkey = ftok("/tmp", 11766)) == (key_t) -1) {
    perror("IPC error: ftok"); exit(1);
  }

  // create sync start semaphore
  if ((sync_start_semid = semget(semkey, 1, 0)) <0){
    perror("creating the semaphone\n");
    return -1;
  }

  /**********
   * ready semaphore setup
   *********/
  if ((semkey_ready = ftok("/tmp", 11767)) == (key_t) -1) {
    perror("IPC error: ftok"); exit(1);
  }

  // create sync start semaphore
  if ((ready_semid = semget(semkey_ready, 1, 0)) <0){
    perror("creating the semaphone\n");
    return -1;
  }


  //req_create_reserve(sock, period_sec, period_nsec, exec_sec,exec_nsec,priority)
  if ((rid = zsv_create_reserve(schedfd,
				2, // period_secs
				0, // period_nsecs
				1, // zsinstant_sec
				100000000, // zsinstant_nsec
				1, // exectime_secs
				250000000, // exectime_nsecs
				0, // nominal_exectime_sec -- same as overloaded
				750000000, // nominal_exectime_nsec -- same as overloaded
				9, // priority
				2  // criticality same as other task
				)
       )<0){
    printf("task 2 create reserve failed\n");
  }

  if (zsv_attach_reserve(schedfd,getpid(),rid)<0){
    printf("task 2 attach reserve failed\n");
  }

  printf("task 2 attached and ready\n");


  // signal parent that I am ready
  sops.sem_num=0;
  sops.sem_op = 1; // up
  sops.sem_flg = 0;
  if (semop(ready_semid,&sops,1)<0){
    printf("error on ready sem up\n");
  }

  // wait for parent to signal go
  sops.sem_num=0;
  sops.sem_op = -1; // down
  sops.sem_flg = 0;
  if (semop(sync_start_semid,&sops,1)<0){
    printf("error on sync star sem down\n");
  }


  for (i=0;i<100;i++){
    printf("task2 rid(%d) iter(%d)\n",rid,i);
    if (firsttime){
      firsttime=0;
      busy_timestamped(1000, timestamp2, TS_BUFFER_SIZE, &tsindex2);
    } else {
      busy_timestamped(780, timestamp2, TS_BUFFER_SIZE, &tsindex2);
    }
    zsv_wait_period(schedfd,rid);
  }
  printf("task 2 DONE!\n");

  zsv_delete_reserve(schedfd,rid);
  
  zsv_close_scheduler(schedfd);
}

int main(){
  struct sched_param p;
  cpu_set_t mask; 
  int ret; 
  sigset_t sigmask;
  sigset_t sigmask1;
  struct sigaction act;
  long idx;
  struct sembuf sops;
  key_t semkey,semkey_ready;
  int rc,ctlsock;
  pthread_t tid;
  int shmid;
  key_t key;
  char *shm, *s;

  union semun  {
    int val;
    struct semid_ds *buf;
    ushort *array;
  } arg;


  key = 5678;

  if ((shmid = shmget(key, 20, IPC_CREAT | 0777)) < 0) {
    perror("shmget");
    exit(1);
  }

  if ((shm = shmat(shmid, NULL, 0)) == (char *) -1) {
    perror("shmat");
    exit(1);
  }
  
  startts = (unsigned long long *) shm;

  CPU_ZERO(&mask); 
  CPU_SET(0, &mask); 
  ret = sched_setaffinity(getpid(), sizeof mask, &mask); 
  
  p.sched_priority = DAEMON_PRIORITY-1;
  if (sched_setscheduler(getpid(),SCHED_FIFO, &p)<0){
    printf("Could not set priority to %d\n",DAEMON_PRIORITY);
    return -1;
  }
  
  /**********
   * sync_start semaphore setup
   *********/
  if ((semkey = ftok("/tmp", 11766)) == (key_t) -1) {
    perror("IPC error: ftok"); exit(1);
  }

  // create sync start semaphore
  if ((sync_start_semid = semget(semkey, 1, IPC_CREAT|0777)) <0){
    perror("creating the semaphone\n");
    return -1;
  }

  arg.val = 0;
  if (semctl(sync_start_semid, 0, SETVAL, arg) < 0){
    printf("Error setting sem to zero\n");
    return -1;
  }
  /**********
   * END -- sync_start semaphore setup
   *********/

  /**********
   * ready semaphore setup
   *********/
  if ((semkey_ready = ftok("/tmp", 11767)) == (key_t) -1) {
    perror("IPC error: ftok"); exit(1);
  }

  // create sync start semaphore
  if ((ready_semid = semget(semkey_ready, 1, IPC_CREAT|0777)) <0){
    perror("creating the semaphone\n");
    return -1;
  }

  arg.val = 0;
  if (semctl(ready_semid, 0, SETVAL, arg) < 0){
    printf("Error setting sem to zero\n");
    return -1;
  }
  /**********
   * END -- ready semaphore setup
   *********/


  if (!fork()){
    key = 5678;
    if ((shmid = shmget(key, 20, 0777)) < 0) {
        perror("shmget");
        exit(1);
    }

    if ((shm = shmat(shmid, NULL, 0)) == (char *) -1) {
        perror("shmat");
        exit(1);
    }

    startts = (unsigned long long *) shm;

    task1();
    FILE* fid1 = fopen("ts-task1.txt","w+");
    if (fid1==NULL){
      printf("erorr opening ts-task1.txt\n");
      return -1;
    }

    for (idx = 0 ; idx < tsindex1 ; idx++){
      fprintf(fid1,"%llu 1\n",(timestamp1[idx]-(*startts))/1000000);
    }
    
    fclose(fid1);
    return 0;
  }

  if (!fork()){
    key = 5678;
    if ((shmid = shmget(key, 20, 0777)) < 0) {
        perror("shmget");
        exit(1);
    }

    if ((shm = shmat(shmid, NULL, 0)) == (char *) -1) {
        perror("shmat");
        exit(1);
    }

    startts = (unsigned long long *) shm;
    task2();
    FILE* fid1 = fopen("ts-task2.txt","w+");
    if (fid1==NULL){
      printf("erorr opening ts-task2.txt\n");
      return -1;
      }

    for (idx = 0 ; idx < tsindex2 ; idx++){
      fprintf(fid1,"%llu 1\n",(timestamp2[idx]-(*startts))/1000000);
    }

    fclose(fid1);
    return 0;
  }

  // wait for tasks to initialize

  sops.sem_num=0;
  sops.sem_op = -2; // down two -- wait for signals from the two children
  sops.sem_flg = 0;
  if (semop(ready_semid, &sops,1)<0){
    printf("error in semop down\n");
  }

  *startts = get_now_ns();

  // Go!
  sops.sem_num=0;
  sops.sem_op = 2; 
  sops.sem_flg = 0;
  if (semop(sync_start_semid, &sops,1)<0){
    printf("error in semop up\n");
  }

  // call again when signal is received
  while (wait(&ret)<0)
    ;

  while(wait(&ret)<0)
    ;
 
  if (semctl(sync_start_semid, 0, IPC_RMID)<0){
    printf("error removing sync start semaphore\n");
  }

  if (semctl(ready_semid, 0, IPC_RMID)<0){
    printf("error removing ready semaphore\n");
  }

  printf("** END OF TEST! **\n");
  return 0;
}
