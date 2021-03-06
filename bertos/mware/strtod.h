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
 * \defgroup strtod Optimized and simplified atof(), strtod() and strtof() implementations
 * \ingroup mware
 * \{
 *
 * \brief Optimized and simplified atof(), strtod(),strtof() implementations.
 *
 * These are faster than the standard ones supplied by libc, and do not require
 * any dynamic memory allocation (for instance, the implementations supplied by
 * newlib for ARM requires it).
 *
 * There are however a few drawbacks:
 *  \li It is not guaranteed that the number is converted to the nearest
 *      approximation of the decimal number supplied, there can be a few
 *      bits of approximation between these implementations and the standard
 *      compliant ones.
 *  \li Infinities and NANs are not handled.
 *  \li Overflows are not cheked.
 *
 * \author Francesco Sacchi <batt@develer.com>
 *
 * $WIZ$ module_name = "strtod"
 */
#ifndef MWARE_STRTOD_H
	#define MWARE_STRTOD_H

#endif    /* MWARE_STRTOD_H */
/** \} */ //mware
