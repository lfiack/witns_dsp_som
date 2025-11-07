#ifndef __GRNLR_H
#define __GRNLR_H

#include "main.h"

#include "coder.h"
#include "sgtl5000.h"

extern h_coder_t h_coder;
extern h_sgtl5000_t h_sgtl5000;

uint8_t grnlr_init(void);
void grnlr_process(void);
void grnlr_process_audio(int16_t * in_buffer, int16_t * out_buffer, uint16_t len);

#endif // __GRNLR_H