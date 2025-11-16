
#ifndef _BOARD_H
#define _BOARD_H

#include <stdint.h>
#include <stdbool.h>

void board_init();

bool board_check_PTT();
bool board_check_side_keys();
bool board_check_M_key();

void board_flashlight_on();
void board_flashlight_off();
void board_flashlight_flash(uint32_t delay);
void board_flashlight_update();

#endif
