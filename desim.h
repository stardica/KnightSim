#ifndef __DESim_H__
#define __DESim_H__

#include "list.h"

#ifndef DEFAULT_STACK_SIZE
#define DEFAULT_STACK_SIZE 32768
#endif

#define TASKING_MAGIC_NUMBER 0x5a5a5a5a
#define STK_OVFL_MAGIC 0x12349876 /* stack overflow check */

typedef int bool;
enum {false, true};

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

/*our assembly fucntions (.s) files these store
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

typedef Time_t count_t;

typedef struct task_s task;
typedef struct context_t context;
typedef struct task_s task;
typedef struct process_record process;
typedef struct eventcount_s eventcount;

//Eventcount objects
struct eventcount_s {
	char * name;		/* string name of event count */
	long long id;
	/*task *tasklist;	 list of tasks waiting on this event */
	context *contextlist;
	count_t count;		/* current value of event */
	struct eventcount_s* eclist; /* pointer to next event count */
};

/* Global eventcount (cycle time) */
extern eventcount etime;
extern eventcount *ectail;
extern eventcount *last_ec; /* to work with context library */

//Context objects
struct context_t{
	jmp_buf buf;		/*state */
	char *name;			/* task name */
	int id;				/* task id */

	count_t count;		/* argument to await */
	context *contextlist;
	void (*start)(void);	/*entry point*/


	unsigned magic;		/* stack overflow check */
	char *stack;		/*stack */
	int stacksize;		/*stack size*/
};

/*struct task_s{
	task *tasklist;	 pointer to next task on the same list

	char *name;			 task name
	count_t count;		 argument to await

	void *cdata2;		 argument (don't ask why)!
	void *arg2;			 arg to the function!
	void (*f)(void *);	 startup function if necessary

	int id;				 task id
	unsigned magic;		 stack overflow check
	context c;		  	 context switch info
};*/

/*struct process_record {
  context c;
};*/



context *current_context;
context *terminated_context;

/*extern task inittask;
extern task *curtask;
extern task *hint;*/

context inittask;
context *curtask;
context *hint;

jmp_buf main_context;

extern long long ecid; //id for each event count

extern count_t last_value;


//DESim user level functions
void desim_init(void);
void simulate(void);
void pause(count_t);			/* wait argument time units */
void await(eventcount *ec, count_t value);  /* wait for event >= arg */
void advance(eventcount *ec);

void etime_init(void);
void initial_task_init(void);
void eventcount_init(eventcount * ec, count_t count, char *ecname);
void eventcount_destroy(eventcount *ec);
eventcount *eventcount_create(char *name);


context *context_create(void (*func)(void), unsigned stacksize, char *name);
void context_init(context *new_context);
void context_next(eventcount *ec, count_t value);
void context_remove_last(context *last_context);
void context_find_next(eventcount *ec, count_t value);
int context_simulate(void);
void context_end(void);
context *context_select(void);
void context_switch(context *ctx);
void context_exit(void);
void context_cleanup(void);
void context_destroy(context *ctx);

//util functions
void desim_list_insert(context *ptr1, context *ptr2);

#endif /*__DESim_H__*/
