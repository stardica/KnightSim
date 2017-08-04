
#ifndef __eventcount_H__
#define __eventcount_H__

#include "desim.h"
#include "list.h"

struct eventcount_s {
	char * name;		/* string name of event count */
	long long id;
	task *tasklist;	/* list of tasks waiting on this event */
	list *tlist;
	count_t count;		/* current value of event */

	struct eventcount_s* eclist; /* pointer to next event count */
};


/* Global time counter */
extern eventcount etime;
extern eventcount *ectail;
extern eventcount *last_ec; /* to work with context library */

void etime_init(void);
eventcount *eventcount_create (char *name);
void eventcount_init(eventcount * ec, count_t count, char *ecname);
void advance(eventcount *ec);
void eventcount_destroy(eventcount *ec);

#endif /*__eventcount_H__*/
