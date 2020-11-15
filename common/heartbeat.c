#include "heartbeat.h"
#include "hw/hw_eth.h"
#include "state.h"
#include "../common/version.h"

#include <drv/timer.h>
#include <kern/rtask.h>
#include <lwip/sockets.h>
#include <lwip/netif.h> // netif_default
#include <drv/eth.h>
#include <verstag.h>
#include <stdio.h>

#define LOG_LEVEL  LOG_LVL_INFO
#define LOG_FORMAT LOG_FMT_TERSE
#include <cfg/log.h>

#define HEARTBEAT_INTERVAL 5000
#define HEARTBEAT_TIMEOUT  HEARTBEAT_INTERVAL

static struct sockaddr_in sa;
static int sock;

static char message[60];

static bool heartbeat_task(UNUSED_ARG(void *, user_data))
{
	MacAddress mac;

	mac = eth_mac();
	snprintf(message, sizeof(message),
	         "%s %02X:%02X:%02X:%02X:%02X:%02X %s %s",
	         ip4addr_ntoa(&netif_default->ip_addr),
	         mac.addr[0], mac.addr[1], mac.addr[2], mac.addr[3], mac.addr[4],
	         mac.addr[5], HW_TYPE, vers_tag);

	lwip_sendto(sock, message, sizeof(message), 0, (struct sockaddr *)&sa,
	            sizeof(struct sockaddr_in));
	return true;
}

void heartbeat_init(void)
{
	sock = lwip_socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == -1)
		LOG_ERR("Failed to open heartbeat socket\n");
	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(HEARTBEAT_PORT);
	sa.sin_addr.s_addr = INADDR_BROADCAST;

	RTask *rt = rtask_add(heartbeat_task, HEARTBEAT_INTERVAL, NULL);
	ASSERT(rt);
}

void NORETURN heartbeat_proc(void)
{
	MacAddress mac;
	sock = lwip_socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == -1)
		LOG_ERR("Failed to open heartbeat socket\n");
	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(HEARTBEAT_PORT);
	sa.sin_addr.s_addr = INADDR_BROADCAST;

	while (1)
	{
		mac = eth_mac();
		/* Set serial number */
		snprintf(message, sizeof(message),
		         "%s %02X:%02X:%02X:%02X:%02X:%02X %s %s",
		         ip4addr_ntoa(&netif_default->ip_addr),
		         mac.addr[0], mac.addr[1], mac.addr[2], mac.addr[3], mac.addr[4],
		         mac.addr[5], HW_TYPE, vers_tag);

		lwip_sendto(sock, message, sizeof(message), 0, (struct sockaddr *)&sa,
		            sizeof(struct sockaddr_in));
		timer_delay(HEARTBEAT_TIMEOUT);
	}
}
