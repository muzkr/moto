#ifndef _FLASH_H
#define _FLASH_H

#include <stdint.h>

void flash_program_page(uint32_t addr, const uint8_t *buf);

#endif //
