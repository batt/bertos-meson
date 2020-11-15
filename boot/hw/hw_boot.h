#ifndef HW_BOOT_H
#define HW_BOOT_H

#include <io/cm3.h>

#define FLASH_BOOT_SIZE   (128 * 1024)
#define TRIM_START        (FLASH_BOOT_SIZE / FLASH_PAGE_SIZE_BYTES)
#define MAX_FIRMWARE_SIZE ((F_SIZE * 1024) - FLASH_BOOT_SIZE)
#define MIN_FIRMWARE_SIZE 8192

#endif //HW_BOOT_H
