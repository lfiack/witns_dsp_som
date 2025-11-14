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

#define BUFFER_LENGTH 5000

void buffer_push(uint16_t xSample);
uint16_t buffer_getReverse(uint32_t xElement);
uint16_t buffer_getForward(uint32_t xElement);
uint16_t buffer_getAbsolute(uint32_t xElement);
uint16_t buffer_iteratorGet();

#endif /* INC_BUFFER_H_ */
