#ifndef TCP_SOCKET_H
#define TCP_SOCKET_H

#include <io/kfile.h>
#include <lwip/sockets.h>

#define TCP_NOT_CONN -1000

typedef enum OpenMode
{
	ReadOnly = 0x1,
	WriteOnly = 0x2,
	ReadWrite = ReadOnly | WriteOnly,
} OpenMode;

typedef struct TcpSocket
{
	KFile fd;
	int sock;
	struct sockaddr_in addr;
	OpenMode mode;
	int error;
} TcpSocket;

#define KFT_TCPSOCKET MAKE_ID('T', 'S', 'C', 'K')

INLINE TcpSocket *TCPSOCKET_CAST(KFile *fd)
{
	ASSERT(fd->_type == KFT_TCPSOCKET);
	return (TcpSocket *)fd;
}

void tcpsocket_init(TcpSocket *socket, const char *address, uint16_t port, OpenMode mode);

#endif // TCP_SOCKET_H
