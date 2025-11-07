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
#include "ssd1306.h"
#include "ssd1306_tests.h"
#include "ssd1306_fonts.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define SAI_TX_BUFFER_LENGTH (480*2)
#define SAI_RX_BUFFER_LENGTH (480*2)
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
static const unsigned char garfield_128x64 [] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0xFF, 0xF8, 0x7F, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x3F, 0xFF, 0xCF, 0xFF, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x3F, 0xFB, 0xE3, 0xFF, 0xF3, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x3F, 0xC0, 0xF3, 0xFF, 0xF8, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x00, 0x1F, 0xD9, 0xFC, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x0F, 0x00, 0x1E, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x0E, 0x1F, 0xF8, 0x03, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x03, 0x87, 0xF0, 0x07, 0x00, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x18, 0x3B, 0x80, 0x18, 0x00, 0x30, 0xC3, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0xFE, 0x00, 0x60, 0x00, 0x18, 0xC3, 0x01, 0xC0, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x3F, 0xFC, 0x01, 0x80, 0x00, 0x0F, 0xC3, 0x8F, 0xC0, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x07, 0xF0, 0x03, 0x00, 0x00, 0x0C, 0x07, 0xF0, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x0F, 0xE0, 0x0C, 0x00, 0x00, 0x0C, 0x07, 0xFF, 0xF8, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x38, 0xC0, 0x18, 0x00, 0x00, 0x08, 0x0F, 0xFF, 0xF0, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x71, 0x80, 0x30, 0x00, 0x00, 0x08, 0x0F, 0xFC, 0xFC, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0xF7, 0x00, 0x60, 0x00, 0x00, 0x18, 0x3F, 0xFE, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x01, 0xFF, 0x00, 0x60, 0x00, 0x00, 0x18, 0x7F, 0xFE, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x03, 0xFF, 0x00, 0xC0, 0x00, 0x00, 0x30, 0xFF, 0x1F, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x03, 0x83, 0xE0, 0xC0, 0x00, 0x00, 0x60, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x07, 0x03, 0x1F, 0xFF, 0xFF, 0xFF, 0xC0, 0x00, 0x03, 0x80, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x0E, 0x01, 0x00, 0xE7, 0xC0, 0x03, 0x80, 0x00, 0x0F, 0x80, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x0F, 0x01, 0x80, 0xE0, 0x00, 0x06, 0x00, 0x00, 0xFF, 0x80, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x0F, 0x83, 0x7F, 0xF0, 0x00, 0x38, 0x00, 0x26, 0xFF, 0x80, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x0F, 0xDC, 0x18, 0x1F, 0x87, 0xE0, 0x00, 0x26, 0xEF, 0x80, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x0F, 0xB8, 0x0E, 0x18, 0x00, 0x30, 0x00, 0x04, 0xCF, 0x80, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x0F, 0x30, 0x0F, 0xC0, 0x00, 0x18, 0x00, 0x00, 0x8F, 0x80, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x07, 0x30, 0x38, 0x70, 0x00, 0x18, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x07, 0x18, 0x00, 0x1F, 0x00, 0x18, 0x3B, 0x80, 0x0E, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x03, 0x8E, 0x00, 0x03, 0x80, 0x70, 0x1F, 0xF8, 0x1C, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x01, 0xFF, 0x80, 0x00, 0x03, 0xE0, 0x1F, 0xFF, 0xF8, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x7F, 0x86, 0x00, 0x3F, 0x1F, 0x07, 0xFF, 0xE0, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x0F, 0xC3, 0x00, 0x00, 0x3F, 0x01, 0xFF, 0x80, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x01, 0xFF, 0x80, 0x00, 0x3F, 0xC7, 0xFC, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x73, 0xE0, 0x07, 0xFF, 0xFF, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x00, 0x07, 0xC0, 0x3F, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x3F, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x00, 0x3E, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0xA4, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x70, 0x00, 0x00, 0x16, 0x01, 0xC0, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0xE0, 0x00, 0x00, 0x1E, 0x01, 0xC0, 0x3E, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x01, 0xC0, 0x00, 0x00, 0x0C, 0x01, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x03, 0x80, 0x00, 0x00, 0x04, 0x01, 0xF3, 0xFF, 0xC0, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x0C, 0x01, 0xEF, 0xF7, 0xE0, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x18, 0x00, 0x07, 0xF0, 0x60, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x18, 0x00, 0x07, 0x80, 0x70, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x30, 0x00, 0x06, 0x04, 0xF0, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x38, 0x00, 0x00, 0x00, 0x60, 0x00, 0x0C, 0x1C, 0xF0, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0xC0, 0x00, 0x1F, 0xFF, 0xF0, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x01, 0x80, 0x00, 0x01, 0xFD, 0xF0, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0xFD, 0xF0, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x07, 0x00, 0x00, 0x01, 0xC1, 0xE0, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x07, 0x00, 0x00, 0x03, 0x81, 0xE0, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x07, 0x00, 0x00, 0x07, 0xFF, 0xC0, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x03, 0xC0, 0x00, 0x39, 0xFF, 0x80, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x03, 0x80, 0x00, 0x01, 0xF0, 0x01, 0xE3, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0xE0, 0x00, 0x00, 0x1F, 0x03, 0x8D, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x03, 0xFC, 0x3D, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x01, 0xF8, 0x00, 0x03, 0xC0, 0x3F, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xFF, 0xFF, 0xC0, 0x3F, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xFF, 0xF1, 0xC0, 0x3F, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xFF, 0xE0, 0xC0, 0x3F, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xFF, 0xE0, 0xC0, 0x3F, 0x9E, 0x00, 0x00, 0x00, 0x00, 0x00
};
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

