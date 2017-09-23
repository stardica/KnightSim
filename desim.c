#include "desim.h"

#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

list *ctxdestroylist = NULL;
list *ctxlist = NULL;
list *ecdestroylist = NULL;

context *last_context = NULL;
context *current_context = NULL;
context *ctxhint = NULL;
eventcount *etime = NULL;

long long ecid = 0;


context *terminated_context = NULL;


void desim_init(void){

	char buff[100];

	//other globals
	ctxdestroylist = desim_list_create(4);
	ctxlist = desim_list_create(4);
	ecdestroylist = desim_list_create(4);

	//set up etime
	memset(buff,'\0' , 100);
	snprintf(buff, 100, "etime");
	etime = eventcount_create(strdup(buff));


	return;
}

eventcount *eventcount_create(char *name){

	eventcount *ec_ptr = NULL;

	ec_ptr = (eventcount *)malloc(sizeof(eventcount));
	assert(ec_ptr);

	eventcount_init(ec_ptr, 0, name);

	//for destroying the ec later
	desim_list_insert(ecdestroylist, 0, ec_ptr);

	return ec_ptr;
}

void context_create(void (*func)(void), unsigned stacksize, char *name){

	/*stacksize should be multiple of unsigned size */
	assert ((stacksize % sizeof(unsigned)) == 0);

	context *new_context_ptr = NULL;

	new_context_ptr = (context *) malloc(sizeof(context));
	assert(new_context_ptr);

	new_context_ptr->count = etime->count;
	new_context_ptr->name = name;
	new_context_ptr->stack = (char *)malloc(stacksize);
	assert(new_context_ptr->stack);
	new_context_ptr->stacksize = stacksize;
	new_context_ptr->magic = STK_OVFL_MAGIC; // for stack overflow check
	new_context_ptr->start = func; /*assigns the head of a function*/

	context_init(new_context_ptr);

	//put the new ctx in the global ctx list
	desim_list_insert(ctxlist, 0, new_context_ptr);

	//for destroying the context later
	desim_list_insert(ctxdestroylist, 0, new_context_ptr);

	return;
}

void eventcount_init(eventcount * ec, count count, char *ecname){

	ec->name = ecname;
	ec->id = ecid++;
	ec->count = count;
	ec->ctxlist = desim_list_create(4);
    return;
}

void advance(eventcount *ec){

	int i = 0;

	/* advance the ec's count */
	ec->count++;

	/*check ec's ctx list*/
	context *context_ptr = NULL;

	/*if here, there is a ctx(s) waiting on this ec AND
	 * its ready to run ready to run
	 * find the end of the ec's task list
	 * and set all ctx time to current time (etime.cout)*/
	LIST_FOR_EACH_L(ec->ctxlist, i, 0)
	{
		context_ptr = desim_list_get(ec->ctxlist, i);


		if(context_ptr && (context_ptr->count == ec->count))
		{
			//printf("context_ptr->count %llu ec->count %llu name %s", context_ptr->count, ec->count, ec->name);
			//fflush(stdout);
			assert(context_ptr->count == ec->count);
			context_ptr->count = etime->count;
			context_ptr = desim_list_dequeue(ec->ctxlist);
			desim_list_enqueue(ctxlist, context_ptr);
		}
		else
		{
			//no context or context is not ready to run
			if(context_ptr)
				assert(context_ptr->count > ec->count);
			break;
		}
	}

	return;
}

void context_terminate(void){

	/*we are deliberately allowing a context to terminate it self
	destroy the context and switch to the next context*/

	context *ctx_ptr = NULL;

	/*remove from global ctx and destroy lists*/
	ctx_ptr = desim_list_dequeue(ctxlist);
	ctx_ptr = desim_list_remove(ctxdestroylist, ctx_ptr);
	assert(last_context == ctx_ptr);

	context_destroy(ctx_ptr);

	/*Its ok to leave the eventcount,
	 * it will be destroyed on exit*/

	/*switch to next context simulation should continue*/
	context_switch(context_select());

	return;
}


void context_destroy(context *ctx_ptr){

	free(ctx_ptr->name);
	free(ctx_ptr->stack);
	ctx_ptr->start = NULL;
	free(ctx_ptr);
	ctx_ptr = NULL;

	return;
}


void eventcount_destroy(eventcount *ec_ptr){

	free(ec_ptr->name);
	desim_list_clear(ec_ptr->ctxlist);
	desim_list_free(ec_ptr->ctxlist);
	free(ec_ptr);
	ec_ptr = NULL;

	return;
}


void pause(count value){

	await(etime, etime->count + value);

	return;
}

context *context_select(void){

	/*get next ctx to run*/
	current_context = desim_list_get(ctxlist, 0);

	if(!current_context)
	{
		/*if there isn't a ctx on the global context list
		we are ready to advance in cycles, pull from etime*/
		current_context = desim_list_dequeue(etime->ctxlist);

		/*if there isn't a ctx in etime's ctx list the simulation is
		deadlocked this is a user simulation implementation problem*/
		if(!current_context)
		{
			warning("DESim: deadlock detected now exiting... all contexts are in an await state.\n"
					"There is a problem with the simulation implementation.\n");
			context_end();
		}

		/*put at head of ec's ctx list*/
		desim_list_insert(ctxlist, 0, current_context);
	}

	//etime is now the current cycle count determined by the ctx's count
  	etime->count = current_context->count;

  	return current_context;
}



