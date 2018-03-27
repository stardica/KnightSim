#include "knightsim.h"

#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "../include/rdtsc.h"

/* Globals*/
list *ctxdestroylist = NULL;
list *ecdestroylist = NULL;
context *current_context = NULL;
eventcount *etime = NULL;
jmp_buf main_context;
jmp_buf halt_context;
long long ecid = 0;

#define HASHSIZE 16
#define HASHBY 0xF
context *ctx_hash_table[HASHSIZE];
unsigned int ctx_hash_table_count = 0;


void KnightSim_init(void){

	char buff[100];

	//should be power of two
	//hash table size should be a power of two
	if((HASHSIZE & (HASHSIZE - 1)) != 0)
		fatal("This is the optimized version of KnightSim.\n"
				"HASHSIZE = %d but must be a power of two.\n", HASHSIZE);

	//other globals
	ctxdestroylist = KnightSim_list_create(4);
	//ctxlist = KnightSim_list_create(4);
	ecdestroylist = KnightSim_list_create(4);

	//set up etime
	memset(buff,'\0' , 100);
	snprintf(buff, 100, "etime");
	etime = eventcount_create(strdup(buff));

	return;
}


void KnightSim_dump_queues(void){

	int i = 0;
	eventcount *ec_ptr = NULL;
	context *ctx_ptr = NULL;

	printf("KnightSim_dump_queues():\n");

	printf("eventcounts\n");
	LIST_FOR_EACH_L(ecdestroylist, i, 0)
	{
		ec_ptr = (eventcount*)KnightSim_list_get(ecdestroylist, i);

		printf("ec name %s count %llu\n", ec_ptr->name, ec_ptr->count);
		ctx_ptr = ec_ptr->ctx_list;

		if(ctx_ptr)
			printf("\tctx %s ec count %llu ctx count %llu\n",
					ctx_ptr->name, ec_ptr->count, ctx_ptr->count);
	}
	printf("\n");

	return;
}


eventcount *eventcount_create(char *name){

	eventcount *ec_ptr = NULL;

	ec_ptr = (eventcount *)malloc(sizeof(eventcount));
	assert(ec_ptr);

	eventcount_init(ec_ptr, 0, name);

	//for destroying the ec later
	KnightSim_list_insert(ecdestroylist, 0, ec_ptr);

	return ec_ptr;
}

void ctx_hash_insert(context *context_ptr, unsigned int where){

	if(!ctx_hash_table[where])
	{
		//nothing here
		ctx_hash_table[where] = context_ptr;
		ctx_hash_table[where]->batch_next = NULL;
		ctx_hash_table_count++;
	}
	else
	{
		//something here stick it at the head
		context_ptr->batch_next = ctx_hash_table[where]; //set new ctx as head
		ctx_hash_table[where] = context_ptr; //move old ctx down
	}

	return;
}


void context_create(void (*func)(context *), unsigned stacksize, char *name, int id){

	/*stacksize should be multiple of unsigned size */
	assert ((stacksize % sizeof(unsigned)) == 0);

	context *new_context_ptr = NULL;

	new_context_ptr = (context *) malloc(sizeof(context));
	assert(new_context_ptr);

	new_context_ptr->count = etime->count;
	new_context_ptr->name = name;
	new_context_ptr->id = id;
	new_context_ptr->stack = (char *)malloc(stacksize);
	assert(new_context_ptr->stack);
	/*printf("ptr 0x%08llx\n", (long long)new_context_ptr->stack);*/
	new_context_ptr->stacksize = stacksize;
	new_context_ptr->magic = STK_OVFL_MAGIC; // for stack overflow check
	new_context_ptr->start = func; /*assigns the head of a function*/

	//move data unto the context's stack
	context_data * context_data_ptr = (context_data *) malloc(sizeof(context_data));
	context_data_ptr->ctx_ptr = new_context_ptr;
	context_data_ptr->context_id = new_context_ptr->id;//new_context_ptr->id;

	//copy data to context's stack then free the local copy.
	memcpy((void *)new_context_ptr->stack, context_data_ptr, sizeof(context_data));
	free(context_data_ptr);

	context_init(new_context_ptr);

	//start up new context
#if defined(__linux__) && defined(__i386__)
	current_context = new_context_ptr;
	if (!setjmp32_2(halt_context))
	{
	  longjmp32_2(new_context_ptr->buf, 1);
	}
#elif defined(__linux__) && defined(__x86_64)

	current_context = new_context_ptr;
	if (!setjmp64_2(halt_context))
	{
		longjmp64_2(new_context_ptr->buf, 1);
	}
#else
#error Unsupported machine/OS combination
#endif

	//changes
	assert(etime); //make sure the init is above any context create funcs
	ctx_hash_insert(new_context_ptr, new_context_ptr->count & HASHBY);

	//for destroying the context later
	KnightSim_list_enqueue(ctxdestroylist, new_context_ptr);

	return;
}

