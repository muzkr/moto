#include "usb_fs.h"
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include "main.h"
#include "log.h"
#include "fat.h"
#include "version.h"
#include "flashlight.h"
#include "uf2.h"
#include "flash.h"

#define BL_SIZE 0x2800 // 10 KB
// #define FLASH_FW_ADDR (FLASH_BASE + BL_SIZE)
#define FLASH_FW_ADDR (FLASH_BASE + 64 * 1024) // This is for test! 0x08010000
#define FLASH_FW_SIZE (FLASH_END + 1 - FLASH_FW_ADDR)

static_assert(0 == FLASH_FW_ADDR % FLASH_PAGE_SIZE);

// Number of pages taken by the internal firmware area
#define FLASH_FW_PAGE_NUM (FLASH_FW_SIZE / FLASH_PAGE_SIZE)

#define SECTOR_NUM 32000
#define SECTOR_SIZE FAT_DEFAULT_SECTOR_SIZE

#define BOOT_SECTOR 0
#define FAT_SECTOR 1
#define FAT_SECTOR_NUM 125
#define ROOT_SECTOR (FAT_SECTOR + FAT_SECTOR_NUM)
#define ROOT_SECTOR_NUM 32
#define DATA_SECTOR (ROOT_SECTOR + ROOT_SECTOR_NUM)
#define DATA_SECTOR_NUM (SECTOR_NUM - DATA_SECTOR)

#define FAT_ENTRY_SIZE 2
#define FAT_ENTRIES_PER_SECTOR (SECTOR_SIZE / FAT_ENTRY_SIZE)

#define DIR_ENTRIES_PER_SECTOR (SECTOR_SIZE / FAT_DIR_ENTRY_SIZE)

#define DATA_SECTOR_TO_FAT_ENTRY(n) (2 + (n))
#define FAT_ENTRY_TO_SECTOR(n) ((n) / FAT_ENTRIES_PER_SECTOR)

#define _VOLUME_CREATE_DATE FAT_MK_DATE(2025, 11, 1)
#define _VOLUME_CREATE_TIME FAT_MK_TIME(9, 0, 0)

#define VOLUME_ID ((_VOLUME_CREATE_DATE) << 16) | (_VOLUME_CREATE_TIME)
#define VOLUME_LABEL "MOTO       "
#define BPB_MEDIA 0xf0

#define MOTO_URL "https://github.com/muzkr/moto"

// Root entry assign ------

#define VOLUME_LABEL_ROOT_ENTRY 0
#define MOTO_INFO_ROOT_ENTRY 1
#define UF2_INFO_ROOT_ENTRY 2
#define INDEX_HTM_ROOT_ENTRY 3
#define CURRENT_UF2_ROOT_ENTRY 4

// Data sectors assign -----

#define MOTO_INFO_SECTOR 0             // Data sector of MOTO.TXT
#define UF2_INFO_SECTOR 1              // Data sector of INFO_UF2.TXT
#define INDEX_HTM_SECTOR 2             // First data sector of INDEX.HTM
#define CURRENT_UF2_SECTOR 3           // First data sector of CURRENT.UF2
#define CURRENT_UF2_SECTOR_NUM_MAX 472 // Max number of data sectors of CURRENT.UF2

static_assert(FLASH_FW_PAGE_NUM <= CURRENT_UF2_SECTOR_NUM_MAX);

// ------------

static const fat_boot_sector_t BOOT_SECTOR_RECORD = {
    .jump_boot = {0xeb, 0, 0},
    .OEM_name = "MOTO    ",
    .sector_size = SECTOR_SIZE,
    .sectors_per_cluster = 1,
    .reserved_sectors = FAT_SECTOR - BOOT_SECTOR,
    .num_FATs = 1,
    .root_entries = ROOT_SECTOR_NUM * DIR_ENTRIES_PER_SECTOR,
    .total_sectors16 = SECTOR_NUM,
    .media = BPB_MEDIA,
    .FAT_sectors16 = FAT_SECTOR_NUM,
    .sectors_per_track = SECTOR_NUM,
    .num_heads = 1,
    .drive_num = 0x00,
    .boot_signature = FAT_BOOT_SIGNATURE_ENABLE,
    .volume_ID = VOLUME_ID,
    .volume_label = VOLUME_LABEL,
    .fs_type = "FAT16   ",
};

