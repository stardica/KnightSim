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

list *ctxlist = NULL;

context initctx;
context *curctx = &initctx;
context *ctxhint = NULL;
eventcount etime;
eventcount *ectail = NULL;
eventcount *last_ec = NULL; /* to work with context library */
long long ecid = 0;
count_t last_value = 0;
context *current_context = NULL;
context *terminated_context = NULL;




void desim_init(void){

	//set up etime
	etime.name = strdup("etime");
	etime.id = 0;
	etime.count = 0;
	etime.nextcontext = NULL;
	etime.nextec = NULL;
	etime.ctxlist = desim_list_create(4);

	//set up initial task
	initctx.nextcontext = NULL;
	initctx.name = strdup("init ctx");
	initctx.count = 0;
	initctx.start = NULL;
	initctx.id = -1;
	initctx.magic = STK_OVFL_MAGIC;
	//initctx.ctxlist = desim_list_create(4);

	//other globals
	ctxlist = desim_list_create(4);

	return;
}

eventcount *eventcount_create(char *name){

	eventcount *ec = NULL;

	ec = (eventcount *)malloc(sizeof(eventcount));
	assert(ec);

	eventcount_init(ec, 0, name);

	return ec;
}

void context_create(void (*func)(void), unsigned stacksize, char *name){

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
	//new_context_ptr->ctxlist = desim_list_create(4);

	/*list work. forms a singly linked list, with newest task at the head.
	etime.contextlist points to the head of the list*/
	new_context_ptr->nextcontext = etime.nextcontext; 	/*delete me*/
	etime.nextcontext = new_context_ptr;				/*delete me*/

	context_init(new_context_ptr);

	//put the new ctx in the global ctx list
	desim_list_push(ctxlist, new_context_ptr);

	return;
}

void eventcount_init(eventcount * ec, count_t count, char *ecname){

	ec->name = ecname;
	ec->id = ecid++;
	//ec->tasklist = NULL;
	ec->nextcontext = NULL;
	ec->count = count;
	ec->nextec = NULL;
	ec->ctxlist = desim_list_create(4);

	//create singly linked list
    if (ectail == NULL)
    {
    	//always points to first event count created.
    	etime.nextec = ec;
    }
    else
    {
    	//old tail!!
    	ectail->nextec = ec;
    }

    //point to new tail!
    ectail = ec;

    return;
}

/* advance(ec) -- increment an event count. */
/* Don't use on time event count! */
void advance(eventcount *ec){

	context *context_ptr = NULL;
	context *context_pptr = NULL;

	/* advance the ec's count */
	ec->count++;

	context_ptr = ec->nextcontext;

	/* check for no tasks being enabled */
	printf("ec name %s value %llu\n", ec->name, ec->count);

	if(!context_ptr)
		printf("no ctx yet\n");
	else
		printf("ec %s val %llu ctx %s val %llu\n", ec->name, ec->count, context_ptr->name, context_ptr->count);

	/*compare waiting ctx to current ctx
	and if the waiting ctx is ready continue
	if the ctx's count is greater than the ec's count
	the ctx must continue to wait*/
	if ((context_ptr == NULL) || (context_ptr->count > ec->count))
	{
		//no context waiting on this eventcount
		printf("advance returning ctx null\n");
		printf("----\n");
		return;
	}

	/*if here, there is a ctx(s) waiting on this ec AND its ready to run*/

	/*find the end of the ec's task list
	and set all ctx time to current time (etime.cout)*/
	do
	{
		/* set tasks time equal to current time */
		context_ptr->count = etime.count;
		printf("traversing list ctx ptr name %s val %llu\n", context_ptr->name, context_ptr->count);
		context_pptr = context_ptr;
		context_ptr = context_ptr->nextcontext;

		if(!context_ptr)
			printf("ctx ptr is null!\n");

		/*when ctx_ptr is null you have reached the end of the list.
		when ctx_ptr > ec count the ctx must continue to wait*/

	}while(context_ptr && (context_ptr->count == ec->count));


	/*take the last element of the ec and point it to etime next.
	i.e. join it to the ctx list*/
	context_pptr->nextcontext = etime.nextcontext;

	if(context_pptr->nextcontext)
		printf("context_pptr->nextcontext pts to %s\n", context_pptr->nextcontext->name);
	else
		printf("context_pptr->nextcontext pts to null");


	/*join the list*/
	etime.nextcontext = ec->nextcontext;

	printf("etime next %s\n", etime.nextcontext->name);

	/*if their remains some ctx that are waiting on ec link them back
	if not this will now point to null*/
	ec->nextcontext = context_ptr;
	if(!ec->nextcontext)
		printf("yes ec->contextlist = null\n");


	printf("----\n");
	return;
}



