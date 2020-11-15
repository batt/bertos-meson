#ifndef CRC32_H
#define CRC32_H

#include <cfg/compiler.h>

/*
 * \param oldcrc Should be 0 the first time, the previous computed crc when
 *               calling more times this function
 */
uint32_t crc32(const void *buf, int len, uint32_t oldcrc);

#endif //CRC32_H