//uint32_t irq_counter = 0;
//
//void HAL_SAI_TxCpltCallback(SAI_HandleTypeDef *hsai)
//{
//	if (SAI1_Block_A == hsai->Instance)
//	{
//		irq_counter++;
//	}
//}

#define __sdram __attribute__((section(".sdram_data")))

#define AUDIO_BUFFER_LENGTH 2097152	// 8MB
__sdram uint32_t audio_buffer[AUDIO_BUFFER_LENGTH];

//h_sgtl5000_t h_sgtl5000 =
//{
//		.hi2c = &hi2c1,
//		.hsai_tx = &hsai_BlockA1,
//		.hsai_rx = &hsai_BlockB1,
//		.i2c_address = 0x0A
//};

static int16_t sai_tx_buffer[SAI_TX_BUFFER_LENGTH];
static int16_t sai_rx_buffer[SAI_RX_BUFFER_LENGTH];

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
			printf("%s%s\r\n", fno.fname,
					(fno.fattrib & AM_DIR) ? "/" : "");
#endif
		}
		f_closedir(&dir);
	} else {
		printf("f_opendir error (%d)\r\n", res);
	}
}

void read_text_file(const char *filename)
{
	FIL file;
	FRESULT res;
	char buffer[128];
	UINT bytes_read;

	res = f_open(&file, filename, FA_READ);
	if (res == FR_OK) {
		printf("Reading %s:\r\n", filename);
		do {
			res = f_read(&file, buffer, sizeof(buffer)-1, &bytes_read);
			if (res != FR_OK) break;
			buffer[bytes_read] = '\0';
			printf("%s", buffer);
		} while (bytes_read == sizeof(buffer)-1);
		printf("\r\n-- End of file --\r\n");
		f_close(&file);
	} else {
		printf("f_open error (%d)\r\n", res);
	}
}


static int tx_cplt_counter = 0;
void HAL_SAI_TxCpltCallback(SAI_HandleTypeDef *hsai)
{
	if (SAI1_Block_A == hsai->Instance)
	{
		tx_cplt_counter++;
		// TODO Temp juste pour voir
		//HAL_SAI_DMAStop(&hsai_BlockA2);
	}
}

static int tx_half_cplt_counter = 0;
void HAL_SAI_TxHalfCpltCallback(SAI_HandleTypeDef *hsai)
{
	if (SAI1_Block_A == hsai->Instance)
	{
		tx_half_cplt_counter++;
	}
}

