#include <desim.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define LOOP 3
#define LATENCY 4

eventcount *ec_p;
eventcount *ec_c;

void producer(void);
void consumer(void);

int main(void){

	char buff[100];

	//user must initialize DESim
	desim_init();

	//create the user defined eventcounts
	memset(buff,'\0' , 100);
	snprintf(buff, 100, "ec_p");
	ec_p = eventcount_create(strdup(buff));

	memset(buff,'\0' , 100);
	snprintf(buff, 100, "ec_c");
	ec_c = eventcount_create(strdup(buff));
	printf("Event counts created\n");
	FFLUSH

	//create the user defined contexts
	memset(buff,'\0' , 100);
	snprintf(buff, 100, "producer");
	context_create(producer, 32768, strdup(buff));

	memset(buff,'\0' , 100);
	snprintf(buff, 100, "consumer");
	context_create(consumer, 32768, strdup(buff));
	printf("Contexts created\n");
	FFLUSH

	/*starts simulation and won't return until simulation
	is complete or all contexts complete*/
	printf("Simulate %d interactions\n", LOOP);
	simulate();
	printf("End simulation\n");

	return 1;
}

void producer(void){

	printf("producer:\n\t init\n");

	//OMG figure out who we are!!!
	//thread *thread_ptr = thread_get_ptr(pthread_self());

	count i = 0;
	count j = 1;

	while(i < LOOP)
	{
		/**********do work here***********/
		printf("\t advancing ec_c cycle %llu\n", CYCLE);
		advance(ec_c);

		//fatal("producer after pause\n");

		printf("\t await ec_p cycle %llu\n", CYCLE);
		await(ec_p, j);

		//thread_sleep(thread_ptr);

		j++;
		printf("producer:\n");
		printf("\t advanced and doing work cycle %llu\n", CYCLE);

		//inc loop
		i++;
	}

	printf("\t exiting and simulation ending cycle %llu\n", CYCLE);

	return;
}

void consumer(void){

	printf("consumer:\n\t init\n");

	//OMG figure out who we are!!!
	//thread *thread_ptr = thread_get_ptr(pthread_self());

	count i = 1;

	while(1)
	{
		//await work
		printf("\t await ec_c cycle %llu\n", CYCLE);
		await(ec_c, i);
		i++;
		printf("consumer:\n\t advanced and doing work cycle %llu\n", CYCLE);

		//thread_sleep(thread_ptr);


		/**********do work here***********/

		//charge latency
		printf("\t charging latency %d cycle %llu\n", LATENCY, CYCLE);
		pause(LATENCY);
		printf("\t resuming from latency cycle %llu\n", CYCLE);

		/*advance producer ctx*/
		printf("\t advancing ec_p cycle %llu\n", CYCLE);
		advance(ec_p);
	}

	fatal("consumer should never exit cycle %llu\n", CYCLE);

	return;
}
