package edu.cmu.sei.ZSRM;

public class ZSRMVScheduler{
    static {
	try {
	    cz.adamh.utils.NativeUtils.loadLibraryFromJar("/lib/libzsrmvsched.so");
	} catch(Exception e){
	    e.printStackTrace();
	}
    }

    public native boolean isAdmissible(ZSRMVReserve[] reserveSet);

    // for testing
    public static void main(String[] args){
	ZSRMVScheduler sched = new ZSRMVScheduler();
	ZSRMVReserve rsvtable[] = new ZSRMVReserve[2];

	rsvtable[0] = new ZSRMVReserve(0,   // period sec
				       400, // period nsec
				       0,   // zs sec
				       0,   // zs nsec
				       0,   // exec sec
				       200, // exec nsec
				       0,   // nominal exec sec
				       200, // nominal exec nsec
				       1    // criticality
				       );  
	rsvtable[1] = new ZSRMVReserve(0,   // period sec
				       800, // period nsec
				       0,   // zs sec
				       0,   // zs nsec
				       0,   // exec sec
				       500, // exec nsec
				       0,   // nominal exec sec
				       250, // nominal exec nsec
				       2    // criticality
				       );

	if(sched.isAdmissible(rsvtable)){
	    System.out.println("Schedulable");
	    for (int i=0;i<rsvtable.length;i++){
		System.out.println("\t zs="+rsvtable[i].zsinstant_sec+":"+
				   rsvtable[i].zsinstant_nsec);
	    }
	} else {
	    System.out.println("Not schedulable");
	}
	
    }
}
