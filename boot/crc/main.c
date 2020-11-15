/**********************************************************************
 *
 * Filename:    main.c
 * 
 * Description: A simple test program for the CRC implementations.
 *
 * Notes:       To test a different CRC standard, modify crc.h.
 *
 * 
 * Copyright (c) 2000 by Michael Barr.  This software is placed into
 * the public domain and may be used for any purpose.  However, this
 * notice must not be changed or removed and no warranty is either
 * expressed or implied by its publication or distribution.
 **********************************************************************/

#include <stdio.h>
#include <string.h>

#include "crc.h"
#define WIDTH  (8 * sizeof(crc))
#define TOPBIT (1 << (WIDTH - 1))

#undef REFLECT_DATA
#define REFLECT_DATA(X) ((unsigned char)reflect((X), 8))
#undef REFLECT_REMAINDER
#define REFLECT_REMAINDER(X) ((crc)reflect((X), WIDTH))

static unsigned long
reflect(unsigned long data, unsigned char nBits)
{
	unsigned long reflection = 0x00000000;
	unsigned char bit;

	/*
	 * Reflect the data about the center bit.
	 */
	for (bit = 0; bit < nBits; ++bit)
	{
		/*
		 * If the LSB bit is set, set the reflection of it.
		 */
		if (data & 0x01)
		{
			reflection |= (1 << ((nBits - 1) - bit));
		}

		data = (data >> 1);
	}

	return (reflection);

} /* reflect() */

crc crcSlow(unsigned char const message[], int nBytes, crc rem)
{
	crc remainder = rem;
	int byte;
	unsigned char bit;

	/*
     * Perform modulo-2 division, a byte at a time.
     */
	for (byte = 0; byte < nBytes; ++byte)
	{
		/*
         * Bring the next byte into the remainder.
         */
		remainder ^= (REFLECT_DATA(message[byte]) << (WIDTH - 8));

		/*
         * Perform modulo-2 division, a bit at a time.
         */
		for (bit = 8; bit > 0; --bit)
		{
			/*
             * Try to divide the current data bit.
             */
			if (remainder & TOPBIT)
			{
				remainder = (remainder << 1) ^ POLYNOMIAL;
			}
			else
			{
				remainder = (remainder << 1);
			}
		}
	}

	/*
     * The final remainder is the CRC result.
     */
	return (REFLECT_REMAINDER(remainder) ^ FINAL_XOR_VALUE);

} /* crcSlow() */

/** Reverses bit order. MSB -> LSB and LSB -> MSB. */
unsigned int reverse(unsigned int x)
{
	unsigned int ret = 0;
	for (int i = 0; i < 32; ++i)
	{
		if (x & 0x1 != 0)
		{
			ret |= (0x80000000 >> i);
		}
		else
		{
		}
		x = (x >> 1);
	}
	return ret;
}

unsigned int crc32(unsigned char *message, unsigned int msgsize, unsigned int crc)
{
	unsigned int i, j; // byte counter, bit counter
	unsigned int byte;
	unsigned int poly = 0xedb88320; //0x04C11DB7;
	i = 0;
	for (i = 0; i < msgsize; ++i)
	{
		byte = message[i];    // Get next byte.
		byte = reverse(byte); // 32-bit reversal.
		for (j = 0; j <= 7; ++j)
		{ // Do eight times.
			if ((int)(crc ^ byte) < 0)
				crc = (crc << 1) ^ poly;
			else
				crc = crc << 1;
			byte = byte << 1; // Ready next msg bit.
		}
	}
	return reverse(~crc);
}

void main(void)
{
	FILE *fp = fopen("bani.txt", "rb");
	unsigned char buf[512];
#if 0
	unsigned char  test[] = "123456789";
	crc crc_val = INITIAL_REMAINDER;
	while (!feof(fp))
	{
		size_t rd = fread(buf, sizeof(char), 512, fp);
		crc_val = crcSlow(buf, rd, crc_val);
	}
	printf("crc: %x\n", crc_val);
#endif

#if 1
	unsigned int val = 0;
	while (!feof(fp))
	{
		size_t rd = fread(buf, sizeof(char), 512, fp);
		val = crc32(buf, rd, val);
	}
	printf("crc: %x\n", val >> 8 | val << 24);
#endif

	/*
	 * Print the check value for the selected CRC algorithm.
	 */
	//printf("The check value for the %s standard is 0x%X\n", CRC_NAME, CHECK_VALUE);

	/*
	 * Compute the CRC of the test message, slowly.
	 */
	///printf("The crcSlow() of \"123456789\" is 0x%X\n", crcSlow(test, strlen(test)));

	/*
	 * Compute the CRC of the test message, more efficiently.
	 */
	//crcInit();
	//printf("The crcFast() of \"123456789\" is 0x%X\n", crcFast(test, strlen(test)));

} /* main() */
