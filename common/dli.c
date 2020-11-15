/**
 * \file
 *
 * \author Francesco Sacchi <batt@develer.com>
 *
 * \brief Device Life Information.
 *
 */

#include "dli.h"
#include "config.h"
#include "protocol.h"

#include "common/mac.h"
#include "common/ip.h"

#include <io/kblock.h>
#include <io/kfile_block.h>
#include <cfg/compiler.h>
#include <algo/crc.h>
#include <cfg/log.h>
#include <mware/ini_reader.h>
#include <mware/parser.h>
#include <kern/sem.h>

#include <string.h>
#include <stdlib.h> /* strtol */
#include <stdio.h>

typedef struct FatEntry
{
#define FAT_TAG 0x0FA7
	uint16_t tag;
#define FAT_VERSION 0
	uint16_t version;
	uint32_t counter;
	uint16_t last_file;
	uint16_t crc;
} PACKED FatEntry;

static KBlock *boot;
static KBlock *fat;
static KBlock *ini[2];
static Semaphore *i2c_sem;

static KFileBlock file[2], boot_file, fat_file;
static FatEntry last_entry;

#define BOOT_SIZE 1024
#define FAT_SIZE  1024
#define INI_SIZE  4096

static bool fat_read(KFile *fd, int addr, FatEntry *entry)
{
	ASSERT(addr < 2);
	sem_obtain(i2c_sem);
	kfile_seek(fd, addr * sizeof(FatEntry), KSM_SEEK_SET);
	bool err = (kfile_read(fd, entry, sizeof(FatEntry)) != sizeof(FatEntry));
	sem_release(i2c_sem);

	if (err)
	{
		LOG_ERR("Error reading block\n");
		return false;
	}

	if (entry->tag != FAT_TAG)
	{
		LOG_ERR("Wrong tag\n");
		return false;
	}

	if (entry->version != FAT_VERSION)
	{
		LOG_ERR("Wrong version\n");
		return false;
	}

	uint16_t crc = crc16(CRC16_INIT_VAL, entry, sizeof(FatEntry) - sizeof(uint16_t));
	if (crc != entry->crc)
		LOG_ERR("CRC error\n");

	return (crc == entry->crc);
}

/* Requires I2C semaphore locked */
static bool fat_write(KFile *fd, int addr, FatEntry *entry)
{
	ASSERT(addr < 2);
	entry->tag = FAT_TAG;
	entry->version = FAT_VERSION;
	entry->crc = crc16(CRC16_INIT_VAL, entry, sizeof(FatEntry) - sizeof(uint16_t));

	kfile_seek(fd, addr * sizeof(FatEntry), KSM_SEEK_SET);
	bool err = (kfile_write(fd, entry, sizeof(FatEntry)) == sizeof(FatEntry));
	return err;
}

static void fat_default(FatEntry *entry)
{
	entry->counter = 0;
	entry->last_file = 0;
}

static bool findLast(KFile *fat, FatEntry *entry)
{
	FatEntry entry0, entry1;

	bool err0 = !fat_read(fat, 0, &entry0);
	bool err1 = !fat_read(fat, 1, &entry1);

	if (err0 && err1)
	{
		fat_default(entry);
		return false;
	}
	else if (err0)
		memcpy(entry, &entry1, sizeof(*entry));
	else if (err1)
		memcpy(entry, &entry0, sizeof(*entry));
	else
	{
		if ((int32_t)entry0.counter - (int32_t)entry1.counter > 0)
			memcpy(entry, &entry0, sizeof(*entry));
		else
			memcpy(entry, &entry1, sizeof(*entry));
	}
	return true;
}

INLINE KFile *currIni(void)
{
	ASSERT(last_entry.last_file < 2);
	return &file[last_entry.last_file].fd;
}

INLINE KFile *nextIni(void)
{
	ASSERT(last_entry.last_file < 2);
	++last_entry.last_file;
	last_entry.last_file &= 1;
	ASSERT(last_entry.last_file < 2);
	return &file[last_entry.last_file].fd;
}

static bool fat_update(KFile *fat, FatEntry *entry)
{
	entry->counter++;
	return fat_write(fat, entry->last_file, entry);
}

static void dli_dump(void)
{
	int c;
	bool new_line = true;

	sem_obtain(i2c_sem);

	kfile_seek(currIni(), 0, KSM_SEEK_SET);
	while ((c = kfile_getc(currIni())) != EOF)
	{
		if (c == '\n' && new_line)
			continue;
		else if (c == '\n')
			new_line = true;
		else
			new_line = false;

		protocol_printf("%c", c);
	}

	sem_release(i2c_sem);
}

