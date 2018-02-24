#ifndef __KnightSim_H__
#define __KnightSim_H__

#ifndef DEFAULT_STACK_SIZE
#define DEFAULT_STACK_SIZE 32768
#endif

#define STK_OVFL_MAGIC 0x12349876 /* stack overflow check */

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

typedef Time_t count_t;
typedef __jmp_buf jmp_buf;

/*our assembly fucntion (.s) files these store
and load CPU register values. For KnightSim
the stack pointer and instruction pointer are
all we really care about.*/
#if defined(__linux__) && defined(__x86_64)

extern "C" int setjmp64_2(jmp_buf __env);
extern "C" void longjmp64_2(jmp_buf __env, int val);
extern "C"  long long encode64(long long val);
extern "C" long long decode64(long long val);

#elif defined(__linux__) && defined(__i386__)

extern "C" int setjmp32_2(jmp_buf __env);
extern "C" void longjmp32_2(jmp_buf __env, int val);
extern "C" int encode32(int val);
extern "C" int decode32(int val);

#else
#error Unsupported machine/OS combination
#endif

struct list_t{
	char *name;
	int count;  /* Number of elements in the list */
	int size;  /* Size of allocated vector */
	int head;  /* Head element in vector */
	int tail;  /* Tail element in vector */
	void **elem;  /* Vector of elements */
};

//Eventcount objects
struct eventcount_t{
	char * name;		/* string name of event count */
	long long id;
	struct list_t *ctxlist;
	count_t count;		/* current value of event */
	struct context_t *ctx_list;
	//pthread_mutex_t count_mutex;
};

//Context objects
struct context_t{
	jmp_buf buf;		/*state */
	char *name;			/* task name */
	int id;				/* task id */
	count_t count;		/* argument to await */
	void (*start)(void);	/*entry point*/
	unsigned magic;		/* stack overflow check */
	char *stack;		/*stack */
	int stacksize;		/*stack size*/
	struct context_t * batch_next;
};


typedef struct context_t context;
typedef struct eventcount_t eventcount;
typedef struct list_t list;
typedef struct thread_t thread;

extern jmp_buf main_context;
extern jmp_buf halt_context;

extern context *current_context;

extern list *ecdestroylist;

extern eventcount *etime;

#define CYCLE etime->count

extern unsigned long long etime_start;
extern unsigned long long etime_time;

//KnightSim user level functions
void KnightSim_init(void);
eventcount *eventcount_create(char *name);
void context_create(void (*func)(void), unsigned stacksize, char *name, int id);
void simulate(void);
void await(eventcount *ec, count_t value);
void advance(eventcount *ec);
void pause(count_t value);
void context_init_halt(void);

//KnightSim private functions
void ctx_hash_insert(context *context_ptr, unsigned int where);
void eventcount_init(eventcount * ec, count_t count, char *ecname);
void eventcount_destroy(eventcount *ec);
void context_init(context *new_context);
void context_start(void);
void context_terminate(void);
int context_simulate(jmp_buf buf);
void context_end(jmp_buf buf);
context *context_select(void);
void context_switch(context *ctx_ptr);
void context_destroy(context *ctx_ptr);
void KnightSim_clean_up(void);

//KnightSim/PKnightSim util stuff
#define FFLUSH fflush(stderr); fflush(stdout);

void KnightSim_pause(void);
void KnightSim_dump_queues(void);

void warning(const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));
void fatal(const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));

#define LIST_FOR_EACH_L(list_ptr, iter, iter_start_value) \
	for ((iter) = iter_start_value; (iter) < (list_ptr->count); (iter)++)

#define LIST_FOR_EACH_LG(list_ptr, iter, iter_start_value) \
	for ((iter) = iter_start_value; (iter) <= (KnightSim_list_count(list_ptr)); (iter)++)

#define INLIST(X) (((X) + list_ptr->size) % list_ptr->size)

list *KnightSim_list_create(unsigned int size);
void KnightSim_list_insert(list *list_ptr, int index, void *elem);
void *KnightSim_list_get(list *list_ptr, int index);
int KnightSim_list_count(list *list_ptr);
void KnightSim_list_enqueue(list *list_ptr, void *elem);
void *KnightSim_list_dequeue(list *list_ptr);
void KnightSim_list_add(list *list_ptr, void *elem);
void KnightSim_list_grow(list *list_ptr);
void *KnightSim_list_remove_at(list *list_ptr, int index);
void *KnightSim_list_remove(list *list_ptr, void *elem);
int KnightSim_list_index_of(list *list_ptr, void *elem);
void KnightSim_list_clear(list *list_ptr);
void KnightSim_list_free(list *list_ptr);

#endif /*__KnightSim_H__*/
