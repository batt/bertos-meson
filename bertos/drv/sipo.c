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
 *
 * \brief SIPO Module
 *
 * The SIPO module transforms a serial input in a parallel output. Please check hw_sipo.h
 * file to customize hardware related parameters.
 *
 * \author Andrea Grandi <andrea@develer.com>
 * \author Daniele Basile <asterix@develer.com>
 */

#include "sipo.h"

#include "hw/hw_sipo.h"
#include "cfg/cfg_sipo.h"

#include <cfg/compiler.h>
#include <cfg/log.h>

#include <io/kfile.h>
#include <struct/fifo.h>

#include <string.h>

#define SIPO_DATAORDER_START(order)        (order ? SIPO_DATAORDER_START_LSB : SIPO_DATAORDER_START_MSB)
#define SIPO_DATAORDER_SHIFT(shift, order) (order ? ((shift) <<= 1) : ((shift) >>= 1))

/**
 * Write a char in sipo shift register
 */
INLINE uint8_t sipo_putchar(int bus, uint8_t c, uint8_t bit_order, uint8_t clock_pol)
{
	uint8_t shift = SIPO_DATAORDER_START(bit_order);
	uint8_t recv = 0;

	for (int i = 0; i < 8; i++)
	{
		if ((c & shift) == 0)
			SIPO_SI_LOW(bus);
		else
			SIPO_SI_HIGH(bus);

		int bit = SIPO_SI_CLOCK(bus, clock_pol);
		if (bit)
			recv |= shift;

		SIPO_DATAORDER_SHIFT(shift, bit_order);
	}
	return recv;
}

#if CONFIG_SIPO_RECV_BUF_SIZE != 0

static int sipo_error(struct KFile *_fd)
{
	Sipo *fd = SIPO_CAST(_fd);
	return fd->status;
}

static int sipo_flush(struct KFile *_fd)
{
	Sipo *fd = SIPO_CAST(_fd);
	FIFO_FLUSH(&fd->fifo);
	return 0;
}

static void sipo_clearerr(struct KFile *_fd)
{
	Sipo *fd = SIPO_CAST(_fd);
	fd->status = 0;
}

/**
 * Read a buffer from the sipo fifo.
 */
static size_t sipo_read(struct KFile *_fd, void *_buf, size_t size)
{
	uint8_t *buf = (uint8_t *)_buf;
	Sipo *fd = SIPO_CAST(_fd);

	while (!FIFO_ISEMPTY(&fd->fifo) && size)
	{
		*buf++ = FIFO_POP(&fd->fifo);
		size--;
	}

	return buf - (uint8_t *)_buf;
}

#endif

/**
 * Write a buffer into the sipo register and, when finished, give a load pulse.
 */
static size_t sipo_write(struct KFile *_fd, const void *_buf, size_t size)
{
	const uint8_t *buf = (const uint8_t *)_buf;
	Sipo *fd = SIPO_CAST(_fd);
	size_t write_len = size;

	ASSERT(buf);

	SIPO_SET_SI_LEVEL(fd->bus);
	SIPO_SET_CLK_LEVEL(fd->bus, fd->clock_pol);
	SIPO_SET_LD_LEVEL(fd->bus, fd->load_device, fd->load_pol);

	// Load into the shift register all the buffer bytes
	while (size--)
	{
		uint8_t c = sipo_putchar(fd->bus, *buf++, fd->bit_order, fd->clock_pol);
#if CONFIG_SIPO_RECV_BUF_SIZE != 0
		if (!FIFO_ISFULL(&fd->fifo))
			FIFO_PUSH(&fd->fifo, c);
		else
			fd->status |= SIPO_RXFIFOOVERRUN;
#else
		(void)c;
#endif
	}

	// We finsh to load bytes, so load it.
	SIPO_LOAD(fd->bus, fd->load_device, fd->load_pol);

	return write_len;
}

/**
 * Initialize the SIPO
 */
void sipo_init(Sipo *fd, int bus)
{
	ASSERT(fd);
	ASSERT(bus < SIPO_MAX_BUS);

	memset(fd, 0, sizeof(Sipo));
	kfile_init(&fd->fd);

	//Set kfile struct type as a generic kfile structure.
	DB(fd->fd._type = KFT_SIPO);

	fd->bus = bus;

	// Set up SIPO writing functions.
	fd->fd.write = sipo_write;

#if CONFIG_SIPO_RECV_BUF_SIZE != 0
	FIFO_INIT(&fd->fifo);
	fd->fd.read = sipo_read;
	fd->fd.error = sipo_error;
	fd->fd.clearerr = sipo_clearerr;
	fd->fd.flush = sipo_flush;
#endif

	SIPO_INIT_PIN(bus);

	/* Enable sipo output */
	SIPO_ENABLE(bus);
}