void eventcount_init(eventcount * ec, count_t count, char *ecname){

	ec->name = ecname;
	ec->id = ecid++;
	ec->count = count;
	ec->ctx_list = NULL;
    return;
}

void advance(eventcount *ec, context *my_ctx){

	/* advance the ec's count */
	ec->count++;

	/*if here, there is a ctx(s) waiting on this ec AND
	 * its ready to run
	 * find the end of the ec's task list
	 * and set all ctx time to current time (etime.cout)*/

	if(ec->ctx_list && ec->ctx_list->count == ec->count)
	{
		//there is a context on this ec and it's ready
		ec->ctx_list->batch_next = my_ctx->batch_next;
		my_ctx->batch_next = ec->ctx_list;
		ec->ctx_list =  NULL;
	}

	return;
}



void context_terminate(context * my_ctx){

	/*we are deliberately allowing a context to terminate it self
	destroy the context and switch to the next context*/

	//set curr ctx to next ctx in list (NOTE MAYBE NULL!!)
	my_ctx = my_ctx->batch_next; //drops this context

	if(my_ctx)
	{
		long_jump(my_ctx->buf);
	}
	else
	{
		etime->count++;
		long_jump(context_select());
	}

	return;
}

void context_init_halt(context * my_ctx){

	//set up new context
#if defined(__linux__) && defined(__i386__)
	if (!setjmp32_2(my_ctx->buf))
	{
	  longjmp32_2(halt_context, 1);
	}
#elif defined(__linux__) && defined(__x86_64)
	if (!setjmp64_2(my_ctx->buf))
	{
		longjmp64_2(halt_context, 1);
	}
#else
#error Unsupported machine/OS combination
#endif
}


void context_destroy(context *ctx_ptr){

	assert(ctx_ptr != NULL);
	ctx_ptr->start = NULL;
	free(ctx_ptr->stack);
	free(ctx_ptr->name);
	free(ctx_ptr);
	ctx_ptr = NULL;

	return;
}


void eventcount_destroy(eventcount *ec_ptr){

	free(ec_ptr->name);
	free(ec_ptr);
	ec_ptr = NULL;

	return;
}

void * context_select(void){

	/*get next ctx to run*/
	if(ctx_hash_table_count)
	{
		do
		{
			current_context = ctx_hash_table[etime->count & HASHBY];
		}while(!current_context && etime->count++);

		ctx_hash_table[etime->count & HASHBY] = NULL;
		ctx_hash_table_count--;
	}
	else
	{
		//simulation has ended
		etime->count--;
		context_end(main_context);
	}

	return (void *)current_context->buf;
}

void pause(count_t value, context * my_ctx){
	//we only ever pause on etime.

	//update value to etime's count
	value += etime->count;

	//Get a pointer to next context first NOTE MAYBE NULLL!!!!
	context *head_ptr = my_ctx;
	my_ctx = my_ctx->batch_next;

	//insert my self into the hash table
	ctx_hash_insert(head_ptr, value & HASHBY);

	if(!set_jump(head_ptr->buf)) //update current context
	{
		if(my_ctx)
		{
			long_jump(my_ctx->buf);
		}
		else
		{
			etime->count++;
			long_jump(context_select());
		}
	}

	return;
}


