#include <desim.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

eventcount *e1;
eventcount *e2;
eventcount *e3;

void context1(void);
void context2(void);
void context3(void);

int done = 0;
int loop = 1;

int main(void){

	char buff[100];

	//user must initialize DESim
	desim_init();


	memset(buff,'\0' , 100);
	snprintf(buff, 100, "2");
	e3 = eventcount_create(strdup(buff));

	//create eventcounts
	memset(buff,'\0' , 100);
	snprintf(buff, 100, "1");
	e2 = eventcount_create(strdup(buff));

	memset(buff,'\0' , 100);
	snprintf(buff, 100, "3");
	e1 = eventcount_create(strdup(buff));
	printf("Event counts created\n");


	//create contexts
	memset(buff,'\0' , 100);
	snprintf(buff, 100, "3");
	context_create(context3, 32768, strdup(buff));

	memset(buff,'\0' , 100);
	snprintf(buff, 100, "2");
	context_create(context2, 32768, strdup(buff));

	memset(buff,'\0' , 100);
	snprintf(buff, 100, "1");
	context_create(context1, 32768, strdup(buff));
	printf("Contexts created\n");

	/*starts simulation and won't return until simulation
	is complete or all contexts complete*/
	printf("Simulate\n");
	simulate();
	printf("End simulation\n");

	//when simulation is complete program will exit
	return 1;
}

void context1(void){

	printf("context1 init\n");

	do
	{
		printf("context1 advance e1 cycle %llu\n", CYCLE);
		advance(e1);

		printf("context1 pausing cycle %llu\n", CYCLE);
		pause(5);
		printf("context1 resuming cycle %llu\n", CYCLE);

	}while(!done);

	printf("context1 exiting, cycle %llu\n", CYCLE);

	return;
}



void context2(void){

	int h = 0;
	int i = 0;
	//int j = 1;
	int k = 1;

	printf("context2 init\n");

	do
	{
		//simulation execution
		printf("context2 await e1 cycle %llu\n", CYCLE);
		await(e1, k);
		k++;
		printf("context2 resuming cycle %llu\n", CYCLE);

		h++;
		while(i<loop)
		{
			printf("context2 advance e2 cycle %llu\n", CYCLE);
			advance(e2);

			/*printf("context2 await e3 cycle %llu\n", CYCLE);
			await(e3, j);
			j++;
			printf("context2 resuming cycle %llu\n", CYCLE);*/

			printf("context2 pausing cycle %llu\n", CYCLE);
			pause(10);
			printf("context2 resuming cycle %llu\n", CYCLE);

			i++;
		}
		i=0;

		if(h >= 2)
			done++;

	}while(1);

	fatal("context2 exiting cycle %llu\n", CYCLE);

	return;
}


void context3(void){

	long long i = 1;

	printf("context3 init\n");

	do
	{
		printf("context3 await e2 cycle %llu\n", CYCLE);
		await(e2, i);
		i++;
		printf("context3 resuming cycle %llu\n", CYCLE);

		//printf("context3 advance e3 cycle %llu\n", CYCLE);
		//advance(e3);

	}while(1);

	fatal("context3 exiting cycle %llu\n", CYCLE);

	return;
}
