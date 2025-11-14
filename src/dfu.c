#include "usb_fs.h"
#include <string.h>
#include <assert.h>
#include "log.h"
#include "fat.h"
#include "version.h"
#include "flashlight.h"
#include "uf2.h"
#include "py32f0xx.h"

#define BL_SIZE 0x2800 // 10 KB
// #define FLASH_FW_ADDR (FLASH_BASE + BL_SIZE)
#define FLASH_FW_ADDR (FLASH_BASE + 96 * 1024) // This is for test!
#define FLASH_FW_SIZE (FLASH_END + 1 - FLASH_FW_ADDR)

static_assert(0 == FLASH_FW_ADDR % FLASH_PAGE_SIZE);

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
#define FAT_ENTRY_TO_SECTOR(n) ((n) / (SECTOR_SIZE / 16))

#define _VOLUME_CREATE_DATE FAT_MK_DATE(2025, 11, 1)
#define _VOLUME_CREATE_TIME FAT_MK_TIME(9, 0, 0)

#define VOLUME_ID ((_VOLUME_CREATE_DATE) << 16) | (_VOLUME_CREATE_TIME)
#define VOLUME_LABEL "MOTO       "
#define BPB_MEDIA 0xf0

// Root entry assign ------

#define VOLUME_LABEL_ROOT_ENTRY 0
#define MOTO_INFO_ROOT_ENTRY 1
#define FIRMWARE_UF2_ROOT_ENTRY 2

// Data sectors assign -----

#define MOTO_INFO_SECTOR 0              // Data sector of MOTO.TXT
#define FIRMWARE_UF2_SECTOR 1           // First data sector of FIRMWARE.UF2
#define FIRMWARE_UF2_SECTOR_NUM_MAX 472 // Max number of data sectors of FIRMWARE.UF2

static const fat_boot_sector_t BOOT_SECTOR_RECORD = {
    .jump_boot = {0xeb, 0, 0},
    .OEM_name = "MOTO    ",
    .sector_size = SECTOR_SIZE,
    .sectors_per_cluster = 1,
    .reserved_sectors = 1,
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
};

static const char MOTO_INFO_CONTENT[] = "Mode: DFU\n"                              //
                                        "Version: " VERSION "\n"                   //
                                        "Support: https://github.com/muzkr/moto\n" //
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
};

// FIRMWARE.UF2 ------

#define FIRMWARE_UF2_SIZE (FLASH_FW_SIZE * 2)
#define FIRMWARE_UF2_SECTOR_NUM (FIRMWARE_UF2_SIZE / 512)

static_assert(0 == FIRMWARE_UF2_SIZE % 512);

#define FIRMWARE_UF2_FAT_ENTRY_FIRST DATA_SECTOR_TO_FAT_ENTRY(FIRMWARE_UF2_SECTOR)
#define FIRMWARE_UF2_FAT_ENTRY_LAST DATA_SECTOR_TO_FAT_ENTRY(FIRMWARE_UF2_SECTOR + FIRMWARE_UF2_SECTOR_NUM)

static fat_dir_entry_t FIRMWARE_UF2_dir_entry = {
    .name = "FIRMWAREUF2",
    .attr = FAT_DIR_ATTR_RO,
    .first_clusterLO = DATA_SECTOR_TO_FAT_ENTRY(FIRMWARE_UF2_SECTOR),
    .file_size = FIRMWARE_UF2_SIZE,
    .create_date = _VOLUME_CREATE_DATE,
    .create_time = _VOLUME_CREATE_TIME,
    .write_date = _VOLUME_CREATE_DATE,
    .write_time = _VOLUME_CREATE_TIME,
};

// ---------------

void vfs_get_cap(uint32_t *sector_num, uint16_t *sector_size)
{
    log("get_cap\n");
    *sector_num = SECTOR_NUM;
    *sector_size = SECTOR_SIZE;
}

