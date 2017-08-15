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

context inittask;
context *curtask = &inittask;
context *hint = NULL;
eventcount etime;
eventcount *ectail = NULL;
eventcount *last_ec = NULL; /* to work with context library */
long long ecid = 0;
count_t last_value = 0;
context *current_context = NULL;
context *terminated_context = NULL;


void desim_init(void){

	//init etime
	etime_init();

	initial_task_init();

	return;
}

void etime_init(void){

	etime.name = NULL;
	etime.id = 0;
	etime.count = 0;
	//etime.tasklist = NULL;
	etime.contextlist = NULL;
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
	//ec->tasklist = NULL;
	ec->contextlist = NULL;
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

	//printf("desim advance ec name %s\n", ec->name);

	context *context_ptr = NULL;
	context *context_pptr = NULL;

	/* advance count */
	ec->count++;

	/* check for no tasks being enabled */
	context_ptr = ec->contextlist;

	if ((context_ptr == NULL) || (context_ptr->count > ec->count))
		return;

	/* advance down task list */
	do
	{
		/* set tasks time equal to current time */
		context_ptr->count = etime.count;
		context_pptr = context_ptr;
		context_ptr = context_ptr->contextlist;

	}while (context_ptr && (context_ptr->count == ec->count));

	/* add list of events to etime */
	context_pptr->contextlist = etime.contextlist;
	etime.contextlist = ec->contextlist;
	ec->contextlist = context_ptr;

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


void initial_task_init(void){

	inittask.contextlist = NULL;
	inittask.name = strdup("initial task");
	inittask.count = 0;
	inittask.start = NULL;
	inittask.id = -1;
	inittask.magic = STK_OVFL_MAGIC;

	return;
}

void pause(count_t count){

	/* if no tasks or still first task, keep running */
	if ((etime.contextlist == NULL) || ((etime.count + count) <= etime.contextlist->count))
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
void context_find_next(eventcount *ec, count_t value){

	//this EC is not ETIME!
	//tasks are stored on each individual ec
	context *context_pptr = NULL;
	context_pptr = ec->contextlist;

	/* save current task on ec's tasklist */
	if ((context_pptr == NULL) || (value < context_pptr->count))
	{
		assert(curtask);

		/* insert at head of list */
		ec->contextlist = curtask;
		curtask->contextlist = context_pptr;  //same as ec->tasklist = pptr
	}
	else
	{

		/* insert in middle of list */
		context *context_ptr = NULL;

		bool time_list = (ec == &etime);

		if (time_list && hint && (value >= hint->count))
		{
			context_ptr = hint->contextlist;
			context_pptr = hint;
		}
		else
		{
			context_ptr = context_pptr->contextlist;
		}

		//go down the list until you find an older task
		while (context_ptr && (value >= context_ptr->count))
		{
			context_pptr = context_ptr;
			context_ptr = context_ptr->contextlist;
		}


		if (curtask)
		{
			context_pptr->contextlist = curtask;
			curtask->contextlist = context_ptr;
		}

		if (time_list)
		{
			hint = curtask;
		}
	}

	curtask->count = value;

	/*get next task to run*/
	curtask = etime.contextlist;

	if (hint == curtask)
	{
		hint = NULL;
	}

	if (curtask == NULL)
	{
		context_cleanup();

		context_end();
	}

	assert(curtask->count >= etime.count);

  	etime.contextlist = curtask->contextlist;
  	etime.count = curtask->count;

  	return;
}

context *context_select(void){

	if (last_ec)
	{
		context_find_next(last_ec, last_value);
		last_ec = NULL;
	}
	else
	{
		curtask = etime.contextlist;

		if (hint == curtask)
		{
			hint = NULL;
		}

		if (curtask == NULL)
		{
			context_cleanup();
			context_end();
		}

		etime.contextlist = curtask->contextlist;
		etime.count = curtask->count;
	}

	return curtask;
}

void context_next(eventcount *ec, count_t value){

	context *ctx = NULL;
	last_ec = ec;
	last_value = value;

	ctx = context_select();
	assert(ctx);

	context_switch(ctx);

	return;
}


/* await(ec, value) -- suspend until ec.c >= value. */
void await (eventcount *ec, count_t value){

	/*todo*/
	/*check for stack overflows*/

	if (ec->count >= value)
		return;

	context_next(ec, value);

	return;
}



void simulate(void){

	//simulate
	if(!context_simulate())
	{
		context *next_context = NULL;

		next_context = context_select();
		assert(next_context);

		context_switch(next_context);

		context_cleanup();

		context_end();
	}

	return;
}

context *context_create(void (*func)(void), unsigned stacksize, char *name){

	/*stacksize should be multiple of unsigned size */
	assert ((stacksize % sizeof(unsigned)) == 0);

	context *new_context_ptr = NULL;

	new_context_ptr = (context *) malloc(sizeof(context));
	assert(new_context_ptr);

	new_context_ptr->count = etime.count;
	new_context_ptr->name = name;
	new_context_ptr->stack = (char *)malloc(stacksize);
	assert(new_context_ptr->stack);
	new_context_ptr->stacksize = stacksize;
	new_context_ptr->magic = STK_OVFL_MAGIC; // for stack overflow check
	new_context_ptr->start = func; /*assigns the head of a function*/

	/*list work. forms a singly linked list, with newest task at the head.
	etime points to the head of the list*/
	//desim_list_insert(new_context_ptr->contextlist, etime.contextlist);
	new_context_ptr->contextlist = etime.contextlist;
	etime.contextlist = new_context_ptr;

	context_init(new_context_ptr);

	return new_context_ptr;
}


// delete last inserted task
void context_remove_last(context *last_context){

	assert (etime.contextlist == last_context);
	etime.contextlist = last_context->contextlist;
	last_context->contextlist = NULL;

	return;
}

void context_destroy(context *ctx){

	/* free stack space */
	free(ctx->stack);
}


/*context.c*******************************************/
/* current process and terminated process */



void context_cleanup(void){

  if (terminated_context)
  {
    context_destroy (terminated_context);
    terminated_context = NULL;
  }

  return;
}


void context_stub(void){

  printf("context stub\n");

  if(terminated_context)
  {
    context_destroy (terminated_context);
    terminated_context = NULL;
  }


  (*current_context->start)();

  /*if current current process returns i.e. hits the bottom of its function
  it will return here. Then we need to terminate the process*/

  printf("exiting\n");

  terminated_context = current_context;

  context_switch(context_select());

  return;
}

void context_exit (void){

  terminated_context = current_context;
  context_switch (context_select ());
  return;
}


#if defined(__linux__) && defined(__i386__)

void context_switch (context *ctx)
{

	if (!current_context || !setjmp32_2(current_context->buf))
	{
	  current_context = ctx;
	  longjmp32_2(ctx->buf, 1);
	}
	if (terminated_context)
	{
	  context_destroy (terminated_context);
	  terminated_context = NULL;
	}

}

void context_init (context *new_context){

	new_context->buf[5] = ((int)context_stub);
	new_context->buf[4] = ((int)((char*)new_context->stack + new_context->stacksize - 4));
}

void context_end(void){

	longjmp32_2(main_context, 1);
	return;
}

int context_simulate(void){

	return setjmp32_2(main_context);
}

#elif defined(__linux__) && defined(__x86_64)

void context_switch (context *ctx){

	//setjmp returns 1 if jumping to this position via longjmp
	if (!current_context || !setjmp64_2(current_context->buf))
	{
		/*ok, this is deep wizardry....
		note that the jump is to the next context and the
		setjmp is for the current context
		if the */

		current_context = ctx;
		longjmp64_2(ctx->buf, 1);
	}

	if (terminated_context)
	{
		context_destroy(terminated_context);
		terminated_context = NULL;
	}

	return;
}


void context_init(context *new_context){

	/*these are in this function because they are architecture dependent.
	don't move these out of this function!!!!*/

	new_context->buf[7] = (long long)context_stub; /*where the initial jump will go to*/
	new_context->buf[6] = (long long)((char *)new_context->stack + new_context->stacksize - 4); /*points to top of the virtual stack*/
	return;
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
