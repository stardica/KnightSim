#include "desim.h"

#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <pthread.h>


list *ctxdestroylist = NULL;
list *ctxlist = NULL;
list *ecdestroylist = NULL;

context *last_context = NULL;
context *current_context = NULL;
context *ctxhint = NULL;
eventcount *etime = NULL;

long long ecid = 0;
long long ctxid = 0;
long long threadid = 0;


context *terminated_context = NULL;

pthread_mutex_t threads_created_mutex;
pthread_cond_t thread_created;

pthread_mutex_t sim_end_mutex;
pthread_cond_t sim_end;

pthread_mutex_t thread_launch_mutex;

count thread_count = 0;



void desim_init(void){

#ifdef NUM_THREADS
	warning("desim_init(): DESim is running in parallel mode.\n");
	FFLUSH
#endif

	char buff[100];

	//other globals
	ctxdestroylist = desim_list_create(4);
	ctxlist = desim_list_create(4);
	ecdestroylist = desim_list_create(4);

	//set up etime
	memset(buff,'\0' , 100);
	snprintf(buff, 100, "etime");
	etime = eventcount_create(strdup(buff));

	//create the thread pool
#ifdef NUM_THREADS
	thread_pool_create();
#endif

	return;
}

#ifdef NUM_THREADS
void thread_pool_create(void){

	int i = 0;
	thread *new_thread = NULL;

	pthread_mutex_init(&threads_created_mutex, NULL);
	pthread_cond_init (&thread_created, NULL);

	pthread_mutex_init(&thread_launch_mutex, NULL);

	threadlist = desim_list_create(NUM_THREADS);

	/*initialize the thread hash table*/

	/*we are hashing based on the address of each
	pthread_t. The table size can not be a power
	of two. Pthread_t addresses are aligned to 32bit and 64bit
	addresses. We just and some slack and make the table size
	and odd number*/
	for(i = 0; i < HASHSIZE; i++)
		thread_hash_table[i] = NULL;

	for(i = 0; i < NUM_THREADS; i++)
	{
		//create threads here
		new_thread = thread_create();
		assert(new_thread);

		desim_list_insert(threadlist, 0, new_thread);

		if(thread_hash_table[new_thread->thread_handle % HASHSIZE] != NULL)
			fatal("thread_pool_create(): more than one thread hashed to the same index, fix me.\n");

		assert(thread_hash_table[new_thread->thread_handle % HASHSIZE] == NULL);
		thread_hash_table[new_thread->thread_handle % HASHSIZE] = new_thread;
	}

	/*we must wait until all threads are created and awaiting work
	 * when the last thread is created main will be signaled (thread_created)*/
	pthread_mutex_lock(&threads_created_mutex);
	while(thread_count < NUM_THREADS)
	{
		pthread_cond_wait(&thread_created, &threads_created_mutex);
	}
	pthread_mutex_unlock(&threads_created_mutex);

	warning("thread_pool_create(): Num threads %d threadlist size %d\n", (int) thread_count, desim_list_count(threadlist));
	FFLUSH

	return;
}

thread *thread_create(void){

	thread *new_thread = NULL;

	new_thread = (thread *)malloc(sizeof(thread));
	assert(new_thread);

	thread_init(new_thread);

	return new_thread;
}

void thread_init(thread *new_thread){

	new_thread->id = threadid++;
	new_thread->self = new_thread;
	pthread_mutex_init(&new_thread->run_mutex, NULL);
	pthread_cond_init (&new_thread->run, NULL);
	pthread_mutex_init(&new_thread->pause_mutex, NULL);
	pthread_cond_init (&new_thread->pause, NULL);
	new_thread->return_val = pthread_create(&new_thread->thread_handle, NULL, (void *)thread_control, (void *) new_thread);

	return;
}

void thread_sync(thread *thread_ptr){

	pthread_cond_wait(&thread_ptr->run, &thread_ptr->run_mutex);

	return;
}

void thread_control(thread *self){

	//signal main if this is the last thread
	pthread_mutex_lock(&threads_created_mutex);
	thread_count++;

	if(thread_count == NUM_THREADS)
		pthread_cond_signal(&thread_created);

	pthread_mutex_unlock(&threads_created_mutex);


	while(1){

		//printf("thread_control(): thread awaiting id %d\n", self->id);
		assert(self->context == NULL);
		thread_sync(self); //wait here until given work to do

		assert(self->context);
		//warning("thread_control(): thread id %d processing context id %d\n", self->id, self->context->id);

		if(!context_simulate(self->home))
			thread_context_switch(self->context->buf);

		//printf("thread_control(): thread has returned id %d\n", self->id);
		//FFLUSH
	}

	return;
}

