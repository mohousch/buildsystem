/*
 * @brief stv6306.c
 *
 * @author Pedro Aguilar <pedro@duolabs.com>
 *
 * @brief Availink avl2108 - DVBS/S2 Satellite demod driver with Sharp BS2S7HZ6360 tuner
 *
 * Copyright (C) 2009-2010 Duolabs Spa
 * 2011 adapted by konfetti for use with ufs922, octagon1008 and atevio7500
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#define STV6306_DEMOD_FW "dvb-fe-stv6306.fw" /*< Tuner fw used for working in internal mode */

#include <linux/firmware.h>

#include "avl2108.h"
#include "avl2108_reg.h"

extern short paramDebug;
#define TAGDEBUG "[stv6306] "

#define dprintk(level, x...) do { \
		if ((paramDebug) && (paramDebug > level)) printk(TAGDEBUG x); \
	} while (0)

struct stv6306_state
{
	struct avl2108_equipment_s equipment;
	u8 internal;
	struct i2c_adapter *i2c;
};

/* *********** external tuner ************** */

/**
 * @brief Tuner lock
 * @param fe Ptr to the generic DVB frontend struct
 * @param freq Frequency to be locked
 * @param lpf Low Pass Filter = TUNER_LPF
 * @return Returns AVL2108_OK if successful, AVL2108_ERROR_* otherwise
 */
static u16 stv6306_tuner_lock_ext(struct dvb_frontend *fe, u32 freq, u32 srate, u32 lpf)
{
	struct stv6306_state *state = fe->tuner_priv;
	u8 data[4];
	u16 ret, wsize, div, mod, shift;
	u32 l_lpf;
#warning "____THINK ON THIS: SETTING THIS TO ZERO AND THEN MAKE AN _AND_ BELOW IS NOT THE BEST____"
	memset(data, 0, sizeof(data));
	if (freq < 9500)
		return AVL2108_ERROR_GENERIC;
	else if (freq < 9860)
	{
		data[3] &= ~(0x7 << 5);
		data[3] |= (0x5 << 5);
		wsize = 16;
		shift = 1;
	}
	else if (freq < 10730)
	{
		data[3] &= ~(0x7 << 5);
		data[3] |= (0x6 << 5);
		wsize = 16;
		shift = 1;
	}
	else if (freq < 11540)
	{
		data[3] &= ~(0x7 << 5);
		data[3] |= (0x7 << 5);
		wsize = 32;
		shift = 1;
	}
	else if (freq < 12910)
	{
		data[3] &= ~(0x7 << 5);
		data[3] |= (0x1 << 5);
		wsize = 32;
		shift = 0;
	}
	else if (freq < 14470)
	{
		data[3] &= ~(0x7 << 5);
		data[3] |= (0x2 << 5);
		wsize = 32;
		shift = 0;
	}
	else if (freq < 16150)
	{
		data[3] &= ~(0x7 << 5);
		data[3] |= (0x3 << 5);
		wsize = 32;
		shift = 0;
	}
	else if (freq < 17910)
	{
		data[3] &= ~(0x7 << 5);
		data[3] |= (0x4 << 5);
		wsize = 32;
		shift = 0;
	}
	else if (freq < 19720)
	{
		data[3] &= ~(0x7 << 5);
		data[3] |= (0x5 << 5);
		wsize = 32;
		shift = 0;
	}
	else if (freq <= 21500)
	{
		data[3] &= ~(0x7 << 5);
		data[3] |= (0x6 << 5);
		wsize = 32;
		shift = 0;
	}
	else
		return AVL2108_ERROR_GENERIC;
	mod = (freq / 10) % wsize;
	div = (freq / 10) / wsize;
	data[3] &= ~(0x1 << 4);
	if (wsize == 16)
		data[3] |= (0x1 << 4);
	data[3] &= ~(0x1 << 1);
	data[3] |= (u8)(shift << 1);
	data[1] &= ~(0x1f << 0);
	data[1] |= (u8)(mod << 0);
	data[1] &= ~(0x7 << 5);
	data[1] |= (u8)(div << 5);
	data[0] &= ~(0x1f << 0);
	data[0] |= (u8)((div >> 3) << 0);
	/* Charge pump */
	data[2] &= ~(0x3 << 5);
	data[2] |= (0x2 << 5);
	/* BB Gain */
	data[0] &= ~(0x3 << 5);
	data[0] |= (0x3 << 5);
	/* Commit */
	data[0] &= 0x7f;
	data[2] |= 0x80;
	data[2] &= ~(0x7 << 2);
	data[3] &= ~(0x3 << 2);
	ret = state->equipment.demod_i2c_repeater_send(fe->demodulator_priv, data, 4);
	if (ret != AVL2108_OK)
		return ret;
	data[2] |= (0x1 << 2);
	ret = state->equipment.demod_i2c_repeater_send(fe->demodulator_priv, data + 2, 1);
	if (ret != AVL2108_OK)
		return ret;
	msleep(/* 12*/ 3);
	/* Set LPF */
	l_lpf = ((lpf / 10) - 10) / 2 + 3 ;
	data[2] |= (((l_lpf >> 1) & 0x1) << 3);
	data[2] |= (((l_lpf >> 0) & 0x1) << 4);
	data[3] |= (((l_lpf >> 3) & 0x1) << 2);
	data[3] |= (((l_lpf >> 2) & 0x1) << 3);
	ret = state->equipment.demod_i2c_repeater_send(fe->demodulator_priv, data + 2, 2);
	dprintk(50, "Leaving %s() with status %u\n", __func__, ret);
	return ret;
}

