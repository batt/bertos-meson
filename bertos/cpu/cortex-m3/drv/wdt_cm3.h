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
 * Copyright 2012 Develer S.r.l. (http://www.develer.com/)
 *
 * -->
 *
 * \brief Watchdog interface for Cortex-M3 architecture.
 *
 *
 * \author Mattia Barbon <mattia@develer.com>
 *
 */
#ifndef DRV_WDT_CM3_H
#define DRV_WDT_CM3_H

#include <cfg/compiler.h> // INLINE

#if CPU_CM3_STM32F2 /* should work for STM32F1 as well */
	#include <io/cm3.h>

struct stm32_iwdg
{
	reg32_t KR;
	reg32_t PR;
	reg32_t RLR;
	reg32_t SR;
};

	#define IWDG ((struct stm32_iwdg *)IWDG_BASE)

	#define IWDG_RESET        0xAAAA
	#define IWDG_ENABLE_WRITE 0x5555
	#define IWDG_START        0xCCCC

	#define IWDG_PR_32 0x3

/* timeout is in msecs */
INLINE void wdt_start(uint32_t timeout)
{
	ASSERT(timeout < 4000); /* values greater than 4000 require a different prescaler */

	/* set prescaler to /32 (1Khz timer) */
	IWDG->KR = IWDG_ENABLE_WRITE;
	IWDG->PR = IWDG_PR_32;

	/* set counter */
	IWDG->KR = IWDG_ENABLE_WRITE;
	IWDG->RLR = timeout;

	/* start */
	IWDG->KR = IWDG_START;
}

/* on STM32Fx, watchdog can't be stopped */

INLINE void wdt_reset(void)
{
	IWDG->KR = IWDG_RESET;
}
#else
	#error "Missing watchdog support for yout CPU"
#endif

#endif //DRV_WDT_CM3_H
