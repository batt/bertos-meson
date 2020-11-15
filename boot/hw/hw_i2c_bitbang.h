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
 * Copyright 2003, 2004, 2006, 2008 Develer S.r.l. (http://www.develer.com/)
 * Copyright 2000 Bernie Innocenti <bernie@codewiz.org>
 * All Rights Reserved.
 * -->
 *
 * \brief Macro for I2C bitbang operation.
 *
 *
 *
 * \author Francesco Sacchi <batt@develer.com>
 */

#ifndef HW_I2C_BITBANG_H
#define HW_I2C_BITBANG_H

#define SDA_HI                                                    \
	do                                                            \
	{ /* Implement me:Set SDA High by setting SDA pin as input */ \
	} while (0)
#define SDA_LO                                                                   \
	do                                                                           \
	{ /* Implement me:Set SDA Low by setting SDA pin as open collector output */ \
	} while (0)
#define SCL_HI                                                    \
	do                                                            \
	{ /* Implement me:Set SCL High by setting SCL pin as input */ \
	} while (0)
#define SCL_LO                                                                   \
	do                                                                           \
	{ /* Implement me:Set SCL Low by setting SCL pin as open collector output */ \
	} while (0)

#define SCL_IN (true) /* Implement me: read SDA pin state */
#define SDA_IN (true) /* Implement me: read SCL pin state */

/**
 * This macro should set SDA and SCL lines as input.
 */
#define I2C_BITBANG_HW_INIT \
	do                      \
	{ /* Implement me! */   \
	} while (0)

/**
 * Half bit delay routine used to generate the correct timings.
 */
#define I2C_HALFBIT_DELAY() \
	do                      \
	{ /* Implement me! */   \
	} while (0)

/*
 * New api
 */
#include <cfg/compiler.h>
#include <drv/gpio_stm32.h>
#include <cfg/macros.h>
#include <drv/timer.h>

typedef struct I2cPins
{
	stm32_gpio *port;
	uint32_t clock_en;
	int scl, sda;
} I2cPins;

extern const I2cPins i2c_pins[];

#define I2C_CONF_LEN 2

#define GP_OUTPUT 1
#define GPIOI2C   (i2c_pins[dev].port)
#define SDA       (i2c_pins[dev].sda)
#define SCL       (i2c_pins[dev].scl)
#define SDA2      (i2c_pins[dev].sda * 2)
#define SCL2      (i2c_pins[dev].scl * 2)

INLINE void i2c_sdaHi(int dev)
{
	ASSERT(dev < I2C_CONF_LEN);

	/* Set SDA High by setting SDA pin as input */
	GPIOI2C->MODER &= ~(0x3 << SDA2);
}

INLINE void i2c_sdaLo(int dev)
{
	ASSERT(dev < I2C_CONF_LEN);

	/* Set SDA Low by setting SDA pin as open collector output */
	GPIOI2C->MODER |= GP_OUTPUT << SDA2;
}

INLINE void i2c_sclHi(int dev)
{
	ASSERT(dev < I2C_CONF_LEN);

	/* Set SDA High by setting SCL pin as input */
	GPIOI2C->MODER &= ~(0x3 << SCL2);
}

INLINE void i2c_sclLo(int dev)
{
	ASSERT(dev < I2C_CONF_LEN);

	/* Set SCL Low by setting SCL pin as open collector output */
	GPIOI2C->MODER |= GP_OUTPUT << SCL2;
}

INLINE bool i2c_sdaIn(int dev)
{
	ASSERT(dev < I2C_CONF_LEN);

	/* read SDA pin state */
	return stm32_gpioPinRead(GPIOI2C, BV(SDA));
}

INLINE bool i2c_sclIn(int dev)
{
	ASSERT(dev < I2C_CONF_LEN);

	/* read SCL pin state */
	return stm32_gpioPinRead(GPIOI2C, BV(SCL));
}

/**
 * Half bit delay routine used to generate the correct timings.
 */
INLINE void i2c_halfbitDelay(int dev)
{
	(void)(dev);
	timer_udelay(1);
	//cpu_relax();
}

/**
 * This macro should set SDA and SCL lines as input.
 */
INLINE void i2c_bitbangInit(int dev)
{
	ASSERT(dev < I2C_CONF_LEN);

	RCC->AHB1ENR |= i2c_pins[dev].clock_en;

	stm32_gpioPinConfig(GPIOI2C, BV(SDA) | BV(SCL), GPIO_MODE_IN_FLOATING, GPIO_SPEED_50MHZ);
	stm32_gpioPinWrite(GPIOI2C, BV(SDA), false);
	stm32_gpioPinWrite(GPIOI2C, BV(SCL), false);

	GPIOI2C->OTYPER |= BV(SDA);
	GPIOI2C->OTYPER |= BV(SCL);
}

#endif /* HW_I2C_BITBANG_H */
