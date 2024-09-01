/*
 * STM-Frontend Platform header file
 *
 * Copyright (c) STMicroelectronics 2005
 *
 * Author:Peter Bennett <peter.bennett@st.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef __STM_FRONTEND_H
#define __STM_FRONTEND_H

#define STM_DEMOD_ALL 0x00000f
#define STM_TUNER_ALL 0x0000f0
#define STM_LNB_ALL 0x00f000

#define STM_GET_CHANNEL(x) ((x & 0x0f0000) >> 16)

enum plat_frontend_options
{
	STM_DEMOD_299 = 0x000001,
	STM_DEMOD_360 = 0x000002,
	STM_DEMOD_NONE = 0x00000f,
	STM_DEMOD_AUTO = 0,

	STM_TUNER_STB6000 = 0x000010,
	STM_TUNER_SHARP = 0x000020,
	STM_TUNER_TMM = 0x000030,
	STM_TUNER_AUTO = 0,

	STM_DISEQC_299 = 0x000100,
	STM_DISEQC_STONCHIP = 0x000200,

	STM_LNB_LNBH221 = 0x001000,
	STM_LNB_AUTO = 0,

	STM_TSM_CHANNEL_0 = 0x000000,
	STM_TSM_CHANNEL_1 = 0x010000,
	STM_TSM_CHANNEL_2 = 0x020000,
	STM_TSM_CHANNEL_3 = 0x030000,

	STM_SERIAL_NOT_PARALLEL = 0x100000,
	STM_INVERT_CLOCK = 0x200000,
	STM_PACKET_CLOCK_VALID = 0x400000
};

struct plat_frontend_channel
{
	enum plat_frontend_options option;
	unsigned char demod_i2c_bus;
	unsigned char demod_i2c_address;
	unsigned char demod_i2c_ad01;

	unsigned char tuner_i2c_bus;
	unsigned char tuner_i2c_address;

	unsigned char lnb_i2c_bus;
	unsigned char lnb_i2c_address;

	unsigned char lock;
	unsigned char drop;

	unsigned char pio_reset_bank;
	unsigned char pio_reset_pin;
};

struct plat_frontend_config
{
	unsigned long diseqc_address;
	unsigned long diseqc_irq;
	unsigned char diseqc_pio_bank;
	unsigned char diseqc_pio_pin;

	unsigned long tsm_base_address;

	unsigned long tsm_sram_buffer_size;
	unsigned long tsm_num_pti_alt_out;
	unsigned long tsm_num_1394_alt_out;

	int nr_channels;
	struct plat_frontend_channel *channels;
};

#endif
