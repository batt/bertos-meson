/**
 * Recurrent task module.
 *
 * This module is a convenient method to handle multiple recurrent low priority
 * tasks. It creates a process with the default priority and schedules all the
 * tasks internally.
 * You can execute all the operations you want in each callback, since they
 * are executed in a different thread from the caller.
 *
 * Your callback may return true if you want the task to be scheduled
 * again, or false if you want the task to end.
 *
 * Interval time for each task should be fairly high (>20 ms) to avoid
 * blocking the whole CPU on this low priority job.
 *
 * \note rtask_add() may block.
 */
#ifndef RTASK_H
	#define RTASK

	#include <cfg/compiler.h>

struct RTask;
typedef bool (*rtask_cb_t)(void *user_data);

typedef struct RTask RTask;

struct RTask *rtask_add(rtask_cb_t cb, mtime_t interval, void *cb_data);

#endif // RTASK_H
