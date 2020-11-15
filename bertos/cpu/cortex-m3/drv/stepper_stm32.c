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
 * \brief Stepper driver interface implementation.
 *
 * This module use the three timer on the at91 family, to generate a
 * six periodic variable pwm waveform. The pulse width is fix, and could
 * change by setting the STEPPER_DELAY_ON_COMPARE_C define, but you make
 * an attention to do this, becouse the pulse width is not exactly 
 * STEPPER_DELAY_ON_COMPARE_C. The pulse width depend also to latency 
 * time of cpu to serve an interrupt, this generate an pwm waveform affect
 * to noise. This noise not effect the period but only the pulse width,
 * becouse the raising edge is generate by hardware comply with the our
 * period settings.
 *
 * Note: is most important to set STEPPER_DELAY_ON_COMPARE_C value minor
 * than a interrupt time service, becouse the falling edge must be happen
 * inside to inerrupt service to guarantee a correct functionaly of pwm
 * generator.
 * 
 *
 *
 * \author Francesco Sacchi <batt@develer.com>
 */

#include "stepper_stm32.h"

#include "cfg/cfg_stepper.h"
#include <cfg/macros.h>
#include <cfg/debug.h>
#include <cfg/seq.h>

#include <cpu/types.h>
#include <cpu/irq.h>
#include <drv/irq_cm3.h>

#include <io/cm3.h>

#define LOG_LEVEL  LOG_LVL_INFO
#define LOG_FORMAT LOG_FMT_TERSE
#include <cfg/log.h>

#define DEFINE_STEPPER_ISR_1(_irq_num)        \
	static DECLARE_ISR(stepper_irq##_irq_num) \
	{                                         \
		stepper_irq(_irq_num);                \
	}

#define FWDDCL_STEPPER_ISR_1(_irq_num) \
	static void stepper_irq##_irq_num(void);

#define DEFINE_STEPPER_ISR(n) \
	FOR_N(DEFINE_STEPPER_ISR_1, n)

#define FWDDCL_STEPPER_ISR(n) \
	FOR_N(FWDDCL_STEPPER_ISR_1, n)

FWDDCL_STEPPER_ISR(CONFIG_TC_STEPPER_MAX_NUM);

///< Static array of timer counter struct for stepper.
static struct TimerCounter stepper_timers[] =
    {
        {
            //Timer Counter settings for TIMER9
            .timer = TIM9,
            .clk_reg = &RCC->APB2ENR,
            .clk_mask = RCC_APB2ENR_TIM9EN,
            .irq_id = TIM1_BRK_IRQHANDLER, //Line shared between TIM1 e TIM9
            .isr = &stepper_irq0,
        },
};

STATIC_ASSERT(countof(stepper_timers) == CONFIG_TC_STEPPER_MAX_NUM);

INLINE void stepper_irq(int irq_num)
{
	STEPPER_STROBE_ON;
	struct TimerCounter *timer = &stepper_timers[irq_num];

	if (timer->do_pulse)
	{
		STEPPER_SET_DRIVERSTEP(timer->motor->index, timer->pulse_idx);
		timer->pulse_idx += hw_stepper_dir[timer->motor->index];
		timer->pulse_idx %= STEPPER_MAX_DRIVERSTEP;
	}

	timer->callback(timer->motor);
	/* Ack IRQ */
	timer->timer->SR = 0;

	STEPPER_STROBE_OFF;
}

DEFINE_STEPPER_ISR(CONFIG_TC_STEPPER_MAX_NUM);

/**
 * Timer couter setup.
 *
 * This function apply to select timer couter all needed settings.
 * Every settings are stored in stepper_timers[].
 */
void stepper_tc_setup(int index, stepper_isr_t callback, struct Stepper *motor)
{

	ASSERT(index < CONFIG_TC_STEPPER_MAX_NUM);

	motor->timer = &stepper_timers[index];

	motor->timer->timer->DIER = 0;
	motor->timer->timer->CR1 = 0;
	motor->timer->timer->CR2 = 0;
	motor->timer->timer->EGR = TIM_EGR_CC1G;
	motor->timer->timer->CCMR[0].VAL = TIM_CCMR1_CC1S_OUT;
	motor->timer->timer->CCER = 0;
	motor->timer->timer->CNT = 0;
	motor->timer->timer->CCR[0] = 0;
	motor->timer->timer->ARR = 0xFFFF;

	// Timer clock is feed from APB2 which is CPUFREQ /2
	motor->timer->timer->PSC = (STEPPER_PRESCALER / 2) - 1;

	// Set the callback
	sysirq_setHandler(motor->timer->irq_id, motor->timer->isr);

	// Disable interrupt on select timer counter
	stepper_tc_irq_disable(motor->timer);

	//Register callback
	motor->timer->callback = callback;
	motor->timer->motor = motor;

	//Start timer
	motor->timer->timer->CR1 = TIM_CR1_CEN;
}

/**
 * Timer counter init.
 */
void stepper_tc_init(void)
{
	STEPPER_STROBE_INIT;

	STATIC_ASSERT(CONFIG_NUM_STEPPER_MOTORS <= CONFIG_TC_STEPPER_MAX_NUM);

	/*
	 * Enable timer counter:
	 * - power on all timer counter
	 * - disable all interrupt
	 */
	for (int i = 0; i < CONFIG_TC_STEPPER_MAX_NUM; i++)
	{
		*stepper_timers[i].clk_reg |= stepper_timers[i].clk_mask;
		stepper_timers[i].timer->SR = 0;
		stepper_timers[i].timer->DIER = 0;
		stepper_timers[i].pulse_idx = 0;
	}
}
