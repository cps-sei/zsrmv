#include <stdio.h>
#include "zsrmv.h"

struct reserve table1[] = {
  {.period_ns = 400L,
   .exectime_ns = 200L,
   .nominal_exectime_ns = 200L,
   .exectime_in_rm_ns = 0L,
   .pid =0,
   .criticality = 1
  },
  {.period_ns = 800L,
   .exectime_ns = 500L,
   .nominal_exectime_ns = 250L,
   .exectime_in_rm_ns = 0L,
   .pid =0,
   .criticality = 2
  },
};

struct reserve table2[] = {
  {.period_ns = 100L,
   .exectime_ns = 10L,
   .nominal_exectime_ns = 5L,
   .exectime_in_rm_ns = 0L,
   .pid =0,
   .criticality = 1
  },
  {.period_ns = 200L,
   .exectime_ns = 20L,
   .nominal_exectime_ns = 10L,
   .exectime_in_rm_ns = 0L,
   .pid =0,
   .criticality = 1
  },
  {.period_ns = 400L,
   .exectime_ns = 40L,
   .nominal_exectime_ns = 20L,
   .exectime_in_rm_ns = 0L,
   .pid =0,
   .criticality = 1
  },
  {.period_ns = 800L,
   .exectime_ns = 80L,
   .nominal_exectime_ns = 40L,
   .exectime_in_rm_ns = 0L,
   .pid =0,
   .criticality = 1
  },
  {.period_ns = 1600L,
   .exectime_ns = 160L,
   .nominal_exectime_ns = 80L,
   .exectime_in_rm_ns = 0L,
   .pid = 0,
   .criticality = 1
  }
};

int main(int argc, char *argv[])
{
  int idx;
  int i;
  int selectedIdx;
  unsigned long long Z;

  // test set membership
  printf("For task[P:%llu, Crit:%d] isHigherPrioHigherCrit(task[P:%llu,Crit:%d]) = %d\n",
	 table2[2].period_ns,table2[2].criticality,
	 table2[1].period_ns,table2[1].criticality,
	 isHigherPrioHigherCrit(&table2[2],&table2[1])
	 );

  printf("For task[P:%llu, Crit:%d] isLowerPrioHigherCrit(task[P:%llu,Crit:%d]) = %d\n",
	 table1[0].period_ns,table1[0].criticality,
	 table1[1].period_ns,table1[1].criticality,
	 isLowerPrioHigherCrit(&table1[0],&table1[1])
	 );

  printf("For task[P:%llu, Crit:%d] isLowerPrioHigherCrit(task[P:%llu,Crit:%d]) = %d\n",
	 table1[0].period_ns,table1[0].criticality,
	 table1[1].period_ns,table1[1].criticality,
	 isLowerPrioHigherCrit(&table1[0],&table1[1])
	 );

  printf("For task[P:%llu, Crit:%d] isHigherPrioLowerCrit(task[P:%llu,Crit:%d]) = %d\n",
	 table1[1].period_ns,table1[1].criticality,
	 table1[0].period_ns,table1[0].criticality,
	 isHigherPrioLowerCrit(&table1[1],&table1[0])
	 );
  
  printf("For task[P:%llu, Crit:%d] isHigherPrioSameCrit(task[P:%llu,Crit:%d]) = %d\n",
	 table2[4].period_ns,table2[4].criticality,
	 table2[3].period_ns,table2[3].criticality,
	 isHigherPrioSameCrit(&table2[4],&table2[4])
	 );


  // print relative sets

  printf("Higher Priority Higher Criticality Than task[P:%llu,Crit:%d]\n",table2[2].period_ns,table2[2].criticality);
  idx=0;
  while((selectedIdx = getNextInSet(table2, &idx, 5, &table2[2], isHigherPrioHigherCrit)) >=0){
    printf("task[P:%llu, crit:%d]\n",table2[selectedIdx].period_ns, table2[selectedIdx].criticality);
  }
  printf("----------------\n");
  
  printf("Higher Priority Lower Criticality Than task[P:%llu,Crit:%d]\n",table2[4].period_ns,table2[4].criticality);
  idx=0;
  while((selectedIdx = getNextInSet(table2, &idx, 5, &table2[4], isHigherPrioLowerCrit)) >=0){
    printf("task[P:%llu, crit:%d]\n",table2[selectedIdx].period_ns, table2[selectedIdx].criticality);
  }
  printf("----------------\n");


  printf("Higher Priority Same Criticality Than task[P:%llu,Crit:%d]\n",table2[0].period_ns,table2[0].criticality);
  idx=0;
  while((selectedIdx = getNextInSet(table2, &idx, 5, &table2[0], isHigherPrioSameCrit)) >=0){
    printf("task[P:%llu, crit:%d]\n",table2[selectedIdx].period_ns, table2[selectedIdx].criticality);
  }
  printf("----------------\n");

  printf("Higher Priority Same Criticality Than task[P:%llu,Crit:%d]\n",table2[4].period_ns,table2[4].criticality);
  idx=0;
  while((selectedIdx = getNextInSet(table2, &idx, 5, &table2[4], isHigherPrioSameCrit)) >=0){
    printf("task[P:%llu, crit:%d]\n",table2[selectedIdx].period_ns, table2[selectedIdx].criticality);
  }
  printf("----------------\n");

  
  // print response times

  printf("task[P:%llu,Crit:%d].response:%llu\n",
	 table1[1].period_ns,table1[1].criticality,
	 getResponseTimeCritNs(table1,2,&table1[1])
	 );

  printf("admit(task[P:%llu,Crit:%d])=%d\n",
	table1[1].period_ns,table1[1].criticality,
	admit(table1,2,&table1[1],&Z)
	);
  printf("\t Z:%llu\n",Z);

  printf("admit(task[P:%llu,Crit:%d])=%d\n",
	 table1[0].period_ns,table1[0].criticality,
	 admit(table1,2,&table1[0],&Z)
	 );
  printf("\t Z:%llu\n",Z);

  printf("******************\n");
  printf("*   full taskset *\n");
  printf("******************\n");

  for (i=0; i<5;i++){
    printf("Testing for task[P:%llu]\n",table2[i].period_ns);
    printf("admit(task[P:%llu,Crit:%d])=%d\n",
	   table2[i].period_ns,table2[i].criticality,
	   admit(table2,5,&table2[i],&Z)
	   );
    printf("\t Z:%llu\n",Z);    
  }
  
  return 0;
}