void eventcount_destroy(eventcount *ec){

	eventcount	*front;
	eventcount	*back;
	unsigned	found = 0;

	assert(ec != NULL);

	// --- This function should unlink the eventcount from the etime eclist ---
	assert(etime.nextec != NULL);

	front = etime.nextec;
	if (ec == front)
	{
		etime.nextec = ec->nextec;
		free ((void*)ec);
		if (ectail == ec)
		{
			ectail = NULL;
		}
	}
	else
	{
	   back = etime.nextec;
	   front = front->nextec;
	   while ((front != NULL) && (!found))
	   {
		   if (ec == front)
		   {
			   back->nextec = ec->nextec;
			   free ((void*)ec);
			   found = 1;
		   }
		   else
		   {
			   back = back->nextec;
			   front = front->nextec;
		   }
	   }

	   assert(found == 1);

	   if (ectail == ec)
	   {
		   ectail = back;
	   }
	}
}


void pause(count_t count){

	printf("pause\n");

	/* if no tasks or still first task, keep running */
	if ((etime.nextcontext == NULL) || ((etime.count + count) <= etime.nextcontext->count))
	{
		printf("no tasks or count is low\n");
		etime.count += count;
	}
	else /* switch to next task */
	{
		printf("awaiting etime.count + count\n");
		printf("---\n");
		await(&etime, etime.count + count);
	}

	return;
}

void context_find_next_2(eventcount *ec, count_t value){

	printf("context_find_next_2\n");

	/*we are here because of an await
	stop the currect context and drop it into
	the ec that this context is awaiting*/

	/*determine if the ec has another context
	in its awaiting ctx list*/
	context *context_ptr = NULL;
	context_ptr = desim_list_get(ec->ctxlist, 0);

	if((context_ptr == NULL) || (value < context_ptr->count))
	{
		/*the current ec has no ctx link to it
		or an awaiting ctx is older and can run
		after the current ctx (i.e. doesn't matter
		when they execute)*/

		printf("ec has no awaiting ctx or an awaiting ctx is older than this ctx\n");
		assert(curctx);

		/*get the current ctx*/
		context_ptr = desim_list_dequeue(ctxlist);
		assert(context_ptr);

		/*put at head of ec's ctx list*/

		desim_list_insert(ec->ctxlist, 0, context_ptr);

		/*note, the list is self ordering on a cycle by
		cycle basis*/
		/*context_ptr = desim_list_get(ec->ctxlist, 0);
		printf("global ctx list size %d\n", desim_list_count(ctxlist));
		printf("ec list size %d name %s\n", desim_list_count(ec->ctxlist), context_ptr->name);
		exit(0);*/

	}
	else
	{
		/*if here there is a task in the list that should run after
		the current ctx resumse at a later point. This occurs mostly
		when useing pause from a ctx that controls the actual simulated
		cycle countmain ctx*/


	}



	return;
}


