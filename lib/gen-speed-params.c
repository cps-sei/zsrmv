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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sched.h>
#include "../zsrmvapi.h"

#define SPEED_PARAMS_FILENAME "speed_params.h"
#define LINE_LENGTH 100
#define TEST_TIME_MS 1000

int main(int argc, char *argv[])
{
  unsigned long long begin=0;
  unsigned long long end=0;
  unsigned long long elapsed=0;
  unsigned long long diff=0;
  unsigned long long cdiff=0;
  unsigned long long test_time_ns=0;
  double ddiff=0.0;
  char *strloops;
  long current_loops_one_ms=0;
  long new_loops_one_ms=0;
  char line[LINE_LENGTH];
  struct sched_param p;
  int i;

  unsigned long long tsbuf[10];
  long bidx;
  
  FILE *fid;

  if (argc != 2){
    printf("usage: %s <computer+processor description + frequency>\n",argv[0]);
    return -1;
  }

  fid = fopen(SPEED_PARAMS_FILENAME,"r+");
  if (fid == NULL){
    printf("could not open %s for rw\n",SPEED_PARAMS_FILENAME);
    return -1;
  }

  while (fgets(line,LINE_LENGTH,fid) != NULL){
    if (strstr(line,"IN_LOOP_ONE_MS")){
	strtok(line," \t\n"); // return #define
	strtok(NULL," \t\n"); // returns IN_LOOP_ONE_MS
	strloops = strtok(NULL," \t\n"); // returns numeric constant
	if (strloops != NULL){
	  current_loops_one_ms = atol(strloops);
	  printf("current_loops_one_ms=%ld\n",current_loops_one_ms);
	}
	break;
    }
  }

  // set high fixed priority

  p.sched_priority = 60;
  if (sched_setscheduler(getpid(), SCHED_FIFO,&p)<0){
    printf("could not change my priority. Make sure you execute with sudo\n");
    return -1;
  }

  begin = get_now_ns();
  busy_timestamped(TEST_TIME_MS,tsbuf,10,&bidx);
  end = get_now_ns();
  elapsed = end-begin;
  
  test_time_ns = TEST_TIME_MS * 1000000L;
  
  if (elapsed > test_time_ns){
    diff = elapsed - test_time_ns;
    ddiff = diff *1.0;
    ddiff = ddiff / elapsed;
    new_loops_one_ms = current_loops_one_ms - (current_loops_one_ms * ddiff);
    //new_loops_one_ms = current_loops_one_ms * (10000000L/elapsed);
    printf("need to reduce constant by %f percent to %ld \n",ddiff,new_loops_one_ms);
  } else if (elapsed < test_time_ns){
    diff = test_time_ns - elapsed;
    ddiff = diff * 1.0;
    ddiff = ddiff / elapsed;
    //new_loops_one_ms = current_loops_one_ms * (10000000L/elapsed);
    new_loops_one_ms = current_loops_one_ms + (current_loops_one_ms * ddiff);
    printf("need to increase constant by %f percent to %ld \n",ddiff,new_loops_one_ms);
  } else {
    printf("just right!\n");
  }

  fclose(fid);
  fid = fopen(SPEED_PARAMS_FILENAME,"w+");
  if (fid == NULL){
    printf("could not open %s for w\n",SPEED_PARAMS_FILENAME);
    return -1;
  }

  fprintf(fid,"// %s\n",argv[1]);
  fprintf(fid,"#define IN_LOOP_ONE_MS %ld\n",new_loops_one_ms);
  fclose(fid);
}
