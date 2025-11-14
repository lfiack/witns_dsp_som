/*
 * grnlr.c
 *
 *  Created on: 11 nov. 2020
 *      Author: Laurent Fiack
 */

#include "grnlr.h"

#include "buffer.h"
// #include "parameters.h"

#include "usart.h"
#include <stdio.h>
#include <string.h>

/* To increase the pitch we want to skip some sample
 * To lower the pitch we want to output several times the same sample
 * (We could do some interpolation but eh...)
 * I don't want to use float (why not?),
 * so I multiply the grain sample iterator (playhead) by a big value
 * and divide it by a magic number depending on the semitone thanks to the following array
 */
#define ITERATOR_FACTOR 10000
static const uint32_t magic_numbers[49] = {
		2500, 2648, 2806, 2973,	3149, 3337, 3535, 3745, 3968, 4204, 4454, 4719,
		5000, 5297, 5612, 5946, 6299, 6674, 7071, 7491, 7937, 8408, 8908, 9438,
		10000, 10594, 11224, 11892, 12599, 13348, 14142, 14983, 15874, 16817, 17817, 18877,
		20000, 21189, 22449, 23784, 25198, 26696, 28284, 29966, 31748, 33635, 35635, 37754,
		40000
};

//static char print_buf[10];

/* Compute the envelope and apply it to the incoming sample
 * Return the processed sample
 */
float grnlr_shaper(float sSample, float xShape, uint32_t xIterator, uint32_t xSize) {
	// There's ancient black magic that I don't want to touch here
	uint16_t sShape = (uint16_t)(xShape * 4096);
	uint32_t sMax = ((sShape%1024) * (xSize/2)) / 1024;
	int32_t sEnvelope = 0;

	// Ramp down to Triangle
	if (sShape < 1024) {
		if (xIterator < sMax) {
			sEnvelope = (xIterator * 4096)/sMax;
		}
		else {
			sEnvelope = ((xSize - xIterator) * 4096) / (xSize - sMax);
		}
	}
	// Triangle to Square
	else if (sShape < 2048) {
		if (xIterator < (xSize/2)-sMax) {
			sEnvelope = (xIterator * 4096)/((xSize/2)-sMax);
		}
		else if (xIterator < (xSize/2) + sMax) {
			sEnvelope = 4096;
		}
		else {
			sEnvelope = ((xSize - xIterator) * 4096)/((xSize/2)-sMax);
		}
	}
	// Square to Triangle again
	else if (sShape < 3072) {
		sMax = (xSize/2) - sMax;
		if (xIterator < (xSize/2)-sMax) {
			sEnvelope = (xIterator * 4096)/((xSize/2)-sMax);
		}
		else if (xIterator < (xSize/2) + sMax) {
			sEnvelope = 4096;
		}
		else {
			sEnvelope = ((xSize - xIterator) * 4096)/((xSize/2)-sMax);
		}
	}
	// Triangle to ramp up
	else if (sShape < 4096) {
		sMax += (xSize/2);

		if (xIterator < sMax) {
			sEnvelope = (xIterator * 4096)/sMax;
		}
		else {
			sEnvelope = ((xSize - xIterator) * 4096) / (xSize - sMax);
		}
	}

	sSample = sSample * sEnvelope;
	sSample = sSample / 4096.0f;

	return sSample;
}

/* Find an available grain, configure the start and the end position
 * Initialize the iterator (playhead position) and start it
 */
void grnlr_startGrain(h_grnlr_t * hg, uint16_t xPosition, uint16_t xSize) {
	static uint16_t grainIterator = 0;

	int i = 0;

	while(hg->grain[grainIterator].playing == 1 && i < MAX_GRAIN) {
		if (grainIterator < MAX_GRAIN-1) {
			grainIterator++;
		}
		else {
			grainIterator = 0;
		}
		i++;
	}
//	sprintf(print_buf, "%d %d\r\n", grainIterator, i);
//	HAL_UART_Transmit(&huart2, (uint8_t*) print_buf, strlen(print_buf), HAL_MAX_DELAY);
	if (i < MAX_GRAIN) {
		hg->grain[grainIterator].playing = 1;
		hg->grain[grainIterator].startPos = (xPosition+buffer_iteratorGet(&hg->h_buffer)) % hg->h_buffer.buffer_length;
		hg->grain[grainIterator].iterator = 0;
		hg->grain[grainIterator].finalPos = hg->grain[grainIterator].startPos + xSize;
//		HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
	}
}

int32_t grnlr_random(int32_t min, int32_t max) {
    static uint32_t lfsr = 0xACE1CAFE;
    int32_t sRandom;
    int32_t range = max - min;
    int32_t offset = min;

    int32_t a, b, c, d, msb;

    a = lfsr & 1;
    b = (lfsr >> 2) & 1;
    c = (lfsr >> 3) & 1;
    d = (lfsr >> 5) & 1;

    msb = (a^b)^(c^d);

    lfsr >>= 1;
    lfsr |= (msb<<31);


    sRandom = (int32_t)lfsr;
    sRandom /= (0xFFFFFFFF/range);
    sRandom += offset;

    return sRandom;
}

