#ifndef __GUI_H
#define __GUI_H

#include "main.h"

uint8_t gui_init(void);
uint8_t gui_update_levels(float in_left, float in_right, float out_left, float out_right);
uint8_t gui_display_select_arrows(void);
uint8_t gui_display_edit_arrows(void);

void gui_tests(void);

#endif // __GUI_H