static const fat_dir_entry_t VOLUME_LABEL_DIR_ENTRY = {
    .name = VOLUME_LABEL,
    .attr = FAT_DIR_ATTR_VOLUME_ID,
    .create_date = _VOLUME_CREATE_DATE,
    .create_time = _VOLUME_CREATE_TIME,
    .write_date = _VOLUME_CREATE_DATE,
    .write_time = _VOLUME_CREATE_TIME,
    .last_access_date = _VOLUME_CREATE_DATE,
};

// ---------

static const char MOTO_INFO_CONTENT[] = //
    "Moto Bootloader\r\n"               //
    "Mode: DFU\r\n"                     //
    "Version: " MOTO_VERSION "\r\n"     //
    "Support: " MOTO_URL "\r\n"         //
    ;

#define MOTOR_INFO_CONTENT_SIZE (sizeof(MOTO_INFO_CONTENT) - 1)

static_assert(MOTOR_INFO_CONTENT_SIZE <= SECTOR_SIZE);

static const fat_dir_entry_t MOTO_INFO_DIR_ENTRY = {
    .name = "MOTO    TXT",
    .attr = FAT_DIR_ATTR_RO,
    .first_clusterLO = DATA_SECTOR_TO_FAT_ENTRY(MOTO_INFO_SECTOR),
    .file_size = MOTOR_INFO_CONTENT_SIZE,
    .create_date = _VOLUME_CREATE_DATE,
    .create_time = _VOLUME_CREATE_TIME,
    .write_date = _VOLUME_CREATE_DATE,
    .write_time = _VOLUME_CREATE_TIME,
    .last_access_date = _VOLUME_CREATE_DATE,
};

// ---------

static const char UF2_INFO_CONTENT[] =         //
    "UF2 Bootloader Moto-" MOTO_VERSION "\r\n" //
    "Model: Moto Bootloader\r\n"               //
    "Board-ID: UVK5-V3-variants\r\n"           //
    ;

#define UF2_INFO_CONTENT_SIZE (sizeof(UF2_INFO_CONTENT) - 1)

static_assert(UF2_INFO_CONTENT_SIZE <= SECTOR_SIZE);

static const fat_dir_entry_t UF2_INFO_DIR_ENTRY = {
    .name = "INFO_UF2TXT",
    .attr = FAT_DIR_ATTR_RO,
    .first_clusterLO = DATA_SECTOR_TO_FAT_ENTRY(UF2_INFO_SECTOR),
    .file_size = UF2_INFO_CONTENT_SIZE,
    .create_date = _VOLUME_CREATE_DATE,
    .create_time = _VOLUME_CREATE_TIME,
    .write_date = _VOLUME_CREATE_DATE,
    .write_time = _VOLUME_CREATE_TIME,
    .last_access_date = _VOLUME_CREATE_DATE,
};

// ---------

static const char INDEX_HTM_CONTENT[] =     //
    "<!doctype html>\n"                     //
    "<html>"                                //
    "<body>"                                //
    "<script>\n"                            //
    "location.replace(\"" MOTO_URL "\");\n" //
    "</script>"                             //
    "</body>"                               //
    "</html>\n"                             //
    ;

#define INDEX_HTM_CONTENT_SIZE (sizeof(INDEX_HTM_CONTENT) - 1)

static_assert(INDEX_HTM_CONTENT_SIZE <= SECTOR_SIZE);

static const fat_dir_entry_t INDEX_HTM_DIR_ENTRY = {
    .name = "INDEX   HTM",
    .attr = FAT_DIR_ATTR_RO,
    .first_clusterLO = DATA_SECTOR_TO_FAT_ENTRY(INDEX_HTM_SECTOR),
    .file_size = INDEX_HTM_CONTENT_SIZE,
    .create_date = _VOLUME_CREATE_DATE,
    .create_time = _VOLUME_CREATE_TIME,
    .write_date = _VOLUME_CREATE_DATE,
    .write_time = _VOLUME_CREATE_TIME,
    .last_access_date = _VOLUME_CREATE_DATE,
};

// CURRENT.UF2 ------

#define CURRENT_UF2_FAT_ENTRY_FIRST DATA_SECTOR_TO_FAT_ENTRY(CURRENT_UF2_SECTOR)
#define CURRENT_UF2_FAT_ENTRY_LAST (CURRENT_UF2_FAT_ENTRY_FIRST + FLASH_FW_PAGE_NUM)

