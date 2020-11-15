#include "cfg/cfg_boot.h"
#include "hw/hw_boot.h"
#include "hw/hw_eth.h"
#include "hw/hw_led.h"
#include "crc32.h"
#include "common/mac.h"
#include "common/board_id.h"
#include "common/ip.h"
#include "common/system.h"

#include "drv/eth_stm32.h"
#include "verstag.h"

#include "telnet.h"
#include "common/heartbeat.h"
#include "common/eth_cfg.h"

#define LOG_LEVEL  BOOT_LOG_LEVEL
#define LOG_FORMAT BOOT_LOG_FORMAT
#include <cfg/log.h>

#include <cfg/debug.h>
#include <cpu/irq.h>
#include <kern/proc.h>
#include <drv/timer.h>
#include <drv/i2c.h>
#include <drv/eeprom.h>
#include <drv/flash.h>
#include <drv/eth.h>
#include <io/cm3.h> // FLASH_BASE
#include <io/kfile_block.h>
#include <kern/proc.h> // Process
#include <net/tftp.h>

#include <netif/ethernetif.h>

#include <lwip/ip.h>
#include <lwip/ip_addr.h>
#include <lwip/netif.h>
#include <lwip/tcpip.h>
#include <lwip/dhcp.h>

/* Network interface global variables */
static struct ip4_addr ipaddr, netmask, gw;
static struct netif netif;

static Flash internal_flash;
static KFileBlock flash;
static TftpSession server;

static uint8_t buf[sizeof(Tftpframe)];

static Process *telnet_proc = NULL;
PROC_DEFINE_STACK(telnet_stack, KERN_MINSTACKSIZE * 3);
PROC_DEFINE_STACK(heartbeat_stack, KERN_MINSTACKSIZE * 3);

void (*rom_start)(void) NORETURN;
#define START_APP() rom_start()

// in ms
#define RECV_TIMEOUT 5000

I2c i2c;
Eeprom eep;
char mac_str[MAC_ADDR_STR_LEN + 1];
char ip_str[IP_ADDR_STR_LEN];
struct in_addr board_ip;

typedef struct BootCfg
{
	int id;
	int eeprom_dev;
	const char filename[16];
	const char tag[16];
	const Stm32EthCfg *eth_cfg;
} BootCfg;

#define DEF_BOOT_TAG  "kk0000"
#define DEF_BOOT_FILE DEF_BOOT_TAG ".czf"

#define KK348_ID 0
static const BootCfg boot_cfg[] = {
    {
        .id = BOARD_ID_KK348,
        .eeprom_dev = I2C_BITBANG0,
        .filename = "kk348.czf",
        .tag = "kk348",
    },
    {
        .id = BOARD_ID_KK353,
        .eeprom_dev = I2C_BITBANG1,
        .filename = "kk353.czf",
        .tag = "kk353",
    },
    {
        .id = BOARD_ID_KK354,
        .eeprom_dev = I2C_BITBANG1,
        .filename = "kk354.czf",
        .tag = "kk354",
    },
    {
        .id = BOARD_ID_STM3220G,
        .eeprom_dev = I2C_BITBANG1,
        .filename = "kk354.czf",
        .tag = "kk354",
    },
};
STATIC_ASSERT(BOARD_CNT == countof(boot_cfg));

#define EEPROM_ADDRESS 7
#define EEPROM_TYPE    EEPROM_24XX128

