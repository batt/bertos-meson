#include "system.h"
#include "common/config.h"
#include "common/dli.h"
#include "hw/hw_eth.h"
#include "common/mac.h"
#include "common/ip.h"
#include "common/state.h"
#include "common/protocol.h"
#include "../common/version.h"
#include "common/board_id.h"
#include "common/eth_cfg.h"

#include <drv/timer.h>
#include <drv/eth.h>
#include "drv/eth_stm32.h"
#include <mware/parser.h>
#include <verstag.h>

#include <string.h> // strnlen

#include <netif/ethernetif.h>

#include <lwip/ip.h>
#include <lwip/ip_addr.h>
#include <lwip/netif.h>
#include <lwip/tcpip.h>
#include <lwip/dhcp.h>
#include <lwip/sockets.h>

#define LOG_LEVEL LOG_LVL_INFO
#include <cfg/log.h>

#define IP4_ADDR_LEN (15 + 1)
static char syslog_addr[IP4_ADDR_LEN];
static char ip_addr[IP4_ADDR_LEN];
reload_func_t system_reload_func;

static SetPRetVals setMac(const struct ConfigEntry *e, char *val, bool);

DECLARE_CONF(mac_cfg, NULL,
             (VAR, char, a, 0, SERIALNO_KEY, MAC_DEFAULT, {.p = NULL}, {.p = NULL}, {.p = NULL}, {.p = NULL}, setMac));

static SetPRetVals setMac(const struct ConfigEntry *e, char *val, bool use_default)
{
	SetPRetVals ret = SRV_OK;
	MacAddress mac;
	LOG_INFO("MAC address [%.17s]\n", val);

	size_t size = strlen(val);

	if (size > MAC_ADDR_STR_LEN)
	{
		LOG_INFO("Value too long, clamping\n");
		val[MAC_ADDR_STR_LEN] = '\0';
		ret = SRV_OK_CLAMPED;
	}

	if (!mac_decode(val, &mac))
		goto error;

	eth_setMac(mac);
	return ret;

error:
	if (use_default)
	{
		LOG_INFO("Error parsing MAC address, using default [%s]\n", e->default_val);
		mac_decode(e->default_val, &mac);
		eth_setMac(mac);
	}
	return SRV_CONV_ERR;
}

MAKE_CMD(reset, "", "",
         ({
	         (void)args;
	         protocol_reply(RC_OK, "Bye bye!");
	         protocol_close();
#if CPU_ARM
	         RSTC_CR = RSTC_KEY | BV(RSTC_EXTRST) | BV(RSTC_PERRST) | BV(RSTC_PROCRST);
#else
	#define SCB_AIRCR         ((reg32_t *)(SCB_BASE + 0xC))
	#define SCB_AIRCR_VECTKEY 0x05FA0000
	#define SCB_AIRCR_RESET   BV(2)

	         *SCB_AIRCR = SCB_AIRCR_VECTKEY | (*SCB_AIRCR & 0xffff) | SCB_AIRCR_RESET;
#endif
	         RC_OK;
         }),
         0);

MAKE_CMD(quit, "", "",
         ({
	         (void)args;
	         protocol_reply(RC_OK, "Closing connection.");
	         protocol_close();
	         RC_OK;
         }),
         0);

MAKE_CMD(ping, "", "",
         ({
	         (void)args;
	         protocol_reply(RC_OK, "Pong!");
	         RC_OK;
         }),
         0);

MAKE_CMD(ver, "", "s",
         ({
	         (void)args;
	         protocol_reply(RC_OK, NULL);
	         protocol_printf("%s\n", vers_tag);
	         RC_OK;
         }),
         0);

MAKE_CMD(hw_ver, "", "d",
         ({
	         (void)args;
	         protocol_reply(RC_OK, NULL);
	         protocol_printf("%d\n", hw_version());
	         RC_OK;
         }),
         0);

MAKE_CMD(board_id, "", "ds",
         ({
	         (void)args;
	         protocol_reply(RC_OK, NULL);
	         protocol_printf("%d\n%s\n", board_id(), board_name());
	         RC_OK;
         }),
         0);

MAKE_CMD(set_state, "d", "", ({
	         ResultCode ret = RC_OK;
	         if (args[1].l == ST_SERVICE || args[1].l == ST_IDLE)
	         {
		         st_setState(args[1].l);
		         protocol_reply(RC_OK, NULL);
	         }
	         else
	         {
		         protocol_reply(RC_ERROR, "Invalid state.");
		         ret = RC_ERROR;
	         }
	         ret;
         }),
         0);

MAKE_CMD(get_state, "", "d", ({
	         (void)args;
	         protocol_printf("%d %s\n%d\n", RC_OK, st_name(), st_state());
	         RC_OK;
         }),
         0);

MAKE_CMD(get_errors, "", "d", ({
	         (void)args;
	         uint32_t err = system_errors();
	         if (err == 0)
		         protocol_reply(RC_OK, "All OK!");
	         else
		         protocol_reply(RC_OK, "Errors found!");
	         protocol_printf("%d\n", err);
	         RC_OK;
         }),
         0);

MAKE_CMD(set_syslog_addr, "s", "",
         ({
	         LOG_INFO("Syslog address: %.16s\n", args[1].str.p);
	         if (strncmp(syslog_addr, args[1].str.p, IP4_ADDR_LEN) != 0)
	         {
		         memcpy(syslog_addr, args[1].str.p, IP4_ADDR_LEN);
		         dli_set("syslog_addr", syslog_addr);
	         }
	         protocol_reply(RC_OK, NULL);
	         system_reload();
	         RC_OK;
         }),
         0);

