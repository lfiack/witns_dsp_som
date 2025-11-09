#include "vu.h"

// initialize a meter
// fs_hz: sample rate, block_size: number of samples processed at once (or 1), tau_ms: RMS time constant in ms
void vumeter_init(h_vumeter_t *m, float fs_hz, uint32_t block_size, float tau_ms,
                                float peak_decay_per_s, float hold_ms, float min_db)
{
    m->ms = 0.0f;
    m->peak = 0.0f;
    m->peak_hold = 0.0f;
    m->hold_cnt = 0;
    // calculate alpha for block-domain smoothing:
    // alpha = exp(-block_duration / tau)
    float block_dur_s = (float)block_size / fs_hz;
    float tau_s = tau_ms / 1000.0f;
    if (tau_s <= 0.0f) tau_s = 0.05f; // safety
    m->alpha = expf(-block_dur_s / tau_s);

    // peak decay per block:
    // peak_decay_per_s is factor per second (e.g. 0.5 -> halves per second),
    // convert to per-block multiplicative factor:
    if (peak_decay_per_s <= 0.0f) peak_decay_per_s = 0.5f;
    m->peak_decay = powf(peak_decay_per_s, block_dur_s);

    m->hold_blocks = (uint32_t)ceilf((hold_ms / 1000.0f) / block_dur_s);
    m->min_db = min_db;
}

// process a block of samples (float, interleaved mono buffer pointer, length samples)
// returns nothing, updates struct with new ms/peak/peak_hold
void vumeter_process_block_float(h_vumeter_t *m, const float *buf, uint32_t len)
{
    // compute block mean-square and peak
    float block_ms = 0.0f;
    float block_peak = 0.0f;
    for (uint32_t i = 0; i < len; ++i) {
        float s = buf[i];
        float abs_s = fabsf(s);
        if (abs_s > block_peak) block_peak = abs_s;
        block_ms += s * s;
    }
    block_ms = (len > 0) ? (block_ms / (float)len) : 0.0f;

    // smooth mean-square
    m->ms = m->alpha * m->ms + (1.0f - m->alpha) * block_ms;

    // update peak (instantaneous), then decay
    if (block_peak > m->peak) {
        m->peak = block_peak;
        // refresh hold
        m->peak_hold = m->peak;
        m->hold_cnt = m->hold_blocks;
    } else {
        // decay peak
        m->peak *= m->peak_decay;
        if (m->hold_cnt > 0) {
            m->hold_cnt--;
            if (m->hold_cnt == 0) {
                // clear hold only when expired; peak_hold follows decayed peak
                m->peak_hold = m->peak;
            }
        } else {
            // no hold active, peak_hold follows decayed peak
            m->peak_hold = m->peak;
        }
    }
}

// get dBFS from meter (approx). If rms==0 returns min_db.
float vumeter_get_dbfs(h_vumeter_t *m)
{
    float rms = sqrtf(m->ms);
    if (rms <= 1e-12f) return m->min_db;
    float db = 20.0f * log10f(rms);
    if (db < m->min_db) db = m->min_db;
    if (db > 0.0f) db = 0.0f;
    return db;
}

// get peak dBFS (uses peak_hold)
float vumeter_get_peak_dbfs(h_vumeter_t *m)
{
    float p = m->peak_hold;
    if (p <= 1e-12f) return m->min_db;
    float db = 20.0f * log10f(p);
    if (db < m->min_db) db = m->min_db;
    if (db > 0.0f) db = 0.0f;
    return db;
}

// map dB to pixels (0..pixels_max)
uint32_t db_to_pixels(float db, float min_db, uint32_t pixels_max)
{
    // db in [min_db .. 0]
    float norm = (db - min_db) / (-min_db); // 0..1
    if (norm < 0.0f) norm = 0.0f;
    if (norm > 1.0f) norm = 1.0f;
    return (uint32_t)(norm * (float)pixels_max + 0.5f);
}

// map dB to float (0..1)
float db_to_f(float db, float min_db)
{
    // db in [min_db .. 0]
    float norm = (db - min_db) / (-min_db); // 0..1
    return norm;
}

// Call this per audio buffer (float samples, interleaved stereo)
// inbuf: [L0, R0, L1, R1, ...] length = frames * 2
void vumeters_process_block_float_interleaved(h_vumeter_t *m, const float *inbuf, uint32_t frames)
{
    // temporary small arrays per channel (or process in-place)
    // For performance we directly accumulate per channel into temporaries:
    // We'll build mono arrays by processing interleaved on the fly.
    // For each meter, compute block mean-square and block peak quickly:
    float ms[2] = {0,0};
    float peak[2] = {0,0};

    for (uint32_t i=0;i<frames;i++) {
        float l = inbuf[2*i + 0];
        float r = inbuf[2*i + 1];

        ms[0] += l*l; 
        if (fabsf(l) > peak[0]) peak[0] = fabsf(l);
        ms[1] += r*r; 
        if (fabsf(r) > peak[1]) peak[1] = fabsf(r);
    }
    // left/right
    for (int ch=0; ch<2; ch++) {
        float block_ms = ms[ch] / (float)frames;
        // IIR update:
        m[ch].ms = m[ch].alpha * m[ch].ms + (1.0f - m[ch].alpha) * block_ms;
        // peak handling:
        if (peak[ch] > m[ch].peak) {
            m[ch].peak = peak[ch];
            m[ch].peak_hold = m[ch].peak;
            m[ch].hold_cnt = m[ch].hold_blocks;
        } else {
            m[ch].peak *= m[ch].peak_decay;
            if (m[ch].hold_cnt > 0) {
                m[ch].hold_cnt--;
                if (m[ch].hold_cnt == 0) m[ch].peak_hold = m[ch].peak;
            } else {
                m[ch].peak_hold = m[ch].peak;
            }
        }
    }
}