#ifndef __TASKING_H__
#define __TASKING_H__

#include "contexts.h"

#define TASKING_MAGIC_NUMBER 0x5a5a5a5a


#ifdef TIME_64
typedef unsigned long long Time_t;
#else
typedef long long Time_t;
#endif


typedef Time_t count_t;


typedef struct task_s {
	struct task_s *tasklist;	/* pointer to next task on the same list */
	char *name;			/* task name */
	count_t count;		/* argument to await */
  
	void *cdata2;		/* argument (don't ask why)! */
	void *arg2;			/* arg to the function! */
	void (*f)(void *);	/* startup function if necessary */

	int id;				/* task id */
	unsigned magic;		/* stack overflow check */
	context_t c;		/* context switch info */
}task;

extern task *curtask;

typedef volatile struct eventcount_s eventcount;

struct eventcount_s {
	task * tasklist;	/* list of tasks waiting on this event */
	char * name;		/* string name of eventcount */
	count_t count;		/* current value of event */
	volatile struct eventcount_s* eclist; /* pointer to next eventcount */
};

extern eventcount etime;	/* time counter */


//DESim application interface
task * create_task(void (*)(void), unsigned, char *);
eventcount *new_eventcount (char *name);
void simulate (void);
int simulate_end(void);
void end_tasking(void);
void epause(count_t);			/* wait argument time units */
void advance(eventcount *);			/* increment eventcount */
void await(eventcount *, count_t);  /* wait for event >= arg */

//task manipulation functions
count_t get_time (void);
void set_id(unsigned id);
char *get_task_name(void);
void set_id(unsigned id);
unsigned get_id(void);
void remove_last_task(task *);
void initialize_event_count(eventcount *, count_t, char *);
void delete_event_count(eventcount *);

#endif /* __TASKING_H__ */
