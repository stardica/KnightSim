/*
 * DESim.c
 *
 *  Created on: Aug 3, 2017
 *      Author: stardica
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "desim.h"


long long ecid = 0;
count_t last_value = 0;


void desim_init(void){

	//init etime
	etime_init();

	initial_task_init();

	return;
}

/*eventcount.c********************/
eventcount etime;
eventcount *ectail = NULL;
eventcount *last_ec = NULL; /* to work with context library */


void etime_init(void){

	etime.name = NULL;
	etime.id = 0;
	etime.count = 0;
	etime.tasklist = NULL;
	etime.eclist = NULL;

	return;
}

eventcount *eventcount_create(char *name){

	eventcount *ec = NULL;

	ec = (eventcount *)malloc(sizeof(eventcount));
	assert(ec);

	eventcount_init(ec, 0, name);

	return ec;
}



void eventcount_init(eventcount * ec, count_t count, char *ecname){

	ec->name = ecname;
	ec->id = ecid++;
	ec->tasklist = NULL;
	ec->count = count;
	ec->eclist = NULL;


	//create singly linked list
    if (ectail == NULL)
    {
    	//always points to first event count created.
    	etime.eclist = ec;
    }
    else
    {
    	//old tail!!
    	ectail->eclist = ec;
    }

    //move to new tail!
    ectail = ec;

    return;
}

/* advance(ec) -- increment an event count. */
/* Don't use on time event count! */
void advance(eventcount *ec){

	task *ptr, *pptr;

	/* advance count */
	ec->count++;

	/* check for no tasks being enabled */
	ptr = ec->tasklist;

	if ((ptr == NULL) || (ptr->count > ec->count))
		return;

	/* advance down task list */
	do
	{
		/* set tasks time equal to current time */
		ptr->count = etime.count;
		pptr = ptr;
		ptr = ptr->tasklist;
	} while (ptr && (ptr->count == ec->count));

	/* add list of events to etime */
	pptr->tasklist = etime.tasklist;
	etime.tasklist = ec->tasklist;
	ec->tasklist = ptr;

	return;
}



void eventcount_destroy(eventcount *ec){

	eventcount	*front;
	eventcount	*back;
	unsigned	found = 0;

	assert(ec != NULL);

	// --- This function should unlink the eventcount from the etime eclist ---
	assert(etime.eclist != NULL);

	front = etime.eclist;
	if (ec == front)
	{
		etime.eclist = ec->eclist;
		free ((void*)ec);
		if (ectail == ec)
		{
			ectail = NULL;
		}
	}
	else
	{
	   back = etime.eclist;
	   front = front->eclist;
	   while ((front != NULL) && (!found))
	   {
		   if (ec == front)
		   {
			   back->eclist = ec->eclist;
			   free ((void*)ec);
			   found = 1;
		   }
		   else
		   {
			   back = back->eclist;
			   front = front->eclist;
		   }
	   }

	   assert(found == 1);
	   if (ectail == ec)
	   {
		   ectail = back;
	   }
	}
}


/*tasking.h**********************************/

task inittask;
task *curtask = &inittask;
task *hint = NULL;

void initial_task_init(void){

	inittask.tasklist = NULL;
	inittask.name = strdup("--this-should-never-happen--");
	inittask.count = 0;
	inittask.cdata2 = NULL;
	inittask.arg2 = NULL;
	inittask.f = NULL;
	inittask.id = -1;
	inittask.magic = STK_OVFL_MAGIC;

	return;
}

/* epause(N) -- wait N cycles.  Equivalent to await(etime, etime.c+N) */
void epause (count_t count){

	/* if no tasks or still first task, keep running */
	if ((etime.tasklist == NULL) || ((etime.count + count) <= etime.tasklist->count))
	{
		//printf("here %llu\n", etime.count);

		etime.count += count;

		//printf("here %llu\n", etime.count);

	}
	else /* switch to next task */
	{

		//printf("here\n");

		await(&etime, etime.count + count);
	}

	return;
}

/* save current task on ec, return next task to run */
static inline void find_next_task(eventcount *ec, count_t value){

	//this EC is not ETIME!
	//tasks are stored on each individual ec
	task *pptr = NULL;
	pptr = ec->tasklist;

	//ec->tasklist;

	/* save current task on ec's tasklist */
	if (( pptr == NULL) || (value < pptr->count))
	{
		/* insert at head of list */
		if (curtask)
		{
			ec->tasklist = curtask;
			curtask->tasklist = pptr;  //same as ec->tasklist = pptr
		}
		else
		{
			printf("desim no current task?\n");
			exit(0);
		}

	}
	else
	{

		/* insert in middle of list */
		task *ptr;

		bool time_list = (ec == &etime);

		if (time_list && hint && (value >= hint->count))
		{
			ptr = hint->tasklist;
			pptr = hint;
		}
		else
		{
			ptr = pptr->tasklist;
		}

		//go down the list until you find an older task
		while (ptr && (value >= ptr->count))
		{
			pptr = ptr;
			ptr = ptr->tasklist;
		}


		if (curtask)
		{
			pptr->tasklist = curtask;
			curtask->tasklist = ptr;
		}

		if (time_list)
		{
			hint = curtask;
		}
	}

	curtask->count = value;

	/*get next task to run*/
	curtask = etime.tasklist;

	if (hint == curtask)
	{
		hint = NULL;
	}

	if (curtask == NULL)
	{
		context_cleanup ();

		context_end();
	}

	assert (curtask->count >= etime.count);

  	etime.tasklist = curtask->tasklist;
  	etime.count = curtask->count;

  	//printf("curtask %s \n", curtask->name);
	//printf("etime %s count %d\n", etime.tasklist->name, etime.tasklist->count);

  	return;
}

