
#include "flash_programmer.h"
#include <stdbool.h>
#include <string.h>
#include "log.h"

#define FLASH_PAGE_SIZE 256

static bool check_block(const uf2_block_t *block, uint32_t base_addr, uint32_t page_num)
{
    if (UF2_MAGIC_START0 != block->magic_start0 || UF2_MAGIC_START1 != block->magic_start1)
    {
        return false;
    }
    if (UF2_FLAG_NOFLASH & block->flags)
    {
        return false;
    }
    if (FLASH_PAGE_SIZE != block->payload_size)
    {
        return false;
    }
    if (0 != block->target_addr % FLASH_PAGE_SIZE)
    {
        return false;
    }
    if (block->target_addr < base_addr || block->target_addr >= (base_addr + page_num * FLASH_PAGE_SIZE))
    {
        return false;
    }
    if (block->num_blocks > page_num)
    {
        return false;
    }
    if (block->block_no >= block->num_blocks)
    {
        return false;
    }

    return true;
}

static bool program_finished(const flash_programmer_t *programmer)
{
    if (!programmer->_in_progress)
    {
        return false;
    }
    for (uint32_t i = 0; i < programmer->_block_num; i++)
    {
        if (!programmer->block_map[i])
        {
            return false;
        }
    }

    return true;
}

void flash_programmer_init(flash_programmer_t *programmer)
{
    programmer->_in_progress = false;
}

flash_program_result_t flash_program(flash_programmer_t *programmer, const uf2_block_t *block)
{
    if (!check_block(block, programmer->base_addr, programmer->page_num))
    {
        return FLASH_PROGRAM_REJECT;
    }

    bool start_flag = false;

    if (!programmer->_in_progress)
    {
        start_flag = true;
        programmer->_in_progress = true;
        programmer->_block_num = block->num_blocks;
        memset(programmer->block_map, 0, block->num_blocks);
        // log("fw program start: %d\n", block->num_blocks);
    }
    else
    {
        if (programmer->block_map[block->block_no])
        {
            // Repeated
            // log("fw program repeat\n");
            return FLASH_PROGRAM_REJECT;
        }
    }

    // log("fw program: %d, %08x\n", block->block_no, block->target_addr);

    programmer->program_page_func(block->target_addr, block->data);
    programmer->block_map[block->block_no] = true;

    if (program_finished(programmer))
    {
        // log("fw program finished\n");
        return FLASH_PROGRAM_FINISH;
    }

    return start_flag ? FLASH_PROGRAM_START : FLASH_PROGRAM_IN_PROGRESS;
}
