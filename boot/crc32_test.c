#include "crc32.h"
#include "tftp.h"

#include <cfg/debug.h>
#include <cfg/test.h>
#include <stdio.h>

FILE *fp;

int crc32_testSetup(void)
{
	kdbg_init();
	fp = fopen("/tmp/fw.bin", "r");
	ASSERT(fp);
	return 0;
}

int crc32_testTearDown(void)
{
	fclose(fp);
	return 0;
}

static uint8_t buf[sizeof(Tftpframe)];

int crc32_testRun(void)
{
	uint32_t size;
	uint32_t crc;

	ASSERT(fread(&size, 1, sizeof(size), fp) == sizeof(size));
	ASSERT(fread(&crc, 1, sizeof(crc), fp) == sizeof(crc));
	kprintf("size %d, crc %08x\n", size, crc);

	uint32_t crc_new = 0;
	size_t rd = 0;
	while (size > 0)
	{
		size_t bytes = MIN((size_t)size, sizeof(buf));
		rd = fread(buf, 1, bytes, fp);
		ASSERT(rd == bytes);
		crc_new = crc32(buf, rd, crc_new);
		size -= rd;
	}

	kprintf("computed crc %08x\n", crc_new);
	ASSERT(crc_new == crc);
	return 0;
}

TEST_MAIN(crc32);
