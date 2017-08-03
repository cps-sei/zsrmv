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
#include <zsrmvapi.h>
#include <limits.h>

int mark_draw_timeline(FILE *fid, unsigned long long ts, int offset, int margin, int height,
		       int lineoffset, char* color, char *taskname, int drawtimeline,
		       char *marker_name, int arrow, int textoffset, unsigned long long min_ts,unsigned long long max_ts)
{
  static unsigned long long startts=0L;
  static unsigned long long endts=0L;

  startts = min_ts;
  endts = max_ts;
  
  if (drawtimeline){
    printf(" DRAW TIMELINE TASK(%s) FROM(%llu) TO(%llu) AT Y(%d) \n",
	   taskname, startts+margin,endts+margin,(offset+height));
    fprintf(fid,"<line x1=\"%llu\" y1=\"%d\" x2=\"%llu\" y2=\"%d\" stroke=\"%s\" stroke-width=\"4\" />\n",
	    startts+margin,(offset+height),endts+margin,(offset+height),color);
    fprintf(fid, "<text x=\"%d\" y=\"%d\">%s:</text>\n",0,offset+(height/2),taskname);
  } else {
    if (startts == 0L){
      startts = ts;
    }
    
    if (endts < ts){
      endts = ts;
    }
    fprintf(fid,"<line x1=\"%llu\" x2=\"%llu\" y1=\"%d\" y2=\"%d\" stroke=\"%s\" stroke-width=\"2\" />\n",
	    ts+margin,ts+margin,(lineoffset+offset+height+10),(lineoffset+offset+height-10),color);
    if (arrow){
      fprintf(fid,"<line x1=\"%llu\" x2=\"%llu\" y1=\"%d\" y2=\"%d\" stroke=\"%s\" stroke-width=\"2\" />\n",
	      ts+margin,ts+margin+5,(lineoffset+offset+height-10),(lineoffset+offset+height+2),color);
      fprintf(fid,"<line x1=\"%llu\" x2=\"%llu\" y1=\"%d\" y2=\"%d\" stroke=\"%s\" stroke-width=\"2\" />\n",
	      ts+margin,ts+margin-5,(lineoffset+offset+height-10),(lineoffset+offset+height+2),color);
      fprintf(fid,"<text x=\"%llu\" y=\"%d\">%s%llu</text>\n",ts+margin+textoffset,(lineoffset+offset+height+10),marker_name, ts);
    } else {
      fprintf(fid,"<text x=\"%llu\" y=\"%d\" transform=\"rotate(90,%llu,%d)\">%s%llu</text>\n",
	      ts+margin+textoffset,(lineoffset+offset+height+10),ts+margin+textoffset,(lineoffset+offset+height+10),marker_name,ts);
    }
  }
  return 0;
}

int draw_segment(FILE *fid, int offset, int margin, int height, unsigned long long ts, int event,
		 char* color, char *taskname, unsigned long long min_ts, unsigned long long max_ts)
{
  static unsigned long long startts=0L;
  static int prev_event=-1;
  static unsigned long long prevts=0L;
  
  if (startts == 0L){
    startts = ts;
  }

  startts = min_ts;

  // substract start of plot
  //ts = ts - startts;// + margin;
  
  if (event == TRACE_EVENT_PREEMPTED){
    if (prev_event == TRACE_EVENT_RESUMED){
      // ready to build box
    
      fprintf(fid,"<rect x=\"%llu\" y=\"%d\" width=\"%llu\" height=\"%d\" stroke=\"%s\" stroke-width=\"4\" fill=\"%s\" />\n",
	      prevts+margin, offset, (ts-prevts), height, color,color);
      mark_draw_timeline(fid,ts,offset,margin,height,0,"black", taskname,0,"",0, 5, min_ts, max_ts);
      //printf("mark suspend at: %llu\n",ts);
      prev_event = -1;
    } else {
      mark_draw_timeline(fid,ts,offset,margin,height,0,"red", taskname,0,"",0, 5, min_ts, max_ts);
    }
  }  else if (event == TRACE_EVENT_RESUMED){
    prev_event = event;
    prevts = ts;
    mark_draw_timeline(fid,ts,offset,margin,height,0,"black",taskname, 0, "", 0, -10, min_ts,max_ts);
    //printf("mark resumed at: %llu\n",ts);
  } else if (event == TRACE_EVENT_BUDGET_ENFORCEMENT){
    //printf("mark enforcement at: %llu\n",ts);
    mark_draw_timeline(fid,ts,offset,margin,height,60,"red",taskname, 0,"E ", 1, 5, min_ts,max_ts);
  } else if (event == TRACE_EVENT_WFNP){
    mark_draw_timeline(fid,ts,offset,margin,height,120,"blue",taskname,0,"W ", 1, 5, min_ts,max_ts);
  } else if (event == TRACE_EVENT_START_PERIOD){
    mark_draw_timeline(fid,ts,offset,margin,height,90,"blue",taskname,0,"S ", 1, 5, min_ts,max_ts);
  } else if (event == TRACE_EVENT_START_PERIOD_PERIODIC_WAIT) {    
    mark_draw_timeline(fid,ts,offset,margin,height,90,"blue",taskname,0,"S ", 1, 5, min_ts,max_ts);
  } else if (event == TRACE_EVENT_START_PERIOD_NON_PERIODIC_WAIT_WAKEUP) {    
    mark_draw_timeline(fid,ts,offset,margin,height,90,"blue",taskname,0,"U ", 1, 5, min_ts,max_ts);
  } else if (event == TRACE_EVENT_START_PERIOD_NON_PERIODIC_WAIT_NO_WAKEUP) {
    mark_draw_timeline(fid,ts,offset,margin,height,90,"blue",taskname,0,"X ", 1, 5, min_ts,max_ts);
  } else if (event == TRACE_EVENT_DONT_WFNP){
    mark_draw_timeline(fid,ts,offset,margin,height,120,"blue",taskname,0,"N ", 1, 5, min_ts,max_ts);
  } else if (event == TRACE_EVENT_WAIT_RELEASE_BLOCKED) {
    mark_draw_timeline(fid,ts,offset,margin,height,90,"blue",taskname,0,"R ", 1, 5, min_ts,max_ts);
  } else if (event == TRACE_EVENT_WAIT_RELEASE_NOT_BLOCKED) {
    mark_draw_timeline(fid,ts,offset,margin,height,90,"blue",taskname,0,"r ", 1, 5, min_ts,max_ts);
  } else if (event == TRACE_EVENT_END_PERIOD) {
    mark_draw_timeline(fid,ts,offset,margin,height,90,"blue",taskname,0,"D ", 1, 5, min_ts,max_ts);
  } else {
    // unknown event
    char ename[20];
    sprintf(ename,"?[%d]",event);
    mark_draw_timeline(fid,ts,offset,margin,height,120,"red",taskname,0,ename, 1, 5, min_ts,max_ts);
  }
}