MAKE_CMD(get_syslog_addr, "", "s",
         ({
	         (void)args;
	         int ret = RC_OK;
	         if (strnlen(syslog_addr, IP4_ADDR_LEN))
	         {
		         protocol_reply(RC_OK, NULL);
		         protocol_printf("%s\n", syslog_addr);
	         }
	         else
	         {
		         protocol_reply(RC_ERROR, "Syslog address not set");
		         ret = RC_ERROR;
	         }
	         ret;
         }),
         0);

bool system_syslogAddrOk(void)
{
	return strnlen(syslog_addr, IP4_ADDR_LEN) > 0;
}

const char *system_syslogAddr(void)
{
	char *ret = NULL;
	if (strlen(syslog_addr) > 0)
		ret = syslog_addr;
	return ret;
}

void system_setReloadCb(reload_func_t cb)
{
	system_reload_func = cb;
}

void system_reload(void)
{
	if (system_reload_func)
		system_reload_func();
}

/* Network interface global variables */
static struct ip4_addr ipaddr, netmask, gw;
static struct netif netif;

static const IpCfg ip_cfg[] = {
    {
        .id = BOARD_ID_KK348,
        .ip = IP_DEFAULT_KK348,
        .eth_cfg = &eth_cfg_kk348,
    },

    {
        .id = BOARD_ID_KK353,
        .ip = IP_DEFAULT_KK353,
        .eth_cfg = &eth_cfg_kk353_kk354,
    },

    {
        .id = BOARD_ID_KK354,
        .ip = IP_DEFAULT_KK354,
        .eth_cfg = &eth_cfg_kk353_kk354,
    },

    {
        .id = BOARD_ID_STM3220G,
        .ip = IP_DEFAULT_KK348,
        .eth_cfg = &eth_cfg_stm3220g,
    },
};
STATIC_ASSERT(BOARD_CNT == countof(ip_cfg));

const IpCfg *system_ethConfig(void)
{
	const IpCfg *cfg = board_findCfg(board_id(), ip_cfg);
	if (cfg)
		return cfg;
	else
		return &ip_cfg[BOARD_DEFAULT];
}

const char *system_defaultIP(void)
{
	const IpCfg *cfg = board_findCfg(board_id(), ip_cfg);
	if (cfg)
		return cfg->ip;
	else
		return ip_cfg[BOARD_DEFAULT].ip;
}

void system_init(void)
{
	(void)a;
	static struct in_addr board_ip;

	const IpCfg *cfg = board_findCfg(board_id(), ip_cfg);
	if (!cfg)
	{
		cfg = &ip_cfg[BOARD_DEFAULT];
		LOG_WARN("Using default system configuration for board %s\n", board_name());
	}

	eth_init(&cfg->eth_cfg->e);
	config_register(&mac_cfg);
	config_load(&mac_cfg);

	/* Get static IP (if any) */
	dli_get(IP_KEY, system_defaultIP(), ip_addr, IP_ADDR_STR_LEN);
	if (!inet_aton(ip_addr, &board_ip))
		inet_aton(system_defaultIP(), &board_ip);

	/* Initialize TCP/IP stack */
	tcpip_init(NULL, NULL);
	LOG_INFO("Tcp initialized\n");

	/* Bring up the network interface */
	if (!netif_add(&netif, &ipaddr, &netmask, &gw, NULL, ethernetif_init, tcpip_input))
	{
		LOG_ERR("Error adding netif\n");
	}
	netif_set_default(&netif);
	netif_set_up(&netif);

#ifdef DHCP_ENABLE
	// acquire address
	dhcp_start(&netif);
	LOG_INFO("dhcp start\n");
	ticks_t start = timer_clock();
	bool dhcp_ok = true;
	while (!netif.ip_addr.addr)
	{
		timer_delay(100);

		if (standalone() && (timer_clock() - start > ms_to_ticks(10000)))
		{
			dhcp_stop(&netif);
			dhcp_ok = false;
			netif.ip_addr.addr = board_ip.s_addr;
		}
	}
	if (dhcp_ok)
		LOG_INFO(">>> dhcp ok: ip = %s\n", ip_ntoa(&netif.ip_addr.addr));
	else
		LOG_INFO(">>> dhcp FAIL: using static ip = %s\n", ip_ntoa(&netif.ip_addr.addr));
#else
	netif.ip_addr.addr = board_ip.s_addr;
#endif

	REGISTER_CMD(reset);
	REGISTER_CMD(quit);
	REGISTER_CMD(ping);
	REGISTER_CMD(ver);
	REGISTER_CMD(hw_ver);
	REGISTER_CMD(board_id);
	REGISTER_CMD(set_state);
	REGISTER_CMD(get_state);
	REGISTER_CMD(get_syslog_addr);
	REGISTER_CMD(set_syslog_addr);
	REGISTER_CMD(get_errors);
	dli_get("syslog_addr", IP_SYSLOG_DEFAULT, syslog_addr, IP4_ADDR_LEN);
	kprintf("Syslog addr:%s\n", syslog_addr);
	system_reload();

	LOG_INFO("*****************BOOTING*****************\n");
	LOG_INFO("Starting %s, version %s\n", HW_TYPE, VERS_TAG);
	MacAddress mac = eth_mac();
	LOG_INFO("Serial number: %02X:%02X:%02X:%02X:%02X:%02X\n", mac.addr[0], mac.addr[1], mac.addr[2], mac.addr[3], mac.addr[4], mac.addr[5]);
	LOG_INFO("IP address: %s\n", ip4addr_ntoa(&netif.ip_addr));
	LOG_INFO("SYSLOG address: %s\n", syslog_addr);
}
