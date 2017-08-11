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

