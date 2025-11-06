#include "gui.h"

#include "ssd1306.h"
#include "ssd1306_fonts.h"

#include <stdio.h> // For tests

const unsigned char up_arrows_8x8 [] = {
    0x00,0x00,0x18,0x3C,0x7E,0xFF,0x00,0x00
};

const unsigned char down_arrows_8x8 [] = {
    0x00,0x00,0xFF,0x7E,0x3C,0x18,0x00,0x00
};

const unsigned char left_arrows_8x8 [] = {
    0x04, 0x0C, 0x1C, 0x3C, 0x3C, 0x1C, 0x0C, 0x04
};

const unsigned char right_arrows_8x8 [] = {
    0x20, 0x30, 0x38, 0x3C, 0x3C, 0x38, 0x30, 0x20
};

static void erase_param(void);

uint8_t gui_init(void)
{
    // Reset (while we're coding)
    HAL_GPIO_WritePin(OLED_RESET_GPIO_Port, OLED_RESET_Pin, GPIO_PIN_RESET);
    HAL_Delay(100);
    HAL_GPIO_WritePin(OLED_RESET_GPIO_Port, OLED_RESET_Pin, GPIO_PIN_SET);
    HAL_Delay(100);

    ssd1306_Init();

    // ssd1306_UpdateScreen();

    return 0;
}

uint8_t gui_update_levels(float in_left, float in_right, float out_left, float out_right)
{
    if (in_left >= 1.0f) in_left = 1.0f;
    if (in_left <= 0.0f) in_left = 0.0f;
    if (in_right >= 1.0f) in_right = 1.0f;
    if (in_right <= 0.0f) in_right = 0.0f;

    if (out_left >= 1.0f) out_left = 1.0f;
    if (out_left <= 0.0f) out_left = 0.0f;
    if (out_right >= 1.0f) out_right = 1.0f;
    if (out_right <= 0.0f) out_right = 0.0f;

    uint8_t inl = 31 - (in_left*31);
    uint8_t inr = 31 - (in_right*31);
    uint8_t outl = 31 - (out_left*31);
    uint8_t outr = 31 - (out_right*31);

    // Erase
    ssd1306_FillRectangle(0, 0, 10, 31, Black);
    ssd1306_FillRectangle(117, 0, 127, 31, Black);

    // Left and right input level
    ssd1306_FillRectangle(0, 31, 4, inl, White);
    ssd1306_FillRectangle(6, 31, 10, inr, White);

    // Left and right output level
    ssd1306_FillRectangle(117, 31, 121, outl, White);
    ssd1306_FillRectangle(123, 31, 127, outr, White);

    ssd1306_UpdateScreen();

    return 0;
}

uint8_t gui_display_select_arrows(void)
{
    // Erase edit arrows
    ssd1306_FillRectangle(16,24,24,32, Black);
    ssd1306_FillRectangle(103,24,111,32, Black);

    // Arrows to select a param
    ssd1306_DrawBitmap(16, 0, up_arrows_8x8, 8, 8, White);
    ssd1306_DrawBitmap(16, 8, down_arrows_8x8, 8, 8, White);

    // ssd1306_UpdateScreen();

    return 0;
}

uint8_t gui_display_edit_arrows(void)
{
    // Erase select arrows
    ssd1306_FillRectangle(16,0,24,16, Black);

    // Arrows to edit a param
    ssd1306_DrawBitmap(16, 24, left_arrows_8x8, 8, 8, White);
    ssd1306_DrawBitmap(103, 24, right_arrows_8x8, 8, 8, White);

    // ssd1306_UpdateScreen();

    return 0;
}

uint8_t gui_display_float(float param)
{
    if (param >= 1.0f) param = 1.0f;
    if (param <= 0.0f) param = 0.0f;
    uint8_t val = 26 + (param*74);    // 26..100

    erase_param();

    // Value of the param
    ssd1306_FillRectangle(26, 25, val, 30, White);

    // ssd1306_UpdateScreen();

    return 0;
}

