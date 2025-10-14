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
#include "adc.h"
#include "dma.h"
#include "fatfs.h"
#include "i2c.h"
#include "sai.h"
#include "sdio.h"
#include "usart.h"
#include "usb_otg.h"
#include "gpio.h"
#include "fmc.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "sgtl5000.h"
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

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
int __io_putchar(int chr)
{
	HAL_UART_Transmit(&huart4, (uint8_t *)&chr, 1, HAL_MAX_DELAY);
	return chr;
}

FMC_SDRAM_CommandTypeDef command;
#define REFRESH_COUNT       ((uint32_t)0x056A)   /* SDRAM refresh counter (90MHz SDRAM clock) */
#define SDRAM_BANK_ADDR                 ((uint32_t)0xD0000000)

/* #define SDRAM_MEMORY_WIDTH            FMC_SDRAM_MEM_BUS_WIDTH_8 */
#define SDRAM_MEMORY_WIDTH            FMC_SDRAM_MEM_BUS_WIDTH_16

/* #define SDCLOCK_PERIOD                   FMC_SDRAM_CLOCK_PERIOD_2 */
#define SDCLOCK_PERIOD                FMC_SDRAM_CLOCK_PERIOD_3

#define SDRAM_TIMEOUT     ((uint32_t)0xFFFF)

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

static void SDRAM_Initialization_Sequence(SDRAM_HandleTypeDef *hsdram, FMC_SDRAM_CommandTypeDef *Command)
{
	__IO uint32_t tmpmrd =0;
	/* Step 3:  Configure a clock configuration enable command */
	Command->CommandMode 			 = FMC_SDRAM_CMD_CLK_ENABLE;
	Command->CommandTarget 		 = FMC_SDRAM_CMD_TARGET_BANK2;
	Command->AutoRefreshNumber 	 = 1;
	Command->ModeRegisterDefinition = 0;

	/* Send the command */
	HAL_SDRAM_SendCommand(hsdram, Command, 0x1000);

	/* Step 4: Insert 100 ms delay */
	HAL_Delay(100);

	/* Step 5: Configure a PALL (precharge all) command */
	Command->CommandMode 			 = FMC_SDRAM_CMD_PALL;
	Command->CommandTarget 	     = FMC_SDRAM_CMD_TARGET_BANK2;
	Command->AutoRefreshNumber 	 = 1;
	Command->ModeRegisterDefinition = 0;

	/* Send the command */
	HAL_SDRAM_SendCommand(hsdram, Command, 0x1000);

	/* Step 6 : Configure a Auto-Refresh command */
	Command->CommandMode 			 = FMC_SDRAM_CMD_AUTOREFRESH_MODE;
	Command->CommandTarget 		 = FMC_SDRAM_CMD_TARGET_BANK2;
	Command->AutoRefreshNumber 	 = 4;
	Command->ModeRegisterDefinition = 0;

	/* Send the command */
	HAL_SDRAM_SendCommand(hsdram, Command, 0x1000);

	/* Step 7: Program the external memory mode register */
	tmpmrd = (uint32_t)SDRAM_MODEREG_BURST_LENGTH_2          |
			SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL   |
			SDRAM_MODEREG_CAS_LATENCY_3           |
			SDRAM_MODEREG_OPERATING_MODE_STANDARD |
			SDRAM_MODEREG_WRITEBURST_MODE_SINGLE;

	Command->CommandMode = FMC_SDRAM_CMD_LOAD_MODE;
	Command->CommandTarget 		 = FMC_SDRAM_CMD_TARGET_BANK2;
	Command->AutoRefreshNumber 	 = 1;
	Command->ModeRegisterDefinition = tmpmrd;

	/* Send the command */
	HAL_SDRAM_SendCommand(hsdram, Command, 0x1000);

	/* Step 8: Set the refresh rate counter */
	/* (15.62 us x Freq) - 20 */
	/* (15.62 * 180) - 20 = 2792 */
	/* Set the device refresh counter */
	HAL_SDRAM_ProgramRefreshRate(hsdram, 2792);
	// was 0x056A for 90MHz

}

uint32_t irq_counter = 0;

void HAL_SAI_TxCpltCallback(SAI_HandleTypeDef *hsai)
{
	if (SAI1_Block_A == hsai->Instance)
	{
		irq_counter++;
	}
}

#define __sdram __attribute__((section(".sdram_data")))

#define AUDIO_BUFFER_LENGTH 2097152	// 8MB
__sdram uint32_t audio_buffer[AUDIO_BUFFER_LENGTH];

h_sgtl5000_t h_sgtl5000 =
{
		.hi2c = &hi2c1,
		.hsai_tx = &hsai_BlockA1,
		.hsai_rx = &hsai_BlockB1,
		.i2c_address = 0x0A
};

