/*
 * grnlr.h
 *
 *  Created on: 11 nov. 2020
 *      Author: Laurent Fiack
 */

#ifndef INC_GRNLR_H_
#define INC_GRNLR_H_

#include "main.h"

#include "buffer.h"

#define DENSITY_ZERO_MARGIN 200
#define MAX_GRAIN 10
#define MAX_DELAY 30000
#define SPREAD_ZERO_MARGIN 100

// A grain is a playing head that plays back in the buffer for a short period of time
typedef struct grain_struct 
{
	uint8_t playing;
	uint32_t startPos;
	uint32_t iterator;
	uint32_t finalPos;
} grain_t;

typedef struct grnlr_param_struct
{
    float size;
	float position;
	float shape;
	float density;
	float feedback;
	float pitch;	// semitone
	float spread;
	uint8_t trigger;
	uint8_t freeze;
} grnlr_param_t;

typedef struct h_grnlr_struct
{
    h_buffer_t h_buffer;
    grain_t grain[MAX_GRAIN];
    uint16_t counter;
    grnlr_param_t param;
} h_grnlr_t;

void grnlr_init(h_grnlr_t * hg, float * buffer, uint32_t buffer_length);
void grnlr_process(h_grnlr_t * hg, float *buf, uint32_t n);

#endif /* INC_GRNLR_H_ */
