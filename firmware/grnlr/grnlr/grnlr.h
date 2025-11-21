/*
 * grnlr.h
 *
 *  Created on: 11 nov. 2020
 *      Author: Laurent Fiack
 */

#ifndef INC_GRNLR_H_
#define INC_GRNLR_H_

#include "main.h"

#define PARAM_SIZE_MIN 0.016f
#define PARAM_SIZE_MAX 1.0f
#define MAX_GRAINS 20
#define GRAIN_MAX_PERIOD 0xFFFFFFFF

// A grain is a playing head that plays back in the buffer for a short period of time
typedef struct grain_struct 
{
	uint8_t playing;
	uint32_t startPos;
	uint32_t iterator;
	uint32_t length;
	uint32_t shape;		// position of the max of the envelope
	float shape_incr;
	float shape_decr;
} grain_t;

typedef struct grnlr_param_struct
{
	float mix;
    float size;
	float position;
	float shape;
	float density;
	float feedback;
	float pitch;
	float spread;
	uint8_t trigger;
	uint8_t freeze;
} grnlr_param_t;

typedef struct h_grnlr_struct
{
	uint32_t audio_fs;
	uint32_t num_channels;
	uint32_t block_size;
	uint32_t buffer_size_s;
	uint32_t buffer_length;
	uint32_t buffer_length_blocks;

	uint32_t size_blocks;
	uint32_t position_blocks;
	uint32_t period_blocks;
	uint32_t time_to_next_grain;

	grnlr_param_t param;
	grain_t grain[MAX_GRAINS];

	uint32_t wr_ptr;
	volatile float * buffer;
} h_grnlr_t;

void grnlr_init(h_grnlr_t * hg, float * buffer, uint32_t audio_fs, uint32_t num_channels, uint32_t block_size, uint32_t buffer_size_s);
void grnlr_compute_params(h_grnlr_t * hg);
void grnlr_process(h_grnlr_t * hg, float *buf, uint32_t len);

#endif /* INC_GRNLR_H_ */