static int rx_cplt_flag = 0;
static int rx_cplt_counter = 0;
void HAL_SAI_RxCpltCallback(SAI_HandleTypeDef *hsai)
{
	if (SAI1_Block_B == hsai->Instance)
	{
		rx_cplt_flag = 1;

		//		for (int i = SAI_RX_BUFFER_LENGTH / 2 ; i < SAI_RX_BUFFER_LENGTH ; i++)
		//		{
		//			sai_tx_buffer[i] = sai_rx_buffer[i];
		//		}

		// TODO Temp juste pour voir
		//HAL_SAI_DMAStop(&hsai_BlockB2);

		rx_cplt_counter++;
	}
}

static int rx_half_cplt_flag = 0;
static int rx_half_cplt_counter = 0;
void HAL_SAI_RxHalfCpltCallback(SAI_HandleTypeDef *hsai)
{
	if (SAI1_Block_B == hsai->Instance)
	{
		rx_half_cplt_flag = 1;

		//		for (int i = 0 ; i < SAI_RX_BUFFER_LENGTH / 2 ; i++)
		//		{
		//			sai_tx_buffer[i] = sai_rx_buffer[i];
		//		}

		rx_half_cplt_counter++;
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
  MX_I2C2_Init();
  /* USER CODE BEGIN 2 */
	printf("\r\n==== WITNS DSP SOM ====\r\n");
	HAL_GPIO_WritePin(OLED_RESET_GPIO_Port, OLED_RESET_Pin, GPIO_PIN_SET);

	if (HAL_GPIO_ReadPin(SD_DETECT_GPIO_Port, SD_DETECT_Pin) == GPIO_PIN_RESET)
	{
		printf("SD Card found and initialized\r\n");
	}
	else
	{
		printf("No SD Card found\r\n");
	}

	SDRAM_Initialization_Sequence(&hsdram1, &command);

	//	for (uint32_t i = 0 ; i < SGTL5000_TX_BUFFER_LENGTH ; i+=2)
	//	{
	//		h_sgtl5000.tx_buffer[i] = i * 65536 / SGTL5000_TX_BUFFER_LENGTH;
	//		//		printf("%u\r\n", h_sgtl5000.tx_buffer[i]);
	//	}
	//	for (uint32_t i = 1 ; i < SGTL5000_TX_BUFFER_LENGTH ; i+=2)
	//	{
	//		h_sgtl5000.tx_buffer[i] = 0;
	//		//		printf("%u\r\n", h_sgtl5000.tx_buffer[i]);
	//	}
	//	HAL_StatusTypeDef ret;
	//ssd1306_UpdateScreen();
	//	__HAL_SAI_ENABLE(h_sgtl5000.hsai_tx);
	//	ret = sgtl5000_init(&h_sgtl5000);
	//	//	HAL_StatusTypeDef ret;
	//	//	ret = sgtl5000_enable(&h_sgtl5000);
	//	//	if (ret != HAL_OK)
	//	//	{
	//	//		printf("Error enabling SGTL5000 %d\r\n", ret);
	//	//		Error_Handler();
	//	//	}
	//	//
	//	//	printf("SGTL5000 enabled\r\n");
	//	//
	//	//
	//	//	ret = sgtl5000_line_in_level(&h_sgtl5000, 0, 0);
	//	//	if (ret != HAL_OK)
	//	//	{
	//	//		printf("Error setting line in level %d\r\n", ret);
	//	//		Error_Handler();
	//	//	}
	//	//
	//	//	printf("Line in level set\r\n");
	//	//
	//	//	ret = sgtl5000_line_out_level(&h_sgtl5000, 0, 0);
	//	//	if (ret != HAL_OK)
	//	//	{
	//	//		printf("Error setting line out level %d\r\n", ret);
	//	//		Error_Handler();
	//	//	}
	//	//
	//	//	printf("Line out level set\r\n");
	//	//
	//	//	ret = sgtl5000_unmute(&h_sgtl5000);
	//	//	if (ret != HAL_OK)
	//	//	{
	//	//		printf("Error unmuting SGTL5000 %d\r\n", ret);
	//	//		Error_Handler();
	//	//	}
	//	//
	//	//	printf("SGTL5000 unmuted\r\n");
	//	//
	//	ret = sgtl5000_i2s_start(&h_sgtl5000);
	//	if (ret != HAL_OK)
	//	{
	//		printf("Error starting I2S %d\r\n", ret);
	//		Error_Handler();
	//	}
	//
	//	printf("I2S Started\r\n");
	//
		FATFS fs;
//		FIL file;
		FRESULT res;
//		DIR dir;
//		FILINFO fno;
//		char path[64];

		res = f_mount(&fs, "", 1);  // "" = "0:", 1 = mount now
		if (res != FR_OK) {
			printf("f_mount error (%d)\r\n", res);
		} else {
			printf("SD mounted successfully\r\n");
		}

		list_files("/");
		read_text_file("readme.txt");

	// Starts MCLK but not the LR clock and other signals
	__HAL_SAI_ENABLE(&hsai_BlockA1);

	/**
	 * For the 32 QFN version of the SGTL5000, the I2C device
	 * address is 0n01010(R/W) where n is determined by
	 * CTRL_ADR0_CS and R/W is the read/write bit from the I2C
	 * protocol.
	 *
	 * CTRL_ADR0_CS is tied to GND
	 * Address is 0001010(R/W)
	 * => 00010100 = 0x14 for Write
	 * => 00010101 = 0x15 for Read
	 * Est-ce que le R/W est ajouté dans la HAL ? Dans ce cas :
	 * => 0001010 = 0x0A -> Eh non!
	 * Cela dit, il rajoute le 1 pour la lecture
	 */

	uint16_t sgtl_address = 0x14;
	uint16_t data;

	h_sgtl5000_t h_sgtl5000;
	h_sgtl5000.hi2c = &hi2c1;
	h_sgtl5000.dev_address = sgtl_address;

	sgtl5000_init(&h_sgtl5000);

	HAL_StatusTypeDef ret;
	ret = sgtl5000_i2c_read_register(&h_sgtl5000, SGTL5000_CHIP_ID, &data);

	if (ret != HAL_OK)
	{
		printf("HAL_I2C_Mem_Read error\r\n");
		Error_Handler();
	}

	printf("CHIP ID = 0x%4X\r\n", data);

	for (int i = 0 ; i < SAI_TX_BUFFER_LENGTH ; i++)
	{
		// Generate a sawtooth at 1kHz
		sai_tx_buffer[i] = i * (0xFFFF/SAI_TX_BUFFER_LENGTH);
	}

	printf("Starting SAI...\r\n");
	// Last parameter is the number of DMA CYCLES (here a cycle is 16 bits/2Bytes)
	HAL_SAI_Receive_DMA(&hsai_BlockB1, (uint8_t*) sai_rx_buffer, SAI_RX_BUFFER_LENGTH);
	HAL_SAI_Transmit_DMA(&hsai_BlockA1, (uint8_t*) sai_tx_buffer, SAI_TX_BUFFER_LENGTH);

	//	ssd1306_Init();
	//	ssd1306_SetCursor(2,0);
	//	ssd1306_WriteString("Proute", Font_11x18, White);
	//	ssd1306_UpdateScreen();

	ssd1306_Init();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	int i = 0;
#define MOD 20
	while (1)
	{
		HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
		HAL_Delay(500);

		if (i%MOD == 0)
		{
			ssd1306_SetCursor(2,1);
			ssd1306_WriteString("I hate", Font_16x15, White);
			ssd1306_SetCursor(2,16);
			ssd1306_WriteString("mondays", Font_16x15, White);
			ssd1306_UpdateScreen();
		}

		if (i%MOD == 2)
		{
			ssd1306_Fill(White);
			ssd1306_DrawBitmap(0,0,garfield_128x64,128,64,Black);
			ssd1306_UpdateScreen();
		}

		i++;
		//		if (HAL_GetTick() % 1000 == 0)
		//		{
		//			printf("rx=%d, %d ; tx=%d, %d\r\n",rx_cplt_counter, rx_half_cplt_counter,tx_cplt_counter, tx_half_cplt_counter);
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
