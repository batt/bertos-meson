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
 * Copyright 2003, 2004, 2006, 2008, 2009 Develer S.r.l. (http://www.develer.com/)
 * Copyright 2000 Bernie Innocenti <bernie@codewiz.org>
 * All Rights Reserved.
 * -->
 *
 * \brief Macro for HW_SIPO_H
 *
 *
 *
 * \author Andrea Grandi <andrea@develer.com>
 * \author Daniele Basile <asterix@develer.com>
 */

#ifndef HW_SIPO_H
#define HW_SIPO_H

#include <cfg/macros.h>

#include <io/stm32.h>

#include <drv/gpio_stm32.h>
#include <drv/clock_stm32.h>
#include <drv/timer.h>

/**
 * Map sipo connection on board.
 */
typedef enum SipoMap
{
	SIPO_CNT
} SipoMap;

#define SI_PIN     BV(5) // PB5
#define CLK_PIN    BV(5) // PA5
#define LOAD_PIN   BV(4) // PA4
#define ENABLE_PIN BV(6) // PA6

/**
 * Define the procedure to drive serial input in sipo device (SI).
 */
#define SIPO_SI_HIGH() stm32_gpioPinWrite((struct stm32_gpio *)GPIOB_BASE, SI_PIN, 1)
#define SIPO_SI_LOW()  stm32_gpioPinWrite((struct stm32_gpio *)GPIOB_BASE, SI_PIN, 0)

/**
 * Drive clock to shift SI data into latch.
 */
#define SIPO_SI_CLOCK(clk_pol)                                           \
	do                                                                   \
	{                                                                    \
		(void)clk_pol;                                                   \
		stm32_gpioPinWrite((struct stm32_gpio *)GPIOA_BASE, CLK_PIN, 1); \
		NOP;                                                             \
		NOP;                                                             \
		NOP;                                                             \
		NOP;                                                             \
		stm32_gpioPinWrite((struct stm32_gpio *)GPIOA_BASE, CLK_PIN, 0); \
	} while (0)

/**
 * Do everything needed in order to load dato into sipo.
 */
#define SIPO_LOAD(device, load_pol)                                       \
	do                                                                    \
	{                                                                     \
		(void)device;                                                     \
		(void)load_pol;                                                   \
		stm32_gpioPinWrite((struct stm32_gpio *)GPIOA_BASE, LOAD_PIN, 1); \
		NOP;                                                              \
		NOP;                                                              \
		NOP;                                                              \
		NOP;                                                              \
		stm32_gpioPinWrite((struct stm32_gpio *)GPIOA_BASE, LOAD_PIN, 0); \
	} while (0)

/**
 * Enable the shift register output.
 */
#define SIPO_ENABLE() stm32_gpioPinWrite((struct stm32_gpio *)GPIOA_BASE, ENABLE_PIN, 0)

/**
 * Set polarity for Load, Clk, SI signals.
 */
#define SIPO_SET_LD_LEVEL(device, load_pol)                               \
	do                                                                    \
	{                                                                     \
		(void)device;                                                     \
		(void)load_pol;                                                   \
		stm32_gpioPinWrite((struct stm32_gpio *)GPIOA_BASE, LOAD_PIN, 0); \
	} while (0)
#define SIPO_SET_CLK_LEVEL(clock_pol)                                    \
	do                                                                   \
	{                                                                    \
		(void)clock_pol;                                                 \
		stm32_gpioPinWrite((struct stm32_gpio *)GPIOA_BASE, CLK_PIN, 0); \
	} while (0)
#define SIPO_SET_SI_LEVEL() \
	do                      \
	{                       \
		SIPO_SI_LOW();      \
	} while (0)

#define GPIO_BASE
/**
 * Do anything that needed to init sipo pins.
 */
#define SIPO_INIT_PIN()                                                                                                            \
	do                                                                                                                             \
	{                                                                                                                              \
		RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;                                                                                       \
		RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;                                                                                       \
		stm32_gpioPinWrite((struct stm32_gpio *)GPIOA_BASE, CLK_PIN | LOAD_PIN | ENABLE_PIN, 1);                                   \
		stm32_gpioPinWrite((struct stm32_gpio *)GPIOB_BASE, SI_PIN, 1);                                                            \
		stm32_gpioPinConfig((struct stm32_gpio *)GPIOA_BASE, CLK_PIN | LOAD_PIN | ENABLE_PIN, GPIO_MODE_OUT_PP, GPIO_SPEED_50MHZ); \
		stm32_gpioPinConfig((struct stm32_gpio *)GPIOB_BASE, SI_PIN, GPIO_MODE_OUT_PP, GPIO_SPEED_50MHZ);                          \
	} while (0)

#endif /* HW_SIPO_H */
