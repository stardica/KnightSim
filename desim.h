#ifndef __DESim_H__
#define __DESim_H__

#ifndef DEFAULT_STACK_SIZE
#define DEFAULT_STACK_SIZE 32768
#endif

#define TASKING_MAGIC_NUMBER 0x5a5a5a5a
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

typedef __jmp_buf jmp_buf;

/*our assembly fucntions (.s) files these store
and load CPU register values. For DESim
the stack pointer and instruction pointer are
all we really care about.*/
#if defined(__linux__) && defined(__x86_64)

int setjmp64_2(jmp_buf __env);
void longjmp64_2(jmp_buf __env, int val);
long long encode64(long long val);
long long decode64(long long val);

#elif defined(__linux__) && defined(__i386__)

int setjmp32_2(jmp_buf __env);
void longjmp32_2(jmp_buf __env, int val);
int encode32(int val);
int decode32(int val);

#else
#error Unsupported machine/OS combination
#endif

typedef Time_t count_t;
typedef struct context_t context;
typedef struct eventcount_s eventcount;
typedef struct list_t list;

struct list_t{
	char *name;
	int count;  /* Number of elements in the list */
	int size;  /* Size of allocated vector */
	int head;  /* Head element in vector */
	int tail;  /* Tail element in vector */
	void **elem;  /* Vector of elements */
};

//Eventcount objects
struct eventcount_s{
	char * name;		/* string name of event count */
	long long id;
	context *nextcontext;
	list *ctxlist;
	count_t count;		/* current value of event */
	eventcount *nextec; /* pointer to eclist head */
};

//Context objects
struct context_t{
	jmp_buf buf;		/*state */
	char *name;			/* task name */
	int id;				/* task id */
	count_t count;		/* argument to await */
	context *nextcontext;
	//list *ctxlist;
	void (*start)(void);	/*entry point*/
	unsigned magic;		/* stack overflow check */
	char *stack;		/*stack */
	int stacksize;		/*stack size*/
};

typedef int bool;
enum {false, true};

/* Globals*/
list *ctxlist;

eventcount etime;
eventcount *ectail;
eventcount *last_ec; /* to work with context library */
context *current_context;
context *terminated_context;
context initctx;
context *curctx;
context *ctxhint;
jmp_buf main_context;
long long ecid; //id for each event count
count_t last_value;

#define newcode 1

//DESim user level functions
void desim_init(void);
eventcount *eventcount_create(char *name);
void context_create(void (*func)(void), unsigned stacksize, char *name);

void simulate(void);
void pause(count_t);
void await(eventcount *ec, count_t value);
void advance(eventcount *ec);
void advance_2(eventcount *ec);

void eventcount_init(eventcount * ec, count_t count, char *ecname);
void eventcount_destroy(eventcount *ec);
void context_init(context *new_context);
void context_stub(void);
void context_next(eventcount *ec, count_t value);
void context_remove_last(context *last_context);
void context_find_next(eventcount *ec, count_t value);
void context_find_next_2(eventcount *ec, count_t value);
int context_simulate(void);
void context_end(void);
context *context_select(void);
void context_switch(context *ctx);
void context_exit(void);
void context_cleanup(void);
void context_destroy(context *ctx);

//DESim util stuff
void warning(const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));
void fatal(const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));

#define LIST_FOR_EACH(list_ptr, iter) \
	for ((iter) = 0; (iter) < desim_list_count((list_ptr)); (iter)++)
#define INLIST(X) (((X) + list_ptr->size) % list_ptr->size)

list *desim_list_create(unsigned int size);
void desim_list_insert(list *list_ptr, int index, void *elem);
void *desim_list_get(list *list_ptr, int index);
int desim_list_count(list *list_ptr);
void desim_list_enqueue(list *list_ptr, void *elem);
void *desim_list_dequeue(list *list_ptr);
void desim_list_add(list *list_ptr, void *elem);
void desim_list_grow(list *list_ptr);
void *desim_list_remove_at(list *list_ptr, int index);
void *desim_list_remove(list *list_ptr, void *elem);
int desim_list_index_of(list *list_ptr, void *elem);
void desim_list_clear(list *list_ptr);
void desim_list_free(list *list_ptr);

#endif /*__DESim_H__*/