uint8_t gui_display_bool(uint8_t param)
{
    erase_param();

    if (param)
    {
        ssd1306_SetCursor(26,22);
        ssd1306_WriteString("OFF", Font_7x10, White);

        // ssd1306_FillRectangle(50, 21, 60, 27, White);
        ssd1306_SetCursor(50,22);
        ssd1306_WriteString("ON", Font_7x10, Black);
    }
    else
    {
        // ssd1306_FillRectangle(26, 21, 46, 27, White);
        ssd1306_SetCursor(26,22);
        ssd1306_WriteString("OFF", Font_7x10, Black);

        ssd1306_SetCursor(50,22);
        ssd1306_WriteString("ON", Font_7x10, White);
    }

    return 0;
}

uint8_t gui_display_func_run(void)
{
    erase_param();

    return 0;
}

uint8_t gui_display_name(const char * name)
{
    // Erasing last name
    ssd1306_FillRectangle(26, 1, 116, 15, Black);

    // Name of the param
    ssd1306_SetCursor(26,1);
    ssd1306_WriteString(name, Font_16x15, White);

    // ssd1306_UpdateScreen();

    return 0;
}

void gui_tests(void)
{
    static uint32_t test = 0;
    static char str[10];

    gui_update_levels(0.0f, 0.0f, 0.0f, 0.0f);

    // Testing parameter names
    gui_display_select_arrows();

    for (int i = 0 ; i < 4 ; i++)
    {
        gui_display_name("Feedback");
        HAL_Delay(1000);

        gui_display_name("Dry");
        HAL_Delay(1000);
    }

    snprintf(str, 10, "%lu", test++);
    gui_display_name(str);

    // Testing parameter value
    gui_display_edit_arrows();

    for (int i = 0 ; i < 4 ; i++)
    {
        for (float param = 0.0f ; param <= 1.0f ; param+=.05f)
        {
            gui_display_float(param);
            HAL_Delay(50);
        }

        for (float param = 1.0f ; param >= 0.0f ; param-=.05f)
        {
            gui_display_float(param);
            HAL_Delay(50);
        }

        gui_display_float(0.0f);
        HAL_Delay(50);
    }

    // Testing volumes
    for (float inl = .0 ; inl <= 1.0f ; inl+=.05f)
    {
        gui_update_levels(inl, 0.0f, 0.0f, 0.0f);
        HAL_Delay(50);
    }

    for (float inr = .0 ; inr <= 1.0f ; inr+=.05f)
    {
        gui_update_levels(1.0f, inr, 0.0f, 0.0f);
        HAL_Delay(50);
    }

    for (float outl = .0 ; outl <= 1.0f ; outl+=.05f)
    {
        gui_update_levels(1.0f, 1.0f, outl, 0.0f);
        HAL_Delay(50);
    }

    for (float outr = .0 ; outr <= 1.0f ; outr+=.05f)
    {
        gui_update_levels(1.0f, 1.0f, 1.0f, outr);
        HAL_Delay(50);
    }
    gui_update_levels(1.0f, 1.0f, 1.0f, 1.0f);

    // Parameter to the max just for fun
    for (float param = 0.0f ; param <= 1.0f ; param+=.05f)
    {
        gui_display_float(param);
        HAL_Delay(50);
    }
    gui_display_float(1.0f);

    for (float inl = 1.0f ; inl >= 0.0f ; inl-=.05f)
    {
        gui_update_levels(inl, 1.0f, 1.0f, 1.0f);
        HAL_Delay(50);
    }

    for (float inr = 1.0f ; inr >= 0.0f ; inr-=.05f)
    {
        gui_update_levels(0.0f, inr, 1.0f, 1.0f);
        HAL_Delay(50);
    }

    for (float outl = 1.0f ; outl >= 0.0f ; outl-=.05f)
    {
        gui_update_levels(0.0f, 0.0f, outl, 1.0f);
        HAL_Delay(50);
    }

    for (float outr = 1.0f ; outr >= 0.0f ; outr-=.05f)
    {
        gui_update_levels(0.0f, 0.0f, 0.0f, outr);
        HAL_Delay(50);
    }
    gui_update_levels(0.0f, 0.0f, 0.0f, 0.0f);

    // Param to 0
    for (float param = 1.0f ; param >= 0.0f ; param-=.05f)
    {
        gui_display_float(param);
        HAL_Delay(50);
    }
    gui_display_float(0.0f);
}

static void erase_param(void)
{
    ssd1306_FillRectangle(20, 20, 100, 31, Black);
}