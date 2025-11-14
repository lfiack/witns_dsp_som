/*
 * buffer.c
 *
 *  Created on: Nov 11, 2020
 *      Author: Laurent Fiack
 */

#include "buffer.h"

static uint16_t audio_buffer[BUFFER_LENGTH];

static uint32_t iterator = 0;

void buffer_push(uint16_t xSample) {
	iterator++;
	iterator%=BUFFER_LENGTH;

	audio_buffer[iterator] = xSample;
}

uint16_t buffer_getReverse(uint32_t xElement) {
	xElement = (iterator+BUFFER_LENGTH) - xElement;
	xElement %= BUFFER_LENGTH;

	return audio_buffer[xElement];
}

uint16_t buffer_getForward(uint32_t xElement) {
	xElement = xElement+iterator+1;
	xElement %= BUFFER_LENGTH;

	return audio_buffer[xElement];
}

uint16_t buffer_getAbsolute(uint32_t xElement) {
	return audio_buffer[xElement%BUFFER_LENGTH];
}

uint16_t buffer_iteratorGet() {
	return iterator;
}
