#include "grnlr.h"

#include <stdio.h>

#include "gui.h"
#include "coder.h"
#include "main.h"
#include "parameter.h"
#include "sd.h"
#include "i2c.h"
#include "sai.h"
#include "sgtl5000.h"

#define PARAM_FILENAME "GRNLR.TXT"

h_coder_t h_coder;
h_param_t h_param;

float feedback = 0.0f;
uint8_t freeze = 0;
float mix = 0.0f;
float volume = 0.0f;
float love = 0.0f;

uint8_t edit_mode = 0;

#define SAI_TX_BUFFER_LENGTH (480*2)
#define SAI_RX_BUFFER_LENGTH (480*2)

static int16_t sai_tx_buffer[SAI_TX_BUFFER_LENGTH];
static int16_t sai_rx_buffer[SAI_RX_BUFFER_LENGTH];

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
	sd_init();

	__HAL_SAI_ENABLE(&hsai_BlockA1);

	h_sgtl5000_t h_sgtl5000;
	h_sgtl5000.hi2c = &hi2c1;
	h_sgtl5000.dev_address = 0x14;

	sgtl5000_init(&h_sgtl5000);

	uint16_t chip_id;
	HAL_StatusTypeDef ret;
	ret = sgtl5000_i2c_read_register(&h_sgtl5000, SGTL5000_CHIP_ID, &chip_id);

	if (ret != HAL_OK)
	{
		printf("HAL_I2C_Mem_Read error\r\n");
		Error_Handler();
	}

	printf("CHIP ID = 0x%4X\r\n", chip_id);

	for (int i = 0 ; i < SAI_TX_BUFFER_LENGTH ; i++)
	{
		// Generate a sawtooth at 1kHz
		sai_tx_buffer[i] = i * (0xFFFF/SAI_TX_BUFFER_LENGTH);
	}

	printf("Starting SAI...\r\n");
	// Last parameter is the number of DMA CYCLES (here a cycle is 16 bits/2Bytes)
	HAL_SAI_Receive_DMA(&hsai_BlockB1, (uint8_t*) sai_rx_buffer, SAI_RX_BUFFER_LENGTH);
	HAL_SAI_Transmit_DMA(&hsai_BlockA1, (uint8_t*) sai_tx_buffer, SAI_TX_BUFFER_LENGTH);

	param_add_float(&h_param, "Volume", &volume);
	param_add_float(&h_param, "Feedback", &feedback);
	param_add_bool(&h_param, "Freeze", &freeze);
	param_add_float(&h_param, "Mix", &mix);
	param_add_float(&h_param, "Love", &love);
	param_add_func(&h_param, "LED", led);

	uint8_t res = sd_load_param(PARAM_FILENAME, &h_param);
	if (res != 0)
	{
		printf("Error opening file %s (%d)\r\n", PARAM_FILENAME, res);
	}

	gui_display_select_arrows();
	grnlr_process_coder(0);

	gui_update_levels(0.0f, 0.0f, 0.0f, 0.0f);

	return 0;
}

void grnlr_process(void)
{
	// uint32_t tick;
	// uint32_t duration;

	for (;;)
	{
		if (coder_is_pb_pressed(&h_coder))
		{
			// tick = HAL_GetTick();
			grnlr_process_pb();
			// duration = HAL_GetTick() - tick;
			// printf("grnlr_process_pb in %lu\r\n", duration);
		}

		int8_t inc = coder_read_increment(&h_coder);

		if (inc != 0)
		{
			// tick = HAL_GetTick();
			grnlr_process_coder(inc);
			// duration = HAL_GetTick() - tick;
			// printf("grnlr_process_coder in %lu\r\n", duration);
		}

		// tick = HAL_GetTick();
		gui_update_levels(0.0f, 0.0f, 0.0f, 0.0f);
		// duration = HAL_GetTick() - tick;
		// printf("gui_update_levels in %lu\r\n", duration);
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
			if (sd_save_param(PARAM_FILENAME, &h_param) != 0)
			{
				printf("Error writing param\r\n");
			}
			break;
		case PARAM_TYPE_FLOAT:
			if (edit_mode)
			{
				edit_mode = 0;
				gui_display_select_arrows();
				if (sd_save_param(PARAM_FILENAME, &h_param) != 0)
				{
					printf("Error writing param\r\n");
				}
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
