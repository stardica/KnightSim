#include "desim.h"

#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>


list *ctxlist = NULL;
list *eclist = NULL;

context initctx;
context *curctx = &initctx;
context *ctxhint = NULL;
eventcount etime;
eventcount *ectail = NULL;
eventcount *last_ec = NULL; /* to work with context library */
long long ecid = 0;
count last_value = 0;
context *current_context = NULL;
context *terminated_context = NULL;


void desim_init(void){

	//set up etime
	etime.name = strdup("etime");
	etime.id = 0;
	etime.count = 0;
	etime.ctxlist = desim_list_create(4);

	//set up initial task
	initctx.name = strdup("init ctx");
	initctx.count = 0;
	initctx.start = NULL;
	initctx.id = -1;
	initctx.magic = STK_OVFL_MAGIC;
	//initctx.ctxlist = desim_list_create(4);

	//other globals
	ctxlist = desim_list_create(4);
	eclist = desim_list_create(4);

	return;
}

eventcount *eventcount_create(char *name){

	eventcount *ec_ptr = NULL;

	ec_ptr = (eventcount *)malloc(sizeof(eventcount));
	assert(ec_ptr);

	eventcount_init(ec_ptr, 0, name);

	//put the new ctx in the global ctx list
	desim_list_insert(eclist, 0, ec_ptr);

	return ec_ptr;
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

	context_init(new_context_ptr);

	//put the new ctx in the global ctx list
	desim_list_insert(ctxlist, 0, new_context_ptr);

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

	/* advance the ec's count */
	ec->count++;

	/*check ec's ctx list*/
	context *context_ptr = NULL;
	context_ptr = desim_list_get(ec->ctxlist, 0);

	/* check for no tasks being enabled */
	//printf("ec name %s value %llu\n", ec->name, ec->count);

	/*if(!context_ptr)
		printf("no ctx yet\n");
	else
		printf("ec %s val %llu ctx %s val %llu\n", ec->name, ec->count, context_ptr->name, context_ptr->count);*/

	/*compare waiting ctx to current ctx
	and if the waiting ctx is ready continue
	if the ctx's count is greater than the ec's count
	the ctx must continue to wait*/
	if ((context_ptr == NULL) || (context_ptr->count > ec->count))
	{
		/*no ctx waiting on this event count or
		the currently awaiting ctx is older*/

		/*printf("advance returning ctx null\n");
		printf("----\n");*/
		return;
	}

	/*if here, there is a ctx(s) waiting on this ec AND its ready to run ready to run*/

	/*find the end of the ec's task list
	and set all ctx time to current time (etime.cout)*/

	/*int i = 0;
	printf("inserting into ctxlist list size %d\n", desim_list_count(ec->ctxlist));*/
	do
	{
		context_ptr = desim_list_dequeue(ec->ctxlist);
		if(context_ptr)
		{
			/*printf("what did i get? %s\n", context_ptr->name);*/
			context_ptr->count = etime.count;
			desim_list_enqueue(ctxlist, context_ptr);
		}

	}while(context_ptr && (context_ptr->count == ec->count));

	/*LIST_FOR_EACH(ctxlist, i)
	{
		context_ptr = desim_list_get(ctxlist, i);
		if(context_ptr)
			printf("ctxlist item name %s\n", context_ptr->name);
	}*/

	/*printf("----\n");*/
	return;
}


void context_destroy(context *ctx_ptr){

	printf("destroying %s\n", ctx_ptr->name);
	free(ctx_ptr->name);
	free(ctx_ptr->stack);
	ctx_ptr = NULL;

	return;
}


void eventcount_destroy(eventcount *ec_ptr){

	printf("destroying ec %s\n", ec_ptr->name);
	free(ec_ptr->name);
	desim_list_clear(ec_ptr->ctxlist);
	desim_list_free(ec_ptr->ctxlist);
	free(ec_ptr);
	ec_ptr = NULL;

	return;
}


void pause(count value){

	await(&etime, etime.count + value);
	return;
}

