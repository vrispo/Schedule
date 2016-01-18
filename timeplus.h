/*
 * timeplus.h
 *
 *  Created on: 04 gen 2016
 *      Author: veronica
 */

#ifndef TIMEPLUS_H_

#define TIMEPLUS_H_

void time_copy(struct timespec *td, struct timespec ts);
void time_add_ms(struct timespec *t, int ms);
int time_cmp(struct timespec t1, struct timespec t2);
void time_sub_ms(struct timespec t1, struct timespec t2, double* ms);

#endif /* TIMEPLUS_H_ */
