#ifndef __DESim_H__
#define __DESim_H__

typedef char boolean;

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

#if defined(__linux__) && defined(__i386__)

int setjmp32_2(jmp_buf __env);
void longjmp32_2(jmp_buf __env, int val);

int DecodeJMPBUF32(int j);
int EncodeJMPBUF32(int j);

#elif defined(__linux__) && defined(__x86_64)

int setjmp64_2(jmp_buf __env);
void longjmp64_2(jmp_buf __env, int val);

long long DecodeJMPBUF64(long long j);
long long EncodeJMPBUF64(long long j);

#else
#error Unsupported machine/OS combination
#endif

typedef Time_t count_t;
typedef struct eventcount_s eventcount;
typedef struct context_t context;
typedef struct task_s task;
typedef struct process_record process;


//id for eventcounts
jmp_buf main_context;
extern long long ecid;
extern count_t last_value;


void desim_init(void);
void end_simulate(void);
int desim_end(void);


#endif /*__DESim_H__*/