static u16 stv6306_tuner_lock_status_ext(struct dvb_frontend *fe)
{
	struct stv6306_state *state = fe->tuner_priv;
	u8 buf;
	u16 ret;
	ret = state->equipment.demod_i2c_repeater_recv(fe->demodulator_priv, &buf, 1);
	if (ret == AVL2108_OK)
		if ((buf & 0x40) == 0)
			ret = AVL2108_ERROR_PREV;
	dprintk(50, "%s(): lock status: %u, buf: 0x%X\n", __func__, ret, buf);
	return ret;
}

/* *********** internal tuner ************** */

static u16 stv6306_load_firmware(struct dvb_frontend *fe)
{
	struct stv6306_state *state = fe->tuner_priv;
	const struct firmware *fw;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,32)
	u8 *buffer;
#endif
	u16 ret = AVL2108_OK;
	u32 buf_size, data_size;
	u32 i = 4;
	int fw_ret;
	dprintk(5, "%s(): Uploading tuner firmware (%s)...\n", __func__, STV6306_DEMOD_FW);
	fw_ret = request_firmware(&fw, STV6306_DEMOD_FW, &state->i2c->dev);
	if (fw_ret)
	{
		printk("%s(): Firmware upload failed. Timeout or file not found \n", __func__);
		return AVL2108_ERROR_GENERIC;
	}
	printk("firmware download done successfull\n");
	data_size = extract_32(fw->data);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,32)
	buffer = kmalloc(fw->size, GFP_KERNEL);
	memcpy(buffer, fw->data, fw->size);
#endif
	printk("data_size %d\n", data_size);
	while (i < data_size)
	{
		buf_size = extract_32(fw->data + i);
		i += 4;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,32)
		/* need write access here, which is permitted under stm24 */
		ret |= state->equipment.demod_i2c_write(fe->demodulator_priv, buffer + i + 1, (u16)(buf_size + 3));
#else
		ret |= state->equipment.demod_i2c_write(fe->demodulator_priv, fw->data + i + 1, (u16)(buf_size + 3));
#endif
		i += 4 + buf_size;
		printk("i %d\n", i);
	}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,32)
	kfree(buffer);
#endif
	dprintk(10, "Leaving %s() with status %u\n", __func__, ret);
	return ret;
}

