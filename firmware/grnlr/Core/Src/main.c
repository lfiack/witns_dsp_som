/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "adc.h"
#include "dma.h"
#include "fatfs.h"
#include "i2c.h"
#include "sai.h"
#include "sdio.h"
#include "tim.h"
#include "usart.h"
#include "usb_otg.h"
#include "gpio.h"
#include "fmc.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>

#include "app.h"
#include "coder.h"
#include "ssd1306.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
#define __sdram __attribute__((section(".sdram_data")))

// #define AUDIO_BUFFER_LENGTH 2097152	// 8MB
// __sdram uint32_t audio_buffer[AUDIO_BUFFER_LENGTH];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */


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

void HAL_SAI_TxHalfCpltCallback(SAI_HandleTypeDef *hsai)
{
	if (SAI1_Block_A == hsai->Instance)
	{
		// Nothing for now
	}
}

void HAL_SAI_TxCpltCallback(SAI_HandleTypeDef *hsai)
{
	if (SAI1_Block_A == hsai->Instance)
	{
		// Nothing for now
	}
}

void HAL_SAI_RxHalfCpltCallback(SAI_HandleTypeDef *hsai)
{
	if (SAI1_Block_B == hsai->Instance)
	{
		h_stats.period = __HAL_TIM_GET_COUNTER(&htim6);
		__HAL_TIM_SET_COUNTER(&htim6, 0);

		//process first half of h_sgtl5000 buffer
		app_process_audio(
				h_sgtl5000.sai_rx_buffer, 
				h_sgtl5000.sai_tx_buffer, 
				AUDIO_BUFFER_LENGTH * AUDIO_NUM_CHANNELS
				);

		h_stats.duration = __HAL_TIM_GET_COUNTER(&htim6);
	}
}

void HAL_SAI_RxCpltCallback(SAI_HandleTypeDef *hsai)
{
	if (SAI1_Block_B == hsai->Instance)
	{
		h_stats.period = __HAL_TIM_GET_COUNTER(&htim6);
		__HAL_TIM_SET_COUNTER(&htim6, 0);

		//process second half of h_sgtl5000 buffer
		app_process_audio(
				&h_sgtl5000.sai_rx_buffer[AUDIO_BUFFER_LENGTH * AUDIO_NUM_CHANNELS], 
				&h_sgtl5000.sai_tx_buffer[AUDIO_BUFFER_LENGTH * AUDIO_NUM_CHANNELS], 
				AUDIO_BUFFER_LENGTH * AUDIO_NUM_CHANNELS
				);

		h_stats.duration = __HAL_TIM_GET_COUNTER(&htim6);
	}
}

void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
	if (I2C2 == hi2c->Instance)
	{
		ssd1306_i2c_mem_write_cb();
	}
}

#define SDRAM_MODEREG_BURST_LENGTH_1             ((uint16_t)0x0000)
#define SDRAM_MODEREG_BURST_LENGTH_2             ((uint16_t)0x0001)
#define SDRAM_MODEREG_BURST_LENGTH_4             ((uint16_t)0x0002)
#define SDRAM_MODEREG_BURST_LENGTH_8             ((uint16_t)0x0004)
#define SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL      ((uint16_t)0x0000)
#define SDRAM_MODEREG_BURST_TYPE_INTERLEAVED     ((uint16_t)0x0008)
#define SDRAM_MODEREG_CAS_LATENCY_2              ((uint16_t)0x0020)
#define SDRAM_MODEREG_CAS_LATENCY_3              ((uint16_t)0x0030)
#define SDRAM_MODEREG_OPERATING_MODE_STANDARD    ((uint16_t)0x0000)
#define SDRAM_MODEREG_WRITEBURST_MODE_PROGRAMMED ((uint16_t)0x0000)
#define SDRAM_MODEREG_WRITEBURST_MODE_SINGLE     ((uint16_t)0x0200)