void thread_await(eventcount *ec, count value){

	//OMG! figure out who we are!!!
	thread *thread_ptr = thread_get_ptr(pthread_self());
	context *context_ptr = NULL;
	int i = 0;

	/*the current context must now wait on this ec to be incremented*/

	/*we are here because of an await
	stop the current context and drop it into
	the ec that this context is awaiting

	determine if the ec has another context
	in its awaiting ctx list*/

	/*continue if the ctx's count is less
	 * than or equal to the ec's count*/

	pthread_mutex_lock(&ec->count_mutex); //lock this section
	if (ec->count >= value){

		//printf("thread_await(): continuing ec->count %d val %d id %d\n", (int) ec->count, (int) value, thread_ptr->id);
		pthread_mutex_unlock(&ec->count_mutex);
		return;
	}

	//printf("thread_await(): halting ec->count %d val %d id %d\n", (int) ec->count, (int) value, thread_ptr->id);
	LIST_FOR_EACH_LG(ec->ctxlist, i, 0)
	{
		context_ptr = desim_list_get(ec->ctxlist, i);
		if(!context_ptr || (context_ptr && value < context_ptr->count))
		{
			/*we are at the head or tail of the list
			insert ctx at this position*/

			//printf("thread_await(): context removed and added to ec id %d\n", thread_ptr->id);
			//set the curctx's value
			thread_ptr->context->count = value;
			desim_list_insert(ec->ctxlist, i, thread_ptr->context);
			break;
		}
	}

	/*Halt this thread. Update the context's last position,
	return the thread and context to their queues.*/
	if(!context_simulate(thread_ptr->context->buf))
	{
		//clear context pointer
		thread_ptr->context = NULL;

		//deschedule thread
		desim_list_enqueue(threadlist, thread_ptr);

		//exit critical section
		pthread_mutex_unlock(&ec->count_mutex);

		/*check for cycle advance! If no contexts are avaiaable
		and all threads are halted the simulation is ready
		to advance in cycles pull all contexts that are ready
		to run from etime's ctx list and launch threads*/

		pthread_mutex_unlock(&thread_launch_mutex);
		if(desim_list_count(ctxlist) == 0 && desim_list_count(threadlist) == 32)
		{
			thread_etime_launch();
		}
		pthread_mutex_lock(&thread_launch_mutex);

		context_end(thread_ptr->home);
	}


	//if here resuming context from last position.
	return;
}