#define NUM_COLORS 5
char *colors[]={"blue","red","green","yellow","black"};

int main(int argc, char *argv[])
{
  int min_rid = INT_MAX;
  int max_rid = 0;
  unsigned long long min_ts = ULLONG_MAX;
  unsigned long long max_ts = 0;
  int exists=0;
  int currentrid;
  int rid,event;
  unsigned long long ts;
  char buffer[100];
  int i;
  int start_rid;
  char taskname[50];
  unsigned long long scale_divisor;
  int numtraces=0;
  int margin=100;

  scale_divisor = 10000000L;

  if (argc != 5){
    printf("usage %s <infile> <outfile> <start rid> <scale divisor>\n",argv[0]);
    return -1;
  }
    
  FILE *fid = fopen(argv[1],"r");
  if (fid == NULL){
    printf("error opening %s\n",argv[1]);
    return -1;
  }
  FILE *ofid = fopen(argv[2],"w+");
  if (ofid == NULL){
    printf("error opening %s\n",argv[2]);
    return -1;
  }

  start_rid = atoi(argv[3]);

  scale_divisor = atol(argv[4]);

  // find out dimensions
  while(fgets(buffer, 100,fid) != NULL){
    numtraces++;
    sscanf(buffer,"%d %llu %d",&rid,&ts,&event);
    // ignore system event rid (rid == -1)
    if (rid >=0){
      if (rid > max_rid)
	max_rid = rid;
      if (rid < min_rid)
	min_rid = rid;
    }
    if (ts > max_ts)
      max_ts = ts;
    if (ts < min_ts)
      min_ts = ts;
  }
  rewind(fid);

  // scale down
  max_ts = max_ts / scale_divisor;
  min_ts = min_ts / scale_divisor;

  // reset starting time to zero
  // min_ts = min_ts - min_ts;
  // max_ts = max_ts - min_ts;
  
  printf("\n--- Dimensions ---\n");
  printf("| NUM TRACES: %d\n",numtraces);
  printf("| min_rid(%d)\n",min_rid);
  printf("| max_rid(%d)\n",max_rid);
  printf("| min_ts(%llu)\n",(min_ts-min_ts));
  printf("| max_ts(%llu)\n",(max_ts-min_ts));
  printf("-----------------\n\n");
  
  fprintf(ofid,"<!DOCTYPE html>\n");
  fprintf(ofid,"<html>\n");
  fprintf(ofid,"<body>\n");
  fprintf(ofid,"<svg width=\"%llu\" height=\"%d\">\n",(max_ts-min_ts)+50+margin,(max_rid-min_rid+1+1)*200+50);

  currentrid=start_rid;
  do {
    exists=0;
    while(fgets(buffer, 100,fid) != NULL){
      sscanf(buffer,"%d %llu %d",&rid,&ts,&event);
      ts = ts / scale_divisor;
      ts = ts - min_ts;
      if (rid == currentrid){
	printf("processing: rid(%d) ts(%llu) event(%s)\n",rid,ts-min_ts,STRING_TRACE_EVENT(event));
	sprintf(taskname,"task%d",rid);
	draw_segment(ofid,(rid*200)+10,margin, 50,ts,event,colors[rid%NUM_COLORS],taskname,min_ts,max_ts);
	exists=1;
      }
    }
    if (exists){
      // draw the horizontal line of time across all events
      mark_draw_timeline(ofid,0,(currentrid*200)+10,margin,50,0,"black",taskname,1,"",0,0, min_ts-min_ts,max_ts-min_ts);
      rewind(fid);
      //draw_segment(ofid,-1,100, 50,(ts/10000000),event,"blue","one");
    }
    currentrid++;
  } while (exists);

  // process system events
  rewind(fid);
  while(fgets(buffer, 100,fid) != NULL){
    sscanf(buffer,"%d %llu %d",&rid,&ts,&event);
    ts = ts / scale_divisor;
    ts = ts - min_ts;
    if (rid == -1){
      // only mark the idle instants
      if (event == TRACE_EVENT_IDLE){
	mark_draw_timeline(fid,ts,((max_rid+1)*200)+10,margin,50,120,"blue","system",0,"I ", 1, 5, min_ts,max_ts);
      }
    }
  }
  
  fprintf(ofid,"</svg>\n");
  fprintf(ofid,"</body>\n");
  fprintf(ofid,"</html>\n");

  fclose(fid);
  fclose(ofid);
}
