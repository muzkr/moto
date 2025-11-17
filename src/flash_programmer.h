#ifndef _FLASH_PROGRAMMER_H
#define _FLASH_PROGRAMMER_H

#include <stdint.h>
#include "uf2.h"

typedef void (*flash_program_page_func_t)(uint32_t addr, const uint8_t *buf);

typedef struct
{
    uint32_t base_addr;
    uint32_t page_num;
    uint8_t *block_map;
    flash_program_page_func_t program_page_func;
    uint32_t _block_num;
    uint8_t _in_progress;
} flash_programmer_t;

typedef enum
{
    FLASH_PROGRAM_REJECT,
    FLASH_PROGRAM_START,
    FLASH_PROGRAM_IN_PROGRESS,
    FLASH_PROGRAM_FINISH,
} flash_program_result_t;

void flash_programmer_init(flash_programmer_t *programmer);
flash_program_result_t flash_program(flash_programmer_t *programmer, const uf2_block_t *block);

#endif // _FLASH_PROGRAMMER_H
