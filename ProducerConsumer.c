#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string.h>

#include <knightsim.h>
#include <rdtsc.h>

#define LOOP 10
#define LATENCY 4
#define NUMPAIRS 1

#define STACKSIZE 16384

eventcount **ec_p;
eventcount **ec_c;

void producer(void);
void consumer(void);
void producer_init(void);
void consumer_init(void);

long long p_pid = 0;
long long c_pid = 0;

unsigned long long sim_start = 0;
unsigned long long sim_time = 0;


int main(void){

	//user must initialize DESim
	KnightSim_init();

	producer_init();

	consumer_init();

	/*starts simulation and won't return until simulation
	is complete or all contexts complete*/

	printf("DESIM: Simulating %d pair(s) and %d interactions\n", NUMPAIRS, LOOP);

	sim_start = rdtsc();

	simulate();

	sim_time += (rdtsc() - sim_start);

	//clean up
	KnightSim_clean_up();

	printf("End simulation time %llu\n", sim_time);

	return 1;
}

void producer_init(void){

	int i = 0;
	char buff[100];

	//create the user defined eventcounts
	ec_p = (eventcount**) calloc(NUMPAIRS, sizeof(eventcount*));
	for(i = 0; i < NUMPAIRS; i++)
	{
		memset(buff,'\0' , 100);
		snprintf(buff, 100, "ec_p_%d", i);
		ec_p[i] = eventcount_create(strdup(buff));
	}

	//create the user defined contexts
	for(i = 0; i < NUMPAIRS; i++)
	{
		memset(buff,'\0' , 100);
		snprintf(buff, 100, "producer_%d", i);
		context_create(producer, STACKSIZE, strdup(buff), i);
	}

	return;
}

void consumer_init(void){

	int i = 0;
	char buff[100];

	ec_c = (eventcount**) calloc(NUMPAIRS, sizeof(eventcount*));
	for(i = 0; i < NUMPAIRS; i++)
	{
		memset(buff,'\0' , 100);
		snprintf(buff, 100, "ec_c_%d", i);
		ec_c[i] = eventcount_create(strdup(buff));
	}

	for(i = 0; i < NUMPAIRS; i++)
	{
		memset(buff,'\0' , 100);
		snprintf(buff, 100, "consumer_%d", i);
		context_create(consumer, STACKSIZE, strdup(buff), i);
	}

	return;
}

void producer(void){

	int my_pid = p_pid++;

	/*count_t i = 1;*/
	count_t j = 1;

	//printf("producer %d:\n\t init\n", my_pid);

	while(j <= LOOP)
	{
		//do work

		pause(LATENCY);

		printf("producer_event cycle %llu\n", CYCLE);

		/*printf("\t advancing %s cycle %llu\n", ec_c[my_pid]->name, CYCLE);*/
		advance(ec_c[my_pid]);

		/*printf("\t await %s cycle %llu\n", ec_p[my_pid]->name, CYCLE);*/
		await(ec_p[my_pid], j);
		j++;
		/*printf("producer %d:\n", my_pid);
		printf("\t advanced and doing work cycle %llu\n", CYCLE);*/

		//inc loop
		//i++;
	}

	longjmp64_2(main_context, 1);

	//context_end((long *)main_context);

	//context_terminate();

	/*printf("\t exiting %llu\n", CYCLE);*/

	return;
}

void consumer(void){

	int my_pid = c_pid++;

	count_t i = 1;

	/*printf("consumer %d:\n\t init\n", my_pid);*/

	while(1)
	{
		//await work
		/*printf("\tetime count\n");
		printf("\t await %s\n", ec_c[my_pid]->name);
		printf("\t cycle %llu\n", etime->count);*/

		await(ec_c[my_pid], i);
		i++;

		//do work

		pause(LATENCY);

		printf("consumer_event cycle %llu\n", CYCLE);


		//charge latency
		/*printf("\t charging latency %d cycle %llu\n", LATENCY, CYCLE);*/

		/*printf("consumer %d:\n\t resuming from latency cycle %llu\n",my_pid, CYCLE);*/

		/*advance producer ctx*/
		/*printf("\t advancing %s cycle %llu\n", ec_p[my_pid]->name, CYCLE);*/

		advance(ec_p[my_pid]);
	}

	fatal("consumer should never exit cycle %llu\n", CYCLE);

	return;
}
