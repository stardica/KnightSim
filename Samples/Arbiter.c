#include <desim.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define LOOP 3
#define LATENCY 4

#define P_TIME (etime->count >> 1)
#define P_PAUSE(p_delay) pause((p_delay)<<1)

#define AWAIT_P_PHI0 if (etime->count & 0x1) pause(1)
#define AWAIT_P_PHI1 if (!(etime->count & 0x1)) pause(1)

eventcount *ec_p;
eventcount *ec_a;
eventcount *ec_c;

void producer(void);
void arbiter(void);
void consumer(void);

count producer_pid = 0;
count producer_ready[2] = {0,0};


int main(void){

	char buff[100];
	int i = 0;

	//user must initialize DESim
	desim_init();

	//create the user defined eventcounts

	//this creates an event couint for producer a and b
	ec_p = (void *) calloc(2, sizeof(eventcount));
	for(i = 0; i < 2; i++)
	{
		memset(buff,'\0' , 100);
		snprintf(buff, 100, "ec_p_%d", i);
		ec_p[i] = *(eventcount_create(strdup(buff)));
	}

	memset(buff,'\0' , 100);
	snprintf(buff, 100, "ec_a");
	ec_a = eventcount_create(strdup(buff));

	memset(buff,'\0' , 100);
	snprintf(buff, 100, "ec_c");
	ec_c = eventcount_create(strdup(buff));
	printf("Event counts created\n");


	//create the user defined contexts
	//l1 i caches
	for(i = 0; i < 2; i++)
	{
		memset(buff,'\0' , 100);
		snprintf(buff, 100, "producer_%d", i);
		context_create(producer, 32768, strdup(buff));
	}

	memset(buff,'\0' , 100);
	snprintf(buff, 100, "arbiter");
	context_create(arbiter, 32768, strdup(buff));


	memset(buff,'\0' , 100);
	snprintf(buff, 100, "consumer");
	context_create(consumer, 32768, strdup(buff));
	printf("Contexts created\n");

	/*starts simulation and won't return until simulation
	is complete or all contexts complete*/
	printf("Simulate %d interactions\n", LOOP);
	simulate();
	printf("End simulation\n");

	return 1;
}

void producer(void){

	count i = 0;
	count j = 1;

	//use this to determine which producer you are
	count my_pid = producer_pid++;

	printf("producer_%d:\n\t init\n", (int)my_pid);
	while(i < LOOP)
	{
		if(producer_ready[my_pid] == 1)
		{
			/*wait for consumer to finish its task*/
			P_PAUSE(1);
		}
		else
		{
			/**********do work here***********/
			printf("producer_%d:\n", (int)my_pid);

			/*set busy (normally this would be based on the state of some
			simulated structure like an input queue*/
			producer_ready[my_pid] = 1;

			printf("\t advancing ec_a cycle %llu\n", CYCLE);
			advance(ec_a);
			printf("\t advanced and doing work cycle %llu\n", CYCLE);

			P_PAUSE(1);

			printf("producer_%d:\n", (int)my_pid);

			//await work
			printf("\t await ec_p cycle %llu\n", CYCLE);
			await(ec_p, j);

			//inc loop
			i++;
		}
	}

	printf("\t exiting and simulation ending cycle %llu\n", CYCLE);

	return;
}

void arbiter(void){

	count i = 1;

	int input = 0;

	printf("arbiter:\n\t init\n");
	while(1)
	{
		//await work
		printf("\t await ec_a cycle %llu\n", CYCLE);
		await(ec_a, i);
		i++;

		//work in the sub clock domain
		AWAIT_P_PHI1;

		printf("arbiter:\n\t advanced and doing work cycle %llu\n", CYCLE);

		assert(producer_ready[1] == 1 || producer_ready[0] == 1);

		if(producer_ready[1] == 1)
			input = 1;
		else if (producer_ready[0] == 1)
			input = 0;

		AWAIT_P_PHI0;
		printf("arbiter:\n");

		producer_ready[input] = 0;

		/*advance producer ctx*/
		printf("\t advancing ec_c cycle %llu\n", CYCLE);
		advance(ec_c);
	}

	fatal("arbiter should never exit cycle %llu\n", CYCLE);

	return;
}

void consumer(void){

	count i = 1;

	printf("consumer:\n\t init\n");
	while(1)
	{
		//await work
		printf("\t await ec_c cycle %llu\n", CYCLE);
		await(ec_c, i);
		i++;
		printf("consumer:\n\t advanced and doing work cycle %llu\n", CYCLE);

		/**********do work here***********/

		//charge latency
		printf("\t charging latency %d cycle %llu\n", LATENCY, CYCLE);
		pause(LATENCY);

		printf("consumer:\n");
		printf("\t resuming from latency cycle %llu\n", CYCLE);

		/*advance producer ctx*/
		printf("\t advancing ec_p cycle %llu\n", CYCLE);
		advance(ec_p);
	}

	fatal("consumer should never exit cycle %llu\n", CYCLE);

	return;
}
