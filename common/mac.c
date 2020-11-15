#include "mac.h"

#define LOG_LEVEL LOG_LVL_INFO
#include <cfg/log.h>

#include <string.h> // strncpy
#include <stdlib.h> // strtol
#include <drv/eth.h>

static const MacAddress invalid_macs[] =
    {
        {{0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
        {{0x44, 0x44, 0x44, 0x44, 0x44, 0x44}},
};

static bool mac_isValid(const MacAddress *mac)
{
	/* First byte must not have multicast bit set */
	if (eth_addrIsMcast(mac))
	{
		LOG_ERR("First byte of MAC address has multicast bit set\n");
		return false;
	}

	for (unsigned i = 0; i < countof(invalid_macs); i++)
	{
		if (eth_addrCmp(mac, &invalid_macs[i]))
		{
			LOG_ERR("MAC address in invalid list\n");
			return false;
		}
	}

	return true;
}

bool mac_decode(const char *str, MacAddress *mac)
{
	char work[MAC_ADDR_STR_LEN + 1];
	char *endptr = NULL;
	long conv;

	size_t size = strnlen(str, MAC_ADDR_STR_LEN + 1);

	if (size < MAC_ADDR_STR_LEN)
	{
		LOG_ERR("Value passed too short\n");
		return false;
	}

	strncpy(work, str, MAC_ADDR_STR_LEN + 1);

	work[MAC_ADDR_STR_LEN] = '\0';

	int m = 0;
	for (int i = 0; i < MAC_ADDR_STR_LEN; i += 3)
	{
		/* Check for correct separator until eol */
		if ((i + 2) < MAC_ADDR_STR_LEN && work[i + 2] != ':' && work[i + 2] != '-')
		{
			LOG_ERR("Wrong MAC address separator '%c' at byte%d\n\n", work[i + 2], m);
			return false;
		}
		else
			work[i + 2] = '\0';

		conv = strtol(&work[i], &endptr, 16);
		if (*endptr != '\0')
		{
			LOG_ERR("Error converting byte%d of MAC address, [%s]\n", m, &work[i]);
			return false;
		}

		mac->addr[m++] = conv;
	}
	return mac_isValid(mac);
}