void thread_advance(eventcount *ec){

	int i = 0;

	/*check ec's ctx list*/
	context *context_ptr = NULL;

	/* advance the ec's count */
	pthread_mutex_lock(&ec->count_mutex); //lock this section
	ec->count++;

	/*if here, there is a ctx(s) waiting on this ec AND
	 * its ready to run ready to run
	 * find the end of the ec's task list
	 * and set all ctx time to current time (etime.cout)*/
	LIST_FOR_EACH_L(ec->ctxlist, i, 0)
	{
		context_ptr = desim_list_get(ec->ctxlist, i);

		if(context_ptr && (context_ptr->count == ec->count))
		{
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
	pthread_mutex_unlock(&ec->count_mutex);

	thread_launch();

	return;
}

void thread_pause(count value){

	return;
}

void thread_sleep(thread *thread_ptr){

	pthread_cond_wait(&thread_ptr->pause, &thread_ptr->pause_mutex);
	return;
}



void thread_etime_launch(void){

	//no mutex needed, as all threads have stopped.
	thread *thread_ptr = NULL;
	context *context_ptr = NULL;
	count next_ctx_count = 0;
	int i = 0;

	//remove the first context from the etime's ctxlist
	context_ptr = desim_list_dequeue(etime->ctxlist);
	next_ctx_count = context_ptr->count;
	desim_list_enqueue(ctxlist, context_ptr);

	//Pull any other contexts that have the same count as the first
	LIST_FOR_EACH_L(etime->ctxlist, i, 0)
	{
		context_ptr = desim_list_get(etime->ctxlist, i);

		if(context_ptr && (next_ctx_count == context_ptr->count))
		{
			context_ptr = desim_list_remove(etime->ctxlist, context_ptr);
			desim_list_enqueue(ctxlist, context_ptr);
		}
		else
		{
			break;
		}
	}

	//we have loaded up the ctx list with contexts to run launch threads

	/*pull a thread from the threads list assign it a context and
	signal the thread to run the context*/
	while(desim_list_count(threadlist) && desim_list_count(ctxlist))
	{
		thread_ptr = desim_list_dequeue(threadlist);
		context_ptr = desim_list_dequeue(ctxlist);
		assert(thread_ptr && context_ptr);

		//warning("thread_launch(): thread %d assigned ctx id %d\n", thread_ptr->id, context_ptr->id);

		//etime is now the current cycle count determined by the ctx's count
		etime->count = context_ptr->count;

		thread_ptr->context = context_ptr;

		pthread_cond_signal(&thread_ptr->run);
	}

	return;
}

void thread_launch(void){

	thread *thread_ptr = NULL;
	context *context_ptr = NULL;

	/*pull a thread from the threads list assign it a context and
	signal the thread to run the context*/
	pthread_mutex_unlock(&thread_launch_mutex);
	assert(desim_list_count(threadlist) != 0);
	while(desim_list_count(threadlist) && desim_list_count(ctxlist))
	{
		thread_ptr = desim_list_dequeue(threadlist);
		context_ptr = desim_list_dequeue(ctxlist);
		assert(thread_ptr && context_ptr);

		//warning("thread_launch(): thread %d assigned ctx id %d\n", thread_ptr->id, context_ptr->id);

		//etime is now the current cycle count determined by the ctx's count
		etime->count = context_ptr->count;

		thread_ptr->context = context_ptr;

		pthread_cond_signal(&thread_ptr->run);
	}
	pthread_mutex_lock(&thread_launch_mutex);

	return;

}

thread *thread_get_ptr(pthread_t thread_handle){

	thread *thread_ptr = NULL;

	thread_ptr = thread_hash_table[thread_handle % HASHSIZE];
	assert(thread_ptr);
	assert(thread_ptr->thread_handle == thread_handle);

	return thread_ptr;
}


void thread_context_start(void){

	//OMG figure out who we are!!!
	thread *thread_ptr = thread_get_ptr(pthread_self());

	(*thread_ptr->context->start)();

	//fatal("context_start(): SIM ENDING\n");

	/*if current current ctx returns i.e. hits the bottom of its function
	it will return here. Then we need to terminate the simulation*/

	context_end(main_context);

	return;
}

void thread_context_switch(jmp_buf buf){

#if defined(__linux__) && defined(__i386__)
	longjmp32_2(buf, 1);
#elif defined(__linux__) && defined(__x86_64)
	longjmp64_2(buf, 1);
#else
#error Unsupported machine/OS combination
#endif

	return;
}

void thread_context_init(context *new_context){

	/*these are in this function because they are architecture dependent.
	don't move these out of this function!!!!*/

	/*instruction pointer and then pointer to top of stack*/

#if defined(__linux__) && defined(__i386__)
	new_context->buf[5] = ((int)thread_context_start);
	new_context->buf[4] = ((int)((char*)new_context->stack + new_context->stacksize - 4));
#elif defined(__linux__) && defined(__x86_64)
	new_context->buf[7] = (long long)thread_context_start;
	new_context->buf[6] = (long long)((char *)new_context->stack + new_context->stacksize - 4);
#else
#error Unsupported machine/OS combination
#endif

	return;
}
#endif

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
	new_context_ptr->id = ctxid++;
	new_context_ptr->stack = (char *)malloc(stacksize);
	assert(new_context_ptr->stack);
	new_context_ptr->stacksize = stacksize;
	new_context_ptr->magic = STK_OVFL_MAGIC; // for stack overflow check
	new_context_ptr->start = func; /*assigns the head of a function*/

#ifdef NUM_THREADS
	thread_context_init(new_context_ptr);
#else
	context_init(new_context_ptr);
#endif

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
	pthread_mutex_init(&ec->count_mutex, NULL); //only used for parallel implementations
    return;
}

void advance(eventcount *ec){

#ifdef NUM_THREADS
	thread_advance(ec);
#else

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

#endif

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

#ifdef NUM_THREADS
	thread_await(etime, etime->count + value);
#else
	await(etime, etime->count + value);
#endif

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
					"Simulation has either ended or there is a problem with the simulation implementation.\n");
			context_end(main_context);
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

#ifdef NUM_THREADS
	thread_await(ec, value);
#else

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
#endif

	return;
}


void simulate(void){

	//simulate
	if(!context_simulate(main_context))
	{
		/*This is the beginning of the simulation
		 * get the first ctx and run it*/
		#ifdef NUM_THREADS
			thread_launch();
			/*pause until sim end*/
			desim_pause();
		#else
			context_switch(context_select());
		#endif
	}

	//clean up
	desim_end();

	return;
}




void desim_pause(void){

	pthread_cond_wait(&sim_end, &sim_end_mutex);

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

	context_end(main_context);

	return;
}

void context_switch(context *ctx_ptr)
{
	//setjmp returns 1 if jumping to this position via longjmp

	/*ok, this is deep wizardry....
	note that the jump is to the next context and the
	setjmp is for the current context*/

	/*printf("ctx switch():\n");*/

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
