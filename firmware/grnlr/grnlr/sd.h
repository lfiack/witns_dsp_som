#ifndef __SD_H
#define __SD_H

#include "main.h"
#include "parameter.h"

uint8_t sd_init(void);
uint8_t sd_ls(const char *path);
uint8_t sd_cat(const char *filename);
uint8_t sd_save_param(const char *filename, h_param_t * h_param);
uint8_t sd_load_param(const char *filename, h_param_t * h_param);

#endif // __SD_H