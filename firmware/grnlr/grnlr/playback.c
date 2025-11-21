#include "playback.h"
#include "main.h"
#include "sgtl5000.h"
#include "stm32f4xx_hal_gpio.h"

uint8_t playback_init(h_playback_t *pb)
{
    pb->start_reading = osSemaphoreNew(1, 0, NULL);

    pb->itr_block_wr = 0;
    pb->itr_block_rd = 0;
    pb->itr = 0;
    pb->eof = 0;
    pb->reading = 0;

    return 0;
}

uint8_t playback_play(h_playback_t * pb, const char * filename)
{
    if (pb->reading == 0)
    {
        printf("play\r\n");
        int ret = tinywav_open_read (
            &pb->tw, 
            filename,
            TW_INTERLEAVED // the samples will be delivered by the read function in interleaved e.g. [LRLRLRLR]
        );
        if (ret == -1)
        {
            printf("error opening file\r\n");
            return 1;
        }
        // Fill the double buffer
        // Might change later to decrease playback latency
        ret = tinywav_read_f(&pb->tw, pb->samples, BLOCK_SIZE * AUDIO_DOUBLE_BUFFER);

        pb->block_size[0] = 512;
        pb->block_size[1] = 512;

        // for (int i = 0 ; i < BLOCK_SIZE * AUDIO_DOUBLE_BUFFER ; i++)
        // {
        //     printf("%d %f\r\n", i, pb->samples[i]);
        // }

        printf("playing the best song in the world\r\n");

        pb->reading = 1;
        pb->itr = 0;
        pb->eof = 0;
        pb->itr_block_rd = 0;
        pb->itr_block_wr = 2;   // We filled block 0 and 1 so next is 2

        // osEventFlagsSet(pb->start_reading,1);
    }
    else 
    {
        printf("stop\r\n");
        pb->reading = 0;
        pb->eof = 1;
        osSemaphoreRelease(pb->start_reading);
    }

    return 0;
}

// Call in the slow superloop
// Grab a new SD block
void playback_process(h_playback_t * pb)
{
    int ret;

    osSemaphoreAcquire(pb->start_reading, osWaitForever);

    if (pb->reading && pb->block_empty)
    {
        uint32_t parity = (pb->itr_block_wr % 2);
        uint32_t offset = parity * BLOCK_SIZE * AUDIO_NUM_CHANNELS;    // for dual-buffer

        ret = tinywav_read_f(&pb->tw, &pb->samples[offset], BLOCK_SIZE);
        if (ret == -1)
        {
            printf("error reading file\r\n");
        }

        if (ret != BLOCK_SIZE)
        {
            printf("ret=%d\r\n",ret);
        }

        pb->block_size[parity] = ret * AUDIO_NUM_CHANNELS;
        
        pb->itr_block_wr++;
        pb->block_empty--;
    }

    if (pb->eof)
    {
        pb->eof = 0;
        tinywav_close_read(&pb->tw);
        printf("file closed\r\n");
    }
}

// Call in the audio interrupt
// Copy a bit of the SD block into the audio buffer
void playback_process_audio(h_playback_t * pb, float *buf, uint32_t n)
{
    if (pb->reading)
    {
        uint32_t parity = (pb->itr_block_rd % 2);
        uint32_t offset = parity * BLOCK_SIZE * AUDIO_NUM_CHANNELS;    // for dual-buffer

        if (pb->block_size[parity] == 0)
        {
            pb->reading = 0;
            pb->eof = 1;
        }

        // copie dans le buffer
        for (int i = 0 ; i < n ; i++)
        {
            buf[i] = (pb->samples[offset + pb->itr]) * 0.2;

            // condition changement de block
            pb->itr++;

            if (pb->itr == pb->block_size[parity])
            {
                pb->itr = 0;
                pb->block_empty++;
                pb->itr_block_rd++;
                osSemaphoreRelease(pb->start_reading);
                // // dummy end condition about 10 seconds
                // if (pb->itr_block_rd == 1000)
                // {
                //     pb->reading = 0;
                //     pb->eof = 1;
                //     HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
                // }
            }
        }
    }
}