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

#include <stdlib.h>
#include <stdio.h>
#include <jni.h>
#include <zsrmvapi.h>
#include "edu_cmu_sei_ZSRM_ZSRMVScheduler.h"


JNIEXPORT jboolean JNICALL Java_edu_cmu_sei_ZSRM_ZSRMVScheduler_isAdmissible
(JNIEnv *env, jobject self, jobjectArray rsvarray){
  int i;
  jint size;
  jclass zsrmvRsvClass;
  jmethodID getPeriodSecID;
  jmethodID getPeriodNsecID;
  jmethodID getZSInstantSecID;
  jmethodID getZSInstantNsecID;
  jmethodID setZSInstantSecID;
  jmethodID setZSInstantNsecID;
  jmethodID getExecSecID;
  jmethodID getExecNsecID;
  jmethodID getNominalExecSecID;
  jmethodID getNominalExecNsecID;
  jmethodID getCriticalityID;
  struct reserve_spec_t *rtable;
  int admissible;
  int error;

  // load class
  zsrmvRsvClass = (*env)->FindClass(env,"edu/cmu/sei/ZSRM/ZSRMVReserve");
  if (zsrmvRsvClass == NULL){
    printf("could not load ZSRMVReserve java class\n");
    return JNI_FALSE;
  }
  
  // load all getter methods
  getPeriodSecID = (*env)->GetMethodID(env,zsrmvRsvClass,"getPeriodSec","()J");
  if (getPeriodSecID == NULL){
    printf("could not load method getPeriodSec\n");
    return JNI_FALSE;
  }
  getPeriodNsecID = (*env)->GetMethodID(env,zsrmvRsvClass,"getPeriodNsec","()J");
  if (getPeriodNsecID == NULL){
    printf("could not load method getPeriodNsec\n");
    return JNI_FALSE;
  }
  getZSInstantSecID = (*env)->GetMethodID(env,zsrmvRsvClass,"getZSInstantSec","()J");
  if (getZSInstantSecID == NULL){
    printf("could not load method getZSInstantSec\n");
    return JNI_FALSE;
  }
  getZSInstantNsecID = (*env)->GetMethodID(env,zsrmvRsvClass,"getZSInstantNsec","()J");
  if (getZSInstantNsecID == NULL){
    printf("could not load method getZSInstantNsec\n");
    return JNI_FALSE;
  }
  setZSInstantSecID = (*env)->GetMethodID(env,zsrmvRsvClass,"setZSInstantSec","(J)V");
  if (setZSInstantSecID == NULL){
    printf("could not load method setZSInstantSec\n");
    return JNI_FALSE;
  }
  setZSInstantNsecID = (*env)->GetMethodID(env,zsrmvRsvClass,"setZSInstantNsec","(J)V");
  if (setZSInstantNsecID == NULL){
    printf("could not load method setZSInstantNsec\n");
    return JNI_FALSE;
  }
  getExecSecID = (*env)->GetMethodID(env,zsrmvRsvClass,"getExecSec","()J");
  if (getExecSecID == NULL){
    printf("could not load method getExecSec\n");
    return JNI_FALSE;
  }
  getExecNsecID = (*env)->GetMethodID(env,zsrmvRsvClass,"getExecNsec","()J");
  if (getExecNsecID == NULL){
    printf("could not load method getExecNsec\n");
    return JNI_FALSE;
  }
  getNominalExecSecID = (*env)->GetMethodID(env,zsrmvRsvClass,"getNominalExecSec","()J");
  if (getNominalExecSecID == NULL){
    printf("could not load method getNominalExecSec\n");
    return JNI_FALSE;
  }
  getNominalExecNsecID = (*env)->GetMethodID(env,zsrmvRsvClass,"getNominalExecNsec","()J");
  if (getNominalExecNsecID == NULL){
    printf("could not load method getNominalExecNsec\n");
    return JNI_FALSE;
  }
  getCriticalityID = (*env)->GetMethodID(env,zsrmvRsvClass,"getCriticality","()I");
  if (getCriticalityID == NULL){
    printf("could not load method getCriticality\n");
    return JNI_FALSE;
  }
  
  // traverse all rsvs
  size =(*env)->GetArrayLength(env,rsvarray);

  rtable = malloc(sizeof(struct reserve_spec_t)*size);
  if (rtable == NULL){
    printf("could not allocate temporal reserve table\n");
    return JNI_FALSE;
  }

  error = 0;
  for (i=0;i<size;i++){
    jobject jrsv = (*env)->GetObjectArrayElement(env,rsvarray,i);
    if ((*env)->ExceptionOccurred(env)){
      printf("Error accessing element %d of rsvarray\n",i);
      error=1;
      break;
    }
    rtable[i].period_sec = (*env)->CallLongMethod(env,jrsv,getPeriodSecID);
    rtable[i].period_nsec =(*env)->CallLongMethod(env,jrsv,getPeriodNsecID);
    rtable[i].zsinstant_sec=(*env)->CallLongMethod(env,jrsv,getZSInstantSecID);
    rtable[i].zsinstant_nsec=(*env)->CallLongMethod(env,jrsv,getZSInstantNsecID);
    rtable[i].exec_sec=(*env)->CallLongMethod(env,jrsv,getExecSecID);
    rtable[i].exec_nsec=(*env)->CallLongMethod(env,jrsv,getExecNsecID);
    rtable[i].nominal_exec_sec=(*env)->CallLongMethod(env,jrsv,getNominalExecSecID);
    rtable[i].nominal_exec_nsec=(*env)->CallLongMethod(env,jrsv,getNominalExecNsecID);
    rtable[i].criticality = (*env)->CallIntMethod(env,jrsv,getCriticalityID);
  }

  if (!error){
    admissible = zsv_is_admissible(rtable,size);
    if (admissible){
      for (i=0;i<size;i++){
	jobject jrsv = (*env)->GetObjectArrayElement(env,rsvarray,i);
	if ((*env)->ExceptionOccurred(env)){
	  printf("Error accessing element %d of rsvarray\n",i);
	  error=1;
	  break;
	}
	(*env)->CallVoidMethod(env,jrsv,setZSInstantSecID,rtable[i].zsinstant_sec);
	(*env)->CallVoidMethod(env,jrsv,setZSInstantNsecID,rtable[i].zsinstant_nsec);
      }
    }
  } else {
    admissible=0;
  }

  free(rtable);
  return (admissible ? JNI_TRUE : JNI_FALSE);
}