void await(eventcount *ec, count value){

	/*todo
	check for stack overflows*/
	assert(ec);

	/*continue if the ctx's count is less
	 * than or equal to the ec's count*/
	if (ec->count >= value)
		return;

	/*the current context must now wait on this ec to be incremented*/

	/*we are here because of an await
	stop the currect context and drop it into
	the ec that this context is awaiting

	determine if the ec has another context
	in its awaiting ctx list*/
	context *context_ptr = NULL;
	int i = 0;

	LIST_FOR_EACH_LG(ec->ctxlist, i, 0)
	{
		context_ptr = desim_list_get(ec->ctxlist, i);
		if(!context_ptr || (context_ptr && value < context_ptr->count))
		{
			/*we are at the head or tail of the list
			insert ctx at this position*/
			context_ptr = desim_list_dequeue(ctxlist);

			//set the curctx's value
			context_ptr->count = value;
			desim_list_insert(ec->ctxlist, i, context_ptr);
			break;
		}

	}

	context_switch(context_select());

	return;
}


void simulate(void){

	//simulate
	if(!context_simulate())
	{
		/*This is the beginning of the simulation
		 * get the first ctx and run it*/
		context_switch(context_select());
	}

	//clean up
	desim_end();

	return;
}

void desim_end(void){

	int i = 0;
	eventcount *ec_ptr = NULL;
	context *ctx_ptr = NULL;

	LIST_FOR_EACH_L(ecdestroylist, i, 0)
	{
		ec_ptr = desim_list_get(ecdestroylist, i);

		if(ec_ptr)
			eventcount_destroy(ec_ptr);
	}




	LIST_FOR_EACH_L(ctxdestroylist, i, 0)
	{
		ctx_ptr = desim_list_get(ctxdestroylist, i);

		if(ctx_ptr)
			context_destroy(ctx_ptr);
	}

	desim_list_clear(ctxdestroylist);
	desim_list_free(ctxdestroylist);

	desim_list_clear(ctxlist);
	desim_list_free(ctxlist);

	desim_list_clear(ecdestroylist);
	desim_list_free(ecdestroylist);

	return;
}


void context_start(void){

  if(terminated_context)
  {
    context_destroy (terminated_context);
    terminated_context = NULL;
  }

  (*current_context->start)();

  /*if current current ctx returns i.e. hits the bottom of its function
  it will return here. Then we need to terminate the simulation*/

  context_end();

  return;
}


void context_switch (context *ctx_ptr)
{
	//setjmp returns 1 if jumping to this position via longjmp

	/*ok, this is deep wizardry....
	note that the jump is to the next context and the
	setjmp is for the current context*/

#if defined(__linux__) && defined(__i386__)
	if (!last_context || !setjmp32_2(last_context->buf))
	{
	  last_context = ctx_ptr;
	  longjmp32_2(ctx_ptr->buf, 1);
	}
#elif defined(__linux__) && defined(__x86_64)
	if (!last_context || !setjmp64_2(last_context->buf))
	{
		last_context = ctx_ptr;
		longjmp64_2(ctx_ptr->buf, 1);
	}
#else
#error Unsupported machine/OS combination
#endif

	return;
}

int context_simulate(void){

#if defined(__linux__) && defined(__i386__)
	return setjmp32_2(main_context);
#elif defined(__linux__) && defined(__x86_64)
	return setjmp64_2(main_context);
#else
#error Unsupported machine/OS combination
#endif
}


void context_end(void){

#if defined(__linux__) && defined(__i386__)
	longjmp32_2(main_context, 1);
#elif defined(__linux__) && defined(__x86_64)
	longjmp64_2(main_context, 1);
#else
#error Unsupported machine/OS combination
#endif

	return;
}

void context_init(context *new_context){

	/*these are in this function because they are architecture dependent.
	don't move these out of this function!!!!*/

	/*instruction pointer and then pointer to top of stack*/

#if defined(__linux__) && defined(__i386__)
	new_context->buf[5] = ((int)context_start);
	new_context->buf[4] = ((int)((char*)new_context->stack + new_context->stacksize - 4));
#elif defined(__linux__) && defined(__x86_64)
	new_context->buf[7] = (long long)context_start;
	new_context->buf[6] = (long long)((char *)new_context->stack + new_context->stacksize - 4);
#else
#error Unsupported machine/OS combination
#endif

	return;
}


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
	free(list_ptr->name);
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


void desim_list_clear(list *list_ptr)
{
	list_ptr->count = 0;
	list_ptr->head = 0;
	list_ptr->tail = 0;
	return;
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


void *desim_list_get(list *list_ptr, int index)
{
	/*Check bounds*/
	if (index < 0 || index >= list_ptr->count)
		return NULL;

	/*Return element*/
	index = (index + list_ptr->head) % list_ptr->size;
	return list_ptr->elem[index];
}


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


void fatal(const char *fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	fprintf(stderr, "fatal: ");
	vfprintf(stderr, fmt, va);
	fprintf(stderr, "\n");
	fflush(NULL);
	exit(1);
}

void warning(const char *fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	fprintf(stderr, "warning: ");
	vfprintf(stderr, fmt, va);
	fprintf(stderr, "\n");
}
