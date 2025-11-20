/*
 * grnlr.c
 *
 *  Created on: 11 nov. 2020
 *      Author: Laurent Fiack
 */

#include "grnlr.h"

#include "main.h"
#include "stm32f4xx_hal_gpio.h"
#include "usart.h"
#include <stdio.h>
#include <string.h>

void grnlr_init(h_grnlr_t * hg, float * buffer, uint32_t audio_fs, uint32_t num_channels, uint32_t block_size, uint32_t buffer_size_s) {
	hg->param.mix = 0.0f;
	hg->param.size = 0.0f;
	hg->param.position = 0.0f;
	hg->param.shape = 0.0f;
	hg->param.density = 0.5f;
	hg->param.feedback = 0.0f;
	hg->param.pitch = 0.5f;	// semitone
	hg->param.spread = 0.0f;
	hg->param.trigger = 0;
	hg->param.freeze = 0;

	hg->audio_fs = audio_fs;
	hg->num_channels = num_channels;
	hg->block_size = block_size;
	hg->buffer_size_s = buffer_size_s;
	hg->buffer_length = buffer_size_s * num_channels * audio_fs;

	hg->block_counter = 0;
}

void grnlr_compute_params(h_grnlr_t * hg)
{
	// We want size, position and period to be multiples of audio blocks for simplicity

	float size_s = PARAM_SIZE_MIN + hg->param.size * (PARAM_SIZE_MAX - PARAM_SIZE_MIN);
	// printf("size = %fs\r\n", size_s);

	hg->size_blocks = size_s * hg->audio_fs / hg->block_size;

	printf("size = %lu blocks\r\n", hg->size_blocks);

	float position_s = size_s + hg->param.position * (hg->buffer_size_s - size_s);

	// printf("position = %fs\r\n", position_s);

	hg->position_blocks = position_s * hg->audio_fs / hg->block_size;

	printf("position = %lu blocks\r\n", hg->position_blocks);

	// 0Hz to MAX_GRAINS/size_s
	float freq_grain_hz = hg->param.density * (float)MAX_GRAINS / size_s;
	// printf("freq grain = %fhz\r\n", freq_grain_hz);

	if (freq_grain_hz > 0.0f)
	{
		// 1Hz => period = AUDIO_FS * NUM_CHANNELS
		hg->period_blocks = (hg->audio_fs / freq_grain_hz) / hg->block_size;
	}
	else 
	{
		hg->period_blocks = GRAIN_MAX_PERIOD;
	}

	printf("period = %lu blocks\r\n", hg->period_blocks);
}

// len takes into account the *2 due to stereo
void grnlr_process(h_grnlr_t * hg, float *buf, uint32_t len)
{
	// grnlr_compute_params(hg);


	// Should overflow in 66 days with 64 samples block size
	hg->block_counter++;
}
