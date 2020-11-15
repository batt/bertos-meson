#include <cfg/log.h>

#include <lwip/sockets.h>
#include <cpu/byteorder.h> // host_to_net16
#include <lwip/inet.h>
#include <cfg/debug.h>

#include "common/system.h"

#include <stdarg.h>
#include <stdio.h>

static struct sockaddr_in sa;
static int sock = -1;
uint32_t log_cnt = 0;

static char log_message[CONFIG_LOG_BUFSIZE];

static void log_init(void)
{
	if (system_syslogAddrOk())
	{
		const char *de_addr = system_syslogAddr();
		sock = lwip_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		ASSERT2(sock != -1, "Failed to open log socket");
		memset(&sa, 0, sizeof(sa));
		sa.sin_family = AF_INET;
		sa.sin_port = host_to_net16((int16_t)CONFIG_LOG_PORT);
		sa.sin_addr.s_addr = inet_addr(de_addr);
	}
}

STATIC_ASSERT(CONFIG_LOG_NET || CONFIG_LOG_SERIAL);

int log_printf(const char *fmt, ...)
{
	va_list ap;
	int len;
	va_start(ap, fmt);

#if CONFIG_LOG_SERIAL
	kvprintf(fmt, ap);
#endif

	len = vsnprintf(log_message, sizeof(log_message), fmt, ap);
	va_end(ap);
	log_message[sizeof(log_message) - 1] = 0;

	if (!IRQ_RUNNING())
	{
		if (sock == -1)
			log_init();

		if (sock != -1)
		{
			lwip_sendto(sock, log_message, len, 0, (struct sockaddr *)&sa, sizeof(struct sockaddr_in));
			log_cnt++;
		}
	}

	return len;
}
