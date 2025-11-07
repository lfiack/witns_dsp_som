#include "grnlr.h"

#include <stdio.h>

#include "gui.h"
#include "main.h"
#include "parameter.h"
#include "sd.h"
#include "i2c.h"
#include "sai.h"

#define PARAM_FILENAME "GRNLR.TXT"

h_coder_t h_coder;
h_param_t h_param;
h_sgtl5000_t h_sgtl5000 = 
{
	.hi2c = &hi2c1,
	.hsai_tx = &hsai_BlockA1,
	.hsai_rx = &hsai_BlockB1,
	.dev_address = 0x14
};

float feedback = 0.0f;
uint8_t freeze = 0;
float mix = 0.0f;
float volume = 0.0f;
float love = 0.0f;

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
	sd_init();
	sgtl5000_init(&h_sgtl5000);

	for (int i = 0 ; i < AUDIO_BUFFER_LENGTH ; i++)
	{
		// Generate a sawtooth at 1kHz
		h_sgtl5000.sai_tx_buffer[i] = i * (0xFFFF/AUDIO_BUFFER_LENGTH);
	}

	printf("Starting SAI...\r\n");
	sgtl5000_start(&h_sgtl5000);

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

void grnlr_process_audio(int16_t * in_buffer, int16_t * out_buffer, uint16_t len)
{
	// Just a bypass for now, left and right
	for (int i = 0 ; i < len ; i++)
	{
		out_buffer[i] = in_buffer[i];
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
