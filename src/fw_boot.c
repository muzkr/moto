#include "fw_boot.h"
#include <stdint.h>
#include "fw.h"
#include "log.h"
#include "py32f0xx.h"

void fw_boot()
{
    const uint32_t *vec = (uint32_t *)FW_ADDR;

    log("boot fw: vec = %08x, sp = %08x, reset handler = %08x\n", (uint32_t)vec, vec[0], vec[1]);

    uint32_t sp = vec[0];
    if (0 != sp % 2 || sp <= SRAM_BASE || sp > (1 + SRAM_END))
    {
        return;
    }

    uint32_t reset_handler = vec[1];
    if (0 == reset_handler % 2)
    {
        return;
    }
    reset_handler -= 1;
    if (reset_handler < FW_ADDR + 8 || reset_handler >= (1 + FLASH_END))
    {
        return;
    }

    extern void fw_boot0(const uint32_t *vec);
    fw_boot0(vec);
}