void await(eventcount *ec, count_t value, context *my_ctx){

	/*todo
	check for stack overflows*/

	/*continue if the ctx's count is less
	 * than or equal to the ec's count*/
	if (ec->count >= value)
		return;

	/*the current context must now wait on this ec to be incremented*/
	if(!ec->ctx_list)
	{
		//nothing in EC's list
		//set the count to await
		my_ctx->count = value;

		//have ec point to halting context
		ec->ctx_list = my_ctx;

		//set curr ctx to next ctx in list (NOTE MAYBE NULL!!)
		my_ctx = my_ctx->batch_next;
	}
	else
	{
		//there is an ec waiting.
		fatal("await(): fixme handle more than one ctx in ec list");
	}

	if(!set_jump(ec->ctx_list->buf)) //update current context
	{
		if(my_ctx) //if there is another context run it
		{
			long_jump(my_ctx->buf);
		}
		else //we are out of contexts so get the next batch
		{
			etime->count++;
			long_jump(context_select());
		}
	}

	return;
}



void simulate(void){

	//simulate
	if(!context_simulate(main_context))
	{
		/*This is the beginning of the simulation
		 * get the first ctx and run it*/
		long_jump(context_select());
	}

	return;
}


void KnightSim_clean_up(void){

	int i = 0;
	eventcount *ec_ptr = NULL;
	context *ctx_ptr = NULL;

	//printf("KnightSim cleaning up\n");

	LIST_FOR_EACH_L(ecdestroylist, i, 0)
	{
		ec_ptr = (eventcount*)KnightSim_list_get(ecdestroylist, i);

		if(ec_ptr)
			eventcount_destroy(ec_ptr);
	}

	LIST_FOR_EACH_L(ctxdestroylist, i, 0)
	{
		ctx_ptr = (context*)KnightSim_list_get(ctxdestroylist, i);

		if(ctx_ptr)
			context_destroy(ctx_ptr);
	}

	KnightSim_list_clear(ctxdestroylist);
	KnightSim_list_free(ctxdestroylist);

	KnightSim_list_clear(ecdestroylist);
	KnightSim_list_free(ecdestroylist);

	return;
}


void context_start(void){

	//start of the context...

#if defined(__linux__) && defined(__x86_64)
	long long address = get_stack_ptr64() - (DEFAULT_STACK_SIZE - sizeof(context_data) - MAGIC_STACK_NUMBER);
	context_data * context_data_ptr = (context_data *)address;

	(*context_data_ptr->ctx_ptr->start)(context_data_ptr->ctx_ptr);

	/*if current current ctx returns i.e. hits the bottom of its function
	it will return here. So, terminate the context and move on*/
	context_terminate(context_data_ptr->ctx_ptr);

#else

	//32 bit is weird we have to hunt around for the struct need to subtract another 24 bytes.
	unsigned long address = get_stack_ptr32() - (DEFAULT_STACK_SIZE - sizeof(context_data) - MAGIC_STACK_NUMBER - 24);
	context_data * context_data_ptr = (context_data *)address;

	(*context_data_ptr->ctx_ptr->start)(context_data_ptr->ctx_ptr);

	/*if current current ctx returns i.e. hits the bottom of its function
	it will return here. So, terminate the context and move on*/
	context_terminate(context_data_ptr->ctx_ptr);

#endif

	fatal("context_start(): Should never be here!\n");

	return;
}


int set_jump(jmp_buf buf){

#if defined(__linux__) && defined(__i386__)
	return setjmp32_2(buf);
#elif defined(__linux__) && defined(__x86_64)
	return setjmp64_2(buf);
#else
#error Unsupported machine/OS combination
#endif
}

