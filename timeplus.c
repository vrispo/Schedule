/*
 * timeplus.c
 *
 *  Created on: 04 gen 2016
 *      Author: veronica
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <math.h>

#include "timeplus.h"

void time_copy(struct timespec *td, struct timespec ts)
{
	td->tv_sec = ts.tv_sec;
	td->tv_nsec = ts.tv_nsec;
}

void time_add_ms(struct timespec *t, int ms)
{
	t->tv_sec += ms/1000;
	t->tv_nsec += (ms%1000)*1000000;
	if(t->tv_nsec > 1000000000) {
		t->tv_nsec -= 1000000000;
		t->tv_sec += 1;
	}
}

int time_cmp(struct timespec t1, struct timespec t2)
{
	if(t1.tv_sec > t2.tv_sec)
		return 1;
	if(t1.tv_sec < t2.tv_sec)
		return -1;
	if(t1.tv_nsec > t2.tv_nsec)
		return 1;
	if(t1.tv_nsec < t2.tv_nsec)
		return -1;
	return 0;
}

void time_sub_ms(struct timespec t1, struct timespec t2, double* ms)
{
int	s,ns;

	s=((t1.tv_sec)-(t2.tv_sec));
	ns=((t1.tv_nsec)-(t2.tv_nsec));
	ns+=(s* 1000000000);

	*ms = fabs(ns * 0.000001);
}

void time_tms_to_ms(struct timespec t1, double* ms)
{
	int	s,ns;

		s=t1.tv_sec;
		ns=t1.tv_nsec;
		ns+=(s* 1000000000);

		*ms = fabs(ns * 0.000001);
}
