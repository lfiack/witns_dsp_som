#ifndef __VUMETER_H
#define __VUMETER_H

#include "main.h"

#include <stdint.h>
#include <math.h>

typedef struct h_vumeter_struct {
    // internal envelope (mean square)
    float ms;          // smoothed mean-square value
    float peak;        // current peak (linear 0..1)
    float peak_hold;   // held peak value (linear 0..1)
    uint32_t hold_cnt; // remaining hold ticks
    // parameters
    float alpha;       // IIR factor for RMS (block-domain)
    float peak_decay;  // per-block decay factor for peak
    uint32_t hold_blocks; // number of blocks to hold peak
    float min_db;      // bottom dB for display (e.g. -60)
} h_vumeter_t;

void vumeter_init(h_vumeter_t *m, float fs_hz, uint32_t block_size, float tau_ms,
                                float peak_decay_per_s, float hold_ms, float min_db);
void vumeters_process_block_float_interleaved(h_vumeter_t *m, const float *inbuf, uint32_t frames);
void vumeter_process_block_float(h_vumeter_t *m, const float *buf, uint32_t len);
float vumeter_get_dbfs(h_vumeter_t *m);
float vumeter_get_peak_dbfs(h_vumeter_t *m);
uint32_t db_to_pixels(float db, float min_db, uint32_t pixels_max);
float db_to_f(float db, float min_db);

#endif // __VUMETER_H