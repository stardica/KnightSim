#ifndef __TASKING_H__
#define __TASKING_H__


#include "desim.h"
#include "contexts.h"
#include "eventcount.h"


#ifndef DEFAULT_STACK_SIZE
#define DEFAULT_STACK_SIZE 32768
#endif

#define TASKING_MAGIC_NUMBER 0x5a5a5a5a
#define STK_OVFL_MAGIC 0x12349876 /* stack overflow check */

struct context_t{
  jmp_buf buf;			/*state */
  char *stack;			/*stack */
  int sz;				/*stack size*/
  void (*start) ();		/*entry point*/
};

struct task_s{
	struct task_s *tasklist;	/* pointer to next task on the same list */
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


extern task inittask;
extern task *curtask;
extern task *hint;

//DESim application interface
task *task_create(void (*func)(void), unsigned stacksize, char *name);

void initial_task_init(void);


void simulate(void);
void epause(count_t);			/* wait argument time units */
void await(eventcount *ec, count_t value);  /* wait for event >= arg */
void switch_context (eventcount *ec, count_t value);


//task manipulation functions
count_t get_time (void);
void set_id(int id);
char *get_task_name(void);
void set_id(int id);
int get_id(void);
void remove_last_task(task *);


#endif /* __TASKING_H__ */
