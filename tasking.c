#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "tasking.h"
#include "eventcount.h"
#include "contexts.h"

static char *end_of_concurrency = "--this-should-never-happen--";

static void (*cleanup_stuff) (void) = NULL;

task inittask;
task *curtask = &inittask;
task *hint = NULL;

void initial_task_init(void){

	inittask.tasklist = NULL;
	inittask.name = NULL;
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
static inline void find_next_task (eventcount *ec, count_t value){

	task *pptr = ec->tasklist;

	/* save current task on ec's tasklist */
	if ((pptr == NULL) || (value < pptr->count))
	{
		/* insert at head of list */
		if (curtask)
		{
			ec->tasklist = curtask;
			curtask->tasklist = pptr;
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
 
	/* get next task to run */
	curtask = etime.tasklist;
	if (hint == curtask)
	{
		hint = NULL;
	}

	if (curtask == NULL)
	{
		context_cleanup ();

		if (cleanup_stuff)
			(*cleanup_stuff) ();

		exit (0);
	}

	assert (curtask->count >= etime.count);
  	etime.tasklist = curtask->tasklist;
  	etime.count = curtask->count;

  	return;
}

process *context_select(void){

	if (last_ec)
	{
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
			//context_cleanup();

			printf("exiting after switch\n");

			assert(cleanup_stuff);

			if (cleanup_stuff)
				(*cleanup_stuff)();
		}

		etime.tasklist = curtask->tasklist;
		etime.count = curtask->count;
	}

	return (process*)&(curtask->c);
}

void switch_context (eventcount *ec, count_t value){

	process *p = NULL;
	last_ec = ec;
	last_value = value;

	p = context_select();

	if (p)
	{
		context_switch(p);
	}


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

	cleanup_stuff = end_simulate;
	inittask.name = end_of_concurrency;
	process * process;

	process = context_select();
	/*assert(!process == NULL);*/
	assert(process);

	context_switch (process);

	context_cleanup();

	if (cleanup_stuff)
	{
		(*cleanup_stuff) ();
		exit (0);
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

	char*     ptr;
	task*     tptr;
	unsigned  size = sizeof(task);

	/* stacksize should be multiple of unsigned size */
	assert ((stacksize % sizeof(unsigned)) == 0);

	ptr = (char *) malloc(size);
	assert(ptr != NULL);

	tptr = (task *) ptr;
	tptr->count = etime.count;
	tptr->name = name;

	assert(tptr->c.stack = (char *)malloc(stacksize));
	tptr->c.sz = stacksize;

	context_init ((process*)&tptr->c, func);

	/* link into task list */
	tptr->tasklist = etime.tasklist;
	etime.tasklist = tptr;

	// for stack overflow check
	tptr->magic = STK_OVFL_MAGIC;

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
