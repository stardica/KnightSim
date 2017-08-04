/*
 * DESim.c
 *
 *  Created on: Aug 3, 2017
 *      Author: stardica
 */

#include "desim.h"
#include "eventcount.h"
#include "tasking.h"


long long ecid = 0;
count_t last_value = 0;


void desim_init(void){

	//init etime
	etime_init();

	initial_task_init();

	return;
}



/*crazy Linux encode decode functions.
undoes the stack pointer protection and fortify source
This may not be portable on some systems or future proof...*/

#if defined(__linux__) && defined(__i386__)

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

void end_simulate(void){

	longjmp32_2(main_context, 1);
	return;
}

int desim_end(void){

	return setjmp32_2(main_context);
}

#elif defined(__linux__) && defined(__x86_64)

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


void end_simulate(void){

	longjmp64_2(main_context, 1);
	return;
}


int desim_end(void){

	return setjmp64_2(main_context);
}

#else
#error Unsupported machine/OS combination
#endif
