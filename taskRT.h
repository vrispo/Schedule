/*
 * taskRT.h
 *
 *  Created on: 04 gen 2016
 *      Author: veronica
 */

#ifndef TASKRT_H_

struct task_par {
	int arg;
	long wcet;
	int period;
	int deadline;
	int priority;
	int dmiss;
	struct timespec at;
	struct timespec dl;
};

#define TASKRT_H_

void set_period(struct task_par *);
void wait_for_period(struct task_par *);
int deadline_miss(struct task_par *);

#endif /* TASKRT_H_ */
