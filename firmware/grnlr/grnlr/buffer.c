/*
 * buffer.c
 *
 *  Created on: Nov 11, 2020
 *      Author: Laurent Fiack
 */

#include "buffer.h"

//static uint16_t audio_buffer[BUFFER_LENGTH];


uint8_t buffer_init(h_buffer_t * hb, float * buffer, uint32_t buffer_length)
{
	hb->audio_buffer = buffer;
	hb->buffer_length = buffer_length;
	hb->iterator = 0;

	for (int i = 0 ; i < buffer_length ; i++)
	{
		buffer[buffer_length] = 0.0f;
	}
	return 0;
}

void buffer_push(h_buffer_t * hb, float sample) {
	hb->iterator++;
	hb->iterator%=hb->buffer_length * 2;	// 2 channels

	hb->audio_buffer[hb->iterator] = sample;

}

float buffer_getReverse(h_buffer_t * hb, uint32_t xElement) {
	xElement = (hb->iterator + (hb->buffer_length * 2)) - xElement;
	xElement %= (hb->buffer_length * 2);

	return hb->audio_buffer[xElement];
}

float buffer_getForward(h_buffer_t * hb, uint32_t xElement) {
	xElement = xElement + hb->iterator + 1;
	xElement %= hb->buffer_length * 2;

	return hb->audio_buffer[xElement];
}

float buffer_getAbsolute(h_buffer_t * hb, uint32_t xElement) {
	return hb->audio_buffer[xElement % (hb->buffer_length * 2)];
}

uint32_t buffer_iteratorGet(h_buffer_t * hb) {
	return hb->iterator;
}
