/*
 * sgtl5000.c
 *
 *  Created on: Nov 22, 2023
 *      Author: laurentf
 */

#include "sgtl5000.h"

static HAL_StatusTypeDef sgtl5000_i2c_read_register(h_sgtl5000_t * h_sgtl5000, sgtl5000_registers_t reg_address, uint16_t * p_data)
{
	HAL_StatusTypeDef ret;

	uint8_t buffer[2];

	ret = HAL_I2C_Mem_Read (
			h_sgtl5000->hi2c,
			h_sgtl5000->dev_address,
			reg_address,
			I2C_MEMADD_SIZE_16BIT,
			buffer,
			2,
			HAL_MAX_DELAY		// Problems if I put other than HAL_MAX_DELAY WTF
	);

	*p_data = (buffer[0] << 8) | buffer[1];

	return ret;
}

static HAL_StatusTypeDef sgtl5000_i2c_write_register(h_sgtl5000_t * h_sgtl5000, sgtl5000_registers_t reg_address, uint16_t data)
{
	HAL_StatusTypeDef ret;
	uint8_t buffer[2];

	buffer[0] = (data >> 8) & 0xFF;
	buffer[1] = data & 0xFF;

	ret = HAL_I2C_Mem_Write(
			h_sgtl5000->hi2c,
			h_sgtl5000->dev_address,
			reg_address,
			I2C_MEMADD_SIZE_16BIT,
			buffer,
			2,
			HAL_MAX_DELAY		// WTF
	);

	return ret;
}

static HAL_StatusTypeDef sgtl5000_i2c_set_bit(h_sgtl5000_t * h_sgtl5000, sgtl5000_registers_t reg_address, uint16_t mask)
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

static HAL_StatusTypeDef sgtl5000_i2c_clear_bit(h_sgtl5000_t * h_sgtl5000, sgtl5000_registers_t reg_address, uint16_t mask)
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

static void sgtl5000_start_clock(h_sgtl5000_t * h_sgtl5000)
{
	__HAL_SAI_ENABLE(h_sgtl5000->hsai_tx);

}