int vfs_sector_read(uint32_t sector, uint8_t *buf, uint32_t size)
{
    log("sector_read: %d, %08x, %d\n", sector, (uint32_t)buf, size);

    if (SECTOR_SIZE != size)
    {
        return 1;
    }

    memset(buf, 0, SECTOR_SIZE);

    if (BOOT_SECTOR == sector)
    {
        memcpy(buf, &BOOT_SECTOR_RECORD, sizeof(BOOT_SECTOR_RECORD));
        fat_set_word(buf + SECTOR_SIZE - 2, FAT_SIGNATURE_WORD);
    }
    else if (sector < ROOT_SECTOR)
    {
        // FAT ----

        sector -= FAT_SECTOR;

        if (0 == sector)
        {
            // Reserved entries
            buf[0] = BPB_MEDIA;
            buf[1] = 0xff;
            buf[2] = 0xff;
            buf[3] = 0xff;

            // Moto info : known entry 2
            fat_set_word(buf + 4, FAT16_ENTRY_EOF);
        }

        // FIRMWARE.UF2
        do
        {
            if (sector < FAT_ENTRY_TO_SECTOR(FIRMWARE_UF2_FAT_ENTRY_FIRST))
            {
                break;
            }
            if (sector > FAT_ENTRY_TO_SECTOR(FIRMWARE_UF2_FAT_ENTRY_LAST - 1))
            {
                break;
            }

            for (uint32_t i = 0; i < FAT_ENTRIES_PER_SECTOR; i++)
            {
                const uint32_t entry = sector * FAT_ENTRIES_PER_SECTOR + i;

                if (entry < FIRMWARE_UF2_FAT_ENTRY_FIRST)
                {
                    continue;
                }
                if (entry >= FIRMWARE_UF2_FAT_ENTRY_LAST)
                {
                    break;
                }

                if (FIRMWARE_UF2_FAT_ENTRY_LAST - 1 == entry)
                {
                    fat_set_word(buf + FAT_ENTRY_SIZE * entry, FAT16_ENTRY_EOF);
                }
                else
                {
                    fat_set_word(buf + FAT_ENTRY_SIZE * entry, 1 + entry);
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
            // Moto info
            memcpy(buf + FAT_DIR_ENTRY_SIZE * MOTO_INFO_ROOT_ENTRY, &MOTO_INFO_DIR_ENTRY, FAT_DIR_ENTRY_SIZE);

            // FIRMWARE.UF2
            memcpy(buf + FAT_DIR_ENTRY_SIZE * FIRMWARE_UF2_ROOT_ENTRY, &FIRMWARE_UF2_dir_entry, FAT_DIR_ENTRY_SIZE);
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

        // FIRMWARE.UF2
        if (FIRMWARE_UF2_SECTOR <= sector && sector < FIRMWARE_UF2_SECTOR + FIRMWARE_UF2_SECTOR_NUM)
        {
            const uint8_t *fw_addr = (uint8_t *)(FLASH_FW_ADDR + 256 * (sector - FIRMWARE_UF2_SECTOR));
            uf2_block_t *block = (uf2_block_t *)buf;
            memcpy(block->data, (void *)fw_addr, 256);
            fat_set_dword((uint8_t *)&block->magic_start0, UF2_MAGIC_START0);
            fat_set_dword((uint8_t *)&block->magic_start1, UF2_MAGIC_START1);
            fat_set_dword((uint8_t *)&block->target_addr, (uint32_t)fw_addr);
            fat_set_dword((uint8_t *)&block->payload_size, 256);
            fat_set_dword((uint8_t *)&block->block_no, sector - FIRMWARE_UF2_SECTOR);
            fat_set_dword((uint8_t *)&block->num_blocks, FIRMWARE_UF2_SECTOR_NUM);
            fat_set_dword((uint8_t *)&block->magic_end, UF2_MAGIC_END);
        }
    }

    return 0;
}

int vfs_sector_write(uint32_t sector, const uint8_t *buf, uint32_t size)
{
    log("sector_write: %d, %08x, %d\n", sector, (uint32_t)buf, size);
    return 0;
}
