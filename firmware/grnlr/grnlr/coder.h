#ifndef __CODER_H
#define __CODER_H

#include "main.h"

typedef struct h_coder_struct
{
    uint8_t pb_pressed;
    int8_t increment;
} h_coder_t;

uint8_t coder_init(h_coder_t * h_coder);

uint8_t coder_is_pb_pressed(h_coder_t * h_coder);
int8_t coder_read_increment(h_coder_t * h_coder);

void coder_pb_pressed_cb(h_coder_t * h_coder);
void coder_increment_cb(h_coder_t * h_coder, int8_t increment);

#endif // __CODER_H