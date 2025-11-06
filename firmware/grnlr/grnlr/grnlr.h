#ifndef __GRNLR_H
#define __GRNLR_H

#include "main.h"

#include "coder.h"

extern h_coder_t h_coder;

uint8_t grnlr_init(void);
void grnlr_process(void);

#endif // __GRNLR_H