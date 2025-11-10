#include "stats.h"
#include "stm32f4xx_hal_tim.h"

uint8_t stats_start(h_stats_t * st)
{
    return HAL_TIM_Base_Start(st->htim);
}

uint8_t stats_process(h_stats_t * st)
{
    if (st->period != 0)
    {
        st->usage = st->duration * 100 / st->period;
    }
    else {
        st->usage = 0;
    }
    if (st->usage > st->max_usage)
        st->max_usage = st->usage;

    return 0;
}

uint8_t stats_usage(h_stats_t * st)
{
    return st->usage;
}

uint8_t stats_max_usage(h_stats_t * st)
{
    return st->max_usage;
}