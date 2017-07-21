all:	ZSRMVScheduler.jar

clean:
	rm -f edu/cmu/sei/ZSRM/ZSRMVScheduler.class edu/cmu/sei/ZSRM/ZSRMVReserve.class edu_cmu_sei_ZSRM_ZSRMVScheduler.h cz/adamh/utils/NativeUtils.class lib/libzsrmvsched.so ZSRMVScheduler.jar
	rm -f *~

ZSRMVScheduler.jar:	edu/cmu/sei/ZSRM/ZSRMVScheduler.class edu/cmu/sei/ZSRM/ZSRMVReserve.class ZSRMVScheduler.h cz/adamh/utils/NativeUtils.class lib/libzsrmvsched.so
	jar -cvf ZSRMVScheduler.jar cz/* edu/* lib/*

cz/adamh/utils/NativeUtils.class:	cz/adamh/utils/NativeUtils.java
	javac cz/adamh/utils/NativeUtils.java

edu/cmu/sei/ZSRM/ZSRMVReserve.class:	edu/cmu/sei/ZSRM/ZSRMVReserve.java
	javac edu/cmu/sei/ZSRM/ZSRMVReserve.java

edu/cmu/sei/ZSRM/ZSRMVScheduler.class:	edu/cmu/sei/ZSRM/ZSRMVScheduler.java edu/cmu/sei/ZSRM/ZSRMVReserve.class cz/adamh/utils/NativeUtils.class
	javac edu/cmu/sei/ZSRM/ZSRMVScheduler.java

ZSRMVScheduler.h:	edu/cmu/sei/ZSRM/ZSRMVScheduler.class
	javah edu.cmu.sei.ZSRM.ZSRMVScheduler

lib/libzsrmvsched.so:	libzsrmvsched.c ZSRMVScheduler.h
	gcc -shared libzsrmvsched.c -o lib/libzsrmvsched.so -lzsv -L../ -I../../ -I${JAVA_HOME}/include -I${JAVA_HOME}/include/linux -fPIC
