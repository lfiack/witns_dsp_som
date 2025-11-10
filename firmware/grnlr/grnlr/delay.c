#include "delay.h"

/**
 * Initialize delay line.
 * 'max_delay_sec' defines the buffer length allocated (maximum possible delay).
 */
void delay_init(h_delay_t *d, float *buffer, uint32_t buffer_size, uint32_t sample_rate, uint8_t channels)
{
    d->buffer = buffer;
    d->size = buffer_size;
    d->write_idx = 0;
    d->feedback = 0.3f;
    d->mix = 0.0f;
    d->delay = 0.3f;
    d->sample_rate = sample_rate;
    d->channels = channels;
    d->delay_samples = (uint32_t)(d->delay_time * sample_rate) * channels;
}

/**
 * Set delay time (in seconds)
 */
void delay_compute_time(h_delay_t * d)
{
    d->delay_time = d->delay * MAX_DELAY_SEC;
    d->delay_samples = (uint32_t)(d->delay_time * d->sample_rate) * d->channels;
    if (d->delay_samples >= d->size)
        d->delay_samples = d->size - d->channels; // avoid overflow
}

/**
 * Process a block of interleaved float samples
 */
void delay_process_block(h_delay_t *d, float *buf, uint32_t n)
{
    delay_compute_time(d);
    for (uint32_t i = 0; i < n; i++) {
        uint32_t read_idx = (d->write_idx + d->size - d->delay_samples) % d->size;

        // Read delayed sample
        float delayed = d->buffer[read_idx];

        // Write input + feedback
        d->buffer[d->write_idx] = buf[i] + delayed * d->feedback;

        // Mix output
        buf[i] = buf[i] * (1.0f - d->mix) + delayed * d->mix;

        // Increment circular pointers
        d->write_idx++;
        if (d->write_idx >= d->size)
            d->write_idx = 0;
    }
}