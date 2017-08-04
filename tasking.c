#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "tasking.h"
#include "eventcount.h"
#include "contexts.h"


//static void (*cleanup_stuff) (void) = NULL;

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
		etime.count += count;
	}
	else /* switch to next task */
	{
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

		boolean time_list = (ec == &etime);

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

		end_simulate();
	}

	assert (curtask->count >= etime.count);

  	etime.tasklist = curtask->tasklist;
  	etime.count = curtask->count;

  	printf("curtask %s \n", curtask->name);
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

			end_simulate();
		}

		/*go through the list newest to oldest
		etime is now pointing back one element in the list*/
		etime.tasklist = curtask->tasklist;
		etime.count = curtask->count;


		printf("curtask %s count %d\n", curtask->name, curtask->count);
		printf("etime %s count %d\n", etime.tasklist->name, etime.tasklist->count);

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

	printf("simulate called\n");

	process *process = NULL;

	//simulate
	if(!desim_end())
	{
		process = context_select();
		assert(process);

		context_switch(process);

		context_cleanup();

		end_simulate();
	}

	return;
}




void context_destroy (process *p){

	/* free stack space */
	free (((context*)p)->stack);
}

/* create_task(task, stacksize) -- create a task with specified stack size. */
/* task is code pointer and closure pointer */
task * task_create(void (*func)(void), unsigned stacksize, char *name){

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


void set_id(int id){

	curtask->id = id;
	return;
}


int get_id(void){

	return curtask->id;
}


char *get_task_name (void){

	return curtask->name ? curtask->name : (char*)"-unknown-";
}


count_t get_time (void){

	return etime.count;
}
