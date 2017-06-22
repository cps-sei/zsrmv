#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <zsrmvapi.h>

int main(int argc, char *argv[])
{
  char line[100];
  char buf[100];
  int rid;
  unsigned long long ts;
  int event;
  unsigned long long divisor;
  unsigned long long max=0,min=ULLONG_MAX;
  
  if (argc != 4){
    printf("usage %s <infile> <outfile> <scale divisor>\n",argv[0]);
    return -1;
  }

  FILE *ifid = fopen(argv[1],"r");

  if (ifid == NULL){
    printf("could not open file :%s for reading\n",argv[1]);
    return -1;
  }

  FILE *ofid = fopen(argv[2],"w+");

  if (ofid == NULL){
    printf("could not open file %s for writing\n",argv[2]);
    return -1;
  }

  divisor = atol(argv[3]);

  while (fgets(line,100,ifid)){
    sscanf(line,"%d %llu %d",&rid,&ts,&event);
    ts = ts / divisor;
    if (ts > max){
      max = ts;
    }

    if (ts < min){
      min = ts;
    }
  }

  rewind(ifid);
  
  while (fgets(line,100,ifid)){
    sscanf(line,"%d %llu %d",&rid,&ts,&event);
    ts = ts / divisor;
    ts = ts - min;
    if (strstr(STRING_TRACE_EVENT(event),"unknown")){
      sprintf(buf,"unknown(%d)",event);
    } else {
      strcpy(buf,STRING_TRACE_EVENT(event));
    }
    fprintf(ofid,"%d %llu %s\n",rid,ts,buf);
  }

  fclose(ofid);
  fclose(ifid);
  
}
