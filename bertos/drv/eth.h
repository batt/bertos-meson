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
 * \author Andrea Righi <arighi@develer.com>
 *
 * \brief Ethernet standard descriptors
 *
 * $WIZ$ module_name = "eth"
 * $WIZ$ module_configuration = "bertos/cfg/cfg_eth.h"
 * $WIZ$ module_supports = "at91sam7x or sam3x"
 */

#ifndef DRV_ETH_H
#define DRV_ETH_H

#include "hw/hw_eth.h"
#include <cpu/types.h>
#include <cfg/debug.h>

#define ETH_ADDR_LEN  6
#define ETH_HEAD_LEN  14
#define ETH_DATA_LEN  1500
#define ETH_FRAME_LEN (ETH_HEAD_LEN + ETH_DATA_LEN)

#define ETH_TYPE_IP 0x0800

/**
 * A MAC address.
 */
typedef struct MacAddress
{
	uint8_t addr[6]; ///< address bytes.
} MacAddress;

/**
 * Determine if ethernet address \a mac is a all zero.
 */
INLINE int eth_addrIsZero(const MacAddress *mac)
{
	return !(mac->addr[0] | mac->addr[1] | mac->addr[2] |
	         mac->addr[3] | mac->addr[4] | mac->addr[5]);
}

/**
 * Determine if ethernet address \a mac is a multicast address.
 */
INLINE int eth_addrIsMcast(const MacAddress *mac)
{
	return (0x01 & mac->addr[0]);
}

/**
 * Determine if ethernet address \a mac is locally-assigned (IEEE 802).
 */
INLINE int eth_addrIsLocal(const MacAddress *mac)
{
	return (0x02 & mac->addr[0]);
}

/**
 * Determine if ethernet address \a mac is broadcast.
 */
INLINE bool eth_addrIsBcast(const MacAddress *mac)
{
	return (mac->addr[0] & mac->addr[1] & mac->addr[2] &
	        mac->addr[3] & mac->addr[4] & mac->addr[5]) == 0xff;
}

/**
 * Check if the ethernet address \a mac is not all zero, is not a multicast
 * address, and is not broadcast.
 */
INLINE bool eth_addrIsValid(const MacAddress *mac)
{
	return !eth_addrIsMcast(mac) && !eth_addrIsZero(mac);
}

/**
 * Compare two ethernet addresses: \a mac1 and \a mac2, returns true if equal.
 */
INLINE bool eth_addrCmp(const MacAddress *mac1, const MacAddress *mac2)
{
	return !((mac1->addr[0] ^ mac2->addr[0]) |
	         (mac1->addr[1] ^ mac2->addr[1]) |
	         (mac1->addr[2] ^ mac2->addr[2]) |
	         (mac1->addr[3] ^ mac2->addr[3]) |
	         (mac1->addr[4] ^ mac2->addr[4]) |
	         (mac1->addr[5] ^ mac2->addr[5]));
}

typedef struct EthCfg
{
	DB(id_t _type;) // Used to keep track, at runtime, of the class type.
} EthCfg;

int eth_init(const EthCfg *);
void eth_cleanup(void);

typedef struct EthernetDescriptor
{
	void *data;
	size_t len;
} EthernetDescriptor;

void eth_send(const struct EthernetDescriptor *list, size_t count);
size_t eth_recv(struct EthernetDescriptor *list, size_t count);

void eth_setMac(MacAddress mac);
MacAddress eth_mac(void);

#endif /* DRV_ETH_H */
