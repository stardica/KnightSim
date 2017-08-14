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

//Eventcount objects
struct eventcount_s {
	char * name;		/* string name of event count */
	long long id;
	task *tasklist;	/* list of tasks waiting on this event */
	count_t count;		/* current value of event */
	struct eventcount_s* eclist; /* pointer to next event count */
};

typedef struct eventcount_s eventcount;

/* Global eventcount (cycle time) */
extern eventcount etime;
extern eventcount *ectail;
extern eventcount *last_ec; /* to work with context library */


//Context objects
struct context_t{
  jmp_buf buf;			/*state */
  char *stack;			/*stack */
  int sz;				/*stack size*/
  void (*start) ();		/*entry point*/
};

struct task_s{
	task *tasklist;	/* pointer to next task on the same list */

	char *name;			/* task name */
	count_t count;		/* argument to await */

	void *cdata2;		/* argument (don't ask why)! */
	void *arg2;			/* arg to the function! */
	void (*f)(void *);	/* startup function if necessary */

	int id;				/* task id */
	unsigned magic;		/* stack overflow check */
	context c;		  	/* context switch info */
};

struct process_record {
  context c;
};


extern process *current_process;
extern process *terminated_process;
extern task inittask;
extern task *curtask;
extern task *hint;

jmp_buf main_context;
extern long long ecid; //id for each event count
extern count_t last_value;


//DESim entry and exit functions
void desim_init(void);
void end_simulate(void);
int desim_end(void);
task *context_create(void (*func)(void), unsigned stacksize, char *name);
void initial_task_init(void);
void simulate(void);
void epause(count_t);			/* wait argument time units */
void await(eventcount *ec, count_t value);  /* wait for event >= arg */
void switch_context (eventcount *ec, count_t value);

//eventcount manipulation functions
void etime_init(void);
eventcount *eventcount_create (char *name);
void eventcount_init(eventcount * ec, count_t count, char *ecname);
void advance(eventcount *ec);
void eventcount_destroy(eventcount *ec);


//task manipulation functions
count_t get_time(void);
char *get_task_name(void);
void remove_last_task(task *);

void context_set_id(int id);
int context_get_id(void);
int context_simulate(void);
void context_end(void);
extern process *context_select(void);
extern void context_switch(process *p);
extern void context_exit(void);
extern void context_cleanup(void);
extern void context_init(process *p, void (*f)(void));
extern void context_destroy(process *p);

#endif /*__DESim_H__*/
