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
 * Copyright 2018 Develer S.r.l. (http://www.develer.com/)
 *
 * -->
 *
 * \brief STM32F2xx SCB definition.
 */

#ifndef STM32_SCB_H
#define STM32_SCB_H

#include <cfg/compiler.h>

#if CPU_CM3_STM32F1 | CPU_CM3_STM32F2

struct stm32_scb
{
	reg32_t CPUID; /*!< CPUID Base Register                              */
	reg32_t ICSR;  /*!< Interrupt Control and State Register             */
	reg32_t VTOR;  /*!< Vector Table Offset Register                     */
	reg32_t AIRCR; /*!< Application Interrupt and Reset Control Register */
	reg32_t SCR;   /*!< System Control Register                          */
	reg32_t CCR;   /*!< Configuration and Control Register               */
	reg32_t SHPR1; /*!< System Handler Priority Register 1               */
	reg32_t SHPR2; /*!< System Handler Priority Register 2               */
	reg32_t SHPR3; /*!< System Handler Priority Register 3               */
	reg32_t SHCRS; /*!< System Handler Control and State Register        */
	reg32_t CFSR;  /*!< UsageFault Status Register                       */
	reg32_t HFSR;  /*!< HardFault Status Register                        */
	reg32_t MMAR;  /*!< MemManage Fault Address Register                 */
	reg32_t BFAR;  /*!< BusFault Address Register                        */
	reg32_t AFSR;  /*!< Auxiliary Fault Status Register                  */
};

	#define SCB ((struct stm32_scb *)SCB_BASE)

	#define SCB_AIRCR_VECTKEY 0x05FA0000
	#define SCB_AIRCR_RESET   BV(2)

#else
	#error Unknown CPU
#endif

#endif /* STM32_SCB_H */
