#ifndef STEPPER_STM32_H
#define STEPPER_STM32_H

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
 * Copyright 2008 Develer S.r.l. (http://www.develer.com/)
 * All Rights Reserved.
 * -->
 *
 * \brief Stepper hardware-specific definitions
 *
 *
 * \author Francesco Sacchi <batt@develer.com>
 */

#include <cfg/compiler.h>
#include <cfg/macros.h>
#include <drv/stepper.h>
#include <io/cm3.h>

/**
 * IRQ callback function type definition.
 */
typedef void (*irq_t)(void);

/**
 * Timer contex structure.
 */
typedef struct TimerCounter
{
	struct stm32_tim *timer;
	reg32_t *clk_reg;
	uint32_t clk_mask;
	int irq_id;
	irq_t isr;
	bool do_pulse;
	unsigned pulse_idx;
	stepper_isr_t callback; ///< Interrupt callback pointer
	struct Stepper *motor;  ///< Stepper context structure
} TimerCounter;

/**
 * Enable interrupt for timer counter compare event.
 */
INLINE void stepper_tc_irq_enable(struct TimerCounter *timer)
{
	timer->timer->DIER = TIM_DIER_CC1IEN;
}

/**
 * Disable interrupt for timer counter compare event.
 */
INLINE void stepper_tc_irq_disable(struct TimerCounter *timer)
{
	timer->timer->DIER = 0;
}

/**
 * Set delay for next interrupt compare event.
 */
INLINE void stepper_tc_setDelay(struct TimerCounter *timer, stepper_time_t delay)
{
	timer->timer->CCR[0] += delay;
}

/**
 * Set delay for next interrupt compare event.
 */
INLINE void stepper_tc_resetTimer(struct TimerCounter *timer)
{
	timer->timer->DIER = 0;
	timer->timer->CCR[0] = 0;
}

/**
 * Programm timer counter to generate a pulse on select TIO output.
 */
INLINE void FAST_FUNC stepper_tc_doPulse(struct TimerCounter *timer)
{
	timer->do_pulse = true;
}

/**
 * Programm timer counter to not generate a pulse on select TIO output.
 */
INLINE void FAST_FUNC stepper_tc_skipPulse(struct TimerCounter *timer)
{
	timer->do_pulse = false;
}

void stepper_tc_setup(int index, stepper_isr_t callback, struct Stepper *motor);
void stepper_tc_init(void);

#endif
