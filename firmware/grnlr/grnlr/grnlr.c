#include "grnlr.h"

#include <stdio.h>

#include "gui.h"

uint8_t grnlr_init(void)
{
    printf("\r\n==== GRNLR ====\r\n");

    gui_init();

    return 0;
}

void grnlr_process(void)
{
    for (;;)
    {
        gui_tests();
    }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  switch(GPIO_Pin)
  {
    case ENC_PB_Pin:
      printf("PB\r\n");
      break;
    case ENC_A_Pin:
    if (HAL_GPIO_ReadPin(ENC_A_GPIO_Port, ENC_A_Pin) != HAL_GPIO_ReadPin(ENC_B_GPIO_Port, ENC_B_Pin))
      {
        printf("UP\r\n");
      }
      if (HAL_GPIO_ReadPin(ENC_A_GPIO_Port, ENC_A_Pin) == HAL_GPIO_ReadPin(ENC_B_GPIO_Port, ENC_B_Pin))
      {
        printf("DOWN\r\n");
      }

      break;
    case ENC_B_Pin:
      break;
    default:
      break;
  }
}