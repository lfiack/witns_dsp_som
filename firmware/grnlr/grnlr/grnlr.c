#include "grnlr.h"

#include <stdio.h>

#include "gui.h"
#include "coder.h"

h_coder_t h_coder;

uint8_t grnlr_init(void)
{
    printf("\r\n==== GRNLR ====\r\n");

    gui_init();
    coder_init(&h_coder);


    gui_display_name("Test coder");
    gui_display_select_arrows();
    gui_display_value(0.0f);
    gui_update_levels(0.0f, 0.0f, 0.0f, 0.0f);

    return 0;
}

void grnlr_process(void)
{
  // gui_tests();

  uint8_t edit_mode = 0;
  float param = 0.0f;

  for (;;)
  {
    if (coder_is_pb_pressed(&h_coder))
    {
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
    }

    int8_t inc = coder_read_increment(&h_coder);
    
    if (inc != 0)
    {
      if (edit_mode)
      {
        param += ((float)inc)*0.05f;

        if (param < 0.0f) param = 0.0f;
        if (param > 1.0f) param = 1.0f;
          
        gui_display_value(param);
      }
    }

    gui_update_levels(0.0f, 0.0f, 0.0f, 0.0f);
  }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  switch(GPIO_Pin)
  {
    case ENC_PB_Pin:
      coder_pb_pressed_cb(&h_coder);
      break;
    case ENC_A_Pin:
    if (HAL_GPIO_ReadPin(ENC_A_GPIO_Port, ENC_A_Pin) != HAL_GPIO_ReadPin(ENC_B_GPIO_Port, ENC_B_Pin))
      {
        coder_increment_cb(&h_coder, 1);
      }
      if (HAL_GPIO_ReadPin(ENC_A_GPIO_Port, ENC_A_Pin) == HAL_GPIO_ReadPin(ENC_B_GPIO_Port, ENC_B_Pin))
      {
        coder_increment_cb(&h_coder, -1);
      }

      break;
    case ENC_B_Pin:
      break;
    default:
      break;
  }
}