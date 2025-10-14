/*
 * sgtl5000.c
 *
 *  Created on: Oct 14, 2025
 *      Author: laurentf
 */

#include "sgtl5000.h"

HAL_StatusTypeDef sgtl5000_i2c_read_register(h_sgtl5000_t * h_sgtl5000, sgtl5000_registers_t reg_address, uint16_t * p_data)
{
	HAL_StatusTypeDef ret;

	uint8_t buffer[2];

	ret = HAL_I2C_Mem_Read (
			h_sgtl5000->hi2c,
			h_sgtl5000->i2c_address << 1,
			reg_address,
			I2C_MEMADD_SIZE_16BIT,
			buffer,
			2,
			1000
	);

	*p_data = (buffer[0] << 8) | buffer[1];

	return ret;
}

HAL_StatusTypeDef sgtl5000_i2c_write_register(h_sgtl5000_t * h_sgtl5000, sgtl5000_registers_t reg_address, uint16_t data)
{
	HAL_StatusTypeDef ret;
	uint8_t buffer[2];

	buffer[0] = (data >> 8) & 0xFF;
	buffer[1] = data & 0xFF;

	ret = HAL_I2C_Mem_Write(
			h_sgtl5000->hi2c,
			h_sgtl5000->i2c_address << 1,
			reg_address,
			I2C_MEMADD_SIZE_16BIT,
			buffer,
			2,
			1000
	);

	return ret;
}

HAL_StatusTypeDef sgtl5000_enable(h_sgtl5000_t * h_sgtl5000)
{
	HAL_StatusTypeDef ret;

	// Start SAI clock for SGTL5000
	__HAL_SAI_ENABLE(h_sgtl5000->hsai_tx);

	ret = sgtl5000_i2c_write_register(h_sgtl5000, SGTL5000_CHIP_ANA_POWER, 0x4060); // VDDD is externally driven with 1.8V
	if (ret != HAL_OK) return ret;

	ret = sgtl5000_i2c_write_register(h_sgtl5000, SGTL5000_CHIP_LINREG_CTRL, 0x006C);  // VDDA & VDDIO both over 3.1V
	if (ret != HAL_OK) return ret;

	ret = sgtl5000_i2c_write_register(h_sgtl5000, SGTL5000_CHIP_REF_CTRL, 0x01F2); // VAG=1.575, normal ramp, +12.5% bias current
	if (ret != HAL_OK) return ret;

	ret = sgtl5000_i2c_write_register(h_sgtl5000, SGTL5000_CHIP_LINE_OUT_CTRL, 0x0F22); // LO_VAGCNTRL=1.65V, OUT_CURRENT=0.54mA
	if (ret != HAL_OK) return ret;

	ret = sgtl5000_i2c_write_register(h_sgtl5000, SGTL5000_CHIP_SHORT_CTRL, 0x4446);  // allow up to 125mA
	if (ret != HAL_OK) return ret;

	ret = sgtl5000_i2c_write_register(h_sgtl5000, SGTL5000_CHIP_ANA_CTRL, 0x0137);  // enable zero cross detectors
	if (ret != HAL_OK) return ret;

	//SGTL is I2S Slave
	ret = sgtl5000_i2c_write_register(h_sgtl5000, SGTL5000_CHIP_ANA_POWER, 0x40FF); // power up: lineout, hp, adc, dac
	if (ret != HAL_OK) return ret;

	ret = sgtl5000_i2c_write_register(h_sgtl5000, SGTL5000_CHIP_DIG_POWER, 0x0073); // power up all digital stuff
	if (ret != HAL_OK) return ret;

	HAL_Delay(400);	// Why ?
	ret = sgtl5000_i2c_write_register(h_sgtl5000, SGTL5000_CHIP_LINE_OUT_VOL, 0x1D1D); // default approx 1.3 volts peak-to-peak
	if (ret != HAL_OK) return ret;

	ret = sgtl5000_i2c_write_register(h_sgtl5000, SGTL5000_CHIP_CLK_CTRL, 0x0008);  // 48 kHz, 256*Fs
	if (ret != HAL_OK) return ret;

	ret = sgtl5000_i2c_write_register(h_sgtl5000, SGTL5000_CHIP_I2S_CTRL, 0x0030); // SCLK=64*Fs, 16bit, I2S format
	if (ret != HAL_OK) return ret;

	// default signal routing is ok?
	ret = sgtl5000_i2c_write_register(h_sgtl5000, SGTL5000_CHIP_SSS_CTRL, 0x0010); // ADC->I2S, I2S->DAC
	if (ret != HAL_OK) return ret;

	ret = sgtl5000_i2c_write_register(h_sgtl5000, SGTL5000_CHIP_ADCDAC_CTRL, 0x0000); // disable dac mute
	if (ret != HAL_OK) return ret;

	ret = sgtl5000_i2c_write_register(h_sgtl5000, SGTL5000_CHIP_DAC_VOL, 0x3C3C); // digital gain, 0dB
	if (ret != HAL_OK) return ret;

	ret = sgtl5000_i2c_write_register(h_sgtl5000, SGTL5000_CHIP_ANA_HP_CTRL, 0x7F7F); // set volume (lowest level)
	if (ret != HAL_OK) return ret;

	ret = sgtl5000_i2c_write_register(h_sgtl5000, SGTL5000_CHIP_ANA_CTRL, 0x0036);  // enable zero cross detectors
	if (ret != HAL_OK) return ret;

	return HAL_OK;
}

