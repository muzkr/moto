#include "usb_fs.h"
#include "dfu.h"
#include "flash_programmer.h"
#include "internal_flash.h"
#include "py25q16.h"
#include "board.h"
#include "main.h"
#include "log.h"

static uint8_t fw_program_map[FW_PAGE_NUM];
static flash_programmer_t fw_programmer = {
    .base_addr = FW_ADDR,
    .page_num = FW_PAGE_NUM,
    .block_map = fw_program_map,
    .program_page_func = internal_flash_program_page,
};

static uint8_t py25q16_program_map[PY25Q16_PAGE_NUM];
static void py25q16_program_page(uint32_t addr, const uint8_t *buf)
{
    py25q16_write(addr, buf, PY25Q16_PAGE_SIZE, false);
}
static flash_programmer_t py25q16_programmer = {
    .base_addr = 0,
    .page_num = PY25Q16_PAGE_NUM,
    .block_map = py25q16_program_map,
    .program_page_func = py25q16_program_page,
};

int usb_fs_sector_write(uint32_t sector, const uint8_t *buf, uint32_t size)
{
    // Initialize
    static bool initialized = false;
    if (!initialized)
    {
        initialized = true;
        flash_programmer_init(&fw_programmer);
        flash_programmer_init(&py25q16_programmer);
    }

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

    // FW program
    do
    {
        const uf2_block_t *block = (uf2_block_t *)buf;
        flash_program_result_t res = flash_program(&fw_programmer, block);

        if (FLASH_PROGRAM_REJECT == res)
        {
            break;
        }

        log("fw program: %d, %08x\n", block->block_no, block->target_addr);

        if (FLASH_PROGRAM_START == res)
        {
            board_flashlight_flash(100);
            log("fw program start: %d\n", block->num_blocks);
        }
        else if (FLASH_PROGRAM_FINISH == res)
        {
            log("fw program finished\n");
            board_flashlight_on();
            main_schedule_reset(500);
        }

    } while (0);

    return 0;
}