HAL_StatusTypeDef sgtl5000_init(h_sgtl5000_t * h_sgtl5000)
{
	HAL_StatusTypeDef ret = HAL_OK;
	uint16_t mask;

	sgtl5000_start_clock(h_sgtl5000);

	uint16_t chip_id;
	ret = sgtl5000_i2c_read_register(h_sgtl5000, SGTL5000_CHIP_ID, &chip_id);
	if (ret != HAL_OK) return ret;

	// TODO replace with propoer error management
	printf("CHIP ID = 0x%4X\r\n", chip_id);

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
	ret = sgtl5000_i2c_clear_bit(h_sgtl5000, SGTL5000_CHIP_ANA_POWER, mask);
	if (ret != HAL_OK) return ret;

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
//	mask = (1 << 5) | (1 << 6);
//	sgtl5000_i2c_set_bit(h_sgtl5000, SGTL5000_CHIP_LINREG_CTRL, mask);
	mask = 0x006C;
	ret = sgtl5000_i2c_write_register(h_sgtl5000, SGTL5000_CHIP_REF_CTRL, mask);
	if (ret != HAL_OK) return ret;

	//---- Reference Voltage and Bias Current Configuration----
	// NOTE: The value written in the next 2 Write calls is dependent
	// on the VDDA voltage value.
	// Set ground, ADC, DAC reference voltage (bits 8:4). The value should
	// be set to VDDA/2. This example assumes VDDA = 1.8 V. VDDA/2 = 0.9 V.
	// The bias current should be set to 50% of the nominal value (bits 3:1)
	// Write CHIP_REF_CTRL 0x004E
	mask = 0x01F2;	// VAG=1.575, normal ramp, +12.5% bias current
	ret = sgtl5000_i2c_write_register(h_sgtl5000, SGTL5000_CHIP_REF_CTRL, mask);
	if (ret != HAL_OK) return ret;

	// Set LINEOUT reference voltage to VDDIO/2 (1.65 V) (bits 5:0)
	// and bias current (bits 11:8) to the recommended value of 0.36 mA
	// for 10 kOhm load with 1.0 nF capacitance
	// Write CHIP_LINE_OUT_CTRL 0x0322
//	mask = 0x0322;	// LO_VAGCNTRL = 1.65V, OUT_CURRENT = 0.36mA (?)
	mask = 0x0F22;	// LO_VAGCNTRL=1.65V, OUT_CURRENT=0.54mA
	ret = sgtl5000_i2c_write_register(h_sgtl5000, SGTL5000_CHIP_LINE_OUT_CTRL, mask);
	if (ret != HAL_OK) return ret;

	//------------Other Analog Block Configurations--------------
	// Configure slow ramp up rate to minimize pop (bit 0)
	// Write CHIP_REF_CTRL 0x004F
	// Déjà fait

	// Enable short detect mode for headphone left/right
	// and center channel and set short detect current trip level
	// to 75 mA
	// Write CHIP_SHORT_CTRL 0x1106
	mask = 0x4446;	// allow up to 125mA
	ret = sgtl5000_i2c_write_register(h_sgtl5000, SGTL5000_CHIP_SHORT_CTRL, mask);
	if (ret != HAL_OK) return ret;

	// Enable Zero-cross detect if needed for HP_OUT (bit 5) and ADC (bit 1)
	// Write CHIP_ANA_CTRL 0x0133
	mask = 0x0004;	// Unmute all + SELECT_ADC = LINEIN
//	mask = 0x0000;	// Unmute all + SELECT_ADC = MIC
	ret = sgtl5000_i2c_write_register(h_sgtl5000, SGTL5000_CHIP_ANA_CTRL, mask);
	if (ret != HAL_OK) return ret;

	//------------Power up Inputs/Outputs/Digital Blocks---------
	// Power up LINEOUT, HP, ADC, DAC
	// Write CHIP_ANA_POWER 0x6AFF
	mask = 0x6AFF;	// LINEOUT_POWERUP, ADC_POWERUP, CAPLESS_HEADPHONE_POWERUP, DAC_POWERUP, HEADPHONE_POWERUP, REFTOP_POWERUP, ADC_MONO = stereo
	// VAG_POWERUP, VCOAMP_POWERUP = 0, LINREG_D_POWERUP, PLL_POWERUP = 0, VDDC_CHRGPMP_POWERUP, STARTUP_POWERUP = 0, LINREG_SIMPLE_POWERUP,
	// DAC_MONO = stereo
	ret = sgtl5000_i2c_write_register(h_sgtl5000, SGTL5000_CHIP_ANA_POWER, mask);
	if (ret != HAL_OK) return ret;
	// Power up desired digital blocks
	// I2S_IN (bit 0), I2S_OUT (bit 1), DAP (bit 4), DAC (bit 5),
	// ADC (bit 6) are powered on
	// Write CHIP_DIG_POWER 0x0073
	mask = 0x0073;	// I2S_IN_POWERUP, I2S_OUT_POWERUP, DAP_POWERUP, DAC_POWERUP, ADC_POWERUP
	ret = sgtl5000_i2c_write_register(h_sgtl5000, SGTL5000_CHIP_DIG_POWER, mask);
	if (ret != HAL_OK) return ret;

	//----------------Set LINEOUT Volume Level-------------------
	// Set the LINEOUT volume level based on voltage reference (VAG)
	// values using this formula
	// Value = (int)(40*log(VAG_VAL/LO_VAGCNTRL) + 15)
	// Assuming VAG_VAL and LO_VAGCNTRL is set to 0.9 V and
	// 1.65 V respectively, the // left LO vol (bits 12:8) and right LO
	// volume (bits 4:0) value should be set // to 5
	// Write CHIP_LINE_OUT_VOL 0x0505
	mask = 0x1111;	// TODO recalculer
	ret = sgtl5000_i2c_write_register(h_sgtl5000, SGTL5000_CHIP_LINE_OUT_VOL, mask);
	if (ret != HAL_OK) return ret;

	/* System MCLK and Sample Clock */

	// Configure SYS_FS clock to 48 kHz
	// Configure MCLK_FREQ to 256*Fs
	// Modify CHIP_CLK_CTRL->SYS_FS 0x0002 // bits 3:2
	// Modify CHIP_CLK_CTRL->MCLK_FREQ 0x0000 // bits 1:0
	mask = 0x0004;	// SYS_FS = 48kHz
	ret = sgtl5000_i2c_write_register(h_sgtl5000, SGTL5000_CHIP_CLK_CTRL, mask);
	if (ret != HAL_OK) return ret;
	// Configure the I2S clocks in master mode
	// NOTE: I2S LRCLK is same as the system sample clock
	// Modify CHIP_I2S_CTRL->MS 0x0001 // bit 7
	// Non, on reste en slave!
	mask = 0x0130;	// DLEN = 16 bits
	ret = sgtl5000_i2c_write_register(h_sgtl5000, SGTL5000_CHIP_I2S_CTRL, mask);
	if (ret != HAL_OK) return ret;

	/* PLL Configuration */
	// Pas utilisé

	/* Input/Output Routing */
	// Laissons tout par défaut pour l'instant
	mask = 0x0010;	// ADC -> DAC
	ret = sgtl5000_i2c_write_register(h_sgtl5000, SGTL5000_CHIP_SSS_CTRL, mask);
	if (ret != HAL_OK) return ret;

	/* Le reste */
	mask = 0x0000;	// Unmute
	ret = sgtl5000_i2c_write_register(h_sgtl5000, SGTL5000_CHIP_ADCDAC_CTRL, mask);
	if (ret != HAL_OK) return ret;

	mask = 0x3C3C;
//	mask = 0x4747;
	ret = sgtl5000_i2c_write_register(h_sgtl5000, SGTL5000_CHIP_DAC_VOL, mask);
	if (ret != HAL_OK) return ret;

	mask = 0x7F7F;
	ret = sgtl5000_i2c_write_register(h_sgtl5000, SGTL5000_CHIP_ANA_HP_CTRL, mask);
	if (ret != HAL_OK) return ret;

	mask = 0x0D0D;
	ret = sgtl5000_i2c_write_register(h_sgtl5000, SGTL5000_CHIP_LINE_OUT_VOL, mask);
	if (ret != HAL_OK) return ret;

//	mask = 0x0251;	// BIAS_RESISTOR = 2, BIAS_VOLT = 5, GAIN = 1
//	sgtl5000_i2c_write_register(h_sgtl5000, SGTL5000_CHIP_MIC_CTRL, mask);

//	for (int i = 0 ; register_map[i] != SGTL5000_DAP_COEF_WR_A2_LSB ; i++)
//	{
//		uint16_t reg = 0;
//		sgtl5000_i2c_read_register(h_sgtl5000, register_map[i], &reg);
//		printf("%02d: [0x%04x] = 0x%04x\r\n", i, register_map[i], reg);
//	}

	return ret;
}

HAL_StatusTypeDef sgtl5000_start(h_sgtl5000_t * h_sgtl5000)
{
	HAL_StatusTypeDef ret;

	// Last parameter is the number of DMA CYCLES (here a cycle is 16 bits/2Bytes)
	ret = HAL_SAI_Receive_DMA(h_sgtl5000->hsai_rx, (uint8_t*) h_sgtl5000->sai_rx_buffer, AUDIO_BUFFER_LENGTH*4);
	if (ret != HAL_OK) return ret;

	ret = HAL_SAI_Transmit_DMA(h_sgtl5000->hsai_tx, (uint8_t*) h_sgtl5000->sai_tx_buffer, AUDIO_BUFFER_LENGTH*4);
	if (ret != HAL_OK) return ret;

	return HAL_OK;
}