static const fat_dir_entry_t CURRENT_UF2_dir_entry = {
    .name = "CURRENT UF2",
    .attr = FAT_DIR_ATTR_RO,
    .first_clusterLO = CURRENT_UF2_FAT_ENTRY_FIRST,
    .file_size = FLASH_FW_SIZE * 2,
    .create_date = _VOLUME_CREATE_DATE,
    .create_time = _VOLUME_CREATE_TIME,
    .write_date = _VOLUME_CREATE_DATE,
    .write_time = _VOLUME_CREATE_TIME,
    .last_access_date = _VOLUME_CREATE_DATE,
};

// ---------------

static struct
{
    uint8_t in_progress;
    uint8_t num_blocks;
} fw_program_state = {0};

static uint8_t fw_program_map[FLASH_FW_PAGE_NUM];

static bool check_fw_block(const uf2_block_t *block)
{
    if (UF2_FLAG_NOFLASH & block->flags)
    {
        return false;
    }
    if (256 != block->payload_size)
    {
        return false;
    }
    if (block->num_blocks > FLASH_FW_PAGE_NUM)
    {
        return false;
    }
    if (block->block_no >= block->num_blocks)
    {
        return false;
    }

    return true;
}

static bool fw_program_finished()
{
    if (!fw_program_state.in_progress)
    {
        return false;
    }
    for (uint32_t i = 0; i < fw_program_state.num_blocks; i++)
    {
        if (!fw_program_map[i])
        {
            return false;
        }
    }

    return true;
}

// ---------------

void usb_fs_get_cap(uint32_t *sector_num, uint16_t *sector_size)
{
    log("get_cap\n");
    *sector_num = SECTOR_NUM;
    *sector_size = SECTOR_SIZE;
}

