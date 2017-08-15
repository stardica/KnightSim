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
typedef struct context_t context;
typedef struct eventcount_s eventcount;

//Eventcount objects
struct eventcount_s {
	char * name;		/* string name of event count */
	long long id;
	context *nextcontext;
	count_t count;		/* current value of event */
	eventcount *nextec; /* pointer to eclist head */
};


//Context objects
struct context_t{
	jmp_buf buf;		/*state */
	char *name;			/* task name */
	int id;				/* task id */
	count_t count;		/* argument to await */
	context *nextcontext;
	void (*start)(void);	/*entry point*/
	unsigned magic;		/* stack overflow check */
	char *stack;		/*stack */
	int stacksize;		/*stack size*/
};


/* Globals*/
eventcount etime;
eventcount *ectail;
eventcount *last_ec; /* to work with context library */
context *current_context;
context *terminated_context;
context inittask;
context *curtask;
context *hint;
jmp_buf main_context;
long long ecid; //id for each event count
count_t last_value;

//DESim user level functions
void desim_init(void);
eventcount *eventcount_create(char *name);
void context_create(void (*func)(void), unsigned stacksize, char *name);
void simulate(void);
void pause(count_t);
void await(eventcount *ec, count_t value);
void advance(eventcount *ec);
void eventcount_init(eventcount * ec, count_t count, char *ecname);
void eventcount_destroy(eventcount *ec);
void context_init(context *new_context);
void context_stub(void);
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