static u16 stv6306_tuner_lock_int(struct dvb_frontend *fe, u32 freq, u32 lpf)
{
	struct stv6306_state *state = fe->tuner_priv;
	u16 ret;
	u16 timeout = 0;
	const u16 max_tries = 10; /* 10 times */
	dprintk(1, "%s(): freq: %u, lpf: %u\n", __func__, freq, lpf);
	ret = state->equipment.demod_i2c_write16(fe->demodulator_priv, REG_TUNER_FREQ_100KHZ, freq);
	ret |= state->equipment.demod_i2c_write16(fe->demodulator_priv, REG_TUNER_LPF_100KHZ, lpf);
	ret |= state->equipment.demod_send_op(DEMOD_OP_TUNER_LOCK, fe->demodulator_priv);
	/* Wait up to 1s if we locked */
	while (state->equipment.demod_get_op_status(fe->demodulator_priv) != AVL2108_OK)
	{
		dprintk(10, "%s(): Waiting for tuner lock: %u\n", __func__, timeout);
		if ((++timeout) >= max_tries)
		{
			eprintk("%s(): %u: Timeout!\n", __func__, timeout);
			ret |= AVL2108_ERROR_TIMEOUT;
			break;
		}
		mdelay(/* 100 */ 20);
	}
	return ret;
}

static u16 stv6306_tuner_lock_status_int(struct dvb_frontend *fe)
{
	struct stv6306_state *state = fe->tuner_priv;
	u16 ret;
	u16 tmp;
	u16 timeout = 0;
	const u16 max_tries = /* 10 */ 15;
	ret = state->equipment.demod_send_op(DEMOD_OP_TUNER_LOCK_ST, fe->demodulator_priv);
	if (ret == AVL2108_OK)
	{
		while (state->equipment.demod_get_op_status(fe->demodulator_priv) != AVL2108_OK)
		{
			if ((++timeout) >= max_tries)
			{
				eprintk("%s(): %u: Timeout!\n", __func__, timeout);
				return (AVL2108_ERROR_TIMEOUT);
			}
			mdelay(/* 100 */ /* 5 */ 3);
		}
		ret |= state->equipment.demod_i2c_read16(fe->demodulator_priv, REG_TUNER_STATUS, &tmp);
		if (ret == AVL2108_OK)
		{
			if (tmp == TUNER_LOCKED)
				ret = AVL2108_OK;
			else
				ret = AVL2108_ERROR_PREV;
		}
	}
	dprintk(1, "%s(): lock status: %u\n", __func__, ret);
	return ret;
}

/* **************** generic functions ********** */

static u16 stv6306_tuner_lock_status(struct dvb_frontend *fe)
{
	struct stv6306_state *state = fe->tuner_priv;
	if (state->internal)
		return stv6306_tuner_lock_status_int(fe);
	else
		return stv6306_tuner_lock_status_ext(fe);
}

static u16 stv6306_tuner_lock(struct dvb_frontend *fe, u32 freq, u32 srate, u32 lpf)
{
	struct stv6306_state *state = fe->tuner_priv;
	dprintk(20, "%s: freq %d, srate %d, lpf %d\n", __func__, freq, srate, lpf);
	if (state->internal)
		return stv6306_tuner_lock_int(fe, freq, lpf);
	else
		return stv6306_tuner_lock_ext(fe, freq, srate, lpf);
}

static u16 stv6306_tuner_init(struct dvb_frontend *fe)
{
	u16 ret = 0;
	dprintk(50, "%s(): >\n", __func__);
	/* nothing to do here ? */
	dprintk(50, "Leaving %s() with status %u\n", __func__, ret);
	return ret;
}

int stv6306_attach(struct dvb_frontend *fe, void *demod_priv, struct avl2108_equipment_s *equipment, u8 internal, struct i2c_adapter *i2c)
{
	struct stv6306_state *state = kmalloc(sizeof(struct stv6306_state), GFP_KERNEL);
	fe->tuner_priv = state;
	state->equipment = *equipment;
	state->internal = internal;
	state->i2c = i2c;
	if (internal)
		equipment->tuner_load_fw = stv6306_load_firmware;
	else
		equipment->tuner_load_fw = NULL;
	equipment->tuner_init = stv6306_tuner_init;
	equipment->tuner_lock = stv6306_tuner_lock;
	equipment->tuner_lock_status = stv6306_tuner_lock_status;
	return 0;
}