static HAL_StatusTypeDef SDRAM_Initialization_Sequence(SDRAM_HandleTypeDef *hsdram)
{
	HAL_StatusTypeDef ret;
	FMC_SDRAM_CommandTypeDef command;
	__IO uint32_t tmpmrd =0;
	/* Step 3:  Configure a clock configuration enable command */
	command.CommandMode 			 = FMC_SDRAM_CMD_CLK_ENABLE;
	command.CommandTarget 		 = FMC_SDRAM_CMD_TARGET_BANK2;
	command.AutoRefreshNumber 	 = 1;
	command.ModeRegisterDefinition = 0;

	/* Send the command */
	ret = HAL_SDRAM_SendCommand(hsdram, &command, 0x1000);
	if (ret != HAL_OK)
	{
		return ret;
	}

	/* Step 4: Insert 100 ms delay */
	HAL_Delay(100);

	/* Step 5: Configure a PALL (precharge all) command */
	command.CommandMode 			 = FMC_SDRAM_CMD_PALL;
	command.CommandTarget 	     = FMC_SDRAM_CMD_TARGET_BANK2;
	command.AutoRefreshNumber 	 = 1;
	command.ModeRegisterDefinition = 0;

	/* Send the command */
	ret = HAL_SDRAM_SendCommand(hsdram, &command, 0x1000);
	if (ret != HAL_OK)
	{
		return ret;
	}


	/* Step 6 : Configure a Auto-Refresh command */
	command.CommandMode 			 = FMC_SDRAM_CMD_AUTOREFRESH_MODE;
	command.CommandTarget 		 = FMC_SDRAM_CMD_TARGET_BANK2;
	command.AutoRefreshNumber 	 = 4;
	command.ModeRegisterDefinition = 0;

	/* Send the command */
	ret = HAL_SDRAM_SendCommand(hsdram, &command, 0x1000);
	if (ret != HAL_OK)
	{
		return ret;
	}


	/* Step 7: Program the external memory mode register */
	tmpmrd = (uint32_t)SDRAM_MODEREG_BURST_LENGTH_2          |
		SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL   |
		SDRAM_MODEREG_CAS_LATENCY_3           |
		SDRAM_MODEREG_OPERATING_MODE_STANDARD |
		SDRAM_MODEREG_WRITEBURST_MODE_SINGLE;

	command.CommandMode = FMC_SDRAM_CMD_LOAD_MODE;
	command.CommandTarget 		 = FMC_SDRAM_CMD_TARGET_BANK2;
	command.AutoRefreshNumber 	 = 1;
	command.ModeRegisterDefinition = tmpmrd;

	/* Send the command */
	ret = HAL_SDRAM_SendCommand(hsdram, &command, 0x1000);
	if (ret != HAL_OK)
	{
		return ret;
	}

	/* Step 8: Set the refresh rate counter */
	/* (15.62 us x Freq) - 20 */
	/* (15.62 * 180) - 20 = 2792 */
	/* Set the device refresh counter */
	return HAL_SDRAM_ProgramRefreshRate(hsdram, 2792);
	// was 0x056A for 90MHz
}

uint32_t sdram_test(uint32_t* buffer, uint32_t size)
{
	uint32_t fail = 0;
	uint32_t tick = HAL_GetTick();
	for (uint32_t i = 0 ; i < size ; i++)
	{
		buffer[i] = i;
	}
	uint32_t new_tick = HAL_GetTick();
	printf("8MB written in %lu ms\r\n", new_tick - tick);
	tick = HAL_GetTick();
	for (uint32_t i = 0 ; i < size ; i++)
	{
		if (buffer[i] != i)
		{
			fail++;
		}
	}
	new_tick = HAL_GetTick();
	printf("8MB read in %lu ms\r\n", new_tick - tick);
	return fail;
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* Configure the peripherals common clocks */
  PeriphCommonClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_FMC_Init();
  MX_ADC3_Init();
  MX_I2C1_Init();
  MX_SAI1_Init();
  MX_SDIO_SD_Init();
  MX_UART4_Init();
  MX_USB_OTG_FS_PCD_Init();
  MX_FATFS_Init();
  MX_I2C2_Init();
  MX_TIM6_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
	if (SDRAM_Initialization_Sequence(&hsdram1) != HAL_OK)
	{
		printf("failed to initialize SDRAM\r\n");
	}

	// app_init();

	// app_process();
  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();  /* Call init function for freertos objects (in cmsis_os2.c) */
  MX_FREERTOS_Init();

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

	while (1)
	{
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	}
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 180;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Activate the Over-Drive mode
  */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief Peripherals Common Clock Configuration
  * @retval None
  */
void PeriphCommonClock_Config(void)
{
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Initializes the peripherals clock
  */
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SAI1|RCC_PERIPHCLK_SDIO
                              |RCC_PERIPHCLK_CLK48;
  PeriphClkInitStruct.PLLI2S.PLLI2SN = 123;
  PeriphClkInitStruct.PLLI2S.PLLI2SP = RCC_PLLI2SP_DIV2;
  PeriphClkInitStruct.PLLI2S.PLLI2SM = 16;
  PeriphClkInitStruct.PLLI2S.PLLI2SR = 2;
  PeriphClkInitStruct.PLLI2S.PLLI2SQ = 10;
  PeriphClkInitStruct.PLLSAI.PLLSAIM = 8;
  PeriphClkInitStruct.PLLSAI.PLLSAIN = 96;
  PeriphClkInitStruct.PLLSAI.PLLSAIQ = 15;
  PeriphClkInitStruct.PLLSAI.PLLSAIP = RCC_PLLSAIP_DIV4;
  PeriphClkInitStruct.PLLI2SDivQ = 1;
  PeriphClkInitStruct.PLLSAIDivQ = 1;
  PeriphClkInitStruct.Clk48ClockSelection = RCC_CLK48CLKSOURCE_PLLSAIP;
  PeriphClkInitStruct.SdioClockSelection = RCC_SDIOCLKSOURCE_CLK48;
  PeriphClkInitStruct.Sai1ClockSelection = RCC_SAI1CLKSOURCE_PLLI2S;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM7 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM7)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1)
	{
	}
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
	/* User can add his own implementation to report the file name and line number,
ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
