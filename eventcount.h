
#ifndef __eventcount_H__
#define __eventcount_H__

#include "tasking.h"

struct eventcount_s {
	char * name;		/* string name of eventcount */
	long long id;
	task * tasklist;	/* list of tasks waiting on this event */
	count_t count;		/* current value of event */
	struct eventcount_s* eclist; /* pointer to next eventcount */
};

/* Global time counter */
extern eventcount etime;
extern eventcount *ectail;
extern eventcount *last_ec; /* to work with context library */

void etime_init(void);

eventcount *eventcount_create (char *name);
void eventcount_init(eventcount * ec, count_t count, char *ecname);
void advance(eventcount *ec);			/* increment eventcount */
void eventcount_destroy(eventcount *ec);

#endif /*__eventcount_H__*/