static const BootCfg *cfg;
static void init(void)
{
	IRQ_ENABLE;
	timer_init();
	LED_INIT();
	LED_ON(true);
	proc_init();
	kdbg_init();
	kprintf(" === %s bootloader === \n", APP_NAME);
	kprintf("Version %s\n", VERS_TAG);

	int board_id = board_id_init();
	cfg = board_findCfg(board_id, boot_cfg);
	if (cfg == NULL)
	{
		int def_id = boot_cfg[BOARD_DEFAULT].id;
		const char *board_name = board_data(def_id)->name;
		LOG_WARN("Boot configuration unavailable for board id %04X, default to %s configuration!\n", board_id, board_name);
		cfg = &boot_cfg[BOARD_DEFAULT];
	}

	eth_init(&system_ethConfig()->eth_cfg->e);

	MacAddress mac;
	/* Initialize DLI eeprom */
	i2c_hw_bitbangInit(&i2c, cfg->eeprom_dev);
	eeprom_init(&eep, &i2c, EEPROM_TYPE, EEPROM_ADDRESS, false);

	/* Read mac address */
	if (kblock_read(&eep.blk, 0, mac_str, 0, sizeof(mac_str)) != sizeof(mac_str) || !mac_decode(mac_str, &mac))
	{
		LOG_ERR("Error reading/decoding MAC address, using default\n");
		mac_decode(MAC_DEFAULT, &mac);
	}
	else
		LOG_INFO("MAC address [%.17s]\n", mac_str);

	eth_setMac(mac);

	/* Read IP address */
	if (kblock_read(&eep.blk, 0, ip_str, MAC_ADDR_STR_LEN + 1, sizeof(ip_str)) != sizeof(ip_str) || !inet_aton(ip_str, &board_ip))
	{
		LOG_ERR("Error reading/decoding IP address, using default [%s]\n", system_defaultIP());
		inet_aton(system_defaultIP(), &board_ip);
	}
	else
		LOG_INFO("IP address read from DLI [%.15s]\n", ip_str);

	flash_init(&internal_flash, FLASH_WRITE_ONCE);
	// trim flash to avoid problems with kfile_block
	LOG_INFO("Trim start: %d, blocks: %ld\n", TRIM_START, internal_flash.blk.blk_cnt - TRIM_START);
	kblock_trim(&internal_flash.blk, TRIM_START, internal_flash.blk.blk_cnt - TRIM_START);
	kfileblock_init(&flash, &internal_flash.blk);

	/* Initialize TCP/IP stack */
	tcpip_init(NULL, NULL);
	LOG_INFO("tcp initialized\n");

	/* Bring up the network interface */
	if (!netif_add(&netif, &ipaddr, &netmask, &gw, NULL, ethernetif_init, tcpip_input))
	{
		LOG_ERR("Error adding netif\n");
	}
	netif_set_default(&netif);
	netif_set_up(&netif);

	LOG_INFO("Init complete\n");
}

static char filename[100];
static uint8_t recv_buf[300];
/*
 * Receive and write the firmware to flash.
 * \return true if successful, false otherwise
 */
static bool receiveFirmware(TftpSession *ctx, KFile *fp)
{
	LOG_INFO("receiveFirmware\n");
	TftpOpenMode mode;
	KFile *tftp = tftp_listen(ctx, filename, sizeof(filename), &mode);
	kfile_seek(fp, 0, KSM_SEEK_SET);

	if (tftp)
	{
		if (mode == TFTP_READ)
		{
			LOG_ERR("Unsupported operation TFTP_RRQ\n");
			return false;
		}

		LOG_INFO("Received write request to file: %s\n", filename);

		/* Check for correct filenames
		 * We check for kk348 filename for backward compatibility for already produced boards.
		 */
		if (!strstr(filename, DEF_BOOT_FILE) && !strstr(filename, cfg->filename) && !strstr(filename, boot_cfg[KK348_ID].filename))
		{
			tftp_setErrorMsg(ctx, "Filename invalid");
			kfile_close(tftp);
			return false;
		}

		size_t rd = 0;
		do
		{
			rd = kfile_read(tftp, recv_buf, sizeof(recv_buf));
			if (kfile_error(tftp))
			{
				LOG_WARN("Error while reading from Tftp, code: %d\n", kfile_error(tftp));
				return false;
			}

			size_t bytes = kfile_write(fp, recv_buf, rd);
			if (bytes != rd)
			{
				LOG_ERR("Error writing to flash memory, error code: %d\n", kfile_error(fp));
				tftp_setErrorMsg(ctx, "Error writing to flash");
				kfile_close(tftp);
				return false;
			}
#if 0
			else
				LOG_INFO("Written %zd bytes\n", bytes);
#endif
		} while (rd == sizeof(recv_buf));
		kfile_flush(fp);
		return true;
	}

	if (ctx->error != TFTP_ERR_TIMEOUT)
		LOG_ERR("tftp_listen error: %d\n", ctx->error);

	return false;
}

