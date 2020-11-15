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
 * Copyright 2010 Develer S.r.l. (http://www.develer.com/)
 *
 * -->
 *
 * \brief Cortex-M3 IRQ management.
 *
 * \author Andrea Righi <arighi@develer.com>
 */

#include "irq_cm3.h"

#include <cfg/debug.h> /* ASSERT() */
#include <cfg/log.h>   /* LOG_ERR() */
#include <cpu/irq.h>

static void (*irq_table[NUM_INTERRUPTS])(void)
    __attribute__((section("vtable")));

/* Priority register / IRQ number table */
static const uint32_t nvic_prio_reg[] =
    {
        /* System exception registers */
        0, NVIC_SYS_PRI1, NVIC_SYS_PRI2, NVIC_SYS_PRI3,

        /* External interrupts registers */
        NVIC_PRI0, NVIC_PRI1, NVIC_PRI2, NVIC_PRI3,
        NVIC_PRI4, NVIC_PRI5, NVIC_PRI6, NVIC_PRI7,
        NVIC_PRI8, NVIC_PRI9, NVIC_PRI10, NVIC_PRI11,
        NVIC_PRI12, NVIC_PRI13,
#if CPU_CM3_STM32F2
        NVIC_PRI14, NVIC_PRI15,
        NVIC_PRI16, NVIC_PRI17, NVIC_PRI18, NVIC_PRI19,
        NVIC_PRI20
#endif
};
STATIC_ASSERT(countof(nvic_prio_reg) > (NUM_INTERRUPTS >> 2) + (NUM_INTERRUPTS % 4 ? 1 : 0));

/* Unhandled IRQ */
static NAKED void unhandled_isr(void)
{
	register uint32_t reg;

	asm volatile("mrs %0, ipsr"
	             : "=r"(reg));
	LOG_ERR("unhandled IRQ %lu\n", reg);
	RESET;
}

static NAKED void hard_fault_isr(void)
{
	register reg32_t *args;

	__asm volatile(
	    "tst lr, #4\t\n" /* Check EXC_RETURN[bit 2] */
	    "ite eq\t\n"
	    "mrseq %0, msp\t\n"
	    "mrsne %0, psp\t\n"
	    : "=r"(args)
	    : /* no input */
	    : /* clobber */
	);

	LOG_ERR("[Hard fault handler - dumping relevant registers]\n");
	LOG_ERR("R0    = 0x%08lx\n", args[0]);
	LOG_ERR("R1    = 0x%08lx\n", args[1]);
	LOG_ERR("R2    = 0x%08lx\n", args[2]);
	LOG_ERR("R3    = 0x%08lx\n", args[3]);
	LOG_ERR("R12   = 0x%08lx\n", args[4]);
	LOG_ERR("LR    = 0x%08lx\n", args[5]);
	LOG_ERR("PC    = 0x%08lx\n", args[6]);
	LOG_ERR("PSR   = 0x%08lx\n", args[7]);
	LOG_ERR("BFAR  = 0x%08lx\n", SCB->BFAR);
	LOG_ERR("CFSR  = 0x%08lx\n", SCB->CFSR);
	LOG_ERR("HFSR  = 0x%08lx\n", SCB->HFSR);
	LOG_ERR("AFSR  = 0x%08lx\n", SCB->AFSR);
	LOG_ERR("SHCRS = 0x%08lx\n", SCB->SHCRS);

	RESET;
}

void sysirq_setPriority(sysirq_t irq, int prio)
{
	uint32_t pos = (irq & 3) * 8;
	reg32_t reg = nvic_prio_reg[irq >> 2];
	uint32_t val;

	val = HWREG(reg);
	val &= ~(0xff << pos);
	val |= prio << pos;
	HWREG(reg) = val;
}

static void sysirq_enable(sysirq_t irq)
{
	/* Enable the IRQ line (only for generic IRQs) */
	if (irq >= 16 && irq < 48)
		NVIC_EN0_R = 1 << (irq - 16);
	else if (irq >= 48)
		NVIC_EN1_R = 1 << (irq - 48);
}

void sysirq_setHandler(sysirq_t irq, sysirq_handler_t handler)
{
	cpu_flags_t flags;

	ASSERT(irq < NUM_INTERRUPTS);

	IRQ_SAVE_DISABLE(flags);
	irq_table[irq] = handler;
	sysirq_setPriority(irq, IRQ_PRIO);
	sysirq_enable(irq);
	IRQ_RESTORE(flags);
}

void sysirq_freeHandler(sysirq_t irq)
{
	cpu_flags_t flags;

	ASSERT(irq < NUM_INTERRUPTS);

	IRQ_SAVE_DISABLE(flags);
	irq_table[irq] = unhandled_isr;
	IRQ_RESTORE(flags);
}

void sysirq_init(void)
{
	cpu_flags_t flags;
	int i;

	IRQ_SAVE_DISABLE(flags);
	for (i = 0; i < NUM_INTERRUPTS; i++)
		irq_table[i] = unhandled_isr;

	/* custom hard fault ISR to print stack trace */
	irq_table[FAULT_HARD] = (sysirq_handler_t)hard_fault_isr;

	/* Update NVIC to point to the new vector table */
	NVIC_VTABLE_R = (size_t)irq_table;
	IRQ_RESTORE(flags);
}
