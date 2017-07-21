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