/* save current task on ec, return next task to run */
void context_find_next(eventcount *ec, count_t value){

	printf("context_find_next\n");
	context *context_pptr = NULL;

	context_pptr = ec->nextcontext;

	/* save current task on ec's tasklist */
	if ((context_pptr == NULL) || (value < context_pptr->count))
	{

		assert(curctx);

		printf("find next ctx null or value low\n");


		/*the current ec has no ctx link to it
		or the linked task still needs to run*/

		/*also adds to top of list*/
		ec->nextcontext = curctx;

		curctx->nextcontext = context_pptr;

	}
	else
	{

		printf("insert into ec's ctx list next name %s\n", context_pptr->name);

		context *context_ptr = NULL;

		bool time_list = (ec == &etime); //determines if this is etime or not.

		printf("ec is ETIME (time list %d)\n", time_list);

		//time list is 1 if this ec is etime
		if (time_list && ctxhint && (value >= ctxhint->count))
		{
			printf("in hint && value\n");

			context_ptr = ctxhint->nextcontext;
			context_pptr = ctxhint;
		}
		else
		{
			context_ptr = context_pptr->nextcontext;

			if(context_ptr)
				printf("ptr (%s) = pptr (%s) pptr next (%s)\n", context_ptr->name, context_pptr->name, context_pptr->nextcontext->name);
			else
				printf("ctx ptr null\n");
		}

		int i = 1;
		//look down the list for an older tasks (i.e. looking for value < ctx count)
		while (context_ptr && (value >= context_ptr->count))
		{
			context_pptr = context_ptr;
			context_ptr = context_ptr->nextcontext;

			if(!context_ptr)
				printf("context_ptr next = null\n");

			printf("entered while %d\n", i);
			i++;
		}


		//Manipulating the order of the list!!
		if (curctx)
		{
			printf("yes curtask %s\n", curctx->name);

			context_pptr->nextcontext = curctx;
			curctx->nextcontext = context_ptr;

			//printf("next next name %s\n", etime.nextcontext->nextcontext->nextcontext->name);

			if(!curctx->nextcontext)
				printf("curtask name %s no next element\n", curctx->name);
		}

		//working with etime, thus hint is the curtask
		if (time_list)
		{
			ctxhint = curctx;
		}
	}


	/*sets when this task should execute next (cycles).
	this is in conjuntion with the ec it is waiting on.*/
	curctx->count = value;

	printf("curtask name %s count %llu\n", curctx->name, curctx->count);

	/*get next task to run*/

	//change curtask to the next task
	curctx = etime.nextcontext;

	if(context_pptr)
		printf("curtask name %s count %llu ctx pptr name %s count %llu nextname %s count %llu\n",
				curctx->name, curctx->count, context_pptr->name, context_pptr->count, context_pptr->nextcontext->name, context_pptr->nextcontext->count);
	else
		printf("curtask name %s count %llu ctx pptr null\n", curctx->name, curctx->count);

	if (ctxhint == curctx)
	{
		printf("hint == curtask\n");
		ctxhint = NULL;
	}

	//if there is no new task exit
	if (curctx == NULL)
	{
		context_cleanup();

		context_end();
	}

	//the new task should be something that has yet to run.
	assert(curctx->count >= etime.count);

	//moves etime to the next ctx in list
  	etime.nextcontext = curctx->nextcontext;

  	if(etime.nextcontext)
  		printf("etime next name %s count %llu etime count %llu\n", etime.nextcontext->name, etime.nextcontext->count, etime.count);
  	else
  		printf("yes etime.next null\n");

  	etime.count = curctx->count; //set etime.count to next ctx's count (why?)

  	printf("---\n");
  	return;
}

context *context_select(void){

	printf("context_select\n");

	if (last_ec)
	{
		printf("here1\n");
		printf("---\n");
		if(newcode)
		{
			context_find_next_2(last_ec, last_value);
		}
		else
		{
			context_find_next(last_ec, last_value);
		}

		last_ec = NULL;
	}
	else
	{
		if(newcode)
		{
			printf("here3\n");

			//set current task here
			curctx = desim_list_get(ctxlist, 0);

			if(curctx)
				printf("curtask name %s\n", curctx->name);

			if (ctxhint == curctx)
			{
				ctxhint = NULL;
			}

			if (curctx == NULL)
			{
				context_cleanup();
				context_end();
			}

			//move down one context
			etime.nextcontext = curctx->nextcontext; /*delete me*/
			printf("before et count %llu curtask count %llu\n", etime.count, curctx->count);

			etime.count = curctx->count;
			printf("after et count %llu curtask count %llu\n", etime.count, curctx->count);

		}
		else
		{
			/*curtask points to the next ctx in the ctx list*/
			printf("here2\n");

			//set current task here
			curctx = etime.nextcontext;

			if(curctx)
				printf("curtask name %s\n", curctx->name);

			if (ctxhint == curctx)
			{
				ctxhint = NULL;
			}

			if (curctx == NULL)
			{
				context_cleanup();
				context_end();
			}

			//move down one context
			etime.nextcontext = curctx->nextcontext;
			printf("before et count %llu curtask count %llu\n", etime.count, curctx->count);

			etime.count = curctx->count;
			printf("after et count %llu curtask count %llu\n", etime.count, curctx->count);
		}
	}

	printf("---\n");
	return curctx;
}

