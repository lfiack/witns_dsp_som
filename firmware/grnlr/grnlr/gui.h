#ifndef __GUI_H
#define __GUI_H

#include "main.h"

uint8_t gui_init(void);
uint8_t gui_update_levels(float *vol, float *peak);
uint8_t gui_display_select_arrows(void);
uint8_t gui_display_edit_arrows(void);
uint8_t gui_display_float(float param);
uint8_t gui_display_bool(uint8_t param);
uint8_t gui_display_func_run(void);
uint8_t gui_display_str(const char * str);
uint8_t gui_display_name(const char * name);

void gui_tests(void);

#endif // __GUI_H