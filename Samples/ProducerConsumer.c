#include "desim.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string.h>
#include "cpucounters.h"

#define LOOP 10
#define LATENCY 4
#define NUMPAIRS 32
#define SECOND 1000000
#define HALFSECOND 500000
#define QUARTERSECOND 250000
#define TWOMILLISECOND 2000
#define ONEMILLISECOND 1000
#define HALFMILISECOND 500
#define QUARTERMILISECOND 250

unsigned long long p_start = 0;
unsigned long long p_time = 0;

eventcount **ec_p;
eventcount **ec_c;

void producer(void);
void consumer(void);
void producer_init(void);
void consumer_init(void);

long long p_pid = 0;
long long c_pid = 0;


int main(void){

	//user must initialize DESim
	desim_init();

	producer_init();

	consumer_init();

	/*starts simulation and won't return until simulation
	is complete or all contexts complete*/
#ifdef NUM_THREADS
	printf("PDESim: Simulating %d threads, %d pair(s), and %d interactions\n", NUM_THREADS, NUMPAIRS, LOOP);
#else
	printf("DESIM: Simulating %d pair(s) and %d interactions\n", NUMPAIRS, LOOP);
#endif

	#ifdef MEASURE
		p_start = RDTSC();
	#endif

	simulate();

	#ifdef MEASURE
		p_time = (RDTSC() - p_start);
	#endif

	//Temporarily here for measurement purposes
	desim_end();

	printf("End simulation time in cycles %llu\n", p_time);

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
		context_create(producer, 32768, strdup(buff), i);
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
		context_create(consumer, 32768, strdup(buff), i);
	}

	return;
}

void producer(void){

#ifdef NUM_THREADS
	thread *thread_ptr = thread_get_ptr(pthread_self());
	int my_pid = thread_ptr->context->id;
#else
	int my_pid = p_pid++;
#endif


	count_t i = 0;
	count_t j = 1;

	printf("producer %d:\n\t init\n", my_pid);

	while(i < LOOP)
	{
		//do work
		usleep(TWOMILLISECOND);

		printf("\t advancing %s cycle %llu\n", ec_c[my_pid]->name, CYCLE);
		advance(ec_c[my_pid]);
		//printf("before ec name %s count %llu\n", ec_c[my_pid]->name, ec_c[my_pid]->count);

		//printf("after ec name %s count %llu\n", ec_c[my_pid]->name, ec_c[my_pid]->count);

		printf("\t await %s cycle %llu\n", ec_p[my_pid]->name, CYCLE);
		await(ec_p[my_pid], j);

		//thread_sleep(thread_ptr);

		j++;
		printf("producer %d:\n", my_pid);
		printf("\t advanced and doing work cycle %llu\n", CYCLE);

		//inc loop
		i++;
	}

	printf("\t exiting %llu\n", CYCLE);

	/*eventcount* ec_ptr = NULL;
	printf("eventcounts\n");
	LIST_FOR_EACH_L(ecdestroylist, i, 0)
	{
		ec_ptr = (eventcount*)desim_list_get(ecdestroylist, i);

		printf("ec name %s count %llu\n", ec_ptr->name, ec_ptr->count);
	}*/


#ifdef NUM_THREADS
	thread_context_terminate();
#else
	context_terminate();
#endif

	return;
}

void consumer(void){

#ifdef NUM_THREADS
	thread *thread_ptr = thread_get_ptr(pthread_self());
	int my_pid = thread_ptr->context->id;
#else
	int my_pid = c_pid++;
#endif

	count_t i = 1;

	printf("consumer %d:\n\t init\n", my_pid);

	while(1)
	{
		//await work
		printf("\t await %s cycle %llu\n", ec_c[my_pid]->name, CYCLE);
		await(ec_c[my_pid], i);
		i++;
		printf("consumer %d:\n\t advanced and doing work cycle %llu\n", my_pid, CYCLE);

		//do work
		usleep(TWOMILLISECOND);

		//charge latency
		printf("\t charging latency %d cycle %llu\n", LATENCY, CYCLE);
		pause(LATENCY);
		printf("consumer %d:\n\t resuming from latency cycle %llu\n",my_pid, CYCLE);

		/*advance producer ctx*/
		printf("\t advancing %s cycle %llu\n", ec_p[my_pid]->name, CYCLE);
		advance(ec_p[my_pid]);
	}

	fatal("consumer should never exit cycle %llu\n", CYCLE);

	return;
}
