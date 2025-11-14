/*
 * buffer.h
 *
 *  Created on: Nov 11, 2020
 *      Author: Laurent Fiack
 */

#ifndef INC_BUFFER_H_
#define INC_BUFFER_H_

/* Circular buffer management */

#include "main.h"
#include "sgtl5000.h"

typedef struct h_buffer_struct
{
    float * audio_buffer;
    uint32_t buffer_length;
    uint32_t iterator;
} h_buffer_t;

uint8_t buffer_init(h_buffer_t * hb, float * buffer, uint32_t buffer_length);
void buffer_push(h_buffer_t * hb, float sample);
float buffer_getReverse(h_buffer_t * hb, uint32_t xElement);
float buffer_getForward(h_buffer_t * hb, uint32_t xElement);
float buffer_getAbsolute(h_buffer_t * hb, uint32_t xElement);
uint32_t buffer_iteratorGet(h_buffer_t * hb);

#endif /* INC_BUFFER_H_ */
