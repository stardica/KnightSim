
#include <assert.h>

#include "desim.h"
#include "tasking.h"
#include "contexts.h"


/* current process and terminated process */
process *current_process = NULL;
process *terminated_process = NULL;


void context_cleanup (void){

  if (terminated_process)
  {
    context_destroy (terminated_process);
    terminated_process = NULL;
  }
}


static void context_stub (void){

  //printf("context stub\n");

  if (terminated_process)
  {
    context_destroy (terminated_process);
    terminated_process = NULL;
  }

  (*current_process->c.start)();

  /*if current current process returns i.e. hits the bottom of its function
  it will return here. Then we need to terminate the process*/

  //printf("exiting\n");

  terminated_process = current_process;

  context_switch (context_select());

}

void context_exit (void){

  terminated_process = current_process;
  context_switch (context_select ());
}


#if defined(__linux__) && defined(__i386__)

void context_switch (process *p)
{

	if (!current_process || !setjmp32_2(current_process->c.buf))
	{
	  current_process = p;
	  longjmp32_2(p->c.buf, 1);
	}
	if (terminated_process)
	{
	  context_destroy (terminated_process);
	  terminated_process = NULL;
	}

}

void context_init (process *p, void (*f)(void)){

	p->c.start = f; /*assigns the head of a function*/
	p->c.buf[5] = ((int)context_stub);
	p->c.buf[4] = ((int)((char*)p->c.stack + p->c.sz - 4));
}

void context_end(void){

	longjmp32_2(main_context, 1);
	return;
}

int context_simulate(void){

	return setjmp32_2(main_context);
}

#elif defined(__linux__) && defined(__x86_64)

void context_switch (process *p){

	//setjmp returns 1 if jumping to this position via longjmp
	if (!current_process || !setjmp64_2(current_process->c.buf))
	{
		/*first round takes us to context stub
		subsequent rounds take us to an await or pause*/

	  current_process = p;
	  longjmp64_2(p->c.buf, 1);
	}

	if (terminated_process)
	{
	  context_destroy (terminated_process);
	  terminated_process = NULL;
	}

  //printf("context switch\n");
}


void context_init (process *p, void (*f)(void)){

	p->c.start = f; /*assigns the head of a function*/
	p->c.buf[7] = (long long)context_stub; /*where the jump will go to*/
	p->c.buf[6] = (long long)((char *)p->c.stack + p->c.sz - 4); /*top of the stack*/
}

void context_end(void){

	longjmp64_2(main_context, 1);
	return;
}


int context_simulate(void){

	return setjmp64_2(main_context);
}

#else
#error Unsupported machine/OS combination
#endif
