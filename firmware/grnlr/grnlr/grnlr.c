#include "grnlr.h"

#include <stdio.h>

#include "gui.h"
#include "coder.h"
#include "main.h"
#include "parameter.h"

h_coder_t h_coder;
h_param_t h_param;

float feedback = 0.0f;
uint8_t freeze = 0;
float mix = 0.0f;
float volume = 0.0f;
float love = 0.75f;

uint8_t edit_mode = 0;

static void grnlr_process_pb(void);
static void grnlr_process_coder(int8_t inc);

void led(void)
{
  HAL_GPIO_TogglePin(LED_GPIO_Port,LED_Pin);
}

uint8_t grnlr_init(void)
{
    printf("\r\n==== GRNLR ====\r\n");

    gui_init();
    coder_init(&h_coder);
    param_init(&h_param);

    param_add_float(&h_param, "Feedback", &feedback);
    param_add_bool(&h_param, "Freeze", &freeze);
    param_add_float(&h_param, "Mix", &mix);
    param_add_float(&h_param, "Volume", &volume);
    param_add_float(&h_param, "Love", &love);

    param_add_func(&h_param, "LED", led);

    gui_display_select_arrows();
    grnlr_process_coder(0);

    gui_update_levels(0.0f, 0.0f, 0.0f, 0.0f);

    return 0;
}

void grnlr_process(void)
{
  // gui_tests();

  for (;;)
  {
    if (coder_is_pb_pressed(&h_coder))
    {
      grnlr_process_pb();
    }

    int8_t inc = coder_read_increment(&h_coder);
    
    if (inc != 0)
    {
      grnlr_process_coder(inc);
    }

    gui_update_levels(0.0f, 0.0f, 0.0f, 0.0f);
  }
}

static void grnlr_process_pb(void)
{
  switch(h_param.list[h_param.itr].type)
  {
    case PARAM_TYPE_FUNC:
      h_param.list[h_param.itr].value.func();
      break;
    case PARAM_TYPE_BOOL:
      // Toggle bool
      *(uint8_t*)h_param.list[h_param.itr].value.bval = 1 - *(uint8_t*)h_param.list[h_param.itr].value.bval;
      gui_display_bool(*(uint8_t*)h_param.list[h_param.itr].value.bval);
      break;
    case PARAM_TYPE_FLOAT:
      if (edit_mode)
      {
        edit_mode = 0;
        gui_display_select_arrows();
      }
      else 
      {
        edit_mode = 1;
        gui_display_edit_arrows();
      }
      break;
    default:
      break;
  }
}

static void grnlr_process_coder(int8_t inc)
{
  if (edit_mode)
  {
    float param = *(float*)h_param.list[h_param.itr].value.fval;

    param += ((float)inc)*0.02f;

    if (param < 0.0f) param = 0.0f;
    if (param > 1.0f) param = 1.0f;

    *(float*)h_param.list[h_param.itr].value.fval = param;
  }
  else // selecting a param
  {
    param_increment_itr(&h_param, inc);

    gui_display_name(h_param.list[h_param.itr].name);
  }

  switch (h_param.list[h_param.itr].type)
  {
    case PARAM_TYPE_FLOAT:
      gui_display_float(*(float*)h_param.list[h_param.itr].value.fval);
      break;
    
    case PARAM_TYPE_BOOL:
      gui_display_bool(*(uint8_t*)h_param.list[h_param.itr].value.bval);
      break;
    case PARAM_TYPE_FUNC:
      gui_display_func_run();
      break;
    default:
      break;
  }
}