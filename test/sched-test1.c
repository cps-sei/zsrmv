#include <zsrmvapi.h>

struct reserve_spec_t rtable1[] = {
  {.period_sec = 0,
   .period_nsec = 400,
   .zsinstant_sec=0,
   .zsinstant_nsec=0,
   .exec_sec=0,
   .exec_nsec=200,
   .nominal_exec_sec=0,
   .nominal_exec_nsec=200,
   .criticality=1
  },
  {.period_sec=0,
   .period_nsec=800,
   .zsinstant_sec=0,
   .zsinstant_nsec=0,
   .exec_sec=0,
   .exec_nsec=500,
   .nominal_exec_sec=0,
   .nominal_exec_nsec=250,
   .criticality=2
  }
};

int main(int argc, char *argv)
{
  int i;

  if (zsv_is_admissible(rtable1,2)){
    printf("Taskset Admitted\n");
    for (i=0;i<2;i++){
      printf("\t task[P:%ld:%ld].zs=%ld:%ld\n",
	     rtable1[i].period_sec,
	     rtable1[i].period_nsec,
	     rtable1[i].zsinstant_sec,
	     rtable1[i].zsinstant_nsec);
    }
  } else {
    printf("Taskset Not Admitted\n");
  }
  return 0;
}
