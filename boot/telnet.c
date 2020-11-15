#include "telnet.h"
#include <lwip/sockets.h> // socket, sockaddr_in, bind, listen
#include <string.h>       // memset
#include <drv/timer.h>

#define LOG_LEVEL  LOG_LVL_INFO
#define LOG_FORMAT LOG_FMT_TERSE
#include <cfg/log.h>

static int sock;
static struct sockaddr_in server_addr;
static char message[] = "666 Firmware error\n";
static int in_sock;

#define COMMAND_PORT 20000

void NORETURN telnet_entry(void)
{
	sock = lwip_socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1)
		LOG_ERR("Failed to open telnet socket\n");

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(COMMAND_PORT);
	server_addr.sin_addr.s_addr = INADDR_ANY;
	if (lwip_bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
		LOG_ERR("Error binding port %hd\n", COMMAND_PORT);

	if (lwip_listen(sock, 0) == -1)
		LOG_ERR("Error listening socket\n");

	while (1)
	{
		in_sock = lwip_accept(sock, NULL, NULL);
		ssize_t sz = lwip_send(in_sock, message, sizeof(message), 0);
		if (sz < 0)
			LOG_ERR("Error sending message.\n");
		lwip_close(in_sock);
	}
}
