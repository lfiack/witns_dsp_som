#ifndef __STATS_H
#define __STATS_H

#include "main.h"
#include "tim.h"

// TIM6: APB1 = 90MHz
// fMeasure = AUDIO_FS / AUDIO_BUFFER_LENGTH
// 48kHz / 64 = 750
// Counter from 0 to 1000 in one block of AUDIO_BUFFER_LENGTH
// ARR*PSC = 90MHz / 750 = 120000
// PSC = 120

typedef struct h_stats_struct 
{
    TIM_HandleTypeDef * htim;
    volatile uint32_t period;
    volatile uint32_t duration;
    uint8_t usage;
    uint8_t max_usage;
} h_stats_t;

uint8_t stats_start(h_stats_t * st);
uint8_t stats_process(h_stats_t * st);
uint8_t stats_usage(h_stats_t * st);
uint8_t stats_max_usage(h_stats_t * st);

#endif // __STATS_H