void list_files(const char *path)
{
    FRESULT res;
    DIR dir;
    FILINFO fno;

#if _USE_LFN
    static char lfn[_MAX_LFN + 1];
    fno.lfname = lfn;
    fno.lfsize = sizeof(lfn);
#endif

    res = f_opendir(&dir, path);  // Ouvre le répertoire
    if (res == FR_OK) {
        while (1) {
            res = f_readdir(&dir, &fno);
            if (res != FR_OK || fno.fname[0] == 0)
                break; // Fin
#if _USE_LFN
            printf("%s%s\n", fno.lfname[0] ? fno.lfname : fno.fname,
                   (fno.fattrib & AM_DIR) ? "/" : "");
#else
            printf("%s%s\n", fno.fname,
                   (fno.fattrib & AM_DIR) ? "/" : "");
#endif
        }
        f_closedir(&dir);
    } else {
        printf("f_opendir error (%d)\n", res);
    }
}

extern FATFS SDFatFS;
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
	/* USER CODE BEGIN 2 */
	printf("\r\n==== WITNS DSP SOM ====\r\n");
	if (HAL_GPIO_ReadPin(SD_DETECT_GPIO_Port, SD_DETECT_Pin) == GPIO_PIN_RESET)
	{
		printf("SD Card found and initialized\r\n");
	}
	else
	{
		printf("No SD Card found\r\n");
	}

	SDRAM_Initialization_Sequence(&hsdram1, &command);

	//	uint32_t *extRAM = (uint32_t*)0xD0000000;
	//	extRAM[0] = 0x12345678;
	//	if (extRAM[0] == 0x12345678) {
	//		printf("OK\r\n");
	//		// OK, SDRAM fonctionne
	//	}
	//	else
	//	{
	//		printf("NOK\r\n");
	//	}
	//
	//	for (int i = 0 ; i < AUDIO_BUFFER_LENGTH ; i++)
	//	{
	//		audio_buffer[i] = i;
	//	}
	//
	//	uint32_t errors = 0;
	//	uint32_t pass = 0;
	//
	//	for (int i = 0 ; i < AUDIO_BUFFER_LENGTH ; i++)
	//	{
	//		if (audio_buffer[i] == i)
	//		{
	//			pass++;
	//		}
	//		else
	//		{
	//			errors++;
	//		}
	//	}
	//
	//	printf("%lu OK %lu KO\r\n", pass, errors);

	for (uint32_t i = 0 ; i < SGTL5000_TX_BUFFER_LENGTH ; i+=2)
	{
		h_sgtl5000.tx_buffer[i] = i * 65536 / SGTL5000_TX_BUFFER_LENGTH;
		//		printf("%u\r\n", h_sgtl5000.tx_buffer[i]);
	}
	for (uint32_t i = 1 ; i < SGTL5000_TX_BUFFER_LENGTH ; i+=2)
	{
		h_sgtl5000.tx_buffer[i] = 0;
		//		printf("%u\r\n", h_sgtl5000.tx_buffer[i]);
	}

	HAL_StatusTypeDef ret;
	ret = sgtl5000_enable(&h_sgtl5000);
	if (ret != HAL_OK)
	{
		printf("Error enabling SGTL5000 %d\r\n", ret);
		Error_Handler();
	}

	printf("SGTL5000 enabled\r\n");


	ret = sgtl5000_line_in_level(&h_sgtl5000, 0, 0);
	if (ret != HAL_OK)
	{
		printf("Error setting line in level %d\r\n", ret);
		Error_Handler();
	}

	printf("Line in level set\r\n");

	ret = sgtl5000_line_out_level(&h_sgtl5000, 0, 0);
	if (ret != HAL_OK)
	{
		printf("Error setting line out level %d\r\n", ret);
		Error_Handler();
	}

	printf("Line out level set\r\n");

	ret = sgtl5000_unmute(&h_sgtl5000);
	if (ret != HAL_OK)
	{
		printf("Error unmuting SGTL5000 %d\r\n", ret);
		Error_Handler();
	}

	printf("SGTL5000 unmuted\r\n");

	ret = sgtl5000_i2s_start(&h_sgtl5000);
	if (ret != HAL_OK)
	{
		printf("Error starting I2S %d\r\n", ret);
		Error_Handler();
	}

	printf("I2S Started\r\n");

//	FATFS fs;
	FIL file;
	FRESULT res;
	DIR dir;
	FILINFO fno;
	char path[64];

	res = f_mount(&SDFatFS, "", 1);  // "" = "0:", 1 = mount now
	if (res != FR_OK) {
		printf("f_mount error (%d)\r\n", res);
	} else {
		printf("SD mounted successfully\r\n");
	}

	list_files("/");
	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1)
	{
		//		if (HAL_GetTick() % 1000 == 0)
		//		{
		//			uint32_t tmp = irq_counter;
		//			irq_counter = 0;
		//			printf("irq_counter = %lu\r\n", tmp);
		//		}

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