// CHIP_ANA_ADC_CTRL
// Actual measured full-scale peak-to-peak sine wave input for max signal
//  0: 3.12 Volts p-p
//  1: 2.63 Volts p-p
//  2: 2.22 Volts p-p
//  3: 1.87 Volts p-p
//  4: 1.58 Volts p-p
//  5: 1.33 Volts p-p
//  6: 1.11 Volts p-p
//  7: 0.94 Volts p-p
//  8: 0.79 Volts p-p
//  9: 0.67 Volts p-p
// 10: 0.56 Volts p-p
// 11: 0.48 Volts p-p
// 12: 0.40 Volts p-p
// 13: 0.34 Volts p-p
// 14: 0.29 Volts p-p
// 15: 0.24 Volts p-p
HAL_StatusTypeDef sgtl5000_line_in_level(h_sgtl5000_t * h_sgtl5000, uint8_t left, uint8_t right)
{
	if (left > 15) left = 15;
	if (right > 15) right = 15;

	return sgtl5000_i2c_write_register(h_sgtl5000, SGTL5000_CHIP_ANA_ADC_CTRL, (left << 4) | right);
}

// CHIP_LINE_OUT_VOL
//  Actual measured full-scale peak-to-peak sine wave output voltage:
//  0-12: output has clipping
//  13: 3.16 Volts p-p
//  14: 2.98 Volts p-p
//  15: 2.83 Volts p-p
//  16: 2.67 Volts p-p
//  17: 2.53 Volts p-p
//  18: 2.39 Volts p-p
//  19: 2.26 Volts p-p
//  20: 2.14 Volts p-p
//  21: 2.02 Volts p-p
//  22: 1.91 Volts p-p
//  23: 1.80 Volts p-p
//  24: 1.71 Volts p-p
//  25: 1.62 Volts p-p
//  26: 1.53 Volts p-p
//  27: 1.44 Volts p-p
//  28: 1.37 Volts p-p
//  29: 1.29 Volts p-p
//  30: 1.22 Volts p-p
//  31: 1.16 Volts p-p
HAL_StatusTypeDef sgtl5000_line_out_level(h_sgtl5000_t * h_sgtl5000, uint8_t left, uint8_t right)
{
	if (left > 31) left = 31;
	else if (left < 13) left = 13;
	if (right > 31) right = 31;
	else if (right < 13) right = 13;

	return sgtl5000_i2c_write_register(h_sgtl5000, SGTL5000_CHIP_LINE_OUT_VOL,(right<<8)|left);
}

HAL_StatusTypeDef sgtl5000_unmute(h_sgtl5000_t * h_sgtl5000)
{
	HAL_StatusTypeDef ret;
	ret = sgtl5000_i2c_write_register(h_sgtl5000, SGTL5000_CHIP_ADCDAC_CTRL, 0x0000);
	if (ret != HAL_OK) return ret;

	ret = sgtl5000_i2c_write_register(h_sgtl5000, SGTL5000_CHIP_DAC_VOL, 0x3C3C);
	if (ret != HAL_OK) return ret;

	return HAL_OK;
}

HAL_StatusTypeDef sgtl5000_i2s_start(h_sgtl5000_t * h_sgtl5000)
{
	return HAL_SAI_Transmit_DMA(h_sgtl5000->hsai_tx, (uint8_t*) h_sgtl5000->tx_buffer, SGTL5000_TX_BUFFER_LENGTH);
}

