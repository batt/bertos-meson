#ifndef MAC_H
#define MAC_H

#include <cfg/compiler.h>
#include <drv/eth.h>

#define MAC_DEFAULT      "00:11:22:33:44:55"
#define MAC_ADDR_STR_LEN 17

/**
 * Decode a MAC address string to an array
 * \param str the string containing the mac address.
 *            The format must be: 00:11:22:33:44:55 or 00-11-22-33-44-55
 * \param mac the MAC address where to save the decoded MAC address.
 * \return true if all is ok, false on convertion errors or if decoded MAC is
 *              not valid.
 * \note str \a mac can be thrashed on errors.
 */
bool mac_decode(const char *str, MacAddress *mac);

#endif