void grnlr_init(h_grnlr_t * hg, float * buffer, uint32_t buffer_length) {
	hg->counter = 0;
	buffer_init(&(hg->h_buffer), buffer, buffer_length);

	hg->param.size = 0.0f;
	hg->param.position = 0.0f;
	hg->param.shape = 0.0f;
	hg->param.density = 0.5f;
	hg->param.feedback = 0.0f;
	hg->param.pitch = 0.5f;	// semitone
	hg->param.spread = 0.0f;
	hg->param.trigger = 0;
	hg->param.freeze = 0;

	for (int i = 0 ; i < MAX_GRAIN ; i++) {
		hg->grain[i].playing = 0;
		hg->grain[i].startPos = 0;
		hg->grain[i].iterator = 0;
		hg->grain[i].finalPos = 0;
	}
}

void grnlr_process(h_grnlr_t * hg, float *buf, uint32_t n)
{
	// uint16_t pShape = 0;
	uint16_t pDensity = hg->param.density * 4096;
	// uint16_t pPitch = 0;	// semitone
	// uint16_t pSpread = 0;

	uint16_t absDensity;

	// Size of the grain in samples (L+R)
	uint32_t pSize = ((uint32_t)(hg->param.size * (hg->h_buffer.buffer_length)) - 1);

	// Position of where the grain is played. Should be pair for L+R alignment
	uint32_t pPosition = ((uint32_t)(hg->param.position * (hg->h_buffer.buffer_length)) - 1) & 0xFFFFFFFE;

	// Where the grains can randomly be around the position
	uint32_t pSpread = (uint32_t)(hg->param.spread * (hg->h_buffer.buffer_length-1));

	// TODO add random later
	// if (pSpread > SPREAD_ZERO_MARGIN) {
	// 	pPosition += grnlr_random(0, pSpread);
	// 	pPosition %= hg->h_buffer.buffer_length;
	// }

	if (hg->param.trigger) {
		// printf("trig\r\n");
		hg->param.trigger = 0;
		grnlr_startGrain(hg, pPosition, pSize);
	}

	// TODO no density for now, just manual trigger
	// Density button turned left: constant delay between triggers
	if (pDensity < (2048-DENSITY_ZERO_MARGIN)) {
		// uint16_t delay = (pSize / MAX_GRAIN) + (pDensity * MAX_DELAY / 2048);
		uint16_t delay = pDensity;

		if (hg->counter > delay) {
			hg->counter = 0;
			grnlr_startGrain(hg, pPosition, pSize);
		}
		else {
			hg->counter++;
		}
	}

	// // Density button turned right: random delay between triggers
	// else if (pDensity > (2048+DENSITY_ZERO_MARGIN)) {
	// 	if (hg->counter > delay) {
	// 		absDensity = 4095-pDensity;
	// 		delay = (pSize / MAX_GRAIN) + (absDensity * MAX_DELAY / 2048);
	// 		delay += grnlr_random(-(delay/2), delay/2);

	// 		hg->counter = 0;
	// 		grnlr_startGrain(hg, pPosition, pSize);
	// 	}
	// 	else {
	// 		hg->counter++;
	// 	}
	// }

	// // Density button on the middle: no trigger
	// else {
	// 	hg->counter = 0;
	// }

	for (uint32_t i = 0; i < n; i++) {
		float grain_sample = 0.0f;

		// Playing back the grains
		for (int g = 0 ; g < MAX_GRAIN ; g++) {
			if (hg->grain[g].playing == 1) {
				if (hg->grain[g].startPos + (hg->grain[g].iterator) < hg->grain[g].finalPos) {
	//				rSample = buffer_getForward(grain[i].startPos+grain[i].iterator);
					float spl = buffer_getAbsolute(&hg->h_buffer, hg->grain[g].startPos + hg->grain[g].iterator);
					grain_sample += grnlr_shaper(spl, hg->param.shape, hg->grain[g].iterator, pSize);

					// TODO No pitch shift for now
//					hg->grain[i].iterator+=magic_numbers[pPitch];
					hg->grain[g].iterator++;
				}
				else {
					hg->grain[g].playing = 0;
				}
			}
		}

		if (hg->param.freeze == 0) {
			float in_spl = buf[i] + grain_sample * hg->param.feedback;
			if (in_spl > 1.0f) in_spl = 1.0f;
			if (in_spl < -1.0f) in_spl = -1.0f;
			buffer_push(&hg->h_buffer, in_spl);
		}

		buf[i] += grain_sample;

		// Dummy playback
		// static uint32_t itr = 0;

		// buf[i] = buffer_getAbsolute(&hg->h_buffer, itr);
		// itr++;
		// itr = itr % hg->h_buffer.buffer_length;
	}
}
