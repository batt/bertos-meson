/**
 * \file
 * <!--
 * This file is part of BeRTOS.
 *
 * Bertos is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * As a special exception, you may use this file as part of a free software
 * library without restriction.  Specifically, if other files instantiate
 * templates or use macros or inline functions from this file, or you compile
 * this file and link it with other files to produce an executable, this
 * file does not by itself cause the resulting executable to be covered by
 * the GNU General Public License.  This exception does not however
 * invalidate any other reasons why the executable file might be covered by
 * the GNU General Public License.
 *
 * Copyright 2009 Develer S.r.l. (http://www.develer.com/)
 * -->
 *
 * \brief Test function for ini_reader module.
 *
 * $test$: cp bertos/cfg/cfg_kfile.h $cfgdir/
 * $test$: echo "#undef CONFIG_KFILE_GETS" >> $cfgdir/cfg_kfile.h
 * $test$: echo "#define CONFIG_KFILE_GETS 1" >> $cfgdir/cfg_kfile.h
 * 
 * \author Luca Ottaviano <lottaviano@develer.com>
 */

#include <emul/kfile_posix.h>
#include <cfg/test.h>

#include <string.h> // strcmp
#include <stdlib.h> // atoi

#include "ini_reader.h"

const char ini_file[] = "./test/ini_reader_file.ini";
static KFilePosix kf;

const char ini_file_out[] = "./test/ini_reader_file_out.ini";
static KFilePosix kf_out;

const char ini_file_out2[] = "./test/ini_reader_file_out2.ini";
static KFilePosix kf_out2;

int ini_reader_testSetup(void)
{
	kdbg_init();
	if (!kfile_posix_init(&kf, ini_file, "r"))
	{
		kprintf("No test file found\n");
		return -1;
	}
	if (!kfile_posix_init(&kf_out, ini_file_out, "w+"))
	{
		kprintf("Error opening  test_out file\n");
		return -1;
	}
	if (!kfile_posix_init(&kf_out2, ini_file_out2, "w+"))
	{
		kprintf("Error opening  test_out file2\n");
		return -1;
	}

	return 0;
}

int ini_reader_testRun(void)
{
	char buf[30];
	memset(buf, 0, 30);

	ASSERT(ini_getString(&kf.fd, "First", "String", "default", buf, 30) != EOF);
	ASSERT(strcmp(buf, "noot") == 0);

	ASSERT(ini_getString(&kf.fd, "Second", "Val", "default", buf, 30) != EOF);
	ASSERT(strcmp(buf, "2") == 0);

	ASSERT(ini_getString(&kf.fd, "First", "Empty", "default", buf, 30) != EOF);
	ASSERT(strcmp(buf, "") == 0);

	ASSERT(ini_getString(&kf.fd, "Second", "Bar", "default", buf, 30) == EOF);
	ASSERT(strcmp(buf, "default") == 0);

	ASSERT(ini_getString(&kf.fd, "Foo", "Bar", "default", buf, 30) == EOF);
	ASSERT(strcmp(buf, "default") == 0);

	ASSERT(ini_getString(&kf.fd, "Second", "Long key", "", buf, 30) == EOF);

	ASSERT(ini_getString(&kf.fd, "Second", "comment", "", buf, 30) != EOF);
	ASSERT(strcmp(buf, "line with #comment") == 0);

	ASSERT(ini_getString(&kf.fd, "Long section with spaces", "value", "", buf, 30) != EOF);
	ASSERT(strcmp(buf, "long value") == 0);

	ASSERT(ini_getString(&kf.fd, "Long section with spaces", "no_new_line", "", buf, 30) != EOF);
	ASSERT(strcmp(buf, "value") == 0);

	ASSERT(ini_setString(&kf.fd, &kf_out.fd, "First", "String", "root") != EOF);
	ASSERT(ini_getString(&kf_out.fd, "First", "String", "default", buf, 30) != EOF);
	ASSERT(strcmp(buf, "root") == 0);

	ASSERT(ini_setString(&kf.fd, &kf_out.fd, "First", "Empty", "false") != EOF);
	ASSERT(ini_getString(&kf_out.fd, "First", "Empty", "default", buf, 30) != EOF);
	ASSERT(strcmp(buf, "false") == 0);

	ASSERT(ini_setString(&kf.fd, &kf_out.fd, "First", "not_present", "true") != EOF);
	ASSERT(ini_getString(&kf_out.fd, "First", "not_present", "default", buf, 30) != EOF);
	ASSERT(strcmp(buf, "true") == 0);

	ASSERT(ini_setString(&kf.fd, &kf_out.fd, "Not Present", "not_present", "false") != EOF);
	ASSERT(ini_getString(&kf_out.fd, "Not Present", "not_present", "default", buf, 30) != EOF);
	ASSERT(strcmp(buf, "false") == 0);

	ASSERT(ini_setString(&kf.fd, &kf_out.fd, "Second", "comment", NULL) != EOF);
	ASSERT(ini_getString(&kf_out.fd, "Second", "comment", "default", buf, 30) == EOF);
	ASSERT(strcmp(buf, "default") == 0);

	ASSERT(ini_getString(&kf_out.fd, "Long section with spaces", "no_new_line", "", buf, 30) != EOF);
	ASSERT(strcmp(buf, "value") == 0);

	ASSERT(ini_setString(&kf.fd, &kf_out.fd, "Long section with spaces", "value", "longer value") != EOF);
	ASSERT(ini_getString(&kf_out.fd, "Long section with spaces", "value", "default", buf, 30) != EOF);
	ASSERT(strcmp(buf, "longer value") == 0);

	char val[30];
	KFile *in = &kf_out.fd;
	KFile *out = &kf_out2.fd;
	KFile *swap;
	for (int i = 0; i < 1000; i++)
	{
		sprintf(buf, "key%04d", i);
		sprintf(val, "%08d", i * 12135);
		ASSERT(ini_setString(in, out, "Test", buf, val) != EOF);
		swap = in;
		in = out;
		out = swap;
	}

	for (int i = 0; i < 1000; i++)
	{
		long value;
		sprintf(buf, "key%04d", i);
		ASSERT(ini_getInteger(in, "Test", buf, -1, &value, 10) != EOF);
		ASSERT(value == i * 12135);
	}

	return 0;
}

int ini_reader_testTearDown(void)
{
	return kfile_close(&kf.fd) + kfile_close(&kf_out.fd) + kfile_close(&kf_out2.fd);
}

TEST_MAIN(ini_reader);
