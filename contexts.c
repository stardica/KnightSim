
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

  printf("context stub\n");

  if (terminated_process)
  {
    context_destroy (terminated_process);
    terminated_process = NULL;
  }

  (*current_process->c.start)();

  /*if current current process returns i.e. hits the bottom of its function
  it will return here. Then we need to terminate the process*/

  printf("exiting\n");

  terminated_process = current_process;

  context_switch (context_select());

}

void context_exit (void){

  terminated_process = current_process;
  context_switch (context_select ());
}

#if defined(__linux__) && defined(__i386__)

void context_switch (process_t *p)
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

void context_init (process_t *p, void (*f)(void)){

  void *stack;
  int n;

  p->c.start = f; /*assigns the head of a function*/
  stack = p->c.stack;
  n = p->c.sz;

  setjmp32_2(p->c.buf);

  #define INIT_SP(p) (int)((char*)(p)->c.stack + (p)->c.sz)
  #define CURR_SP(p) ((p)->c.buf[0].__jmpbuf[4])

  //p->c.buf[0].__jmpbuf[5] = ((int)context_stub);
  //p->c.buf[0].__jmpbuf[4] = ((int)((char*)stack+n-4));
  p->c.buf[5] = ((int)context_stub);
  p->c.buf[4] = ((int)((char*)stack+n-4));
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

  printf("context switch\n");

}

/*
 * Non-portable code is in this function
 */

void context_init (process *p, void (*f)(void)){

  void *stack = NULL;
  int n;

  p->c.start = f; /*assigns the head of a function*/
  stack = p->c.stack;
  assert(stack);

  n = p->c.sz;

  setjmp64_2(p->c.buf);

  #define INIT_SP(p) ((char *)stack + n - 4)
  #define CURR_SP(p) p->c.buf[0].__jmpbuf[4]

  //p->c.buf[0].__jmpbuf[7] = (long long)context_stub;
  //p->c.buf[0].__jmpbuf[6] = (long long)((char *)stack + n - 4);
  p->c.buf[7] = (long long)context_stub;
  p->c.buf[6] = (long long)((char *)stack + n - 4);

}

#else
#error Unsupported machine/OS combination
#endif
