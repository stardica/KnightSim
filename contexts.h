#ifndef __CONTEXTS_H__
#define __CONTEXTS_H__


#include <stdlib.h>
#include <setjmp.h>
#include <sys/time.h>
#include <stdio.h>

#ifndef DEFAULT_STACK_SIZE
#define DEFAULT_STACK_SIZE 32768
#else
#define LARGE_STACK_SIZE 65536
#endif


/*
 * A process record. The first field of a process record must be
 * a context.
 *
 * The user must provide:
 *
 *       struct process_record {
 *             context_t c;        // The c.stack field must be initialized
 *             ...
 *       };
 *
 */
typedef struct process_record process_t;


/*
 * A context consists of the state of the process, which is the
 * value of all registers (including pc) and the execution stack.
 *
 * For implementation purposes, we need the start address of the process
 * so that we can handle termination/invocation in a clean manner.
 *
 */
typedef struct {
  jmp_buf buf;			/*state */
  char *stack;			/*stack */
  int sz;				/*stack size*/
  void (*start) ();		/*entry point*/
} context_t;


struct process_record {
  context_t c;
};


/*
 * At any given instant, "current_process" points to the process record
 * for the currently executing thread of control.
 */
extern process_t *current_process;
extern process_t *terminated_process;


/*
 * The following routine returns the next process to be executed.
 *
 * (Provided by user)
 */
extern process_t *context_select (void);


/*
 * Perform a context switch. This routine must be called with interrupts
 * disabled. It re-enables interrupts.
 */
extern void context_switch (process_t *);

/*
 *
 *  Call once before terminating computation. It must be called with
 *  interrupts disabled.
 *
 */
extern void context_cleanup (void);


/*
 * Initialize the context field of the process record.
 * This procedure must be called with p->c.stack initialized.
 */
extern void context_init (process_t *, void (*f)(void));

/*
 * User must provide a function that deletes the process record.
 */
extern void context_destroy (process_t *);

#endif /* __CONTEXTS_H__ */
