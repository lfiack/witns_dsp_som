#include "app.h"

#include <stdio.h>

#include "gui.h"
#include "main.h"
#include "parameter.h"
#include "sd.h"
#include "i2c.h"
#include "sai.h"
#include "sgtl5000.h"
#include "stats.h"
#include "stm32f4xx_hal_gpio.h"
#include "vu.h"
#include "delay.h"


/* TODO :
 * play sound
 * Rename project to supertofu
 * organise app.c/.h
 * program grnlr sound fx
**/

#define PARAM_FILENAME "PARAM.TXT"

#define VU_INL 0
#define VU_INR 1
#define VU_OUTL 2
#define VU_OUTR 3

#define CHANNELS        2
#define DELAY_LINE_LENGTH     (AUDIO_FS * MAX_DELAY_SEC * CHANNELS)

#define FLASH_FRAMES 50

#define SD_ACTIVE 1
#define SAI_ACTIVE 1

__sdram float delay_line[DELAY_LINE_LENGTH];

h_coder_t h_coder;
h_param_t h_param;
h_sgtl5000_t h_sgtl5000 = 
{
	.hi2c = &hi2c1,
	.hsai_tx = &hsai_BlockA1,
	.hsai_rx = &hsai_BlockB1,
	.dev_address = 0x14
};
// inL, inR, outL, outR
h_vumeter_t vu[4];
h_stats_t h_stats = 
{
	.htim = &htim6
};

h_delay_t h_delay;
h_playback_t h_playback;

float volume = 0.8f;
float gain = 0.5f;
uint8_t mono = 1;
char stats_str[16];

uint8_t edit_mode = 0;

uint32_t flash_frames = 0;

static void app_process_pb(void);
static void app_process_coder(int8_t inc);
static void app_display_gui(void);

void led(void)
{
	HAL_GPIO_TogglePin(LED_GPIO_Port,LED_Pin);
	// flash_frames = FLASH_FRAMES;
}

void play(void)
{
	playback_play(&h_playback, "song.wav");
}

uint8_t app_init(void)
{
	printf("\r\n==== SUPERTOFU ====\r\n");

	gui_init();
	coder_init(&h_coder);
	param_init(&h_param);
	sd_init();
	sgtl5000_init(&h_sgtl5000);
	for (int i = 0 ; i < 4 ; i++)
	{
		vumeter_init(&vu[i], AUDIO_FS, AUDIO_BUFFER_LENGTH, 50.0f, 0.05f, 100.0f, -60.0f);
	}
	stats_start(&h_stats);

	delay_init(&h_delay, delay_line, DELAY_LINE_LENGTH, AUDIO_FS, CHANNELS);

	param_add_float(&h_param, "Gain", &gain);
	param_add_float(&h_param, "Feedback", &h_delay.feedback);
	param_add_float(&h_param, "Mix", &h_delay.mix);
	param_add_float(&h_param, "Delay", &h_delay.delay);
	param_add_func(&h_param, "Play", play);
	param_add_float(&h_param, "Volume", &volume);
	param_add_bool(&h_param, "Mono", &mono);
	param_add_display(&h_param, "CPU %", stats_str);
	param_add_func(&h_param, "LED", led);

	// TODO deactivated SD that caused crashes
#if (SD_ACTIVE == 1)
	uint8_t res = sd_load_param(PARAM_FILENAME, &h_param);
	if (res != 0)
	{
		printf("Error opening file %s (%d)\r\n", PARAM_FILENAME, res);
	}
#endif

#if (SAI_ACTIVE == 1)
	printf("Starting SAI...\r\n");
	sgtl5000_start(&h_sgtl5000);
#endif

	gui_display_select_arrows();
	app_process_coder(0);

	//gui_update_levels(0.0f, 0.0f, 0.0f, 0.0f);

	return 0;
}

void app_process(void)
{
	for (;;)
	{
		if (coder_is_pb_pressed(&h_coder))
		{
			app_process_pb();
		}

		int8_t inc = coder_read_increment(&h_coder);

		if (inc != 0)
		{
			app_process_coder(inc);
		}

		// playback_process(&h_playback);

		snprintf(stats_str, 16, "%3u%% (%u%%)\r\n", stats_usage(&h_stats), stats_max_usage(&h_stats));

		if (h_param.list[h_param.itr].type == PARAM_TYPE_DISPLAY)
		{
			gui_display_str(h_param.list[h_param.itr].value.str);
		}

		float vol[4];
		float peak[4];
		for (int i = 0 ; i < 4 ; i++)
		{
			float db = vumeter_get_dbfs(&vu[i]);
			vol[i] = db_to_f(db, vu[i].min_db);
        	float p_db = vumeter_get_peak_dbfs(&vu[i]);
        	peak[i] = db_to_f(p_db, vu[i].min_db);
			// if (vol[i] > 0.95f)
			// {
			// 	HAL_GPIO_WritePin(LED_GPIO_Port,LED_Pin, GPIO_PIN_SET);
			// 	flash_frames = FLASH_FRAMES;
			// }
		}

		if (flash_frames)
		{
			gui_flash();
			flash_frames--;
			if (flash_frames == 0)
			{
				app_display_gui();
			}
		}
		else 
		{
			gui_update_levels(vol, peak);
		}

		stats_process(&h_stats);
	}
}

