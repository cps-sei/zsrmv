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

#include "zsrmv.h"

int isHigherPrioHigherCrit(struct reserve *thisone, struct reserve *other)
{
  return ((thisone->period_ns >= other->period_ns) && (thisone->criticality < other->criticality));
}

int isLowerPrioHigherCrit(struct reserve *thisone, struct reserve *other)
{
  return ((thisone->period_ns < other->period_ns) && (thisone->criticality < other->criticality));
}

int isHigherPrioSameCrit(struct reserve *thisone, struct reserve *other)
{
  return ((thisone->period_ns >= other->period_ns) && (thisone->criticality == other->criticality));
}

int isHigherPrioLowerCrit(struct reserve *thisone, struct reserve *other)
{
  return ((thisone->period_ns >= other->period_ns) && (thisone->criticality > other->criticality));
}

int getNextInSet(struct reserve *rsvtable, int *cidx, int tablesize, struct reserve *newrsv, int (*inSet)(struct reserve *t, struct reserve *o))
{
  int selectedIdx=-1;

  if (*cidx < 0)
    return -1;

  while ((*cidx < tablesize) &&
	 (!inSet(newrsv, &rsvtable[*cidx]) ||
	  rsvtable[*cidx].pid == -1 ||
	  newrsv ==  &rsvtable[*cidx]
	  )
	 ){
    *cidx +=1;
  }
  
  if (*cidx <tablesize){
    selectedIdx = *cidx;
    *cidx+=1;
  }

  return selectedIdx;
}

unsigned long long getExecTimeHigherPrioHigherCrit(struct reserve *r)
{
  return r->nominal_exectime_ns;
}

unsigned long long getExecTimeHigherPrioSameCrit(struct reserve *r)
{
  return r->exectime_ns;
}

unsigned long long getExecTimeLowerPrioHigherCrit(struct reserve *r)
{
  return (r->nominal_exectime_ns - r->exectime_in_rm_ns);
}

unsigned long long getResponseTimeCritNs(struct reserve *rsvtable, int tablesize, struct reserve *newrsv)
{
  int firsttime=1;
  unsigned long long resp=0L;
  unsigned long long prevResp=0L;
  unsigned long numArrivals=0l;
  int idx=0;
  int selectedIdx=-1;

  resp = newrsv->exectime_ns - newrsv->exectime_in_rm_ns;

  while (firsttime || (resp > prevResp && resp <= newrsv->period_ns)){
    firsttime=0;
    prevResp = resp;

    resp = newrsv->exectime_ns - newrsv->exectime_in_rm_ns;

    // get interference from Higher Priority Higher Criticality taskset
    idx=0;
    while((selectedIdx = getNextInSet(rsvtable, &idx, tablesize, newrsv, isHigherPrioHigherCrit)) >=0){
      numArrivals = prevResp / rsvtable[selectedIdx].period_ns + (prevResp % rsvtable[selectedIdx].period_ns >0 ? 1 : 0);
      resp += numArrivals * getExecTimeHigherPrioHigherCrit(&rsvtable[selectedIdx]);
    }

    // get interference from Lower Priority Higher Criticality taskset
    idx=0;
    while((selectedIdx = getNextInSet(rsvtable, &idx, tablesize, newrsv, isLowerPrioHigherCrit)) >=0){
      numArrivals = prevResp / rsvtable[selectedIdx].period_ns + (prevResp % rsvtable[selectedIdx].period_ns >0 ? 1 : 0);
      resp += numArrivals * getExecTimeLowerPrioHigherCrit(&rsvtable[selectedIdx]);
    }

    // get interference from Higher Priority Same Criticality taskset
    idx=0;
    while((selectedIdx = getNextInSet(rsvtable, &idx, tablesize, newrsv, isHigherPrioSameCrit)) >=0){
      numArrivals = prevResp / rsvtable[selectedIdx].period_ns + (prevResp % rsvtable[selectedIdx].period_ns >0 ? 1 : 0);
      resp += numArrivals * getExecTimeHigherPrioSameCrit(&rsvtable[selectedIdx]);
    }
  }
  return resp;
}

