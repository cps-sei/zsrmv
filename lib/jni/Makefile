all:	ZSRMVSched.class ZSRMVReserve.class libzsrmvsched.so

clean:
	rm -f ZSRMVSched.class ZSRMVReserve.class libzsrmvsched.so ZSRMVSched.h
	rm -f *~

ZSRMVReserve.class:	ZSRMVReserve.java
	javac ZSRMVReserve.java

ZSRMVSched.class:	ZSRMVSched.java ZSRMVReserve.class
	javac ZSRMVSched.java

ZSRMVSched.h:	ZSRMVSched.class
	javah ZSRMVSched

libzsrmvsched.so:	libzsrmvsched.c ZSRMVSched.h
	gcc -shared libzsrmvsched.c -o libzsrmvsched.so -lzsv -L../ -I../../ -I${JAVA_HOME}/include -I${JAVA_HOME}/include/linux -fPIC
