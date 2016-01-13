/*
 * taskRT.c
 *
 *  Created on: 04 gen 2016
 *      Author: veronica
 */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include "timeplus.h"

#include "taskRT.h"



void set_period(struct task_par *tp)
{
struct timespec	t;

	clock_gettime(CLOCK_MONOTONIC, &t);
	time_copy(&(tp->at), t);
	time_copy(&(tp->dl), t);
	time_add_ms(&(tp->at), tp->period);
	time_add_ms(&(tp->dl), tp->deadline);
}

void wait_for_period(struct task_par *tp)
{
	clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &(tp->at), NULL);
	time_add_ms(&(tp->at), tp->period);
	time_add_ms(&(tp->dl), tp->period);
}

int deadline_miss(struct task_par *tp)
{
struct timespec	now;

	clock_gettime(CLOCK_MONOTONIC, &now);
	if(time_cmp(now, tp->dl) > 0) {
		tp->dmiss++;
		return 1;
	}
	return 0;
}

