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


HAL_StatusTypeDef sgtl5000_i2c_set_bit(h_sgtl5000_t * h_sgtl5000, sgtl5000_registers_t reg_address, uint16_t mask)
{
	HAL_StatusTypeDef ret;
	uint16_t data;
	ret = sgtl5000_i2c_read_register(h_sgtl5000, reg_address, &data);
	if (ret != HAL_OK)
	{
		return ret;
	}

	data |= mask;

	ret = sgtl5000_i2c_write_register(h_sgtl5000, reg_address, data);
	return ret;
}

HAL_StatusTypeDef sgtl5000_i2c_clear_bit(h_sgtl5000_t * h_sgtl5000, sgtl5000_registers_t reg_address, uint16_t mask)
{
	HAL_StatusTypeDef ret;
	uint16_t data;
	ret = sgtl5000_i2c_read_register(h_sgtl5000, reg_address, &data);
	if (ret != HAL_OK)
	{
		return ret;
	}

	data &= (~mask);

	ret = sgtl5000_i2c_write_register(h_sgtl5000, reg_address, data);
	return ret;
}

HAL_StatusTypeDef sgtl5000_init(h_sgtl5000_t * h_sgtl5000)
{
	HAL_StatusTypeDef ret = HAL_OK;
	uint16_t mask;

	/* Chip Powerup and Supply Configurations */

	//--------------- Power Supply Configuration----------------
	// NOTE: This next 2 Write calls is needed ONLY if VDDD is
	// Configure VDDD level to 1.8V (bits 3:0)
	// Write CHIP_LINREG_CTRL 0x????
	// OK, pas touche!
	// Power up internal linear regulator (Set bit 9)
	// Write CHIP_ANA_POWER 0x7260
	// Pas touche non plus

	// NOTE: This next Write call is needed ONLY if VDDD is
	// externally driven
	// Turn off startup power supplies to save power (Clear bit 12 and 13)
	// Write CHIP_ANA_POWER 0x4260
	mask = (1 << 12) | (1 << 13);
	sgtl5000_i2c_clear_bit(h_sgtl5000, SGTL5000_CHIP_ANA_POWER, mask);

	// NOTE: The next Write calls is needed only if both VDDA and
	// VDDIO power supplies are less than 3.1V.
	// Enable the internal oscillator for the charge pump (Set bit 11)
	// Write CHIP_CLK_TOP_CTRL 0x0800
	// Enable charge pump (Set bit 11)
	// Write CHIP_ANA_POWER 0x4A60
	// VDDA and VDDIO = 3.3V so not necessary

	// NOTE: The next modify call is only needed if both VDDA and
	// VDDIO are greater than 3.1 V
	// Configure the charge pump to use the VDDIO rail (set bit 5 and bit 6)
	// Write CHIP_LINREG_CTRL 0x006C
	// VDDA and VDDIO = 3.3V so it IS necessary
	mask = (1 << 5) | (1 << 6);
	sgtl5000_i2c_set_bit(h_sgtl5000, SGTL5000_CHIP_LINREG_CTRL, mask);

	//---- Reference Voltage and Bias Current Configuration----
	// NOTE: The value written in the next 2 Write calls is dependent
	// on the VDDA voltage value.
	// Set ground, ADC, DAC reference voltage (bits 8:4). The value should
	// be set to VDDA/2. This example assumes VDDA = 1.8 V. VDDA/2 = 0.9 V.
	// The bias current should be set to 50% of the nominal value (bits 3:1)
	// Write CHIP_REF_CTRL 0x004E
	mask = 0x01FF;	// VAG_VAL = 1.575V, BIAS_CTRL = -50%, SMALL_POP = 1
	sgtl5000_i2c_write_register(h_sgtl5000, SGTL5000_CHIP_REF_CTRL, mask);

	// Set LINEOUT reference voltage to VDDIO/2 (1.65 V) (bits 5:0)
	// and bias current (bits 11:8) to the recommended value of 0.36 mA
	// for 10 kOhm load with 1.0 nF capacitance
	// Write CHIP_LINE_OUT_CTRL 0x0322
//	mask = 0x0322;	// LO_VAGCNTRL = 1.65V, OUT_CURRENT = 0.36mA (?)
	mask = 0x031E;
	sgtl5000_i2c_write_register(h_sgtl5000, SGTL5000_CHIP_LINE_OUT_CTRL, mask);

	//------------Other Analog Block Configurations--------------
	// Configure slow ramp up rate to minimize pop (bit 0)
	// Write CHIP_REF_CTRL 0x004F
	// Déjà fait

	// Enable short detect mode for headphone left/right
	// and center channel and set short detect current trip level
	// to 75 mA
	// Write CHIP_SHORT_CTRL 0x1106
	mask = 0x1106;	// MODE_CM = 2, MODE_LR = 1, LVLADJC = 200mA, LVLADJL = 75mA, LVLADJR = 50mA
	sgtl5000_i2c_write_register(h_sgtl5000, SGTL5000_CHIP_SHORT_CTRL, mask);

	// Enable Zero-cross detect if needed for HP_OUT (bit 5) and ADC (bit 1)
	// Write CHIP_ANA_CTRL 0x0133
	mask = 0x0004;	// Unmute all + SELECT_ADC = LINEIN
//	mask = 0x0000;	// Unmute all + SELECT_ADC = MIC
	sgtl5000_i2c_write_register(h_sgtl5000, SGTL5000_CHIP_ANA_CTRL, mask);

	//------------Power up Inputs/Outputs/Digital Blocks---------
	// Power up LINEOUT, HP, ADC, DAC
	// Write CHIP_ANA_POWER 0x6AFF
	mask = 0x6AFF;	// LINEOUT_POWERUP, ADC_POWERUP, CAPLESS_HEADPHONE_POWERUP, DAC_POWERUP, HEADPHONE_POWERUP, REFTOP_POWERUP, ADC_MONO = stereo
	// VAG_POWERUP, VCOAMP_POWERUP = 0, LINREG_D_POWERUP, PLL_POWERUP = 0, VDDC_CHRGPMP_POWERUP, STARTUP_POWERUP = 0, LINREG_SIMPLE_POWERUP,
	// DAC_MONO = stereo
	sgtl5000_i2c_write_register(h_sgtl5000, SGTL5000_CHIP_ANA_POWER, mask);
	// Power up desired digital blocks
	// I2S_IN (bit 0), I2S_OUT (bit 1), DAP (bit 4), DAC (bit 5),
	// ADC (bit 6) are powered on
	// Write CHIP_DIG_POWER 0x0073
	mask = 0x0073;	// I2S_IN_POWERUP, I2S_OUT_POWERUP, DAP_POWERUP, DAC_POWERUP, ADC_POWERUP
	sgtl5000_i2c_write_register(h_sgtl5000, SGTL5000_CHIP_DIG_POWER, mask);

	//----------------Set LINEOUT Volume Level-------------------
	// Set the LINEOUT volume level based on voltage reference (VAG)
	// values using this formula
	// Value = (int)(40*log(VAG_VAL/LO_VAGCNTRL) + 15)
	// Assuming VAG_VAL and LO_VAGCNTRL is set to 0.9 V and
	// 1.65 V respectively, the // left LO vol (bits 12:8) and right LO
	// volume (bits 4:0) value should be set // to 5
	// Write CHIP_LINE_OUT_VOL 0x0505
	mask = 0x1111;	// TODO recalculer
	sgtl5000_i2c_write_register(h_sgtl5000, SGTL5000_CHIP_LINE_OUT_VOL, mask);

	/* System MCLK and Sample Clock */

	// Configure SYS_FS clock to 48 kHz
	// Configure MCLK_FREQ to 256*Fs
	// Modify CHIP_CLK_CTRL->SYS_FS 0x0002 // bits 3:2
	// Modify CHIP_CLK_CTRL->MCLK_FREQ 0x0000 // bits 1:0
	mask = 0x0004;	// SYS_FS = 48kHz
	sgtl5000_i2c_write_register(h_sgtl5000, SGTL5000_CHIP_CLK_CTRL, mask);
	// Configure the I2S clocks in master mode
	// NOTE: I2S LRCLK is same as the system sample clock
	// Modify CHIP_I2S_CTRL->MS 0x0001 // bit 7
	// Non, on reste en slave!
	mask = 0x0130;	// DLEN = 16 bits
	sgtl5000_i2c_write_register(h_sgtl5000, SGTL5000_CHIP_I2S_CTRL, mask);

	/* PLL Configuration */
	// Pas utilisé

	/* Input/Output Routing */
	// Laissons tout par défaut pour l'instant
//	mask = 0x0000;	// ADC -> DAC
//	sgtl5000_i2c_write_register(h_sgtl5000, SGTL5000_CHIP_SSS_CTRL, mask);

	/* Le reste */
	mask = 0x0000;	// Unmute
	sgtl5000_i2c_write_register(h_sgtl5000, SGTL5000_CHIP_ADCDAC_CTRL, mask);

	mask = 0x3C3C;
//	mask = 0x4747;
	sgtl5000_i2c_write_register(h_sgtl5000, SGTL5000_CHIP_DAC_VOL, mask);

	mask = 0x0251;	// BIAS_RESISTOR = 2, BIAS_VOLT = 5, GAIN = 1
	sgtl5000_i2c_write_register(h_sgtl5000, SGTL5000_CHIP_MIC_CTRL, mask);

//	for (int i = 0 ; register_map[i] != SGTL5000_DAP_COEF_WR_A2_LSB ; i++)
//	{
//		uint16_t reg = 0;
//		sgtl5000_i2c_read_register(h_sgtl5000, register_map[i], &reg);
//		printf("%02d: [0x%04x] = 0x%04x\r\n", i, register_map[i], reg);
//	}

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