process *context_select(void){

	//printf("last ec %s\n", last_ec->name);

	if (last_ec)
	{
		//printf("finding next\n");
		find_next_task (last_ec, last_value);
		last_ec = NULL;
	}
	else
	{
		curtask = etime.tasklist;

		if (hint == curtask)
		{
			hint = NULL;
		}

		if (curtask == NULL)
		{
			context_cleanup();

			printf("exiting after switch\n");

			context_end();
		}

		/*go through the list newest to oldest
		etime is now pointing back one element in the list*/

		etime.tasklist = curtask->tasklist;
		etime.count = curtask->count;

		//printf("curtask %s count %llu\n", curtask->name, curtask->count);
		//printf("etime %s count %llu\n", etime.tasklist->name, etime.tasklist->count);

	}

	return (process*)&(curtask->c);
}

void switch_context (eventcount *ec, count_t value){

	process *p = NULL;
	last_ec = ec;
	last_value = value;

	p = context_select();
	assert(p);



	//if (p)
	//{
	context_switch(p);
	//}


	return;
}


/* await(ec, value) -- suspend until ec.c >= value. */
void await (eventcount *ec, count_t value){

	/*todo*/
	/*check for stack overflows*/

	// if stack overflowed before call to await, it will most likely have
	// thrashed magic.
	/*assert(curtask->magic == STK_OVFL_MAGIC);*/

	if (ec->count >= value)
		return;

	switch_context(ec, value);

	return;
}



void simulate (void){

	//printf("simulate called\n");

	process *process = NULL;

	//simulate
	if(!context_simulate())
	{
		process = context_select();
		assert(process);

		context_switch(process);

		context_cleanup();

		context_end();
	}

	return;
}

/* create_task(task, stacksize) -- create a task with specified stack size. */
/* task is code pointer and closure pointer */
task * context_create(void (*func)(void), unsigned stacksize, char *name){

	task *tptr = NULL;

	/* stacksize should be multiple of unsigned size */
	assert ((stacksize % sizeof(unsigned)) == 0);

	tptr = (task *) malloc(sizeof(task));
	assert(tptr);

	tptr->count = etime.count;
	tptr->name = name;
	tptr->c.stack = (char *)malloc(stacksize);
	assert(tptr->c.stack);
	tptr->c.sz = stacksize;
	tptr->magic = STK_OVFL_MAGIC; // for stack overflow check

	//list work. forms a singly linked list, with newest task at the head.
	tptr->tasklist = etime.tasklist;
	etime.tasklist = tptr;

	context_init ((process*)&tptr->c, func);

	return tptr;
}


// delete last inserted task
void remove_last_task (task *t){

	assert (etime.tasklist == t);
	etime.tasklist = t->tasklist;
	t->tasklist = NULL;

	return;
}

void context_destroy (process *p){

	/* free stack space */
	free (((context*)p)->stack);
}


void context_set_id(int id){

	curtask->id = id;
	return;
}


int context_get_id(void){

	return curtask->id;
}


char *get_task_name (void){

	return curtask->name;
}

/*context.c*******************************************/
/* current process and terminated process */
process *current_process = NULL;
process *terminated_process = NULL;


void context_cleanup (void){

  if (terminated_process)
  {
    context_destroy (terminated_process);
    terminated_process = NULL;
  }
}


static void context_stub (void){

  //printf("context stub\n");

  if (terminated_process)
  {
    context_destroy (terminated_process);
    terminated_process = NULL;
  }

  (*current_process->c.start)();

  /*if current current process returns i.e. hits the bottom of its function
  it will return here. Then we need to terminate the process*/

  //printf("exiting\n");

  terminated_process = current_process;

  context_switch (context_select());

}

void context_exit (void){

  terminated_process = current_process;
  context_switch (context_select ());
}


#if defined(__linux__) && defined(__i386__)

void context_switch (process *p)
{

	if (!current_process || !setjmp32_2(current_process->c.buf))
	{
	  current_process = p;
	  longjmp32_2(p->c.buf, 1);
	}
	if (terminated_process)
	{
	  context_destroy (terminated_process);
	  terminated_process = NULL;
	}

}

void context_init (process *p, void (*f)(void)){

	p->c.start = f; /*assigns the head of a function*/
	p->c.buf[5] = ((int)context_stub);
	p->c.buf[4] = ((int)((char*)p->c.stack + p->c.sz - 4));
}

void context_end(void){

	longjmp32_2(main_context, 1);
	return;
}

int context_simulate(void){

	return setjmp32_2(main_context);
}

#elif defined(__linux__) && defined(__x86_64)

void context_switch (process *p){

	//setjmp returns 1 if jumping to this position via longjmp
	if (!current_process || !setjmp64_2(current_process->c.buf))
	{
		/*first round takes us to context stub
		subsequent rounds take us to an await or pause*/

	  current_process = p;
	  longjmp64_2(p->c.buf, 1);
	}

	if (terminated_process)
	{
	  context_destroy (terminated_process);
	  terminated_process = NULL;
	}

  //printf("context switch\n");
}


void context_init (process *p, void (*f)(void)){

	p->c.start = f; /*assigns the head of a function*/
	p->c.buf[7] = (long long)context_stub; /*where the jump will go to*/
	p->c.buf[6] = (long long)((char *)p->c.stack + p->c.sz - 4); /*top of the stack*/
}

void context_end(void){

	longjmp64_2(main_context, 1);
	return;
}


int context_simulate(void){

	return setjmp64_2(main_context);
}

#else
#error Unsupported machine/OS combination
#endif
