#ifndef __CONTEXTS_H__
#define __CONTEXTS_H__

#include <stdlib.h>
#include <sys/time.h>
#include <stdio.h>
#include <signal.h>

#include "desim.h"

/*
 * A context consists of the state of the process, which is the
 * value of all registers (including pc) and the execution stack.
 *
 * For implementation purposes, we need the start address of the process
 * so that we can handle termination/invocation in a clean manner.
 *
 */


/*
 * At any given instant, "current_process" points to the process record
 * for the currently executing thread of control.
 */
extern process *current_process;

extern process *terminated_process;


/*
 * The following routine returns the next process to be executed.
 * (Provided by user)
 */
extern process *context_select (void);


/*
 * Perform a context switch. This routine must be called with interrupts
 * disabled. It re-enables interrupts.
 */
extern void context_switch (process *p);

extern void context_exit (void);

/*
 *  Call once before terminating computation. It must be called with
 *  interrupts disabled.
 */
extern void context_cleanup (void);


/*
 * Initialize the context field of the process record.
 * This procedure must be called with p->c.stack initialized.
 */
extern void context_init (process *p, void (*f)(void));

/*
 * User must provide a function that deletes the process record.
 */
extern void context_destroy (process *p);

#endif /* __CONTEXTS_H__ */
