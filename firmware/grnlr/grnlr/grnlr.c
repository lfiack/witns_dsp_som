/*
 * grnlr.c
 *
 *  Created on: 11 nov. 2020
 *      Author: Laurent Fiack
 *
 *      DAC on A3
 *      ADC on A2
 */

#include "grnlr.h"

#include "buffer.h"
// #include "parameters.h"

#include "usart.h"
#include <stdio.h>
#include <string.h>

#define DENSITY_ZERO_MARGIN 200
#define MAX_GRAIN 10
#define MAX_DELAY 30000
#define SPREAD_ZERO_MARGIN 100

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

typedef struct {
	uint8_t playing;
	uint16_t startPos;
	uint32_t iterator;
	uint16_t finalPos;
} grain_t;

static grain_t grain[MAX_GRAIN];
static uint16_t counter = 0;

//static char print_buf[10];

/* Compute the envelope and apply it to the incoming sample
 * Return the processed sample
 */
int32_t grnlr_shaper(int32_t sSample, uint16_t xShape, uint32_t xIterator, uint16_t xSize) {
	uint32_t sMax = ((xShape%1024) * (xSize/2)) / 1024;
	int32_t sEnvelope = 0;

	// Ramp down to Triangle
	if (xShape < 1024) {
		if (xIterator < sMax) {
			sEnvelope = (xIterator * 4096)/sMax;
		}
		else {
			sEnvelope = ((xSize - xIterator) * 4096) / (xSize - sMax);
		}
	}
	// Triangle to Square
	else if (xShape < 2048) {
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
	else if (xShape < 3072) {
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
	else if (xShape < 4096) {
		sMax += (xSize/2);

		if (xIterator < sMax) {
			sEnvelope = (xIterator * 4096)/sMax;
		}
		else {
			sEnvelope = ((xSize - xIterator) * 4096) / (xSize - sMax);
		}
	}

//	sprintf(print_buf, "i=%d ", xIterator);
//	HAL_UART_Transmit(&huart2, (uint8_t*) print_buf, strlen(print_buf), HAL_MAX_DELAY);
//
//	sprintf(print_buf, "e=%d s=", sEnvelope);
//	HAL_UART_Transmit(&huart2, (uint8_t*) print_buf, strlen(print_buf), HAL_MAX_DELAY);

	sSample = sSample * sEnvelope;
	sSample = sSample / 4096;

	return sSample;
}

/* Find an available grain, configure the start and the end position
 * Initialize the iterator (playhead position) and start it
 */
void grnlr_startGrain(uint16_t xPosition, uint16_t xSize) {
	static uint16_t grainIterator = 0;

	int i = 0;

	while(grain[grainIterator].playing == 1 && i < MAX_GRAIN) {
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
		grain[grainIterator].playing = 1;
		grain[grainIterator].startPos = (xPosition+buffer_iteratorGet())%BUFFER_LENGTH;
		grain[grainIterator].iterator = 0;
		grain[grainIterator].finalPos = grain[grainIterator].startPos + xSize;
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

void grnlr_init(void) {
	for (int i = 0 ; i < MAX_GRAIN ; i++) {
		grain[i].playing = 0;
		grain[i].startPos = 0;
		grain[i].iterator = 0;
		grain[i].finalPos = 0;
	}
}

uint16_t grnlr_process(uint16_t xSample) {
	uint16_t rSample = 0;
	int32_t sSample = 0;
	//	uint32_t writtenSmple = buffer_get(parameters_positionGet());
	//	writtenSmple *= parameters_feedbackGet();
	//	writtenSmple /= 4096;
	//	writtenSmple += xSample;
	//	buffer_push(writtenSmple);

	//	return buffer_get(parameters_positionGet());

	// uint16_t pSize = parameters_sizeGet();
	// uint16_t pPosition = parameters_positionGet();
	// uint16_t pShape = parameters_shapeGet();
	// uint16_t pDensity = parameters_densityGet();
	// uint16_t pFeedback = parameters_feedbackGet();
	// uint16_t pPitch = parameters_pitchGet();
	// uint16_t pSpread = parameters_spreadGet();
	uint16_t pSize;
	uint16_t pPosition;
	uint16_t pShape;
	uint16_t pDensity;
	uint16_t pFeedback;
	uint16_t pPitch;
	uint16_t pSpread;
	uint8_t pTrigger;
	uint8_t pFreeze;
	int16_t semitone;

	uint16_t absDensity;
	static uint16_t delay = 0;

	pSize = (pSize * (BUFFER_LENGTH-1))/4095;
	pPosition = (pPosition * (BUFFER_LENGTH-1))/4095;
	pSpread = (pSpread * (BUFFER_LENGTH-1))/4095;

	if (pSpread > SPREAD_ZERO_MARGIN) {
//		sprintf(print_buf, "%d ", pPosition);
//		HAL_UART_Transmit(&huart2, (uint8_t*) print_buf, strlen(print_buf), HAL_MAX_DELAY);

		pPosition += grnlr_random(0, pSpread);
		pPosition %= BUFFER_LENGTH;

//		sprintf(print_buf, "%d\r\n", pPosition);
//		HAL_UART_Transmit(&huart2, (uint8_t*) print_buf, strlen(print_buf), HAL_MAX_DELAY);
	}

	if (pTrigger) {
		pTrigger = 0;
		grnlr_startGrain(pPosition, pSize);
//		HAL_UART_Transmit(&huart2, (uint8_t*)"t\r\n", 3, HAL_MAX_DELAY);
	}

//	static int16_t old_semitone = 0;
//	if (semitone != old_semitone) {
//		sprintf(print_buf, "st=%d ", semitone);
//		HAL_UART_Transmit(&huart2, (uint8_t*) print_buf, strlen(print_buf), HAL_MAX_DELAY);
//		sprintf(print_buf, "mn=%d\r\n", magic_numbers[semitone]);
//		HAL_UART_Transmit(&huart2, (uint8_t*) print_buf, strlen(print_buf), HAL_MAX_DELAY);
//		old_semitone = semitone;
//	}

	// Density button turned left: constant delay between triggers
	if (pDensity < (2048-DENSITY_ZERO_MARGIN)) {
//		static uint16_t delay_old = 0;
		uint16_t delay = (pSize / MAX_GRAIN) + (pDensity * MAX_DELAY / 2048);

		if (counter > delay) {
			counter = 0;
			grnlr_startGrain(pPosition, pSize);
		}
		else {
			counter++;
		}

//		if (delay != delay_old) {
//			sprintf(print_buf, "d=%d\r\n", delay);
//			HAL_UART_Transmit(&huart2, (uint8_t*) print_buf, strlen(print_buf), HAL_MAX_DELAY);
//			delay_old = delay;
//		}
	}
	// Density button turned right: random delay between triggers
	else if (pDensity > (2048+DENSITY_ZERO_MARGIN)) {
		if (counter > delay) {
			absDensity = 4095-pDensity;
			delay = (pSize / MAX_GRAIN) + (absDensity * MAX_DELAY / 2048);
			delay += grnlr_random(-(delay/2), delay/2);

			counter = 0;
			grnlr_startGrain(pPosition, pSize);
		}
		else {
			counter++;
		}
	}
	// Density button on the middle: no trigger
	else {
		counter = 0;
	}

	int grainPlaying = 0;

	for (int i = 0 ; i < MAX_GRAIN ; i++) {
		if (grain[i].playing == 1) {
			if (grain[i].startPos+(grain[i].iterator/ITERATOR_FACTOR) < grain[i].finalPos) {
//				rSample = buffer_getForward(grain[i].startPos+grain[i].iterator);
				rSample = buffer_getAbsolute(grain[i].startPos+(grain[i].iterator/ITERATOR_FACTOR));

				sSample += grnlr_shaper(((int32_t)rSample) - 2048, pShape, grain[i].iterator/ITERATOR_FACTOR, pSize);

				grain[i].iterator+=magic_numbers[semitone];

				grainPlaying = 1;
			}
			else {
				grain[i].playing = 0;
			}
		}
	}

	if (!grainPlaying) {
//		HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
	}

	if (sSample < -2048) sSample = -2048;
	else if (sSample > 2047) sSample = 2047;
	rSample = (uint16_t)(sSample+2048);

	if (pFreeze == 0) {
		int32_t sInSample = (int32_t)(xSample)-2048;
		sInSample += ((sSample * (int32_t)pFeedback)/4096);
		if (sInSample < -2048) sInSample = -2048;
		else if (sInSample > 2047) sInSample = 2047;
		buffer_push((uint16_t)(sInSample+2048));
	}

//	if (sSample != 0) {
//		sprintf(print_buf, "%d\r\n", sSample);
//		HAL_UART_Transmit(&huart2, (uint8_t*) print_buf, strlen(print_buf), HAL_MAX_DELAY);
//	}

	return rSample;
}
