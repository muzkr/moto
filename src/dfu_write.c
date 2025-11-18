#include "usb_fs.h"
#include "dfu.h"
#include "internal_flash.h"
#include "py25q16.h"
#include "uf2.h"
#include <string.h>
#include "board.h"
#include "main.h"
#include "log.h"

#define PAGE_SIZE 256
#define PY25Q16_PAGE_NUM_MAX 1024
#define PY25Q16_SIZE_MAX (PAGE_SIZE * PY25Q16_PAGE_NUM_MAX)

typedef void (*program_page_func_t)(uint32_t addr, const uint8_t *buf);

static uint8_t block_map[PY25Q16_PAGE_NUM_MAX > FW_PAGE_NUM ? PY25Q16_PAGE_NUM_MAX : FW_PAGE_NUM] = {0};

static struct
{
    uint32_t base_addr;
    uint32_t num_pages;
    uint32_t num_blocks;
    program_page_func_t program_page_func;
    uint8_t in_progress;
} program_state = {0};

static bool check_block(const uf2_block_t *block)
{
    if (UF2_MAGIC_START0 != block->magic_start0 || UF2_MAGIC_START1 != block->magic_start1)
    {
        return false;
    }
    if (UF2_FLAG_NOFLASH & block->flags)
    {
        return false;
    }
    // Allow less than one page
    if (0 == block->payload_size)
    {
        return false;
    }
    if (block->payload_size > FLASH_PAGE_SIZE)
    {
        return false;
    }
    if (0 != block->target_addr % FLASH_PAGE_SIZE)
    {
        return false;
    }
    if (0 == block->num_blocks)
    {
        return false;
    }
    if (block->block_no >= block->num_blocks)
    {
        return false;
    }

    return true;
}

static bool accept_first_block(const uf2_block_t *block)
{
    const uint32_t target_addr = block->target_addr;

    if (FW_ADDR <= target_addr && target_addr < FW_ADDR + FW_SIZE)
    {
        program_state.base_addr = FW_ADDR;
        program_state.num_pages = FW_PAGE_NUM;
        program_state.program_page_func = internal_flash_program_page;
        program_state.num_blocks = block->num_blocks;
        return true;
    }

    if (PY25Q16_BASE_ADDR <= target_addr && target_addr < PY25Q16_BASE_ADDR + PY25Q16_SIZE)
    {
        uint32_t i = (target_addr - PY25Q16_BASE_ADDR) / PY25Q16_SIZE_MAX;
        uint32_t base_addr = PY25Q16_BASE_ADDR + i * PY25Q16_SIZE_MAX;

        program_state.base_addr = base_addr;
        program_state.num_pages = PY25Q16_PAGE_NUM_MAX;
        program_state.program_page_func = (program_page_func_t)py25q16_write_page;
        program_state.num_blocks = block->num_blocks;
        return true;
    }

    return false;
}

static inline bool accept_subsequent_block(const uf2_block_t *block)
{
    return program_state.base_addr <= block->target_addr //
           && block->target_addr < (program_state.base_addr + program_state.num_pages * PAGE_SIZE);
}

static bool program_finished()
{
    for (uint32_t i = 0; i < program_state.num_blocks; i++)
    {
        if (!block_map[i])
        {
            return false;
        }
    }

    return true;
}

int usb_fs_sector_write(uint32_t sector, const uint8_t *buf, uint32_t size)
{
    if (SECTOR_SIZE != size)
    {
        log("sector_write: %d, %08x, %d\n", sector, (uint32_t)buf, size);
        return 1;
    }
    if (0 != ((uint32_t)buf) % 4)
    {
        log("sector_write: %d, %08x, %d\n", sector, (uint32_t)buf, size);
        return 1;
    }

    do
    {
        const uf2_block_t *block = (uf2_block_t *)buf;
        if (!check_block(block))
        {
            log("invalid block ignored\n");
            break;
        }

        if (!program_state.in_progress)
        {
            if (accept_first_block(block))
            {
                log("program start: %d\n", block->num_blocks);
                program_state.in_progress = true;
                memset(block_map, 0, block->num_blocks);
                board_backlight_flash(50);
            }
            else
            {
                log("first block rejected\n");
                return 1; // Error
            }
        }
        else
        {
            if (accept_subsequent_block(block))
            {
                if (block_map[block->block_no])
                {
                    // Repeat
                    return 0;
                }
            }
            else
            {
                log("subsequent block rejected\n");
                return 1; // Error
            }
        }

        log("program: %d, %08x\n", block->block_no, block->target_addr);
        program_state.program_page_func(block->target_addr, block->data);
        block_map[block->block_no] = true;

        if (program_finished())
        {
            log("program finished\n");
            board_backlight_on(BOARD_DEFAULT_BACKLIGHT_DELAY);

            if (FW_ADDR == program_state.base_addr)
            {
                main_schedule_reset(500);
            }

            program_state.in_progress = false;
        }

        return 0;

    } while (0);

    return 0;
}
