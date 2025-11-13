#ifndef __PLAYBACK_H
#define __PLAYBACK_H

#include "main.h"

#include "stm32f4xx_ll_usb.h"
#include "tinywav.h"
#include "sgtl5000.h"

// #define NUM_CHANNELS 2
// #define SAMPLE_RATE 48000
#define BLOCK_SIZE 512
// #define DOUBLE_BUFFER 2

typedef struct h_playback_struct
{
    TinyWav tw;
    float samples[AUDIO_NUM_CHANNELS * BLOCK_SIZE * AUDIO_DOUBLE_BUFFER];
    uint32_t itr_block_wr;   // Next block to write
    uint32_t itr_block_rd;   // Current reading block
    uint16_t itr;           // Iterator in the block
    uint16_t block_size[AUDIO_DOUBLE_BUFFER];    // How many samples have been written
    uint8_t reading;        // Are we reading a file ?
    uint8_t block_empty;   // And need to be filled
    uint8_t eof;
} h_playback_t;

uint8_t playback_init(h_playback_t * pb);
uint8_t playback_play(h_playback_t * pb, const char * filename);
void playback_process(h_playback_t * pb);
void playback_process_audio(h_playback_t * pb, float *buf, uint32_t n);

#endif // __PLAYBACK_H