static void dli_dumpAll(void)
{
	int c, i = 0;

	kputs("BOOT DATA\n");

	sem_obtain(i2c_sem);
	kfile_seek(&boot_file.fd, 0, KSM_SEEK_SET);
	while ((c = kfile_getc(&boot_file.fd)) != EOF)
	{
		if ((i++ % 16) == 0)
			kputs("\n");
		kprintf("%02X ", c);
	}
	kputs("\n");

	for (i = 0; i < 2; i++)
	{
		FatEntry buf;
		kprintf("FAT%d\n", i);
		kfile_seek(&fat_file.fd, i * sizeof(FatEntry), KSM_SEEK_SET);
		kfile_read(&fat_file.fd, &buf, sizeof(FatEntry));
		kdump(&buf, sizeof(buf));
	}

	bool new_line = false;
	for (i = 0; i < 2; i++)
	{
		KFile *fd = &file[i].fd;
		kprintf("\n\nFILE%d---------------\n", i);
		kfile_seek(fd, 0, KSM_SEEK_SET);
		while ((c = kfile_getc(fd)) != EOF)
		{
			if (c == '\n' && new_line)
				continue;
			else if (c == '\n')
				new_line = true;
			else
				new_line = false;

			kputchar(c);
		}
	}
	sem_release(i2c_sem);
}

static char dli_buf[DLI_VAL_MAX_LEN + 1];

static ResultCode cmd_dli_set(parms *args)
{
	const char *err_msg = NULL;
	size_t value_len = args[2].str.sz;

	if (value_len > DLI_VAL_MAX_LEN)
	{
		err_msg = "Value too long.";
		goto error;
	}

	memcpy(dli_buf, args[2].str.p, value_len);
	char *value = dli_buf;

	value[value_len] = '\0';

	char *key = CONST_CAST(char *, args[1].str.p);
	key[args[1].str.sz] = '\0';

	LOG_INFO("Find entry\n");
	ConfigMetadata *cfgm = NULL;
	const ConfigEntry *e = config_findEntry(&cfgm, key);
	SetPRetVals setp_ret = SRV_OK;

	if (e)
	{
		LOG_INFO("Entry found\n");
		ASSERT(e->setp);

		setp_ret = e->setp(e, value, false);
		LOG_INFO("Reload func address [%p]\n", cfgm->reload);
		if (cfgm->reload)
			cfgm->reload();
	}
	else
		LOG_WARN("DLI key is not known: %s\n", key);

	LOG_INFO("Set value in DLI\n");
	if (setp_ret != SRV_CONV_ERR)
	{
		if (!dli_set(key, value))
		{
			LOG_WARN("Errors occurred while writing to DLI\n");
			err_msg = "Error while writing DLI.";
			goto error;
		}
	}
	else
	{
		err_msg = "Conversion error.";
		goto error;
	}
	if (setp_ret == SRV_OK)
		protocol_reply(RC_OK, NULL);
	else
		protocol_reply(RC_ERROR, "Value clamped!");
	return RC_OK;

error:
	protocol_reply(RC_ERROR, err_msg);
	return RC_ERROR;
}
MAKE_TEMPLATE(dli_set, "ss", "", 0);

static ResultCode cmd_dli_get(parms *args)
{
	if (!dli_get(args[1].str.p, "", dli_buf, sizeof(dli_buf)))
	{
		protocol_reply(RC_ERROR, "Key not found");
		return RC_ERROR;
	}
	protocol_reply(RC_OK, "Key found!");
	protocol_printf("%s\n", dli_buf);

	return RC_OK;
}
MAKE_TEMPLATE(dli_get, "s", "s", 0);

MAKE_CMD(dli_reset, "", "",
         ({
	         (void)args;
	         dli_reset();
	         LOG_INFO("DLI is reset\n");
	         protocol_reply(RC_OK, "DLI is reset");
	         RC_OK;
         }),
         0)

MAKE_CMD(dli_dump, "", "",
         ({
	         (void)args;
	         protocol_reply(RC_OK, NULL);
	         dli_dump();
	         RC_OK;
         }),
         0)

MAKE_CMD(dli_dump_all, "", "",
         ({
	         (void)args;
	         protocol_reply(RC_OK, NULL);
	         dli_dumpAll();
	         RC_OK;
         }),
         0)

