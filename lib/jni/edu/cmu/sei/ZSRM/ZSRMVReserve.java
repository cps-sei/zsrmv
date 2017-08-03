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

package edu.cmu.sei.ZSRM;

public class ZSRMVReserve {
    long period_sec=0;
    long period_nsec=0;
    long zsinstant_sec=0;
    long zsinstant_nsec=0;
    long exec_sec=0;
    long exec_nsec=0;
    long nominal_exec_sec=0;
    long nominal_exec_nsec=0;
    int criticality=0;

    public void setPeriodSec(long p){
	period_sec = p;
    }
    public long getPeriodSec(){
	return period_sec;
    }
    public void setPeriodNsec(long p){
	period_nsec = p;
    }
    public long getPeriodNsec(){
	return period_nsec;
    }
    public void setZSInstantSec(long p){
	zsinstant_sec = p;
    }
    public long getZSInstantSec(){
	return zsinstant_sec;
    }
    public void setZSInstantNsec(long p){
	zsinstant_nsec = p;
    }
    public long getZSInstantNsec(){
	return zsinstant_nsec;
    }
    public void setExecSec(long p){
	exec_sec = p;
    }
    public long getExecSec(){
	return exec_sec;
    }
    public void setExecNsec(long p){
	exec_nsec = p;
    }
    public long getExecNsec(){
	return exec_nsec;
    }
    public void setNominalExecSec(long p){
	nominal_exec_sec = p;
    }
    public long getNominalExecSec(){
	return nominal_exec_sec;
    }
    public void setNominalExecNsec(long p){
	nominal_exec_nsec = p;
    }
    public long getNominalExecNsec(){
	return nominal_exec_nsec;
    }
    public void setCriticality(int c){
	criticality =c;
    }
    public int getCriticality(){
	return criticality;
    }

    public ZSRMVReserve(){
    }

    public ZSRMVReserve(long p_sec,long p_nsec,long zs_sec, long zs_nsec,long e_sec, long e_nsec, long ne_sec, long ne_nsec, int c){
	period_sec = p_sec;
	period_nsec = p_nsec;
	zsinstant_sec = zs_sec;
	zsinstant_nsec = zs_nsec;
	exec_sec = e_sec;
	exec_nsec = e_nsec;
	nominal_exec_sec = ne_sec;
	nominal_exec_nsec = ne_nsec;
	criticality = c;
    }
}