float audio_buffer[AUDIO_BUFFER_LENGTH * AUDIO_NUM_CHANNELS];

void app_process_audio(int16_t * in_buffer, int16_t * out_buffer, uint16_t len)
{
	// int16_t to float conversion
	for (int i = 0 ; i < len ; i++)
	{
		audio_buffer[i] = ((float)in_buffer[i])/32768.0f;
		if (audio_buffer[i] > 0.95f || audio_buffer[i] < -0.95f)
		{
			HAL_GPIO_WritePin(LED_GPIO_Port,LED_Pin, GPIO_PIN_SET);
			flash_frames = FLASH_FRAMES;
		}
	}

	// vumeter_process_block_float(vu, audio_buffer, AUDIO_BUFFER_LENGTH*2);
	vumeters_process_block_float_interleaved(vu, audio_buffer, AUDIO_BUFFER_LENGTH);

	if (mono)
	{
		// Copy left to right for the accordion
		for (int i = 0 ; i < len ; i+=2)
		{
			audio_buffer[i+1] = audio_buffer[i];
		}
	}

	// Applying volume on input signal
	for (int i = 0 ; i < len ; i++)
	{
		audio_buffer[i] *= (gain * 4.0f);	// +6dB
	}

	playback_process_audio(&h_playback, volume, audio_buffer, len);
	delay_process_block(&h_delay, audio_buffer, len);

	// vumeter_process_block_float(&vu[2], audio_buffer, AUDIO_BUFFER_LENGTH*2);
	vumeters_process_block_float_interleaved(&vu[2], audio_buffer, AUDIO_BUFFER_LENGTH);



	// Re-interleaving
	for (int i = 0 ; i < len ; i++)
	{
		if (audio_buffer[i] > 1.0f)
		{
			audio_buffer[i] = 1.0f;
			HAL_GPIO_WritePin(LED_GPIO_Port,LED_Pin, GPIO_PIN_SET);
			flash_frames = FLASH_FRAMES;
		}
		if (audio_buffer[i] < -1.0f)
		{
			audio_buffer[i] = -1.0f;
			HAL_GPIO_WritePin(LED_GPIO_Port,LED_Pin, GPIO_PIN_SET);
			flash_frames = FLASH_FRAMES;
		}

		out_buffer[i] = (int16_t)(audio_buffer[i]*32767.0f);
	}
}

static void app_process_pb(void)
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
			// TODO deactivated SD that caused crashes
#if (SD_ACTIVE == 1)
			if (sd_save_param(PARAM_FILENAME, &h_param) != 0)
			{
				printf("Error writing param\r\n");
			}
#endif
			break;
		case PARAM_TYPE_FLOAT:
			if (edit_mode)
			{
				edit_mode = 0;
				gui_display_select_arrows();
				// TODO deactivated SD that caused crashes
#if (SD_ACTIVE == 1)
				if (sd_save_param(PARAM_FILENAME, &h_param) != 0)
				{
					printf("Error writing param\r\n");
				}
#endif
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

static void app_process_coder(int8_t inc)
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
		case PARAM_TYPE_DISPLAY:
			gui_display_str(h_param.list[h_param.itr].value.str);
		default:
			break;
	}
}

static void app_display_gui(void)
{
	gui_erase();

	gui_display_name(h_param.list[h_param.itr].name);

	switch (h_param.list[h_param.itr].type)
	{
		case PARAM_TYPE_FLOAT:
			gui_display_float(*(float*)h_param.list[h_param.itr].value.fval);
			if (edit_mode)
			{
				gui_display_edit_arrows();
			}
			else 
			{
				gui_display_select_arrows();
			}
			break;

		case PARAM_TYPE_BOOL:
			gui_display_bool(*(uint8_t*)h_param.list[h_param.itr].value.bval);
			break;
		case PARAM_TYPE_FUNC:
			gui_display_func_run();
			break;
		case PARAM_TYPE_DISPLAY:
			gui_display_str(h_param.list[h_param.itr].value.str);
		default:
			break;
	}
}