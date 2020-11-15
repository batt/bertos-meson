/**
 * \file
 * <!--
 * This file is part of BeRTOS.
 *
 * Bertos is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * As a special exception, you may use this file as part of a free software
 * library without restriction.  Specifically, if other files instantiate
 * templates or use macros or inline functions from this file, or you compile
 * this file and link it with other files to produce an executable, this
 * file does not by itself cause the resulting executable to be covered by
 * the GNU General Public License.  This exception does not however
 * invalidate any other reasons why the executable file might be covered by
 * the GNU General Public License.
 *
 * Copyright 2009 Develer S.r.l. (http://www.develer.com/)
 *
 * -->
 *
 * \author Luca Ottaviano <lottaviano@develer.com>
 *
 * \brief Emulation Layer for lwIP
 */

#ifndef LWIP_SYS_ARCH_H
#define LWIP_SYS_ARCH_H

#include <arch/cc.h>
#include <lwip/err.h>

#include <kern/sem.h>
#include <kern/msg.h>
#include <kern/proc.h>

/****************************************************************************/

/*
 * Generic mutex (binary semaphore) prototypes
 *
 * TODO: move this to a different place (i.e., bertos/kern/sem.h).
 */
#include <cpu/byteorder.h> // cpu_atomic_xchg()
#include <cpu/types.h>

#include <struct/list.h>
#include <drv/timer.h>

#define MUTEX_UNLOCKED	1
#define MUTEX_LOCKED	(!MUTEX_UNLOCKED)

typedef struct Mutex
{
	List wait_queue;
	cpu_atomic_t count;
} Mutex;

void mutex_init(struct Mutex *s);
bool mutex_attempt(struct Mutex *s);
void mutex_obtain(struct Mutex *s);
void mutex_release(struct Mutex *s);

typedef Mutex sys_mutex_t;

INLINE err_t sys_mutex_new(sys_mutex_t *mutex)
{
	mutex_init(mutex);
	return ERR_OK;
}

INLINE void	sys_mutex_lock(sys_mutex_t *mutex)
{
	mutex_obtain(mutex);
}

INLINE void sys_mutex_unlock(sys_mutex_t *mutex)
{
	mutex_release(mutex);
}

INLINE void	sys_mutex_free(sys_mutex_t *mutex)
{
	(void)mutex;
}

INLINE int sys_mutex_valid(sys_mutex_t *mutex)
{
	
	return (mutex->wait_queue.head.succ == NULL &&
			mutex->wait_queue.head.pred == NULL &&
			mutex->wait_queue.tail.succ == NULL &&
			mutex->wait_queue.tail.pred == NULL)
		? 0 : 1;
}

INLINE void sys_mutex_set_invalid(sys_mutex_t *mutex)
{
	mutex->wait_queue.head.succ = NULL;
	mutex->wait_queue.head.pred = NULL;
	mutex->wait_queue.tail.succ = NULL;
	mutex->wait_queue.tail.pred = NULL;
}


/****************************************************************************/

typedef Mutex sys_sem_t;
typedef MsgPort sys_mbox_t;
typedef struct Process *sys_thread_t;
typedef cpu_flags_t sys_prot_t;

#define SYS_MBOX_NULL NULL
#define SYS_SEM_NULL  NULL


EXTERN_C_BEGIN

void sys_init(void);

err_t sys_sem_new(sys_sem_t *sem, u8_t count);
void sys_sem_free(sys_sem_t *sem);
void sys_sem_signal(sys_sem_t *sem);
u32_t sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout);

INLINE int sys_sem_valid(sys_sem_t *sem)
{
	return (sem->wait_queue.head.succ == NULL &&
	sem->wait_queue.head.pred == NULL &&
	sem->wait_queue.tail.succ == NULL &&
	sem->wait_queue.tail.pred == NULL)
	 ? 0 : 1;
}

INLINE void sys_sem_set_invalid(sys_sem_t *sem)
{
	sem->wait_queue.head.succ = NULL;
	sem->wait_queue.head.pred = NULL;
	sem->wait_queue.tail.succ = NULL;
	sem->wait_queue.tail.pred = NULL;
}

err_t sys_mbox_new(sys_mbox_t *, int);
void sys_mbox_free(sys_mbox_t *mbox);
void sys_mbox_post(sys_mbox_t *mbox, void *msg);
u32_t sys_arch_mbox_fetch(sys_mbox_t *mbox, void **msg, u32_t timeout);
u32_t sys_arch_mbox_tryfetch (sys_mbox_t *mbox, void **msg);

INLINE int sys_mbox_valid(sys_mbox_t *mbox)
{
	return (mbox->queue.head.succ == NULL &&
	mbox->queue.head.pred == NULL &&
	mbox->queue.tail.succ == NULL &&
	mbox->queue.tail.pred == NULL)
	 ? 0 : 1;
}

INLINE void sys_mbox_set_invalid(sys_mbox_t *mbox)
{
	mbox->queue.head.succ = NULL;
	mbox->queue.head.pred = NULL;
	mbox->queue.tail.succ = NULL;
	mbox->queue.tail.pred = NULL;
}

#define sys_msleep(ms)	timer_delay(ms)

INLINE u32_t sys_now (void)
{
	return ticks_to_ms(timer_clock());
}

EXTERN_C_END

#endif
