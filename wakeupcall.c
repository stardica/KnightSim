/*
 * --- WAKEUPCALL.C ---
 *
 * Chris Holt <xris@ebola.stanford.edu>, 
 * 	based on code written by Steve Woo <swoo@gonzo.stanford.edu>
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "tasking.h"

#define Assert(a,b) do { if (!(a)) { fprintf (stderr, "Assertion failed, file %s, line %d\n", __FILE__, __LINE__); \
fprintf (stderr, "Assertion: " #a "\n"); fprintf (stderr, "ERR: " b "\n"); exit (4); } } while (0)

task* sorted_insert_task (task* target, task* list);
task* remove_task (task* target, task* list);

/* Local declarations */

typedef struct alarm {

	struct alarm *next; /* pointer to next alarm on same list */
	unsigned unused; /* pad to align time */
    long long time; /* time to trigger the alarm */
    eventcount *ec; /* pointer to eventcount to trigger */

} alarm;

typedef struct cell {

	struct cell *next; /* pointer to next cell on same list */
    unsigned unused; /* pad to align data */
    long long data;

} cell;

static task* Wake = NULL;
static alarm* Alarm_List = NULL;
static alarm* Alarm_Freelist = NULL;

static cell* sorted_insert_cell (cell* target, cell* list);
static cell* remove_cell (cell* target, cell* list);

static alarm* sorted_insert_alarm (alarm* target, alarm* list);


/* --- The order of the remaining calls is vital for inlining purposes --- */

cell *sorted_insert_cell (cell* target, cell* list){

   cell *back;
   cell *scan;
   long long data;

   data = target->data;
   scan = list;
   back = NULL;
   while ((scan != NULL) && (scan->data < data))
   {
      back = scan;
      scan = scan->next;
   }

   target->next = scan;

   if (back == NULL)
   {
      /* insert at the front */
      list = target;
   }
   else
   {
      /* insert at somewhere else in the list */
      back->next = target;
   }
   return list;
}

/* --- This function assumes the target is in the list --- */
cell *remove_cell (cell* target, cell* list){

   cell*	scan;
   cell* 	back;

   Assert(target != NULL, "Invalid arguments");
   scan = list;
   back = NULL;

   while ((scan != NULL) && (scan != target))
   {
      back = scan;
      scan = scan->next;
   }

   Assert(scan == target, "pointer missing from list???");

   if (back == NULL)
   {
      /* remove from the front */
      list = scan->next;
   }
   else
   {
      /* remove from somewhere else in the list */
      back->next = scan->next;
   }
   return list;
}


/* Because task, alarm, and cell have the same first three fields, */
/* You can simply cast task or alarm to a cell, and use it's functions. */
/* It's c++ without the cfront :) */

task *sorted_insert_task (task* target, task* list){

   return (task*) sorted_insert_cell((cell*) target, (cell*) list);
}

task *remove_task (task* target, task* list){

   return (task*) remove_cell((cell*) target, (cell*) list);
}

alarm *sorted_insert_alarm (alarm* target, alarm* list){

   return (alarm*) sorted_insert_cell((cell*) target, (cell*) list);
}

void future_advance (eventcount* ec, count_t wakeup){

   alarm* 	new_alarm;

   /*Construct the new alarm event*/
   //fprintf(logF, "In future advance, wakeup time %llu\n", wakeup);

#if 0
   advance(ec);
#endif

#if 1

   if (Alarm_Freelist == NULL)
   {
      new_alarm = (alarm*) malloc(sizeof(alarm));
   } 
   else
   {
      new_alarm = Alarm_Freelist;
      Alarm_Freelist = Alarm_Freelist->next;
   }

   new_alarm->ec = ec;
   new_alarm->time = wakeup;

    /*Insert it*/ 
   Alarm_List = sorted_insert_alarm(new_alarm, Alarm_List);

   if (Alarm_List == new_alarm)
   {
      etime.tasklist = remove_task(Wake, etime.tasklist);
      Wake->count = wakeup;
      etime.tasklist = sorted_insert_task(Wake, etime.tasklist);
   }
#endif

}

void initialize_wakeupcall (task* wakeup_task){

   Wake = wakeup_task;
}



//long long temp_time = 0;


/*void wakeupcall (void *arg){*/
void wakeupcall (void){

	long long next_wake_up_time;
	alarm *current_alarm;
	//const long long max_time = 0x7FFFFFFFFFFFFFFFLL;
	long long max_time = 0x7FFFFFFFFFFFFFFF;


	//Not associated with a procNum.
	Assert(Wake != NULL, "Forgot to initialize Wake...");
	next_wake_up_time = max_time;
	while (1)
	{
		await(&etime, next_wake_up_time);
		// FLDEBUG(('w', "Wakeupcall is awake at time %llu", P_TIME));

		if(!Alarm_List)
		{
			if (etime.count == max_time)
			{
				printf("DEADLOCK_DETECTED!\n");
				fflush(stdout);
				exit(1);
			}
			else
			{
				printf("FATAL ERROR: Alarm fired at etime %llu but list empty\n", etime.count);
				fflush(stdout);
				exit(1);
			}

			return;
		}


		//If we do actually wake up, there better be something on the list
		Assert(Alarm_List != NULL, "Wakeupcall without an alarm??");
		current_alarm = Alarm_List;
		Alarm_List = Alarm_List->next;

		// And it better be the right time
		Assert(etime.count >= current_alarm->time, "Woke up too early?");
		// FLDEBUG(('w', "Wakeupcall advancing count @0x%x (wakeup time %llu)",
		//current_alarm->ec, current_alarm->time >> 1));

		advance(current_alarm->ec);

		//Allow the current_alarm to be reused
      	current_alarm->next = Alarm_Freelist;
      	Alarm_Freelist = current_alarm;

      	//Sleep until next alarm
      	next_wake_up_time = (Alarm_List == NULL) ? max_time : Alarm_List->time;
      	// FLDEBUG(('w', "Next wakeupcall is at time %llu",next_wake_up_time >> 1));
      	if (!strcmp(current_alarm->ec->name, "sim_finish")) break;

      	//printf("task name %s @ time %lld\n", current_alarm->ec->name, etime.count);
      	//temp_time = etime.count;
   }

	return;
}
