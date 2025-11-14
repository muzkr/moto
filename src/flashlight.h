
#ifndef _FLASHLIGHT_H
#define _FLASHLIGHT_H

#include <stdint.h>

void flashlight_init();
void flashlight_on();
void flashlight_off();
void flashlight_flash(uint32_t delay);
void flashlight_update();

#endif
