#ifndef HEARTBEAT_H
#define HEARTBEAT_H

#include <cfg/compiler.h>

#define HEARTBEAT_PORT 23000

void heartbeat_init(void);
void NORETURN heartbeat_proc(void);

#endif // HEARTBEAT_H
