#include "rtask.h"
#include "cfg/cfg_rtask.h"

#include <drv/timer.h>
#include <struct/pool.h>
#include <struct/list.h>
#include <kern/proc.h>
#include <kern/signal.h>
#include <kern/sem.h>
#include <cfg/module.h> // MOD_CHECK

#define LOG_LEVEL  RTASK_LOG_LEVEL
#define LOG_FORMAT RTASK_LOG_FORMAT
#include <cfg/log.h>

#define NEW_TASK SIG_USER0

// TODO: Mixing static and dynamic tests in kernel must be tested with care,
// until then use this workaround
#if CONFIG_KERN_HEAP
	#define PROC_NEW() proc_new(rtask_proc, NULL, CONFIG_RTASK_STACK, NULL)
#else
PROC_DEFINE_STACK(rtask_stack, CONFIG_RTASK_STACK);
	#define PROC_NEW() proc_new(rtask_proc, NULL, sizeof(rtask_stack), rtask_stack)
#endif

struct RTask
{
	Timer t;
	rtask_cb_t callback;
	void *user_data;
};

DECLARE_POOL_STATIC(rtask_pool, RTask, CONFIG_RTASK_POOL_SIZE);
static Process *process = NULL;
static List rt_list;
static Semaphore rtask_sem;
#define RTASK_ATOMIC(code)       \
	do                           \
	{                            \
		sem_obtain(&rtask_sem);  \
		code;                    \
		sem_release(&rtask_sem); \
	} while (0)

static NORETURN void rtask_proc(void)
{
	while (1)
	{
		bool empty;
		RTASK_ATOMIC(
		    empty = LIST_EMPTY(&rt_list);
		    sig_check(NEW_TASK););
		if (empty)
			sig_wait(NEW_TASK);

		ticks_t delay;
		RTASK_ATOMIC(delay = synctimer_nextTimeout(&rt_list));
		timer_delayTicks(delay);

		/* Executes 1 callback */
		Timer *timer;
		RTASK_ATOMIC(
		    timer = (Timer *)LIST_HEAD(&rt_list);
		    ASSERT(timer);
		    /* Retreat the expired timer */
		    REMOVE(&timer->link););

		DB(timer->magic = TIMER_MAGIC_INACTIVE;);
		ASSERT(timer_clock() - timer->tick >= 0);

		/* Execute the associated event */
		event_do(&timer->expire);
	}
}

static void rtask_trampoline(void *_rtask)
{
	RTask *rtask = _rtask;
	if (rtask->callback(rtask->user_data))
		RTASK_ATOMIC(synctimer_readd(&rtask->t, &rt_list));
	else
		RTASK_ATOMIC(pool_free(&rtask_pool, rtask));
}

RTask *rtask_add(rtask_cb_t cb, mtime_t delay, void *cb_data)
{
	// Beware: this function is called from a different process
	// than rtask_proc, so each access to rtask_pool and rt_list
	// must be protected with a semaphore.

	// The semaphore is not yet initialized, disable preemption
	// altogether.
	proc_forbid();
	if (UNLIKELY(process == NULL))
	{
		MOD_CHECK(proc);

		LIST_INIT(&rt_list);
		pool_init(rtask_pool, NULL);
		sem_init(&rtask_sem);
		process = PROC_NEW();
		ASSERT(process);
	}
	proc_permit();

	RTask *rt = NULL;
	RTASK_ATOMIC(rt = (RTask *)pool_alloc(&rtask_pool));
	if (rt)
	{
		rt->callback = cb;
		rt->user_data = cb_data;
		timer_setSoftint(&rt->t, rtask_trampoline, rt);
		timer_setDelay(&rt->t, delay);
		RTASK_ATOMIC(synctimer_add(&rt->t, &rt_list));
		sig_send(process, NEW_TASK);
	}
	else
		LOG_ERR("Failed to allocate RTask\n");
	return rt;
}