void context_next(eventcount *ec, count_t value){

	printf("context next\n");

	last_ec = ec; //these are global because context select doesn't take a variable in
	last_value = value;

	printf("---\n");
	context_switch(context_select());

	return;
}


/* await(ec, value) -- suspend until ec.c >= value. */
void await (eventcount *ec, count_t value){

	/*todo*/
	/*check for stack overflows*/

	/*this current context should not await this ec
	if the ec's count is greator than the provided value*/

	/*for normal ec's the counts are always incremented in their main ctx
	for etime, the count must be given as current time plus the desired delay*/
	if (ec->count >= value)
	{
		printf("await, count >= value\n");

		printf("---\n");
		return;
	}

	/*the current context must now wait on this ec to be incremented*/

	printf("await finding next ctx cur ec %s curtask %s value %llu\n", ec->name, curctx->name, value);

	printf("---\n");
	context_next(ec, value);

	printf("---\n");
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







// delete last inserted task
void context_remove_last(context *last_context){

	assert (etime.nextcontext == last_context);
	etime.nextcontext = last_context->nextcontext;
	last_context->nextcontext = NULL;

	return;
}



void context_destroy(context *ctx){

	/* free stack space */
	free(ctx->stack);
}




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


  printf("----\n");
  (*current_context->start)();

  /*if current current ctx returns i.e. hits the bottom of its function
  it will return here. Then we need to terminate the process*/

  printf("context stub exiting\n");

  terminated_context = current_context;

  context_switch(context_select());

  return;
}



