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
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>

#include <zsrmvapi.h>

#define TRACE_BUF_SIZE 100000
struct trace_rec_t trace[TRACE_BUF_SIZE];

int main(int argc, char *argv[])
{
  int r,i,cnt;
  int schedfd;
  
  if ((schedfd = zsv_open_scheduler())<0){
    printf("Error opening the scheduler");
    return -1;
  }

  r = read(schedfd,trace,TRACE_BUF_SIZE*sizeof(struct trace_rec_t));

  if (r<0){
    printf("error\n");
  } else {
    FILE *ofid = fopen("dump.csv","w+");
    if (ofid == NULL){
      printf("Error opening output file\n");
      return -1;
    }
    cnt = r / sizeof(struct trace_rec_t);
    for (i=0;i<cnt;i++){
      fprintf(ofid, "%d %lld %d\n",
  	     trace[i].rid,
  	     trace[i].timestamp_ns,
  	     trace[i].event_type);
    }
    fclose(ofid);
  }

  zsv_close_scheduler(schedfd);
  
  printf("DONE\n");

  return 0;

}
