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

	hg->wr_ptr = 0;
	hg->buffer = buffer;

	hg->time_to_next_grain = 0;

	hg->audio_fs = audio_fs;
	hg->num_channels = num_channels;
	hg->block_size = block_size;
	hg->buffer_size_s = buffer_size_s;
	hg->buffer_length = (buffer_size_s * num_channels * audio_fs);
	hg->buffer_length -= (hg->buffer_length % (block_size * num_channels));	// Make sure buffer_length is multiple of block_size * num_channels
	hg->buffer_length_blocks = hg->buffer_length / (block_size * num_channels);

	// Initializing the buffer
	for (int i = 0 ; i < hg->buffer_length ; i++)
	{
		// // 1kHz sawtooth
		// buffer[i] = (((i % 960) / 480.0f) - 1.0f) * 0.2f;
		buffer[i] = 0.0f;
	}
}

void grnlr_compute_params(h_grnlr_t * hg)
{
	// We want size, position and period to be multiples of audio blocks for simplicity

	float size_s = PARAM_SIZE_MIN + hg->param.size * (PARAM_SIZE_MAX - PARAM_SIZE_MIN);
	// printf("size = %fs\r\n", size_s);

	hg->size_blocks = size_s * hg->audio_fs / hg->block_size;

	// printf("size = %lu blocks\r\n", hg->size_blocks);

	float position_s = size_s + hg->param.position * (hg->buffer_size_s - size_s);

	// printf("position = %fs\r\n", position_s);

	hg->position_blocks = position_s * hg->audio_fs / hg->block_size;

	// printf("position = %lu blocks\r\n", hg->position_blocks);

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

	// printf("period = %lu blocks\r\n", hg->period_blocks);
}

static void grnlr_start_grain(h_grnlr_t * hg)
{
	// find an empty grain
	for (int i = 0 ; i < MAX_GRAINS ; i++)
	{
		if (0 == hg->grain[i].playing)
		{
			hg->grain[i].playing = 1;

			// writing pointer - position ; beware of negatives
			hg->grain[i].startPos = (hg->buffer_length_blocks + hg->wr_ptr - hg->position_blocks) % hg->buffer_length_blocks;
			hg->grain[i].length = hg->size_blocks;
			hg->grain[i].iterator = 0;
			hg->grain[i].shape = hg->param.shape * hg->grain[i].length;
			hg->grain[i].shape_incr = 1.0f;
			hg->grain[i].shape_decr = 1.0f;
			if (hg->grain[i].shape != 0)
			{
				hg->grain[i].shape_incr = 1.0f / ((hg->block_size * hg->num_channels) * (float)hg->grain[i].shape);
			}
			if (hg->grain[i].shape < hg->grain[i].length)
			{
				hg->grain[i].shape_decr = 1.0f / ((hg->block_size * hg->num_channels) * (float)(hg->grain[i].length-hg->grain[i].shape));
			}

			// printf("shape = %lu ; incr = %f ; decr = %f\r\n", hg->grain[i].shape, hg->grain[i].shape_incr, hg->grain[i].shape_decr);

			return;
		}
	}

	// if no empty grain was found, just don't play one
	printf("No memory for new grain\r\n");	// TODO just for debug
}

static float grnlr_get_shape_factor(h_grnlr_t * hg, uint32_t gr, uint32_t itr)
{
	// Ascending envelope
	if (hg->grain[gr].iterator < hg->grain[gr].shape)
	{
		return hg->grain[gr].shape_incr * (float)(hg->grain[gr].iterator * hg->block_size * hg->num_channels + itr);
	}
	else 	// Descending envelope
	{
		return (1.0f - hg->grain[gr].shape_decr * (float)((hg->grain[gr].iterator - hg->grain[gr].shape) * hg->block_size * hg->num_channels + itr));
	}
}

// len takes into account the *2 due to stereo
void grnlr_process(h_grnlr_t * hg, float *buf, uint32_t len)
{
	// TODO get rid of magic numbers :/
	static float mix[64*2];
	grnlr_compute_params(hg);

	// If density is not zero, a grain might start
	if (GRAIN_MAX_PERIOD != hg->period_blocks)
	{
		hg->time_to_next_grain++;

		// It's time to start a new grain
		if (hg->time_to_next_grain >= hg->period_blocks)
		{
			hg->time_to_next_grain = 0;

			grnlr_start_grain(hg);
		}
	}

	// Playback the grains
	for (uint32_t i = 0 ; i < len ; i++)
	{
		mix[i] = 0.0f;

		for (uint32_t gr = 0 ; gr < MAX_GRAINS ; gr++)
		{
			if (hg->grain[gr].playing)
			{
				// Playing the grains and mixing them
				float shape_factor = grnlr_get_shape_factor(hg, gr, i);
				uint32_t itr = ((hg->grain[gr].startPos + hg->grain[gr].iterator) * hg->block_size * hg->num_channels + i) % hg->buffer_length;
				mix[i] += (shape_factor * hg->buffer[itr]);
			}
		}

		// Stores in the grnlr buffer
		if (0 == hg->param.freeze)
		{
			hg->buffer[hg->wr_ptr * hg->block_size * hg->num_channels + i] = buf[i] + hg->param.feedback * mix[i];
		}

		// Fill the output buffer
		buf[i] = (1 - hg->param.mix) * buf[i] + hg->param.mix * mix[i];
	}

	// Incrementing the grains
	for (uint32_t gr = 0 ; gr < MAX_GRAINS ; gr++)
	{
		if (hg->grain[gr].playing)
		{
			hg->grain[gr].iterator++;
			if (hg->grain[gr].length == hg->grain[gr].iterator)
			{
				hg->grain[gr].playing = 0;
			}
		}
	}

	// Incrementing the granuar buffer write pointer
	if (0 == hg->param.freeze)
	{
		hg->wr_ptr++;

		if (hg->wr_ptr == hg->buffer_length_blocks)
		{
			hg->wr_ptr = 0;
		}
	}
}