static bool crcCheckOk(KFile *fd)
{
	kfile_seek(fd, 0, KSM_SEEK_SET);
	// read firmware size and crc from firmware
	// If memory is not initialized, size is bogus. Clamp max value.
	size_t size;
	uint32_t crc_check;

	size_t rd = kfile_read(fd, &size, sizeof(size_t));
	rd += kfile_read(fd, &crc_check, sizeof(uint32_t));
	LOG_INFO("CRC read with kfile, size: %zu, crc: %08lx\n", size, crc_check);

	ASSERT(rd == 8);
	if (size > MAX_FIRMWARE_SIZE || size < MIN_FIRMWARE_SIZE)
	{
		LOG_ERR("Wrong fw size %zu\n", size);
		return false;
	}

	uint32_t crc_old = 0;
	uint32_t crc_actual = 0;
	uint32_t crc_all = 0;

	crc_old = crc32(boot_cfg[KK348_ID].tag, strlen(boot_cfg[KK348_ID].tag), crc_old);
	crc_actual = crc32(cfg->tag, strlen(cfg->tag), crc_actual);
	crc_all = crc32(DEF_BOOT_TAG, strlen(DEF_BOOT_TAG), crc_all);

	size_t flash_bytes = 0;
	while (size > 0)
	{
		size_t bytes = MIN(size, sizeof(buf));
		rd = kfile_read(fd, buf, bytes);
		ASSERT(rd == bytes);
		flash_bytes += rd;
		crc_old = crc32(buf, rd, crc_old);
		crc_actual = crc32(buf, rd, crc_actual);
		crc_all = crc32(buf, rd, crc_all);
		size -= rd;
	}
	LOG_INFO("Computed CRC(%s): %08lx, CRC(%s): %08lx, CRC(" DEF_BOOT_TAG "): %08lx, firmware_bytes: %zd\n", boot_cfg[KK348_ID].tag, crc_old, cfg->tag, crc_actual, crc_all, flash_bytes);

	if (crc_old == crc_check || crc_actual == crc_check || crc_all == crc_check)
	{
		LOG_INFO("CRC check ok\n");
		return true;
	}
	return false;
}

int main(void)
{
	init();

#ifdef DHCP_ENABLE
	// acquire address
	dhcp_start(&netif);
	LOG_INFO("dhcp start\n");
	ticks_t start = timer_clock();
	bool dhcp_ok = true;
	while (!netif.ip_addr.addr)
	{
		timer_delay(100);

		if (timer_clock() - start > ms_to_ticks(10000))
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

	proc_new(heartbeat_proc, 0, sizeof(heartbeat_stack), heartbeat_stack);
	/* start tftp server */
	tftp_init(&server, TFTP_SERVER_PORT, RECV_TIMEOUT);

	bool ret = receiveFirmware(&server, &flash.fd);
	if (ret)
		LOG_INFO("Firmware transfer ok\n");

	LOG_INFO("CRC check on firmware before jump...\n");
	if (crcCheckOk(&flash.fd))
		goto end;

	/* crc check failed, start fake telnet process */
	LOG_INFO("CRC check failed, waiting for new firmware\n");
	telnet_proc = proc_new(telnet_entry, NULL, sizeof(telnet_stack), telnet_stack);

	do
	{
		while (!receiveFirmware(&server, &flash.fd))
		{
			LOG_INFO("Error receiving firmware, retrying...\n");
		}
	} while (!crcCheckOk(&flash.fd));

end:
	/* load traget address from reset vector (4 bytes offset, 8 bytes length + CRC) */
	rom_start = *(void **)(FLASH_BOOT_SIZE + 8 + 4);
	LOG_INFO("Jump to main application, address 0x%p\n", rom_start);
	eth_cleanup();
	timer_cleanup();
	IRQ_DISABLE;
	LED_ON(false);
	START_APP();
}