void dli_init(KBlock *blk1, KBlock *blk2, KBlock *blk3, KBlock *blk4, Semaphore *_i2c_sem)
{
	ASSERT(blk1->blk_cnt == blk2->blk_cnt);
	ASSERT(blk2->blk_cnt == blk3->blk_cnt);
	ASSERT(blk3->blk_cnt == blk4->blk_cnt);

	ASSERT(blk1->blk_size == blk2->blk_size);
	ASSERT(blk2->blk_size == blk3->blk_size);
	ASSERT(blk3->blk_size == blk4->blk_size);

	ASSERT(blk1->blk_size >= sizeof(FatEntry) * 2);
	boot = blk1;
	fat = blk2;
	ini[0] = blk3;
	ini[1] = blk4;
	i2c_sem = _i2c_sem;

	block_idx_t boot_cnt = DIV_ROUNDUP(BOOT_SIZE, blk1->blk_size);
	block_idx_t fat_cnt = DIV_ROUNDUP(FAT_SIZE, blk1->blk_size);
	block_idx_t ini_cnt = DIV_ROUNDUP(INI_SIZE, blk1->blk_size);

	ASSERT(boot_cnt + fat_cnt + ini_cnt * 2 <= blk1->blk_cnt);

	kblock_trim(boot, 0, boot_cnt);
	kblock_trim(fat, boot_cnt, fat_cnt);
	kblock_trim(ini[0], boot_cnt + fat_cnt, ini_cnt);
	kblock_trim(ini[1], boot_cnt + fat_cnt + ini_cnt, ini_cnt);

	kfileblock_init(&boot_file, boot);
	kfileblock_init(&fat_file, fat);
	kfileblock_init(&file[0], ini[0]);
	kfileblock_init(&file[1], ini[1]);

	findLast(&fat_file.fd, &last_entry);
	REGISTER_CMD(dli_reset);
	REGISTER_CMD(dli_dump);
	REGISTER_CMD(dli_dump_all);
	REGISTER_CMD(dli_get);
	REGISTER_CMD(dli_set);
}

#define DLI_SECTION "dli"

bool dli_get(const char *key, const char *default_val, char *value, size_t len)
{
	bool ret = true;
	sem_obtain(i2c_sem);
	if (strcmp(key, SERIALNO_KEY) == 0)
	{
		kfile_seek(&boot_file.fd, 0, KSM_SEEK_SET);
		size_t size = MIN((size_t)BOOT_SIZE, len);
		if (kfile_read(&boot_file.fd, value, size) == size)
			goto end;
		else
		{
			strncpy(value, default_val, len);
			ret = false;
			goto end;
		}
	}

	if (strcmp(key, IP_KEY) == 0)
	{
		kfile_seek(&boot_file.fd, MAC_ADDR_STR_LEN + 1, KSM_SEEK_SET);
		size_t size = MIN((size_t)BOOT_SIZE, len);
		if (kfile_read(&boot_file.fd, value, size) == size)
			goto end;
		else
		{
			strncpy(value, default_val, len);
			ret = false;
			goto end;
		}
	}

	ret = (ini_getString(currIni(), DLI_SECTION, key, default_val, value, len) != EOF);
end:
	sem_release(i2c_sem);
	return ret;
}

bool dli_set(const char *key, const char *value)
{
	bool ret = true;
	sem_obtain(i2c_sem);

	if (strcmp(key, SERIALNO_KEY) == 0)
	{
		ASSERT(strlen(value) <= BOOT_SIZE);
		kfile_seek(&boot_file.fd, 0, KSM_SEEK_SET);
		ret = (kfile_write(&boot_file.fd, value, strlen(value) + 1) == strlen(value) + 1);
		goto end;
	}

	if (strcmp(key, IP_KEY) == 0)
	{
		ASSERT(strlen(value) <= BOOT_SIZE);
		kfile_seek(&boot_file.fd, MAC_ADDR_STR_LEN + 1, KSM_SEEK_SET);
		ret = (kfile_write(&boot_file.fd, value, strlen(value) + 1) == strlen(value) + 1);
		goto end;
	}

	if (ini_setString(currIni(), nextIni(), DLI_SECTION, key, value) == EOF)
	{
		ret = false;
		goto end;
	}
	/* fat_update() requires the semaphore lock */
	ret = fat_update(&fat_file.fd, &last_entry);
end:
	sem_release(i2c_sem);
	return ret;
}

bool dli_updateStat(const char *stat, int delta)
{
	char *endptr = NULL;
	char range_buf[20];
	if (!dli_get(stat, "0", range_buf, sizeof(range_buf)))
		LOG_WARN("Key not found: %s, use default 0\n", stat);

	long conv = strtol(range_buf, &endptr, 10);
	if (*endptr != '\0')
	{
		LOG_ERR("Error converting saved value for: %s\n", stat);
		return false;
	}

	conv += delta;
	snprintf(range_buf, sizeof(range_buf), "%d", (int)conv);
	if (!dli_set(stat, range_buf))
	{
		LOG_ERR("Error saving updated value for: %s\n", stat);
		return false;
	}
	return true;
}

void dli_reset(void)
{
	sem_obtain(i2c_sem);
	kfile_seek(&boot_file.fd, 0, KSM_SEEK_SET);
	while (kfile_putc(0xff, &boot_file.fd) != EOF)
		;

	kfile_seek(&fat_file.fd, 0, KSM_SEEK_SET);
	while (kfile_putc(0xff, &fat_file.fd) != EOF)
		;

	uint8_t buf[64];
	memset(buf, '\n', sizeof(buf));

	for (int i = 0; i < 2; i++)
	{
		KFile *fd = &file[i].fd;
		kfile_seek(fd, 0, KSM_SEEK_SET);
		while (kfile_write(fd, buf, sizeof(buf)) == sizeof(buf))
			;
	}
	sem_release(i2c_sem);
}
