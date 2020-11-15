#include "protocol.h"
#include "cfg/cfg_parser.h"
#include <common/dli.h>
#include <common/config.h>
#include <mware/formatwr.h>
#include <verstag.h>

#include <lwip/sockets.h>
#include <string.h> // memset

#define LOG_LEVEL  LOG_LVL_WARN
#define LOG_FORMAT LOG_FMT_TERSE
#include <cfg/log.h>

#define COMMAND_PORT 20000

static char readbuf[128];
static int server_sock;
static int connection_sock;

void protocol_close(void)
{
	lwip_close(connection_sock);
}

void protocol_initParser(void)
{
	parser_init();
	LOG_INFO("Protocol initialized\n");
}

struct __sn_state
{
	char *str;
	size_t len;
};

/* Print to a string and convert '\n' in '\r\n' */
static void __sn_put_char(char c, void *ptr)
{
	struct __sn_state *state = (struct __sn_state *)ptr;
	if (state->len && c == '\n')
	{
		--state->len;
		*state->str++ = '\r';
	}
	if (state->len)
	{
		--state->len;
		*state->str++ = c;
	}
}

/* This function is needed because we have to convert all '\n' to '\r\n' */
static int string_nprintf(char *str, size_t size, const char *fmt, va_list ap)
{
	struct __sn_state state;
	state.str = str;
	state.len = size;

	/* Make room for traling '\0'. */
	if (size--)
	{
		state.len = size;
		if (str)
		{
			_formatted_write(fmt, __sn_put_char, &state, ap);

			/* Terminate string. */
			*state.str = '\0';
		}
		else
			ASSERT(0);
	}

	/* Return the actual lenght of the string including the additional '\r' */
	return size - state.len;
}

static char linebuf[128];

int protocol_printf(const char *format, ...)
{
	va_list ap;
	int len;

	va_start(ap, format);
	len = string_nprintf(linebuf, sizeof(linebuf), format, ap);
	va_end(ap);
	lwip_send(connection_sock, linebuf, len, 0);

	return len;
}

void protocol_reply(int code, const char *msg)
{
	if (msg == NULL)
	{
		if (code == RC_OK)
			msg = "Command ok";
		else
			msg = "Error in executing command.";
	}
	if (code != RC_OK)
		LOG_ERR("%s\n", msg);
	protocol_printf("%d %s\n", code, msg);
}

static void protocol_parse(const char *buf)
{
	const struct CmdTemplate *templ;

	// Command check
	templ = parser_get_cmd_template(buf);
	if (!templ)
	{
		protocol_printf("-1 Unknown command.\n\n");
		return;
	}
	LOG_INFO("Got the command \"%s\"\n", templ->name);

	parms args[CONFIG_PARSER_MAX_ARGS];

	// Args Check.
	if (!parser_get_cmd_arguments(buf, templ, args))
	{
		protocol_printf("-2 Invalid arguments.\n\n");
		LOG_ERR("Invalid arguments\n");
		return;
	}

	LOG_INFO("Executing command...\n");
	/* Reserve the first parameter for human readable string */
	args[0].str.p = NULL;
	parser_execute_cmd(templ, args);
	protocol_printf("\n");
}

/*
 * Receive a string from \a sockfd and execute the command.
 * \return false if the connection is closed or on errors, true otherwise
 */
static bool protocol_run(int sockfd)
{
	int i = 0;

	do
	{
		char c;
		ssize_t rd = lwip_recv(sockfd, &c, 1, 0);

		// if rd == 0, the other end has performed an orderly shutdown
		// if rd < 0, socket was broken
		if (rd <= 0)
		{
			if (rd == 0)
				LOG_INFO("Peer closed the connection\n");
			else
				LOG_ERR("Socket error, %d\n", errno);
			return false;
		}

		if (c == '\r')
			continue;
		else if (c == '\n')
		{
			readbuf[i] = '\0';
			break;
		}
		else
			readbuf[i] = c;

		if (++i == countof(readbuf))
		{
			i = 0;
			LOG_ERR("Line too long, buffer overrun\n");
		}
	} while (1);

	LOG_INFO("Received a string of len [%zd] content [%s]\n", i, readbuf);

	// check message minimum length
	if (readbuf[0])
	{
		/* If we enter lines beginning with sharp(#)
		they are stripped out from commands */
		if (readbuf[0] != '#')
		{
			protocol_parse(readbuf);
		}
	}
	return true;
}

/*
 * Create and setup a socket for protocol operations.
 *
 * \return false on errors, true if the server is listening correctly.
 */
bool protocol_init(void)
{
	server_sock = -1;
	struct sockaddr_in addr;
	server_sock = lwip_socket(AF_INET, SOCK_STREAM, 0);
	if (server_sock < 0)
	{
		LOG_ERR("Error opening socket\n");
		return false;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(COMMAND_PORT);
	addr.sin_addr.s_addr = INADDR_ANY;
	if (lwip_bind(server_sock, (struct sockaddr *)&addr, sizeof(addr)) == -1)
	{
		LOG_ERR("Error binding port %hd\n", COMMAND_PORT);
		goto error;
	}

	if (lwip_listen(server_sock, 0) == -1)
	{
		LOG_ERR("Error listening socket\n");
		goto error;
	}

	LOG_INFO("Listening protocol socket created\n");
	return true;

error:
	lwip_close(server_sock);
	return false;
}

void protocol_poll(void)
{
	connection_sock = lwip_accept(server_sock, NULL, NULL);
	if (connection_sock == -1)
	{
		LOG_WARN("Error accepting connection\n");
		return;
	}

// Seconds after we start to send keep-alive pings
#define KEEPALIVE_IDLE 5

// Seconds between two consecutive pings
#define KEEPALIVE_INTERVAL 1

// If we loose more than this counter replies, the socket will be closed
#define KEEPALIVE_COUNTER 5

	// With the above configuration, the timeout will be between 5 and 10 seconds

	/* Configure keepalive timeouts */
	int opt = KEEPALIVE_IDLE;
	lwip_setsockopt(connection_sock, IPPROTO_TCP, TCP_KEEPIDLE, &opt, sizeof(opt));
	opt = KEEPALIVE_INTERVAL;
	lwip_setsockopt(connection_sock, IPPROTO_TCP, TCP_KEEPINTVL, &opt, sizeof(opt));
	opt = KEEPALIVE_COUNTER;
	lwip_setsockopt(connection_sock, IPPROTO_TCP, TCP_KEEPCNT, &opt, sizeof(opt));
	opt = 1;
	lwip_setsockopt(connection_sock, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt));

	LOG_WARN("Accepted telnet connection on socket %d\n", connection_sock);
	protocol_printf("Hello from %s v%s!\n", APP_NAME, VERS_TAG);

	while (protocol_run(connection_sock))
		;

	lwip_close(connection_sock);
	LOG_WARN("Closing telnet socket %d\n", connection_sock);
}
