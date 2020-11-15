#ifndef SYSTEM_H
#define SYSTEM_H

#include "common/eth_cfg.h"

#include <cfg/compiler.h>
#include <cpu/irq.h>

typedef void (*reload_func_t)(void);

void system_init(void);
bool system_syslogAddrOk(void);
const char *system_syslogAddr(void);
void system_setReloadCb(reload_func_t cb);
void system_reload(void);
int system_errors(void);
const char *system_defaultIP(void);

typedef struct IpCfg
{
	int id;
	const char ip[24];
	const Stm32EthCfg *eth_cfg;
} IpCfg;

const IpCfg *system_ethConfig(void);

#endif //SYSTEM_H
