#include "coder.h"

uint8_t coder_init(h_coder_t * h_coder)
{
    h_coder->pb_pressed = 0;
    h_coder->increment = 0;

    return 0;
}

uint8_t coder_is_pb_pressed(h_coder_t * h_coder)
{
    uint8_t pressed = h_coder->pb_pressed;

    if (h_coder->pb_pressed != 0)
    {
        h_coder->pb_pressed--;
    }

    return pressed;
}

int8_t coder_read_increment(h_coder_t * h_coder)
{
    int8_t increment = h_coder->increment;

    h_coder->increment = 0;

    return increment;
}

void coder_pb_pressed_cb(h_coder_t * h_coder)
{
    h_coder->pb_pressed++;
}

void coder_increment_cb(h_coder_t * h_coder, int8_t increment)
{
    h_coder->increment+=increment;
}