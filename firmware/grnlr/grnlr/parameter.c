#include "parameter.h"

uint8_t param_init(h_param_t * h_param)
{
    h_param->size = 0;
    h_param->itr = 0;

    return 0;
}

uint8_t param_add_float(h_param_t * h_param, const char * name, float * value)
{
    if (h_param->size < PARAM_LIST_DEPTH)
    {
        h_param->list[h_param->size].name = name;
        h_param->list[h_param->size].type = PARAM_TYPE_FLOAT;
        h_param->list[h_param->size].value.fval = value;

        h_param->size++;

        return 0;
    }

    return 1;
}

uint8_t param_add_bool(h_param_t * h_param, const char * name, uint8_t * value)
{
        if (h_param->size < PARAM_LIST_DEPTH)
    {
        h_param->list[h_param->size].name = name;
        h_param->list[h_param->size].type = PARAM_TYPE_BOOL;
        h_param->list[h_param->size].value.bval = value;

        h_param->size++;

        return 0;
    }

    return 1;
}

uint8_t param_add_func(h_param_t * h_param, const char * name, void (* func)(void))
{
    if (h_param->size < PARAM_LIST_DEPTH)
    {
        h_param->list[h_param->size].name = name;
        h_param->list[h_param->size].type = PARAM_TYPE_FUNC;
        h_param->list[h_param->size].value.func = func;

        h_param->size++;

        return 0;
    }

    return 1;
}

uint8_t param_increment_itr(h_param_t * h_param, int8_t inc)
{
    if (inc > 0)
    {
        if (h_param->itr < h_param->size-1)
        {
            h_param->itr++;
        }
        else 
        {
            h_param->itr = 0;
        }
    }
    if (inc < 0)
    {
        if (h_param->itr > 0)
        {
            h_param->itr--;
        }
        else {
            h_param->itr = h_param->size - 1;
        }
    }

    return 0;
}