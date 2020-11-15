/**
 * \brief Telnet protocol implementation
 *
 * Commands are implemented and registered in the module that needs them (eg.
 * shutter, mirror...).
 *
 * In output, the first parameter is reserved for human readable string, if
 * applicable, otherwise it must be NULL.
 */
#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <mware/parser.h>
#include <io/kfile.h>

bool protocol_init(void);
void protocol_initParser(void);
void protocol_poll(void);
void protocol_reply(int code, const char *msg);
int protocol_printf(const char *format, ...);
void protocol_close(void);

#endif // PROTOCOL_H
