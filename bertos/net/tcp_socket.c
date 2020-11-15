#include "tcp_socket.h"

#define LOG_LEVEL  LOG_LVL_WARN
#define LOG_FORMAT LOG_FMT_VERBOSE
#include <cfg/log.h>
#include <cpu/byteorder.h>

static int tcpConnect(const struct sockaddr_in *addr, OpenMode mode)
{
	int sock = -1;

	if ((sock = lwip_socket(AF_INET, SOCK_STREAM, 0)) >= 0)
	{
		if (mode & WriteOnly)
		{
			int len = 0;
			lwip_setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &len, sizeof(len));
		}

		if (lwip_connect(sock, (const struct sockaddr *)addr, sizeof(struct sockaddr_in)) != 0)
		{
			LOG_ERR("Connection error\n");
			lwip_close(sock);
			sock = -1;
		}
	}
	else
		LOG_ERR("Cannot create socket\n");

	return sock;
}

static bool reconnect(TcpSocket *socket)
{
	LOG_INFO("Reconnecting...\n");
	// Release old socket if needed
	if (socket->sock >= 0)
	{
		if (lwip_close(socket->sock) != 0)
			LOG_ERR("Error closing socket\n");
		socket->sock = -1;
	}

	// Connect to our peer peer
	if ((socket->sock = tcpConnect(&socket->addr, socket->mode)) < 0)
	{
		LOG_ERR("Reconnect error!\n");
		socket->error = TCP_NOT_CONN;
		return false;
	}

	LOG_INFO("Reconnecting DONE!\n");

	return true;
}

static int tcpsocket_close(KFile *fd)
{
	TcpSocket *socket = TCPSOCKET_CAST(fd);
	int ret = lwip_close(socket->sock);
	socket->sock = -1;
	if (ret)
	{
		LOG_ERR("Close error\n");
		return EOF;
	}
	return 0;
}

static size_t tcpsocket_read(KFile *fd, void *buf, size_t len)
{
	TcpSocket *socket = TCPSOCKET_CAST(fd);
	ssize_t recvd = 0;

	// Try reconnecting if our socket isn't valid
	if (socket->sock < 0)
	{
		if (!reconnect(socket))
			return 0;
	}

	while (len)
	{
		ssize_t rd = lwip_recv(socket->sock, (uint8_t *)buf + recvd, len, 0);

		if (rd < 0)
		{
			socket->error = errno;
			return recvd;
		}

		// Do we have an EOF condition? If so, bailout.
		if (rd == 0 && len != 0)
		{
			LOG_INFO("Connection reset by peer\n");
			socket->error = 0;
			if (tcpsocket_close(fd) == EOF)
				LOG_ERR("Error closing socket, leak detected\n");
			return recvd;
		}
		recvd += rd;
		len -= rd;
	}

	return recvd;
}

static int tcpsocket_error(KFile *fd)
{
	TcpSocket *socket = TCPSOCKET_CAST(fd);
	return socket->error;
}

static void tcpsocket_clearerr(KFile *fd)
{
	TcpSocket *socket = TCPSOCKET_CAST(fd);
	socket->error = 0;
}

static size_t tcpsocket_write(KFile *fd, const void *buf, size_t len)
{
	TcpSocket *socket = TCPSOCKET_CAST(fd);
	ssize_t result;

	// Try reconnecting if our socket isn't valid
	if (socket->sock < 0)
	{
		if (!reconnect(socket))
			return 0;
	}

	result = lwip_send(socket->sock, buf, len, 0);

	LOG_INFO("Result of lwip_send(): %zd, errno %d\n", result, errno);

	if (result < 0)
	{
		LOG_ERR("errno %d, result %d\n", errno, result);
		if (errno == ECONNRESET)
		{
			LOG_INFO("Connection reset\n");
			socket->error = ECONNRESET;
			if (tcpsocket_close(fd) == EOF)
				LOG_ERR("Error closing socket, leak detected\n");
			return 0;
		}
		socket->error = errno;
		return 0;
	}

	return (size_t)result;
}

void tcpsocket_init(TcpSocket *socket, const char *address, uint16_t port, OpenMode mode)
{
	socket->sock = -1;
	socket->fd._type = KFT_TCPSOCKET;
	socket->fd.read = tcpsocket_read;
	socket->fd.error = tcpsocket_error;
	socket->fd.close = tcpsocket_close;
	socket->fd.write = tcpsocket_write;
	socket->fd.clearerr = tcpsocket_clearerr;

	socket->addr.sin_family = AF_INET;
	socket->addr.sin_port = host_to_net16(port);
	socket->mode = mode;
	inet_aton(address, &socket->addr.sin_addr);
}
