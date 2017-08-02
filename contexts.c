
#include <stdio.h>
#include <signal.h>
#include <sys/time.h>
#include "contexts.h"

/* current process and terminated process */
process_t *current_process = NULL;
process_t *terminated_process = NULL;


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

  terminated_process = current_process;

  context_switch (context_select());

}

void context_exit (void){

  terminated_process = current_process;
  context_switch (context_select ());
}

#if defined(__linux__) && defined(__i386__)

int setjmp32_2(jmp_buf __env);
void longjmp32_2(jmp_buf __env, int val);

int DecodeJMPBUF32(int j){

    int retVal;

    asm (	"mov %1,%%edx; "
 		    "ror $0x9,%%edx; "
 		    "xor %%gs:0x18,%%edx; "
 		    "mov %%edx,%0;"
       :"=r" (retVal)  /* output */
       :"r" (j)        /* input */
       :"%edx"         /* clobbered register */
    );

    return retVal;
}

int EncodeJMPBUF32(int j){

    int retVal;

    asm ("mov %1,%%edx; "
 		   "xor %%gs:0x18,%%edx; "
 		   "rol $0x9,%%edx; "
 		   "mov %%edx,%0;"
       :"=r" (retVal)  /* output */
       :"r" (j)        /* input */
       :"%edx"         /* clobbered register */
    );

    return retVal;
}

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

  p->c.buf[0].__jmpbuf[5] = ((int)context_stub);
  p->c.buf[0].__jmpbuf[4] = ((int)((char*)stack+n-4));
}


#elif defined(__linux__) && defined(__x86_64)

int setjmp64_2(jmp_buf __env);
void longjmp64_2(jmp_buf __env, int val);

long long DecodeJMPBUF64(long long j){

      long long retVal;

      asm volatile ("mov %1, %%rax;"
   		    "ror $0x11, %%rax;"
   		    "xor %%fs:0x30, %%rax;"
   		    "mov %%rax, %0;"
         :"=r" (retVal)   /*output*/
         :"r" (j)         /*input*/
         :"%rax"          /*clobbered register*/
      );

      return retVal;
}

long long EncodeJMPBUF64(long long j) {

   long long retVal;

   	asm (	"mov %1, %%rax;"
   			"xor %%fs:0x30, %%rax;"
   			"rol $0x11, %%rax;"
   			"mov %%rax, %0;"
   			:"=r" (retVal)   /*output*/
   			:"r" (j)         /*input*/
   			:"%rax"          /*clobbered register*/
   		);

   	return retVal;
}

void context_switch (process_t *p){


	if (!current_process || !setjmp64_2(current_process->c.buf))
  {
	  current_process = p;
	  //_longjmp(p->c.buf,1);
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

void context_init (process_t *p, void (*f)(void)){

  void *stack;
  int n;

  p->c.start = f; /*assigns the head of a function*/
  stack = p->c.stack;
  n = p->c.sz;

  setjmp64_2(p->c.buf);

  #define INIT_SP(p) ((char *)stack + n - 4)
  #define CURR_SP(p) p->c.buf[0].__jmpbuf[4]

  p->c.buf[0].__jmpbuf[7] = (long long)context_stub;
  p->c.buf[0].__jmpbuf[6] = (long long)((char *)stack + n - 4);

}

#else
#error Unsupported machine/OS combination
#endif