int usb_fs_sector_read(uint32_t sector, uint8_t *buf, uint32_t size)
{
    // log("sector_read: %d, %08x, %d\n", sector, (uint32_t)buf, size);

    if (SECTOR_SIZE != size)
    {
        log("sector_read: %d, %08x, %d\n", sector, (uint32_t)buf, size);
        return 1;
    }
    if (0 != ((uint32_t)buf) % 4)
    {
        log("sector_read: %d, %08x, %d\n", sector, (uint32_t)buf, size);
        return 1;
    }

    memset(buf, 0, SECTOR_SIZE);

    if (BOOT_SECTOR == sector)
    {
        memcpy(buf, &BOOT_SECTOR_RECORD, sizeof(BOOT_SECTOR_RECORD));
        fat_set_word(buf + SECTOR_SIZE - 2, FAT_SIGNATURE_WORD);
    }
    else if (sector < FAT_SECTOR)
    {
        // Preserved
    }
    else if (sector < ROOT_SECTOR)
    {
        // FAT ----

        sector -= FAT_SECTOR;

        if (0 == sector)
        {
            // Reserved entries
            // #0
            buf[0] = BPB_MEDIA;
            buf[1] = 0xff;
            // #1
            buf[2] = 0xff;
            buf[3] = 0xff;

            // MOTO.TXT
            fat_set_word(buf + 2 * FAT_ENTRY_SIZE, FAT16_ENTRY_EOF);
            // INFO_UF2.TXT
            fat_set_word(buf + 3 * FAT_ENTRY_SIZE, FAT16_ENTRY_EOF);
            // INDEX.HTM
            fat_set_word(buf + 4 * FAT_ENTRY_SIZE, FAT16_ENTRY_EOF);
        }

        // CURRENT.UF2
        do
        {
            if (sector < FAT_ENTRY_TO_SECTOR(CURRENT_UF2_FAT_ENTRY_FIRST))
            {
                break;
            }
            if (sector > FAT_ENTRY_TO_SECTOR(CURRENT_UF2_FAT_ENTRY_LAST - 1))
            {
                break;
            }

            // log("CURRENT.UF2 FAT: %d, %d, %d\n", sector, CURRENT_UF2_FAT_ENTRY_FIRST, CURRENT_UF2_FAT_ENTRY_LAST);

            for (uint32_t i = 0; i < FAT_ENTRIES_PER_SECTOR; i++)
            {
                const uint32_t entry = sector * FAT_ENTRIES_PER_SECTOR + i;

                if (entry < CURRENT_UF2_FAT_ENTRY_FIRST)
                {
                    continue;
                }
                if (entry >= CURRENT_UF2_FAT_ENTRY_LAST)
                {
                    break;
                }

                if (CURRENT_UF2_FAT_ENTRY_LAST - 1 == entry)
                {
                    // log("CURRENT.UF2 last FAT entry: %d, %d\n", sector, entry);
                    fat_set_word(buf + FAT_ENTRY_SIZE * i, FAT16_ENTRY_EOF);
                }
                else
                {
                    // log("CURRENT.UF2 FAT entry: %d, %d\n", sector, entry);
                    fat_set_word(buf + FAT_ENTRY_SIZE * i, 1 + entry);
                }
            } // for

        } while (0);
    }
    else if (sector < DATA_SECTOR)
    {
        // Root ----

        sector -= ROOT_SECTOR;

        if (0 == sector)
        {
            // Volume label
            memcpy(buf + FAT_DIR_ENTRY_SIZE * VOLUME_LABEL_ROOT_ENTRY, &VOLUME_LABEL_DIR_ENTRY, FAT_DIR_ENTRY_SIZE);
            // MOTO.TXT
            memcpy(buf + FAT_DIR_ENTRY_SIZE * MOTO_INFO_ROOT_ENTRY, &MOTO_INFO_DIR_ENTRY, FAT_DIR_ENTRY_SIZE);
            // INFO_UF2.TXT
            memcpy(buf + FAT_DIR_ENTRY_SIZE * UF2_INFO_ROOT_ENTRY, &UF2_INFO_DIR_ENTRY, FAT_DIR_ENTRY_SIZE);
            // INDEX.HTM
            memcpy(buf + FAT_DIR_ENTRY_SIZE * INDEX_HTM_ROOT_ENTRY, &INDEX_HTM_DIR_ENTRY, FAT_DIR_ENTRY_SIZE);
            // CURRENT.UF2
            memcpy(buf + FAT_DIR_ENTRY_SIZE * CURRENT_UF2_ROOT_ENTRY, &CURRENT_UF2_dir_entry, FAT_DIR_ENTRY_SIZE);
        }
    }
    else if (sector < SECTOR_NUM)
    {
        // Data -----

        sector -= DATA_SECTOR;

        // MOTO.TXT
        if (MOTO_INFO_SECTOR == sector)
        {
            memcpy(buf, MOTO_INFO_CONTENT, MOTOR_INFO_CONTENT_SIZE);
        }
        // INFO_UF2.TXT
        else if (UF2_INFO_SECTOR == sector)
        {
            memcpy(buf, UF2_INFO_CONTENT, UF2_INFO_CONTENT_SIZE);
        }
        // INDEX.HTM
        else if (INDEX_HTM_SECTOR == sector)
        {
            memcpy(buf, INDEX_HTM_CONTENT, INDEX_HTM_CONTENT_SIZE);
        }
        // CURRENT.UF2
        else if (CURRENT_UF2_SECTOR <= sector && sector < CURRENT_UF2_SECTOR + FLASH_FW_PAGE_NUM)
        {
            const uint32_t fw_addr = FLASH_FW_ADDR + FLASH_PAGE_SIZE * (sector - CURRENT_UF2_SECTOR);
            uf2_block_t *block = (uf2_block_t *)buf;
            memcpy(block->data, (void *)fw_addr, FLASH_PAGE_SIZE);

            block->magic_start0 = UF2_MAGIC_START0;
            block->magic_start1 = UF2_MAGIC_START1;
            block->target_addr = fw_addr;
            block->payload_size = FLASH_PAGE_SIZE;
            block->block_no = sector - CURRENT_UF2_SECTOR;
            block->num_blocks = FLASH_FW_PAGE_NUM;
            block->magic_end = UF2_MAGIC_END;
        }
    }

    return 0;
}

int usb_fs_sector_write(uint32_t sector, const uint8_t *buf, uint32_t size)
{
    // log("sector_write: %d, %08x, %d\n", sector, (uint32_t)buf, size);

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

        if (UF2_MAGIC_START0 != block->magic_start0 || UF2_MAGIC_START1 != block->magic_start1)
        {
            break;
        }
        if (!check_fw_block(block))
        {
            break;
        }

        if (!fw_program_state.in_progress)
        {
            fw_program_state.in_progress = true;
            fw_program_state.num_blocks = block->num_blocks;
            memset(fw_program_map, 0, sizeof(fw_program_map));
            flashlight_flash(100);
            log("fw program start: %d\n", block->num_blocks);
        }
        else
        {
            if (fw_program_map[block->block_no])
            {
                // Repeated
                log("fw program repeat\n");
                break;
            }
        }

        log("fw program: %d, %08x\n", block->block_no, block->target_addr);

        flash_program_page(block->target_addr, block->data);
        fw_program_map[block->block_no] = true;

        if (fw_program_finished())
        {
            log("fw program finished\n");
            flashlight_on();
            main_schedule_reset(100);
        }

    } while (0);

    return 0;
}
