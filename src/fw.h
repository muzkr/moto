#ifndef _FW_H
#define _FW_H

#include "py32f0xx.h"
#include <assert.h>

#define _BL_SIZE 0x2800 // 10 KB
#define FW_ADDR (FLASH_BASE + _BL_SIZE)
// #define FW_ADDR (FLASH_BASE + 64 * 1024) // This is for test! 0x08010000

static_assert(0 == FW_ADDR % FLASH_PAGE_SIZE);

#define FW_SIZE (FLASH_END + 1 - FW_ADDR)
#define FW_PAGE_NUM (FW_SIZE / FLASH_PAGE_SIZE)

#endif // _FW_H