void long_jump(jmp_buf buf){

#if defined(__linux__) && defined(__i386__)
	longjmp32_2(buf, 1);
#elif defined(__linux__) && defined(__x86_64)
	longjmp64_2(buf, 1);
#else
#error Unsupported machine/OS combination
#endif

	return;
}


int context_simulate(jmp_buf buf){
#if defined(__linux__) && defined(__i386__)
	return setjmp32_2(buf);
#elif defined(__linux__) && defined(__x86_64)
	return setjmp64_2(buf);
#else
#error Unsupported machine/OS combination
#endif
}

void context_end(jmp_buf buf){
#if defined(__linux__) && defined(__i386__)
	longjmp32_2(buf, 1);
#elif defined(__linux__) && defined(__x86_64)
	longjmp64_2(buf, 1);
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
	new_context->buf[4] = ((int)((char*)new_context->stack + new_context->stacksize - MAGIC_STACK_NUMBER));

#elif defined(__linux__) && defined(__x86_64)
	new_context->buf[7] = (long long)context_start;
	new_context->buf[6] = (long long)((char *)new_context->stack + new_context->stacksize - MAGIC_STACK_NUMBER);

#else
#error Unsupported machine/OS combination
#endif

	return;
}


//KnightSim Util Stuff

list *KnightSim_list_create(unsigned int size)
{
	assert(size > 0);

	list *new_list;

	/* Create vector of elements */
	new_list = (list*)calloc(1, sizeof(list));
	new_list->size = size < 4 ? 4 : size;
	new_list->elem = (void**)calloc(new_list->size, sizeof(void *));

	/* Return list */
	return new_list;
}


void KnightSim_list_free(list *list_ptr)
{
	free(list_ptr->name);
	free(list_ptr->elem);
	free(list_ptr);
	return;
}


int KnightSim_list_count(list *list_ptr)
{
	return list_ptr->count;
}

void KnightSim_list_grow(list *list_ptr)
{
	void **nelem;
	int nsize, i, index;

	/* Create new array */
	nsize = list_ptr->size * 2;
	nelem = (void**)calloc(nsize, sizeof(void *));

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

void KnightSim_list_add(list *list_ptr, void *elem)
{
	/* Grow list if necessary */
	if (list_ptr->count == list_ptr->size)
		KnightSim_list_grow(list_ptr);

	/* Add element */
	list_ptr->elem[list_ptr->tail] = elem;
	list_ptr->tail = (list_ptr->tail + 1) % list_ptr->size;
	list_ptr->count++;

}


int KnightSim_list_index_of(list *list_ptr, void *elem)
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


void *KnightSim_list_remove_at(list *list_ptr, int index)
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


void *KnightSim_list_remove(list *list_ptr, void *elem)
{
	int index;

	/* Get index of element */
	index = KnightSim_list_index_of(list_ptr, elem);

	/* Delete element at found position */
	return KnightSim_list_remove_at(list_ptr, index);
}


void KnightSim_list_clear(list *list_ptr)
{
	list_ptr->count = 0;
	list_ptr->head = 0;
	list_ptr->tail = 0;
	return;
}

void KnightSim_list_enqueue(list *list_ptr, void *elem)
{
	KnightSim_list_add(list_ptr, elem);
}


void *KnightSim_list_dequeue(list *list_ptr)
{
	if (!list_ptr->count)
		return NULL;

	return KnightSim_list_remove_at(list_ptr, 0);
}


void *KnightSim_list_get(list *list_ptr, int index)
{
	/*Check bounds*/
	if (index < 0 || index >= list_ptr->count)
		return NULL;

	/*Return element*/
	index = (index + list_ptr->head) % list_ptr->size;
	return list_ptr->elem[index];
}


void KnightSim_list_insert(list *list_ptr, int index, void *elem){

	int shiftcount;
	int pos;
	int i;

	/*Check bounds*/
	assert(index >= 0 && index <= list_ptr->count);

	/*Grow list if necessary*/
	if(list_ptr->count == list_ptr->size)
		KnightSim_list_grow(list_ptr);

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