void context_find_next(eventcount *ec, count value){

	/*printf("context_find_next_2\n");*/
	/*int i = 0;*/

	/*we are here because of an await
	stop the currect context and drop it into
	the ec that this context is awaiting*/

	/*determine if the ec has another context
	in its awaiting ctx list*/
	bool isetime = (ec == &etime);
	context *context_ptr = NULL;
	context_ptr = desim_list_get(ec->ctxlist, 0);

	if((context_ptr == NULL) || (value < context_ptr->count))
	{
		/*the current ec has no ctx link to it
		or an awaiting ctx is older and can run
		after the current ctx (i.e. doesn't matter
		when they execute)*/

		/*printf("ec (etime %d) has no awaiting ctx or an awaiting ctx is older than this ctx\n", isetime);*/
		assert(curctx);

		/*get the current ctx*/
		context_ptr = desim_list_dequeue(ctxlist);
		assert(context_ptr == curctx);

		/*put at head of ec's ctx list*/
		desim_list_insert(ec->ctxlist, 0, curctx);

		/*note, the list is self ordering on a cycle by
		cycle basis*/
		/*context_ptr = desim_list_get(ec->ctxlist, 0);
		printf("global ctx list size %d\n", desim_list_count(ctxlist));
		printf("ec list size %d name %s\n", desim_list_count(ec->ctxlist), context_ptr->name);
		exit(0);*/
		/*LIST_FOR_EACH(ctxlist, i)
		{
			context_ptr = desim_list_get(ctxlist, i);
			if(context_ptr)
				printf("ctxlist item name %s\n", context_ptr->name);
		}*/


	}
	else
	{

		fatal("ec/etime has more than one ctx inserted into ctx list\n");

		/*if here there is a ctx in the ec's ctx list that should run after
		the current ctx resumes. This occurs mostly when using pause
		from in a ctx that controls the actual simulated cycle count (etime)
		this can also occur is two seperate ctxs are awaiting the same ec
		and one is set to wait longer than the other*/
		assert(context_ptr);

		printf("insert into ec's ctx list name %s\n", context_ptr->name);

		 //determines if this is etime or not.

		/*--------------------------*/
		printf("isetime? %d\n", isetime);
	}

	/*sets when this task should execute next (cycles).
	this is in conjunction with the ec it is waiting on.*/
	curctx->count = value;
	/*printf("curtask name %s count %llu\n", curctx->name, curctx->count);*/


	/*get next task to run*/
	curctx = desim_list_get(ctxlist, 0);
	//if no curctx all tasks are awaiting, this is a simulation implementation problem

	if(!curctx)
	{
		context_ptr = desim_list_dequeue(etime.ctxlist);
		assert(context_ptr);

		/*printf("ctxlist empty! inserting from etime name %s count %llu\n", context_ptr->name, context_ptr->count);*/

		curctx = context_ptr;
		/*put at head of ec's ctx list*/
		desim_list_insert(ctxlist, 0, context_ptr);

		/*LIST_FOR_EACH(ctxlist, i)
		{
			context_ptr = desim_list_get(ctxlist, i);
			if(context_ptr)
				printf("ctxlist item name %s\n", context_ptr->name);
		}*/
	}

	/*printf("NEW curctx name %s count %llu\n", curctx->name, curctx->count);*/

	if (ctxhint == curctx)
	{
		/*printf("hint == curtask\n");*/
		ctxhint = NULL;
	}

	//if there is no new task exit
	if (curctx == NULL)
	{
		context_end();
	}

	//the new task should be something that has yet to run.
	//assert(curctx->count >= etime.count);

	//moves etime to the next ctx in list
  	etime.count = curctx->count; //set etime.count to next ctx's count (why?)

  	/*printf("---\n");*/
	return;
}


context *context_select(void){

	/*printf("context_select\n");*/

	if (last_ec)
	{
		context_find_next(last_ec, last_value);
		last_ec = NULL;
	}
	else
	{
		/*printf("here2\n");*/

		//set current task here
		curctx = desim_list_get(ctxlist, 0);
		assert(curctx);

		/*if(curctx)
			printf("curtask name %s\n", curctx->name);*/

		if (ctxhint == curctx)
		{
			ctxhint = NULL;
		}

		etime.count = curctx->count;
	}

	/*printf("---\n");*/
	return curctx;
}

void context_next(eventcount *ec, count value){

	/*printf("context next\n")*/;

	last_ec = ec; //these are global because context select doesn't take a variable in
	last_value = value;

	/*printf("---\n");*/
	context_switch(context_select());

	return;
}

/* await(ec, value) -- suspend until ec.c >= value. */
void await (eventcount *ec, count value){

	/*todo*/
	/*check for stack overflows*/

	/*this current context should not await this ec
	if the ec's count is greator than the provided value*/

	/*for normal ec's the counts are always incremented in their main ctx
	for etime, the count must be given as current time plus the desired delay*/
	if (ec->count >= value)
	{
		/*printf("await, count >= value\n");

		printf("---\n");*/
		return;
	}

	/*the current context must now wait on this ec to be incremented*/

	/*printf("await finding next ctx cur ec %s curtask %s value %llu\n", ec->name, curctx->name, value);

	printf("---\n");*/
	context_next(ec, value);

	/*printf("---\n");*/
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
	}

	//clean up
	desim_end();

	return;
}


void desim_end(void){

	int i = 0;
	eventcount *ec_ptr = NULL;

	LIST_FOR_EACH(eclist, i)
	{
		ec_ptr = desim_list_get(eclist, i);
		eventcount_destroy(ec_ptr);
	}

	desim_list_clear(ctxlist);
	desim_list_free(ctxlist);
	ctxlist = NULL;

	return;
}


void context_stub(void){

  /*printf("context stub\n");*/

  if(terminated_context)
  {
    context_destroy (terminated_context);
    terminated_context = NULL;
  }


  /*printf("----\n");*/
  (*current_context->start)();

  /*if current current ctx returns i.e. hits the bottom of its function
  it will return here. Then we need to terminate the process*/

  /*printf("context stub exiting\n");*/

  /*clear the ctx list*/
  context_end();

  return;
}



#if defined(__linux__) && defined(__i386__)

void context_switch (context *ctx_ptr)
{

	if (!current_context || !setjmp32_2(current_context->buf))
	{
	  current_context = ctx_ptr;
	  longjmp32_2(ctx_ptr->buf, 1);
	}

	return;
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

/*int i = 0;*/

void context_switch (context *ctx_ptr){

	/*printf("context_switch");*/


	//setjmp returns 1 if jumping to this position via longjmp
	if (!current_context || !setjmp64_2(current_context->buf))
	{
		/*ok, this is deep wizardry....
		note that the jump is to the next context and the
		setjmp is for the current context*/

		/*i++;*/

		current_context = ctx_ptr;
		longjmp64_2(ctx_ptr->buf, 7);
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