unsigned long long getRMInterference(struct reserve *rsvtable, int tablesize, struct reserve *newrsv, unsigned long long Z)
{
  int idx;
  int selectedIdx;
  unsigned long numArrivals;
  unsigned long long interf=0L;
  
  idx=0;
  while((selectedIdx = getNextInSet(rsvtable, &idx, tablesize, newrsv, isHigherPrioHigherCrit)) >=0){
    numArrivals = Z / rsvtable[selectedIdx].period_ns + (Z % rsvtable[selectedIdx].period_ns >0 ? 1 : 0);
    interf += numArrivals * rsvtable[selectedIdx].nominal_exectime_ns;
  }

  idx=0;
  while((selectedIdx = getNextInSet(rsvtable, &idx, tablesize, newrsv, isHigherPrioLowerCrit)) >=0){
    numArrivals = Z / rsvtable[selectedIdx].period_ns + (Z % rsvtable[selectedIdx].period_ns >0 ? 1 : 0);
    interf += numArrivals * rsvtable[selectedIdx].exectime_ns;
  }

  idx=0;
  while((selectedIdx = getNextInSet(rsvtable, &idx, tablesize, newrsv, isHigherPrioSameCrit)) >=0){
    numArrivals = Z / rsvtable[selectedIdx].period_ns + (Z % rsvtable[selectedIdx].period_ns >0 ? 1 : 0);
    interf += numArrivals * rsvtable[selectedIdx].exectime_ns;
  }
  
  return interf;
}

int admit(struct reserve *rsvtable, int tablesize, struct reserve *newrsv, unsigned long long *calcZ)
{
  unsigned long long resp=0L;
  unsigned long long Z=0L;
  unsigned long long prevZ = 0L;
  unsigned long long interf=0L;
  unsigned long long interf1=0L;
  unsigned long long slack=0L;

  newrsv->exectime_in_rm_ns = 0L;
  Z = 1L;
  prevZ = 0L;

  while (newrsv->period_ns >= resp && Z > prevZ){
    prevZ = Z;
    resp = getResponseTimeCritNs(rsvtable,tablesize,newrsv);
    if (newrsv->period_ns >= resp){
      Z = newrsv->period_ns - resp;

      interf = getRMInterference(rsvtable,tablesize,newrsv,Z);

      if (Z > (interf + newrsv->exectime_in_rm_ns)){
	slack = Z - interf - newrsv->exectime_in_rm_ns;
      } else if (Z == (interf + newrsv->exectime_in_rm_ns)){
	// Got stuck in limit
	// see if there is some slack if we push Z a bit beyond interference window
	if (Z+(resp - newrsv->exectime_ns+newrsv->exectime_in_rm_ns) <= newrsv->period_ns){
	  interf1 = getRMInterference(rsvtable,tablesize,newrsv,Z+(resp-newrsv->exectime_ns+newrsv->exectime_in_rm_ns));
	  if (interf1<=resp - (newrsv->exectime_ns-newrsv->exectime_in_rm_ns)){
	    // some slack upened up
	    Z = Z + (resp-newrsv->exectime_ns+newrsv->exectime_in_rm_ns) ;
	    interf = interf1;
	    slack = Z - interf - newrsv->exectime_in_rm_ns;
	  }
	}
      } else {
	slack = 0L;
      }
      
      if (slack + newrsv->exectime_in_rm_ns < newrsv->exectime_ns){
	newrsv->exectime_in_rm_ns += slack;
      } else {
	newrsv->exectime_in_rm_ns = newrsv->exectime_ns;
	// set Z to period?
	Z = newrsv->period_ns;
      }
    } 
  }

  *calcZ = Z;

  return (newrsv->period_ns >= resp);
}
