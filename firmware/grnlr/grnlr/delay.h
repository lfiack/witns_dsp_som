#ifndef __DELAY_H
#define __DELAY_H

#include "main.h"

#include <math.h>
#include <stdint.h>
#include <string.h>

#define MAX_DELAY_SEC   3

typedef struct h_delay_struct 
{
    float *buffer;       // Circular buffer
    uint32_t size;       // Buffer length in samples
    uint32_t write_idx;  // Current write position
    float feedback;      // Feedback [0.0, 0.99]
    float mix;           // Wet/Dry mix [0.0, 1.0]
    float delay;    // Delay %
    float delay_time;    // Delay time (in seconds)
    uint32_t delay_samples; // Delay time (in samples)
    uint32_t sample_rate;   // Sample rate (Hz)
    uint8_t channels;    // 1 = mono, 2 = stereo
} h_delay_t;

void delay_init(h_delay_t *d, float *buffer, uint32_t buffer_size, uint32_t sample_rate, uint8_t channels);
void delay_process_block(h_delay_t *d, float *buf, uint32_t n);

#endif // __DELAY_H