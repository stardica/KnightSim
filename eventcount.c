/*
 * eventcount.c
 *
 *  Created on: Aug 3, 2017
 *      Author: stardica
 */


#include <assert.h>

#include "desim.h"
#include "eventcount.h"
#include "contexts.h"
#include "tasking.h"

eventcount etime;
eventcount *ectail = NULL;
eventcount *last_ec = NULL; /* to work with context library */


void etime_init(void){

	etime.name = NULL;
	etime.id = 0;
	etime.count = 0;
	etime.tasklist = NULL;
	etime.tlist = list_create();
	etime.eclist = NULL;

	return;
}

eventcount *eventcount_create(char *name){

	eventcount *ec = NULL;

	ec = (eventcount *)malloc(sizeof(eventcount));
	assert(ec);

	eventcount_init(ec, 0, name);

	return ec;
}



void eventcount_init(eventcount * ec, count_t count, char *ecname){

	ec->name = ecname;
	ec->id = ecid++;
	ec->tasklist = NULL;
	ec->count = count;
	ec->eclist = NULL;


	//create singly linked list
    if (ectail == NULL)
    {
    	//always points to first event count created.
    	etime.eclist = ec;
    }
    else
    {
    	//old tail!!
    	ectail->eclist = ec;
    }

    //move to new tail!
    ectail = ec;

    return;
}

/* advance(ec) -- increment an event count. */
/* Don't use on time event count! */
void advance(eventcount *ec){

	task *ptr, *pptr;

	/* advance count */
	ec->count++;

	/* check for no tasks being enabled */
	ptr = ec->tasklist;

	if ((ptr == NULL) || (ptr->count > ec->count))
		return;

	/* advance down task list */
	do
	{
		/* set tasks time equal to current time */
		ptr->count = etime.count;
		pptr = ptr;
		ptr = ptr->tasklist;
	} while (ptr && (ptr->count == ec->count));

	/* add list of events to etime */
	pptr->tasklist = etime.tasklist;
	etime.tasklist = ec->tasklist;
	ec->tasklist = ptr;

	return;
}



void eventcount_destroy(eventcount *ec){

	eventcount	*front;
	eventcount	*back;
	unsigned	found = 0;

	assert(ec != NULL);

	// --- This function should unlink the eventcount from the etime eclist ---
	assert(etime.eclist != NULL);

	front = etime.eclist;
	if (ec == front)
	{
		etime.eclist = ec->eclist;
		free ((void*)ec);
		if (ectail == ec)
		{
			ectail = NULL;
		}
	}
	else
	{
	   back = etime.eclist;
	   front = front->eclist;
	   while ((front != NULL) && (!found))
	   {
		   if (ec == front)
		   {
			   back->eclist = ec->eclist;
			   free ((void*)ec);
			   found = 1;
		   }
		   else
		   {
			   back = back->eclist;
			   front = front->eclist;
		   }
	   }

	   assert(found == 1);
	   if (ectail == ec)
	   {
		   ectail = back;
	   }
	}
}
