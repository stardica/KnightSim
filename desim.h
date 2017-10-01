#ifndef __DESim_H__
#define __DESim_H__

#include <pthread.h>

#ifndef DEFAULT_STACK_SIZE
#define DEFAULT_STACK_SIZE 32768
#endif

#define STK_OVFL_MAGIC 0x12349876 /* stack overflow check */

#ifdef TIME_64
typedef unsigned long long Time_t;
#else
typedef long long Time_t;
#endif

# if __WORDSIZE == 64
typedef long int __jmp_buf[8];
# elif defined  __x86_64__
__extension__ typedef long long int __jmp_buf[8];
# else
typedef int __jmp_buf[6];
# endif

typedef __jmp_buf jmp_buf;

/*our assembly fucntion (.s) files these store
and load CPU register values. For DESim
the stack pointer and instruction pointer are
all we really care about.*/
#if defined(__linux__) && defined(__x86_64)

int setjmp64_2(jmp_buf __env);
void longjmp64_2(jmp_buf __env, int val);
long long encode64(long long val);
long long decode64(long long val);

#elif defined(__linux__) && defined(__i386__)

int setjmp32_2(jmp_buf __env);
void longjmp32_2(jmp_buf __env, int val);
int encode32(int val);
int decode32(int val);

#else
#error Unsupported machine/OS combination
#endif

#define CYCLE etime->count

typedef Time_t count;
typedef struct context_t context;
typedef struct eventcount_t eventcount;
typedef struct list_t list;
typedef struct thread_t thread;

/*typedef int thread_state;
enum {halted, running};*/

typedef int bool;
enum {false, true};

struct list_t{
	char *name;
	int count;  /* Number of elements in the list */
	int size;  /* Size of allocated vector */
	int head;  /* Head element in vector */
	int tail;  /* Tail element in vector */
	void **elem;  /* Vector of elements */
};

//Eventcount objects
struct eventcount_t{
	char * name;		/* string name of event count */
	long long id;
	list *ctxlist;
	count count;		/* current value of event */
};

//Context objects
struct context_t{
	jmp_buf buf;		/*state */
	char *name;			/* task name */
	int id;				/* task id */
	count count;		/* argument to await */
	void (*start)(void);	/*entry point*/
	unsigned magic;		/* stack overflow check */
	char *stack;		/*stack */
	int stacksize;		/*stack size*/
	thread *thread;  /*for parallel execution*/
};

//threads
struct thread_t{
	int id;	/*thread id for debugging*/
	thread * self; /*pointer to myself*/
	int return_val;
	pthread_t thread_handle; /*my thread handle*/
	context *context; /*The context I am to run*/
	pthread_mutex_t state_mutex;
	pthread_cond_t state;
	pthread_mutex_t temp_mutex;
	pthread_cond_t temp;
	jmp_buf home; /*for returning to myself*/
	//jmp_buf sim_end; /*for returning to myself*/
};

/* Globals*/
list *ctxdestroylist;
list *ctxlist;
list *ecdestroylist;
list *threadlist;

eventcount *etime;
context *current_context;
context *terminated_context;
context *curctx;

jmp_buf main_context;
long long ecid; //id for each event count

//PDESim
void desim_thread_pool_create(void);
thread *desim_thread_create(void);
void desim_thread_init(thread *new_thread);
void desim_thread_task(thread *self);
void thread_launch(void);
void thread_context_switch(void);
void thread_context_select(void);

void thread_await(eventcount *ec, count value);
void thread_advance(eventcount *ec);
void thread_pause(count value);


//DESim user level functions
void desim_init(void);
eventcount *eventcount_create(char *name);
void context_create(void (*func)(void), unsigned stacksize, char *name);
void simulate(void);
void await(eventcount *ec, count value);
void advance(eventcount *ec);
void pause(count value);

//DESim private functions
void eventcount_init(eventcount * ec, count count, char *ecname);
void eventcount_destroy(eventcount *ec);
void context_init(context *new_context);
void context_start(void);
void context_terminate(void);
int context_simulate(jmp_buf buf);
void context_end(jmp_buf buf);
context *context_select(void);
void context_switch(context *ctx_ptr);
void context_destroy(context *ctx_ptr);
void desim_end(void);


//DESim util stuff
void warning(const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));
void fatal(const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));

#define LIST_FOR_EACH_L(list_ptr, iter, iter_start_value) \
	for ((iter) = iter_start_value; (iter) < desim_list_count((list_ptr)); (iter)++)

#define LIST_FOR_EACH_LG(list_ptr, iter, iter_start_value) \
	for ((iter) = iter_start_value; (iter) <= desim_list_count((list_ptr)); (iter)++)

#define INLIST(X) (((X) + list_ptr->size) % list_ptr->size)

list *desim_list_create(unsigned int size);
void desim_list_insert(list *list_ptr, int index, void *elem);
void *desim_list_get(list *list_ptr, int index);
int desim_list_count(list *list_ptr);
void desim_list_enqueue(list *list_ptr, void *elem);
void *desim_list_dequeue(list *list_ptr);
void desim_list_add(list *list_ptr, void *elem);
void desim_list_grow(list *list_ptr);
void *desim_list_remove_at(list *list_ptr, int index);
void *desim_list_remove(list *list_ptr, void *elem);
int desim_list_index_of(list *list_ptr, void *elem);
void desim_list_clear(list *list_ptr);
void desim_list_free(list *list_ptr);

#endif /*__DESim_H__*/
