#include <desim.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

eventcount *sim_start;
eventcount *sim_finish;
eventcount *cycle;
eventcount *next_cycle;

context *ctx1;
context *ctx2;
context *ctx3;

void run(void);
void cgm_mem_run(void);
void cpu_gpu_run(void);

int main(void){

	char buff[100];

	/*init desim*/
	desim_init();

	//eventcounts
	memset(buff,'\0' , 100);
	snprintf(buff, 100, "1");
	sim_start = eventcount_create(strdup(buff));

	memset(buff,'\0' , 100);
	snprintf(buff, 100, "2");
	sim_finish = eventcount_create(strdup(buff));

	memset(buff,'\0' , 100);
	snprintf(buff, 100, "3");
	cycle = eventcount_create(strdup(buff));

	printf("ec name %s\n", sim_start->name);
	printf("ec name %s\n", sim_finish->name);
	printf("ec name %s\n", cycle->name);


	//tasks
	memset(buff,'\0' , 100);
	snprintf(buff, 100, "1");
	context_create(cpu_gpu_run, 32768, strdup(buff));

	memset(buff,'\0' , 100);
	snprintf(buff, 100, "2");
	context_create(cgm_mem_run, 32768, strdup(buff));

	memset(buff,'\0' , 100);
	snprintf(buff, 100, "3");
	context_create(run, 32768, strdup(buff));

	printf("num in list %d\n", desim_list_count(ctxlist));
	int i = 0;
	context *ctx = NULL;
	for(i=0; i<desim_list_count(ctxlist); i++)
	{
		ctx = desim_list_get(ctxlist, i);
		printf("ctx name %s i %d\n", ctx->name, i);
	}

	//simulate
	simulate();

	printf("end simulation\n");

	return 1;
}

int done = 0;

void run(void){

	int i = 0;

	printf("run\n");

	do
	{
		printf("advance cycle\n");
		advance(cycle);

		printf("start pause cycle %llu\n", etime.count);

		pause(5);
		printf("end pause cycle %llu done %d\n", etime.count, done);

		//start next cycle
		i++;

		/*if(i == 2)
			exit(0);*/

	}while (!done);


	printf("exiting, time %llu\n", etime.count);

	return;
}


void cgm_mem_run(void){

	int h = 0;
	int i = 0;
	int j = 1;
	int k = 1;


	printf("cgm_mem_run\n");

	while(1)
	{
		//simulation execution
		printf("await cycle\n");
		await(cycle, k); /*************/

		printf("awaited cycle\n");
		k++;

		while(i<1)
		{
			printf("advance sim_start\n");
			advance(sim_start);

			//pause(5);

			//dump stats on exit.
			printf("await sim_finish\n");

			await(sim_finish, j);

			printf("awaited sim_finish\n");


			j++;

			i++;
		}
		i=0;

		h++;

		printf("h %d\n", h);
		if(h >= 2)
			done++;
	}

	printf("BAD mem run\n");

	return;
}


void cpu_gpu_run(void){

	long long i = 1;

	printf("cpu_gpu_run\n");


	while(1)
	{

		printf("await sim_start value %llu\n", i);
		await(sim_start, i);

		printf("awaited sim_start\n");
		i++;

		printf("cpu_gpu_run %llu\n", (i - 1));

		printf("advance sim_finish\n");
		advance(sim_finish);

	}

	printf("bad gpu run\n");

	return;
}