void context_exit (void){

  terminated_context = current_context;
  context_switch (context_select());
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

	printf("context_switch");

	//setjmp returns 1 if jumping to this position via longjmp
	if (!current_context || !setjmp64_2(current_context->buf))
	{
		/*ok, this is deep wizardry....
		note that the jump is to the next context and the
		setjmp is for the current context*/

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


//DESim Util Stuff

list *desim_list_create(unsigned int size)
{
	assert(size > 0);

	list *new_list;

	/* Create vector of elements */
	new_list = calloc(1, sizeof(list));
	new_list->size = size < 4 ? 4 : size;
	new_list->elem = calloc(new_list->size, sizeof(void *));

	/* Return list */
	return new_list;
}


void desim_list_free(list *list_ptr)
{
	free(list_ptr->elem);
	free(list_ptr);
	return;
}


int desim_list_count(list *list_ptr)
{
	return list_ptr->count;
}

void desim_list_grow(list *list_ptr)
{
	void **nelem;
	int nsize, i, index;

	/* Create new array */
	nsize = list_ptr->size * 2;
	nelem = calloc(nsize, sizeof(void *));

	/* Copy contents to new array */
	for (i = list_ptr->head, index = 0;
		index < list_ptr->count;
		i = (i + 1) % list_ptr->size, index++)
		nelem[index] = list_ptr->elem[i];

	/* Update fields */
	free(list_ptr->elem);
	list_ptr->elem = nelem;
	list_ptr->size = nsize;
	list_ptr->head = 0;
	list_ptr->tail = list_ptr->count;
}

void desim_list_add(list *list_ptr, void *elem)
{
	/* Grow list if necessary */
	if (list_ptr->count == list_ptr->size)
		desim_list_grow(list_ptr);

	/* Add element */
	list_ptr->elem[list_ptr->tail] = elem;
	list_ptr->tail = (list_ptr->tail + 1) % list_ptr->size;
	list_ptr->count++;

}


int desim_list_index_of(list *list_ptr, void *elem)
{
	int pos;
	int i;

	/* Search element */
	for (i = 0, pos = list_ptr->head; i < list_ptr->count; i++, pos = (pos + 1) % list_ptr->size)
	{
		if (list_ptr->elem[pos] == elem)
			return i;
	}

	//not found
	return -1;
}


void *desim_list_remove_at(list *list_ptr, int index)
{
	int shiftcount;
	int pos;
	int i;
	void *elem;

	/* check bounds */
	if (index < 0 || index >= list_ptr->count)
		return NULL;

	/* Get element before deleting it */
	elem = list_ptr->elem[(list_ptr->head + index) % list_ptr->size];

	/* Delete */
	if (index > list_ptr->count / 2)
	{
		shiftcount = list_ptr->count - index - 1;
		for (i = 0, pos = (list_ptr->head + index) % list_ptr->size; i < shiftcount; i++, pos = (pos + 1) % list_ptr->size)
			list_ptr->elem[pos] = list_ptr->elem[(pos + 1) % list_ptr->size];
		list_ptr->elem[pos] = NULL;
		list_ptr->tail = INLIST(list_ptr->tail - 1);
	}
	else
	{
		for (i = 0, pos = (list_ptr->head + index) % list_ptr->size; i < index; i++, pos = INLIST(pos - 1))
			list_ptr->elem[pos] = list_ptr->elem[INLIST(pos - 1)];
		list_ptr->elem[list_ptr->head] = NULL;
		list_ptr->head = (list_ptr->head + 1) % list_ptr->size;
	}

	list_ptr->count--;
	return elem;
}


void *desim_list_remove(list *list_ptr, void *elem)
{
	int index;

	/* Get index of element */
	index = desim_list_index_of(list_ptr, elem);

	/* Delete element at found position */
	return desim_list_remove_at(list_ptr, index);
}


void list_clear(struct list_t *list)
{
	list->count = 0;
	list->head = 0;
	list->tail = 0;
	return;
}

void desim_list_push(list *list_ptr, void *elem)
{
	desim_list_add(list_ptr, elem);
}


void *desim_list_pop(list *list_ptr)
{
	if (!list_ptr->count)
		return NULL;

	return desim_list_remove_at(list_ptr, list_ptr->count - 1);
}

void desim_list_enqueue(list *list_ptr, void *elem)
{
	desim_list_add(list_ptr, elem);
}


void *desim_list_dequeue(list *list_ptr)
{
	if (!list_ptr->count)
		return NULL;

	return desim_list_remove_at(list_ptr, 0);
}


/*void *list_top(struct list_t *list)
{
	if (!list->count)
	{
		list->error_code = LIST_ERR_EMPTY;
		return NULL;
	}
	return list_get(list, list->count - 1);
}*/


/*void *list_bottom(struct list_t *list)
{
	if (!list->count)
	{
		list->error_code = LIST_ERR_EMPTY;
		return NULL;
	}
	return list_get(list, 0);
}*/

/*void *list_head(struct list_t *list)
{
	if (!list->count)
	{
		list->error_code = LIST_ERR_EMPTY;
		return NULL;
	}
	return list_get(list, 0);
}*/


/*void *list_tail(struct list_t *list)
{
	if (!list->count)
	{
		list->error_code = LIST_ERR_EMPTY;
		return NULL;
	}
	return list_get(list, list->count - 1);
}*/

void *desim_list_get(list *list_ptr, int index)
{
	/*Check bounds*/
	if (index < 0 || index >= list_ptr->count)
		return NULL;

	/*Return element*/
	index = (index + list_ptr->head) % list_ptr->size;
	return list_ptr->elem[index];
}


/*void list_set(struct list_t *list, int index, void *elem)
{
	 check bounds
	if (index < 0 || index >= list->count)
	{
		list->error_code = LIST_ERR_BOUNDS;
		return;
	}

	 Return element
	index = (index + list->head) % list->size;
	list->elem[index] = elem;
	list->error_code = LIST_ERR_OK;
}*/


void desim_list_insert(list *list_ptr, int index, void *elem){

	int shiftcount;
	int pos;
	int i;

	/*Check bounds*/
	assert(index >= 0 && index <= list_ptr->count);

	/*Grow list if necessary*/
	if(list_ptr->count == list_ptr->size)
		desim_list_grow(list_ptr);

	 /*Choose whether to shift elements on the right increasing 'tail', or
	 * shift elements on the left decreasing 'head'.*/
	if (index > list_ptr->count / 2)
	{
		shiftcount = list_ptr->count - index;
		for (i = 0, pos = list_ptr->tail;
			 i < shiftcount;
			 i++, pos = INLIST(pos - 1))
			list_ptr->elem[pos] = list_ptr->elem[INLIST(pos - 1)];
		list_ptr->tail = (list_ptr->tail + 1) % list_ptr->size;
	}
	else
	{
		for (i = 0, pos = list_ptr->head;
			 i < index;
			 i++, pos = (pos + 1) % list_ptr->size)
			list_ptr->elem[INLIST(pos - 1)] = list_ptr->elem[pos];
		list_ptr->head = INLIST(list_ptr->head - 1);
	}

	list_ptr->elem[(list_ptr->head + index) % list_ptr->size] = elem;
	list_ptr->count++;

	return;
}
