
#ifndef _USB_FS_H
#define _USB_FS_H

#include <stdint.h>

void vfs_get_cap(uint32_t *sector_num, uint16_t *sector_size);
int vfs_sector_read(uint32_t sector, uint8_t *buf, uint32_t size);
int vfs_sector_write(uint32_t sector, const uint8_t *buf, uint32_t size);

#endif