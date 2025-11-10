#ifndef __PARAMETER_H
#define __PARAMETER_H

#include "main.h"

#define PARAM_LIST_DEPTH 16

typedef enum 
{
    PARAM_TYPE_FLOAT,
    PARAM_TYPE_BOOL,
    PARAM_TYPE_FUNC,
    PARAM_TYPE_DISPLAY
} param_type_t;

typedef union {
    float * fval;
    uint8_t * bval;
    void (* func)(void);
    const char * str;
} param_value_t;

typedef struct param_struct 
{
    const char * name;
    param_type_t type;
    param_value_t value;
} param_t;

typedef struct h_param_struct
{
    param_t list[PARAM_LIST_DEPTH];
    uint8_t itr;
    uint8_t size;
} h_param_t;

uint8_t param_init(h_param_t * h_param);
uint8_t param_add_float(h_param_t * h_param, const char * name, float * value);
uint8_t param_add_bool(h_param_t * h_param, const char * name, uint8_t * value);
uint8_t param_add_func(h_param_t * h_param, const char * name, void (* func)(void));
uint8_t param_add_display(h_param_t * h_param, const char * name, const char * str);
uint8_t param_increment_itr(h_param_t * h_param, int8_t inc);

#endif // __PARAMETER_H