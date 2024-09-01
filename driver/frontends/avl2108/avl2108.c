/*
 * @brief avl2108.c
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

#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/firmware.h>
#include <linux/platform_device.h>
#include <linux/dvb/version.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,17)
#include <linux/stm/pio.h>
#else
#include <linux/stpio.h>
#endif

#include "dvb_frontend.h"
#include "avl2108.h"
#include "avl2108_reg.h"

#include "avl2108_platform.h"

#define AVL2108_DEMOD_FW "dvb-fe-avl2108.fw" /*< Demod fw used for locking */

#define I2C_MAX_READ 64
#define I2C_MAX_WRITE 64

/*< Format */
#define format_addr(X, Y) \
	do { \
		Y[0] =(u8)((X) >> 16); \
		Y[1] =(u8)((X) >> 8); \
		Y[2] =(u8)(X); \
	} while (0)

/*< Create a formatted 16 bit array */
#define format_16(X, Y) \
	do { \
		Y[0] =(u8)((X) >> 8); \
		Y[1] =(u8)((X) & 0xFF); \
	} while (0)

/*< Create a formatted 32 bit array */
#define format_32(X, Y) \
	do { \
		Y[0] =(u8)((X) >> 24); \
		Y[1] =(u8)((X) >> 16); \
		Y[2] =(u8)((X) >> 8); \
		Y[3] =(u8)((X) & 0xFF); \
	} while (0)

short paramDebug = 50;
#define TAGDEBUG "[avl2108] "

#define dprintk(level, x...) do { \
		if ((paramDebug) && (paramDebug > level)) printk(TAGDEBUG x); \
	} while (0)

struct avl2108_diseqc_tx_status
{
	u8 tx_done; /*< 1 if transmit finished. 0 if Diseqc is still in transmitting */
	u8 tx_fifo_cnt; /*< How many bytes are still in the transmitter's FIFO */
};

struct avl2108_ver_info
{
	u8 major;
	u8 minor;
	u16 build;
	u8 patch_major;
	u8 patch_minor;
	u16 patch_build;
};

struct avl2108_tuning
{
	u32 frequency;
	u32 symbol_rate; /* In Hz */
	u8 lock_mode; /* 0 = channel lock mode is set to fixed, 1 = channel lock mode is set to adaptive */
	u8 auto_iq_swap; /* Enable/Disable the automatic IQ swap feature */
	u8 delsys;
	u8 iq_swap; /* If auto_iq_swap is enabled and this field is set to 0, the demod doesn't swap the IQ signals */
	/* If auto_iq_swap is disabled and this field is set to 1, the demod swaps the IQ signals */
};

struct avl2108_pllconf
{
	u16 m_r1; /*< PLL register r1 */
	u16 m_r2; /*< PLL register r2 */
	u16 m_r3; /*< PLL register r3 */
	u16 m_r4; /*< PLL register r4 */
	u16 m_r5; /*< PLL register r5 */
	u16 ref_freq; /*< Reference clock in kHz */
	u16 demod_freq; /*< Demod clock in 10kHz */
	u16 fec_freq; /*< FEC clock in 10kHz */
	u16 mpeg_freq; /*< MPEG clock in 10kHz */
};

const struct avl2108_pllconf pll_conf[] =
{
	/* For all parallel modes and all serial modes below 30M (symbol rate) */
	{10, 0, 335, 6, 5, 4000, 11200, 16800, 19200}, /*< Reference clock 4M; --> 112, 168, 192 */
	{10, 0, 299, 6, 5, 4500, 11250, 16875, 19286}, /*< Reference clock 4.5M; --> 112.5, 168.75, 192.86 */
	{10, 1, 269, 6, 5, 10000, 11250, 16875, 19286}, /*< Reference clock 10M; --> 112.5, 168.75, 192.86 */
	{10, 0, 83, 6, 5, 16000, 11200, 16800, 19200}, /*< Reference clock 16M; --> 112, 168, 192 */
	{10, 0, 49, 6, 5, 27000, 11250, 16875, 19286}, /*< Reference clock 27M; --> 112.5, 168.75, 192.86 */

	/* for all modes */
	{10, 0, 335, 6, 4, 4000, 11200, 16800, 22400}, /*< Reference clock 4M; --> 112, 168, 224 */
	{10, 0, 299, 6, 4, 4500, 11250, 16875, 22500}, /*< Reference clock 4.5M; --> 112.5, 168.75, 225 */
	{10, 1, 269, 6, 4, 10000, 11250, 16875, 22500}, /*< Reference clock 10M; --> 112.5, 168.75, 225 */
	{10, 0, 83, 6, 4, 16000, 11200, 16800, 22400}, /*< Reference clock 16M; --> 112, 168, 224 */
	{10, 0, 49, 6, 4, 27000, 11250, 16875, 22500} /*< Reference clock 27M; --> 112.5, 168.75, 225 */
};

const unsigned short pll_array_size = sizeof(pll_conf) / sizeof(struct avl2108_pllconf);

/* saved platform config */
static u8 numFrontends = 0;
static struct platform_frontend_config_s *frontendList = NULL;

/*****************************
 * i2c register ops
 *****************************/

static u16 avl2108_i2c_writereg(struct avl2108_state *state, u8 *data, u16 *size)
{
	struct i2c_msg msg = { .addr = state->config->demod_address,
			   .flags = 0, .buf = data, .len = *size
	};
	int err;
	{
		u8 i;
		u8 dstr[512];
		dstr[0] = '\0';
		for (i = 0; i < *size; i++)
			sprintf(dstr, "%s 0x%02x", dstr, data[i]);
		dprintk(200, "%s(): %u b: %s\n", __func__, *size, dstr);
	}
	if ((err = i2c_transfer(state->i2c, &msg, 1)) != 1)
	{
		eprintk("%s(): error: %i, size %d\n", __func__, err, *size);
		return AVL2108_ERROR_I2C; //return -EREMOTEIO;
	}
	return AVL2108_OK;
}

static u16 avl2108_i2c_readreg(struct avl2108_state *state, u8 *data, u16 *size)
{
	int ret;
	u8 res2[2] = { 0, 0 };
	u8 res4[4] = { 0, 0, 0, 0 };
	struct i2c_msg msg;
	msg.addr = state->config->demod_address;
	msg.flags = I2C_M_RD;
	if (*size == 2)
	{
		msg.buf = res2;
		msg.len = 2;
	}
	else if (*size == 4)
	{
		msg.buf = res4;
		msg.len = 4;
	}
	else
	{
		eprintk("%s(): Unsupported data size: %d\n", __func__, *size);
		return AVL2108_ERROR_I2C;
	}
	if ((ret = i2c_transfer(state->i2c, &msg, 1)) != 1)
	{
		eprintk("%s(): error: %i\n", __func__, ret);
		return AVL2108_ERROR_I2C; //return -EREMOTEIO;
	}
	if (*size == 2)
	{
		data[0] = res2[0];
		data[1] = res2[1];
#if 0
		dprintk(10, "%s(): 0x%02x 0x%02x\n", __func__, res2[0], res2[1]);
#endif
	}
	else
	{
		data[0] = res4[0];
		data[1] = res4[1];
		data[2] = res4[2];
		data[3] = res4[3];
#if 0
		dprintk(10, "%s(): 0x%02x 0x%02x 0x%02x 0x%02x\n", __func__, res4[0], res4[1], res4[2], res4[3]);
#endif
	}
	return AVL2108_OK;
}

static u16 avl2108_i2c_read(struct avl2108_state *state, u32 offset, u8 *buf, u16 buf_size)
{
	u16 ret;
	u8 buf_tmp[3];
	u16 x1 = 3,
		x2 = 0;
	u16 size;
	format_addr(offset, buf_tmp);
	ret = avl2108_i2c_writereg(state, buf_tmp, &x1);
	if (ret == AVL2108_OK)
	{
		if (buf_size & 1)
			size = buf_size - 1;
		else
			size = buf_size;
		while (size > I2C_MAX_READ)
		{
			x1 = I2C_MAX_READ;
			ret |= avl2108_i2c_readreg(state, buf + x2, &x1);
			x2 += I2C_MAX_READ;
			size -= I2C_MAX_READ;
		}
		if (size != 0)
			ret |= avl2108_i2c_readreg(state, buf + x2, &size);
		if (buf_size & 1)
		{
			x1 = 2;
			ret |= avl2108_i2c_readreg(state, buf_tmp, &x1);
			buf[buf_size - 1] = buf_tmp[0];
		}
	}
	return ret;
}

static u16 avl2108_i2c_write(void *_state, u8 *buf, u16 buf_size)
{
	struct avl2108_state *state = (struct avl2108_state *) _state;
	u8 buf_tmp[5], *x3;
	u16 ret = 0, x1, x2 = 0, tmp;
	u16 size;
	u32 addr;
	if (buf_size < 3)
		return (AVL2108_ERROR_GENERIC);
	/* Actual data size */
	buf_size -= 3;
	/* Dump address */
	addr = buf[0];
	addr = addr << 8;
	addr += buf[1];
	addr = addr << 8;
	addr += buf[2];
	if (buf_size & 1)
		size = buf_size - 1;
	else
		size = buf_size;
	tmp = (I2C_MAX_WRITE - 3) & 0xfffe; /* How many bytes data we can transfer every time */
	x2 = 0;
	while (size > tmp)
	{
		x1 = tmp + 3;
		/* Save the data */
		buf_tmp[0] = buf[x2];
		buf_tmp[1] = buf[x2 + 1];
		buf_tmp[2] = buf[x2 + 2];
		x3 = buf + x2;
		format_addr(addr, x3);
		ret |= avl2108_i2c_writereg(state, buf + x2, &x1);
		/* Restore data */
		buf[x2] = buf_tmp[0];
		buf[x2 + 1] = buf_tmp[1];
		buf[x2 + 2] = buf_tmp[2];
		addr += tmp;
		x2 += tmp;
		size -= tmp;
	}
	x1 = size + 3;
	/* Save the data */
	buf_tmp[0] = buf[x2];
	buf_tmp[1] = buf[x2 + 1];
	buf_tmp[2] = buf[x2 + 2];
	x3 = buf + x2;
	format_addr(addr, x3);
	ret |= avl2108_i2c_writereg(state, buf + x2, &x1);
	/* Restore data */
	buf[x2] = buf_tmp[0];
	buf[x2 + 1] = buf_tmp[1];
	buf[x2 + 2] = buf_tmp[2];
	addr += size;
	x2 += size;
	if (buf_size & 1)
	{
		format_addr(addr, buf_tmp);
		x1 = 3;
		ret |= avl2108_i2c_writereg(state, buf_tmp, &x1);
		x1 = 2;
		ret |= avl2108_i2c_readreg(state, buf_tmp + 3, &x1);
		buf_tmp[3] = buf[x2 + 3];
		x1 = 5;
		ret |= avl2108_i2c_writereg(state, buf_tmp, &x1);
	}
	return ret;
}

static u16 avl2108_i2c_read16(void *_state, u32 addr, u16 *data)
{
	struct avl2108_state *state = (struct avl2108_state *) _state;
	u16 ret;
	u8 buf[2];
	ret = avl2108_i2c_read(state, addr, buf, 2);
	if (ret == AVL2108_OK)
		*data = extract_16(buf);
	return ret;
}

static u16 avl2108_i2c_read32(struct avl2108_state *state, u32 addr, u32 *data)
{
	u16 ret;
	u8 buf[4];
	ret = avl2108_i2c_read(state, addr, buf, 4);
	if (ret == AVL2108_OK)
		*data = extract_32(buf);
	return ret;
}

static u16 avl2108_i2c_write16(void *_state, u32 addr, u16 data)
{
	struct avl2108_state *state = (struct avl2108_state *) _state;
	u16 ret;
	u8 buf[5], *p;
	format_addr(addr, buf);
	p = buf + 3;
	format_16(data, p);
	ret = avl2108_i2c_write(state, buf, 5);
	return ret;
}

static u16 avl2108_i2c_write32(struct avl2108_state *state, u32 addr, u32 data)
{
	u16 ret;
	u8 buf[7], *p;
	format_addr(addr, buf);
	p = buf + 3;
	format_32(data, p);
	ret = avl2108_i2c_write(state, buf, 7);
	return ret;
}

/*****************************
 * i2c repeater. The demod acts
 * as a repeater allowing us
 * to talk to the tuner
 *****************************/

static u16 avl2108_i2c_repeater_get_status(struct avl2108_state *state)
{
	u16 ret;
	u8 buf[2];
	ret = avl2108_i2c_read(state, REG_I2C_CMD + I2C_CMD_LEN - 2 /* this is 0x0416 */, buf, 2);
	if (ret == AVL2108_OK)
	{
		if (buf[1] != 0)
			ret = AVL2108_ERROR_PREV;
	}
	return ret;
}

static u16 avl2108_i2c_repeater_exec(struct avl2108_state *state, u8 *buf, u8 size)
{
	u16 ret = AVL2108_OK;
	u32 i = 0;
	while (avl2108_i2c_repeater_get_status(state) != AVL2108_OK)
	{
		if (20 < i++)
		{
			ret = AVL2108_ERROR_PREV;
			break;
		}
		msleep(/* 10*/ /* 5 */ 3);
	}
	if (ret == AVL2108_OK)
		ret = avl2108_i2c_write(state, buf, size);
	dprintk(50, "Leaving %s() with status %u\n", __func__, ret);
	return ret;
}

static u16 avl2108_i2c_repeater_send(void *_state, u8 *buf, u16 size)
{
	struct avl2108_state *state = (struct avl2108_state *) _state;
	u8 tmp_buf[I2C_CMD_LEN + 3];
	u16 i, j;
	u16 cmd_size;
	memset(tmp_buf, 0, I2C_CMD_LEN + 3);
	if (size > I2C_CMD_LEN - 3)
		return AVL2108_ERROR_GENERIC;
	cmd_size = ((size + 3) % 2) + 3 + size;
	format_addr(REG_I2C_CMD + I2C_CMD_LEN - cmd_size, tmp_buf);
	i = 3 + ((3 + size) % 2); /* skip one byte if the size +3 is odd */
	for (j = 0; j < size; j++)
		tmp_buf[i++] = buf[j];
	tmp_buf[i++] = (u8)size;
	tmp_buf[i++] = state->config->tuner_address;
	tmp_buf[i++] = I2C_WRITE;
	return avl2108_i2c_repeater_exec(state, tmp_buf, (u8)(cmd_size + 3));
}

static u16 avl2108_i2c_repeater_recv(void *_state, u8 *buf, u16 size)
{
	struct avl2108_state *state = (struct avl2108_state *) _state;
	u16 ret = AVL2108_OK;
	u16 timeout = 0;
	u8 tmp_buf[I2C_RSP_LEN];
	if (size > I2C_RSP_LEN)
		return AVL2108_ERROR_GENERIC;
	format_addr(REG_I2C_CMD + I2C_CMD_LEN - 4, tmp_buf);
	tmp_buf[3] = 0x0;
	tmp_buf[4] = (u8)size;
	tmp_buf[5] = state->config->tuner_address;
	tmp_buf[6] = I2C_READ;
	ret = avl2108_i2c_repeater_exec(state, tmp_buf, 7);
	if (ret == AVL2108_OK)
	{
		while (avl2108_i2c_repeater_get_status(state) != AVL2108_OK)
		{
			if ((++timeout) >= 10)
			{
				ret = AVL2108_ERROR_TIMEOUT;
				return ret;
			}
			msleep(/*100*/ /* 20 */ 10);
		}
		ret = avl2108_i2c_read(state, REG_I2C_RSP, buf, size);
	}
	dprintk(50, "Leaving %s() with status %u\n", __func__, ret);
	return ret;
}

static u16 avl2108_i2c_repeater_init(u16 bus_clk, struct avl2108_state *state)
{
	u8 buf[5];
	u16 ret;
	ret = avl2108_i2c_write16(state, REG_I2C_SPEED_KHZ, bus_clk);
	format_addr(REG_I2C_CMD + I2C_CMD_LEN - 2, buf);
	buf[3] = 0x01;
	buf[4] = I2C_INIT;
	ret |= avl2108_i2c_repeater_exec(state, buf, 5);
	dprintk(50, "Leaving %s() with status %u\n", __func__, ret);
	return ret;
}

/*****************************
 * Demod utils
 *****************************/

static u16 avl2108_get_op_status(void *_state)
{
	struct avl2108_state *state = (struct avl2108_state *) _state;
	u16 ret;
	u8 buf[2];
	ret = avl2108_i2c_read(state, REG_RX_CMD, buf, 2);
	if (ret == AVL2108_OK)
	{
		if (buf[1] != 0)
			ret = AVL2108_ERROR_PREV;
	}
	dprintk(50, "Leaving %s() with status buf[0]: 0x%02x, buf[1]: 0x%02x\n", __func__, buf[0], buf[1]);
	return ret;
}

static u16 avl2108_send_op(u8 ucOpCmd, void *_state)
{
	struct avl2108_state *state = (struct avl2108_state *) _state;
	u16 ret;
	u8 buf[2];
	u16 x1;
	ret = avl2108_get_op_status(state);
	if (ret == AVL2108_OK)
	{
		buf[0] = 0;
		buf[1] = ucOpCmd;
		x1 = extract_16(buf);
		ret |= avl2108_i2c_write16(state, REG_RX_CMD, x1);
	}
	dprintk(50, "Leaving %s() with status %u\n", __func__, ret);
	return ret;
}

u16 avl2108_cpu_halt(struct dvb_frontend *fe)
{
	struct avl2108_state *state = fe->demodulator_priv;
	u16 ret, i = 0;
	ret = avl2108_send_op(DEMOD_OP_HALT, state);
	if (ret == AVL2108_OK)
	{
		while (i++ < 20)
		{
			ret = avl2108_get_op_status(state);
			if (ret == AVL2108_OK)
				break;
			else
				mdelay(/* 10 */ 2);
		}
	}
	dprintk(50, "Leaving %s() with status %u\n", __func__, ret);
	return ret;
}

/*****************************
 * Demod init
 *****************************/

/**
 * @brief Setup PLL
 * @param state
 */
static u16 avl2108_setup_pll(struct avl2108_state *state, const struct avl2108_pllconf *pll_ptr)
{
	u16 ret;
	dprintk(5, "%s()\n", __func__);
	ret = avl2108_i2c_write32(state, (PLL_R3), pll_ptr->m_r3);
	ret |= avl2108_i2c_write32(state, (PLL_R2), pll_ptr->m_r2);
	ret |= avl2108_i2c_write32(state, (PLL_R1), pll_ptr->m_r1);
	ret |= avl2108_i2c_write32(state, (PLL_R4), pll_ptr->m_r4);
	ret |= avl2108_i2c_write32(state, (PLL_R5), pll_ptr->m_r5);
	ret |= avl2108_i2c_write32(state, (PLL_SOFTVALUE_EN), 1);
	ret |= avl2108_i2c_write32(state, (REG_RESET), 0);
	/* Reset */
	avl2108_i2c_write32(state, (REG_RESET), 1);
	return ret;
}

/**
 * @brief Load demod firmware
 * @param fe Ptr to the generic DVB frontend struct
 */
static int avl2108_load_firmware(struct dvb_frontend *fe)
{
	struct avl2108_state *state = fe->demodulator_priv;
	const struct firmware *fw;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,32)
	u8 *buffer;
#endif
	u32 buf_size, data_size;
	u32 i = 4;
	u16 ret;
	int fw_ret;
	dprintk(5, "%s()\n", __func__);
	ret = avl2108_i2c_write32(state, REG_CORE_RESET_B, 0);
	dprintk(10, "%s(): Uploading demod firmware (%s)...\n", __func__, AVL2108_DEMOD_FW);
	fw_ret = request_firmware(&fw, AVL2108_DEMOD_FW, &state->i2c->dev);
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
		ret |= avl2108_i2c_write(state, buffer + i + 1, (u16)(buf_size + 3));
#else
		ret |= avl2108_i2c_write(state, fw->data + i + 1, (u16)(buf_size + 3));
#endif
		i += 4 + buf_size;
	}
	printk("out loop\n");
	ret |= avl2108_i2c_write32(state, 0x00000000, 0x00003ffc);
	ret |= avl2108_i2c_write16(state, REG_CORE_RDY_WORD, 0x0000);
	ret |= avl2108_i2c_write32(state, REG_ERROR_MSG, 0x00000000);
	ret |= avl2108_i2c_write32(state, REG_ERROR_MSG + 4, 0x00000000);
	ret |= avl2108_i2c_write32(state, REG_CORE_RESET_B, 1);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,32)
	kfree(buffer);
#endif
	dprintk(10, "Leaving %s() with status %u\n", __func__, ret);
	return ret;
}

static u16 avl2108_get_mode(struct avl2108_state *state, u8 *pFunctionalMode)
{
	u16 ret;
	u16 x1;
	ret = avl2108_i2c_read16(state, REG_FUNCTIONAL_MODE, &x1);
	*pFunctionalMode = (u8)(x1 & 0x1);
	return ret;
}

static u16 avl2108_channel_lock(struct dvb_frontend *fe, struct avl2108_tuning *tuning)
{
	struct avl2108_state *state = fe->demodulator_priv;
	u16 ret = AVL2108_OK;
	u32 autoIQ_Detect;
	u16 Standard;
	u8 func_mode;
	dprintk(20, "%s: iq_swap %d, auto_iq %d, symbol_rate %d\n", __func__,
			tuning->iq_swap,
			tuning->auto_iq_swap,
			tuning->symbol_rate);
	ret |= avl2108_get_mode(state, &func_mode);
	if (func_mode == FUNC_MODE_DEMOD)
	{
		if ((tuning->symbol_rate > 800000) && (tuning->symbol_rate < 50000000))
		{
			if (tuning->lock_mode == LOCK_MODE_ADAPTIVE)
			{
				/* fixme: atevio7500 sets here 0. nevertheless 1 is the correct
				 * value and seems to work.
				 */
				ret |= avl2108_i2c_write16(state, REG_LOCK_MODE, 1);
				if (tuning->symbol_rate < 3000000)
					ret |= avl2108_i2c_write16(state, REG_CARRIER_FREQ_HALF_RANGE_MHZ, 300);
				else
					ret |= avl2108_i2c_write16(state, REG_CARRIER_FREQ_HALF_RANGE_MHZ, 500);
			}
			else
				ret |= avl2108_i2c_write16(state, REG_LOCK_MODE, 0);
			ret |= avl2108_i2c_write32(state, REG_SPEC_INV, tuning->iq_swap);
			Standard = (u16)(tuning->delsys);
			autoIQ_Detect = tuning->auto_iq_swap;
			if ((Standard == CI_FLAG_DVBS2_UNDEF) || (autoIQ_Detect == 1))
				Standard = 0x14;
			ret |= avl2108_i2c_write16(state, REG_DECODE_MODE, Standard);
			ret |= avl2108_i2c_write16(state, REG_IQ_SWAP_MODE, (u16)autoIQ_Detect);
			ret |= avl2108_i2c_write32(state, REG_SRATE_HZ, tuning->symbol_rate);
			ret |= avl2108_send_op(DEMOD_OP_INIT_GO, state);
		}
		else
			ret = AVL2108_ERROR_GENERIC;
	}
	else
		ret = AVL2108_ERROR_GENERIC;
	dprintk(10, "Leaving %s() with status %u\n", __func__, ret);
	return ret;
}

/**
 * @brief Verify that the demod has completed its initialization procedure
 * @param fe Ptr to the generic DVB frontend struct
 */
static int avl2108_get_demod_status(struct dvb_frontend *fe)
{
	struct avl2108_state *state = fe->demodulator_priv;
	u16 r;
	u8 buf[2];
	u32 x1 = 0;
	dprintk(5, "%s()\n", __func__);
	r = avl2108_i2c_read32(state, REG_CORE_RESET_B, &x1);
	r |= avl2108_i2c_read16(state, REG_CORE_RDY_WORD, (u16 *)buf);
	if ((AVL2108_OK == r))
	{
		if ((x1 == 0) || (buf[0] != 0x5a) || (buf[1] != 0xa5))
			r = AVL2108_ERROR_GENERIC;
	}
	dprintk(10, "Leaving %s() with status %u\n", __func__, r);
	return r;
}

static int avl2108_demod_init(struct dvb_frontend *fe)
{
	struct avl2108_state *state = fe->demodulator_priv;
	u16 ret, mpeg_data_clk = 0;
	/* Set clk to match the PLL */
	ret = avl2108_i2c_write16(state, REG_DMD_CLK_MHZ, state->config->demod_freq);
	ret |= avl2108_i2c_write16(state, REG_FEC_CLK_MHZ, state->config->fec_freq);
	ret |= avl2108_i2c_write16(state, REG_MPEG_CLK_MHZ, state->config->mpeg_freq);
	ret |= avl2108_i2c_write32(state, REG_FORMAT, 1);
	dprintk(10, "%s: demod_freq %d, fec_freq %d, mpeg_freq %d\n", __func__,
			state->config->demod_freq,
			state->config->fec_freq,
			state->config->mpeg_freq);
	/* Set AGC polarization */
	ret |= avl2108_i2c_write32(state, REG_RF_AGC_POL, state->config->agc_polarization);
	if (state->config->agc_ref > 0)
		ret |= avl2108_i2c_write16(state, REG_AAGC_REF, state->config->agc_ref);
	dprintk(10, "%s: agc_pol %d, agc_ref %d\n", __func__, state->config->agc_polarization, state->config->agc_ref);
	/* Set MPEG data */
	ret |= avl2108_i2c_write32(state, REG_MPEG_MODE, state->config->mpeg_mode);
	ret |= avl2108_i2c_write16(state, REG_MPEG_SERIAL, state->config->mpeg_serial);
	ret |= avl2108_i2c_write16(state, REG_MPEG_POS_EDGE, state->config->mpeg_clk_mode);
	dprintk(10, "%s: mpeg_mode %d, mpeg_serial %d, mpeg_clk_mode %d\n", __func__,
			state->config->mpeg_mode,
			state->config->mpeg_serial,
			state->config->mpeg_clk_mode);
	/* Enable MPEG output */
	/*ret |= avl2108_i2c_write32(state, REG_MPEG_OUTPUT, 2);*/
	/* Disable MPEG persistent clock mode */
	/* This setting has no effect on AVL2108LG */
	ret |= avl2108_i2c_write16(state, REG_MPEG_PERSISTENT_CLK_MODE, mpeg_data_clk);
	dprintk(10, "Leaving %s() with status %u\n", __func__, ret);
	return ret;
}

u16 avl2108_save_config(struct dvb_frontend *fe, u32 *buf32, u16 *buf16)
{
	struct avl2108_state *state = fe->demodulator_priv;
	u16 ret;
	ret = avl2108_i2c_read32(state, REG_RF_AGC_POL, buf32++);
	ret |= avl2108_i2c_read32(state, REG_MPEG_ERR_SIGNAL_POL, buf32++);
	ret |= avl2108_i2c_read32(state, REG_MPEG_MODE, buf32++);
	ret |= avl2108_i2c_read32(state, REG_MPEG_SERIAL_OUT_SEL, buf32++);
	ret |= avl2108_i2c_read32(state, REG_MPEG_SERIAL_BIT_SEQ, buf32++);
	ret |= avl2108_i2c_read32(state, REG_SPEC_INV, buf32++);
	ret |= avl2108_i2c_read32(state, REG_ALPHA, buf32++);
	ret |= avl2108_i2c_read32(state, REG_ALPHA_SETTING, buf32++);
	ret |= avl2108_i2c_read32(state, REG_BLIND_SCAN_SRATE_TO_HZ, buf32);
	ret |= avl2108_i2c_read16(state, REG_TUNER_SLAVE_ADDR, buf16++);
	ret |= avl2108_i2c_read16(state, REG_TUNER_LPF_MARGIN_100KHZ, buf16++);
	ret |= avl2108_i2c_read16(state, REG_TUNER_MAX_LPF_100KHZ, buf16++);
	ret |= avl2108_i2c_read16(state, REG_TUNER_LPF_100KHZ, buf16++);
	ret |= avl2108_i2c_read16(state, REG_AAGC_REF, buf16++);
	ret |= avl2108_i2c_read16(state, REG_MPEG_POS_EDGE, buf16++);
	ret |= avl2108_i2c_read16(state, REG_MPEG_SERIAL, buf16++);
	ret |= avl2108_i2c_read16(state, REG_MPEG_SERIAL_CLK_N, buf16++);
	ret |= avl2108_i2c_read16(state, REG_MPEG_SERIAL_CLK_D, buf16++);
	ret |= avl2108_i2c_read16(state, REG_ERR_MODE_CTRL, buf16++);
	ret |= avl2108_i2c_read16(state, REG_CARRIER_FREQ_HALF_RANGE_MHZ, buf16++);
	ret |= avl2108_i2c_read16(state, REG_BLIND_SCAN_RETRIES, buf16++);
	ret |= avl2108_i2c_read16(state, REG_BLIND_SCAN_CARRIER_DB, buf16++);
	ret |= avl2108_i2c_read16(state, 1536, buf16++);
	ret |= avl2108_i2c_read16(state, REG_MPEG_PERSISTENT_CLK_MODE, buf16++);
	ret |= avl2108_i2c_read16(state, REG_BLIND_SCAN_CARIER_FREQ_TO_KHZ, buf16);
	dprintk(10, "Leaving %s() with status %u\n", __func__, ret);
	return ret;
}

u16 avl2108_restore_config(struct dvb_frontend *fe, u32 *buf32, u16 *buf16)
{
	struct avl2108_state *state = fe->demodulator_priv;
	u16 ret;
	ret = avl2108_i2c_write32(state, REG_RF_AGC_POL, *buf32++);
	ret |= avl2108_i2c_write32(state, REG_MPEG_ERR_SIGNAL_POL, *buf32++);
	ret |= avl2108_i2c_write32(state, REG_MPEG_MODE, *buf32++);
	ret |= avl2108_i2c_write32(state, REG_MPEG_SERIAL_OUT_SEL, *buf32++);
	ret |= avl2108_i2c_write32(state, REG_MPEG_SERIAL_BIT_SEQ, *buf32++);
	ret |= avl2108_i2c_write32(state, REG_SPEC_INV, *buf32++);
	ret |= avl2108_i2c_write32(state, REG_ALPHA, *buf32++);
	ret |= avl2108_i2c_write32(state, REG_ALPHA_SETTING, *buf32++);
	ret |= avl2108_i2c_write32(state, REG_BLIND_SCAN_SRATE_TO_HZ, *buf32);
	ret |= avl2108_i2c_write16(state, REG_TUNER_SLAVE_ADDR, *buf16++);
	ret |= avl2108_i2c_write16(state, REG_TUNER_LPF_MARGIN_100KHZ, *buf16++);
	ret |= avl2108_i2c_write16(state, REG_TUNER_MAX_LPF_100KHZ, *buf16++);
	ret |= avl2108_i2c_write16(state, REG_TUNER_LPF_100KHZ, *buf16++);
	ret |= avl2108_i2c_write16(state, REG_AAGC_REF, *buf16++);
	ret |= avl2108_i2c_write16(state, REG_MPEG_POS_EDGE, *buf16++);
	ret |= avl2108_i2c_write16(state, REG_MPEG_SERIAL, *buf16++);
	ret |= avl2108_i2c_write16(state, REG_MPEG_SERIAL_CLK_N, *buf16++);
	ret |= avl2108_i2c_write16(state, REG_MPEG_SERIAL_CLK_D, *buf16++);
	ret |= avl2108_i2c_write16(state, REG_ERR_MODE_CTRL, *buf16++);
	ret |= avl2108_i2c_write16(state, REG_CARRIER_FREQ_HALF_RANGE_MHZ, *buf16++);
	ret |= avl2108_i2c_write16(state, REG_BLIND_SCAN_RETRIES, *buf16++);
	ret |= avl2108_i2c_write16(state, REG_BLIND_SCAN_CARRIER_DB, *buf16++);
	ret |= avl2108_i2c_write16(state, 1536, *buf16++);
	ret |= avl2108_i2c_write16(state, REG_MPEG_PERSISTENT_CLK_MODE, *buf16++);
	ret |= avl2108_i2c_write16(state, REG_BLIND_SCAN_CARIER_FREQ_TO_KHZ, *buf16);
	dprintk(10, "Leaving %s() with status %u\n", __func__, ret);
	return ret;
}

/**
 * @brief The AVL2108 can have two funcitonal modes: blidn scan or 'normal' mode
 */
u16 avl2108_set_functional_mode(struct dvb_frontend *fe)
{
	struct avl2108_state *state = fe->demodulator_priv;
	u16 ret;
	u16 speed;
	u16 x2[15];
	u32 tmp = 0;
	u32 x1[9];
	ret = avl2108_i2c_read16(state, REG_I2C_SPEED_KHZ, &speed);
	ret |= avl2108_save_config(fe, x1, x2);
	if (ret == AVL2108_OK)
		ret = avl2108_load_firmware(fe);
	if (ret == AVL2108_OK)
	{
		do //wait for AVL2108 boot up.
		{
			mdelay(/* 10 */ 5);
			ret = avl2108_get_demod_status(fe);
			if (tmp++ > 20)
				break;
		}
		while (ret != AVL2108_OK);
		ret |= avl2108_i2c_write16(state, REG_DMD_CLK_MHZ, state->config->demod_freq);
		ret |= avl2108_i2c_write16(state, REG_FEC_CLK_MHZ, state->config->fec_freq);
		ret |= avl2108_i2c_write32(state, REG_FORMAT, 1);
		/* Use external control */
		ret |= avl2108_i2c_write16(state, REG_TUNER_USE_INTERNAL_CTRL, 0);
		ret |= avl2108_restore_config(fe, x1, x2);
		ret |= avl2108_i2c_repeater_init(speed, state);
	}
	dprintk(10, "Leaving %s() with status %u\n", __func__, ret);
	return ret;
}

/**
 * @brief Get chip and patch (debug) versions
 * @return Returns AVL2108_OK if successful, AVL2108_ERROR_* otherwise
 */
u16 avl2108_get_version(struct avl2108_state *state, struct avl2108_ver_info *version)
{
	u32 tmp;
	u8 buf[4];
	u16 ret;
	ret = avl2108_i2c_read32(state, REG_ROM_VER, &tmp);
	if (ret == AVL2108_OK)
	{
		format_32(tmp, buf);
		version->major = buf[0];
		version->minor = buf[1];
		version->build = buf[2];
		version->build = ((u16)((version->build) << 8)) + buf[3];
	}
	tmp = 0;
	ret = avl2108_i2c_read32(state, REG_MPEG_OUTPUT, &tmp);
	tmp &= 0x00000001;
	if (tmp != 0)
		version->minor++;
	ret = avl2108_i2c_read32(state, REG_PATCH_VER, &tmp);
	if (ret == AVL2108_OK)
	{
		format_32(tmp, buf);
		version->patch_major = buf[0];
		version->patch_minor = buf[1];
		version->patch_build = buf[2];
		version->patch_build = ((u16)((version->patch_build) << 8)) + buf[3];
	}
	dprintk(10, "Leaving %s() with status %u\n", __func__, ret);
	return ret;
}

int avl2108_set_voltage(struct dvb_frontend *fe, fe_sec_voltage_t voltage)
{
	struct avl2108_state *state = fe->demodulator_priv;
	int ret = 0;
	dprintk(10, "%s(%p, %d)\n", __FUNCTION__, fe, voltage);
	ret = state->equipment.lnb_set_voltage(state->lnb_priv, fe, voltage);
	return ret;
}

/*****************************
 * DiSEqC, LNB and tone
 *****************************/

/**
 * @brief Initialise DiSEqC
 * @param fe A ptr to a DVB API struct dvb_frontend
 * @return Returns AVL2108_OK if successful, AVL2108_ERROR_* otherwise
 */
static int avl2108_diseqc_init(struct dvb_frontend *fe)
{
	struct avl2108_state *state = fe->demodulator_priv;
	u16 ret = AVL2108_OK;
	u32 x1;
	u32 diseqc_info;
	diseqc_info = DISEQC_RX_150 | DISEQC_WAVEFORM_NORMAL | DISEQC_TX_GAP_15;
	ret |= avl2108_i2c_write32(state, REG_DISEQC_SRST, 1);
	ret |= avl2108_i2c_write32(state, REG_DISEQC_SAMP_FRAC_N, 200); /* 2M = 200 * 10kHz */
	ret |= avl2108_i2c_write32(state, REG_DISEQC_SAMP_FRAC_D, state->config->demod_freq);
	ret |= avl2108_i2c_write32(state, REG_DISEQC_TONE_FRAC_N, ((DISEQC_TONE_FREQ) << 1));
	ret |= avl2108_i2c_write32(state, REG_DISEQC_TONE_FRAC_D, state->config->demod_freq * 10);
	/* Initialize the tx_control */
	ret |= avl2108_i2c_read32(state, REG_DISEQC_TX_CTRL, &x1);
	x1 &= 0x00000300;
	x1 |= 0x20; /* Reset tx_fifo */
	x1 |= ((u32)(diseqc_info & 0x000F) << 6);
	x1 |= ((u32)(diseqc_info & 0x0F00 >> 8) << 4);
	x1 |= (1 << 3); /* Enable tx gap */
	ret |= avl2108_i2c_write32(state, REG_DISEQC_TX_CTRL, x1);
	x1 &= ~(0x20); /* Release tx_fifo reset */
	ret |= avl2108_i2c_write32(state, REG_DISEQC_TX_CTRL, x1);
	/* Initialize the rx_control */
	x1 = ((u32)(diseqc_info & 0x0F00 >> 8) << 2);
	x1 |= (1 << 1); /* Activate the receiver */
	x1 |= (1 << 3); /* Envelop high when tone present */
	ret |= avl2108_i2c_write32(state, REG_DISEQC_RX_CTRL, x1);
	x1 = (u32)(diseqc_info & 0xF000 >> 12);
	ret |= avl2108_i2c_write32(state, REG_DISEQC_RX_MSG_TMR, x1);
	ret |= avl2108_i2c_write32(state, REG_DISEQC_SRST, 0);
	if (ret == AVL2108_OK)
		state->diseqc_status = DISEQC_STATUS_INIT;
	dprintk(10, "Leaving %s() with status %u\n", __func__, ret);
	return 0;
}

/**
 * @brief Check if it is safe to switch Diseqc operation mode
 * @param fe A ptr to a DVB API struct dvb_frontend
 * @return Returns AVL2108_OK if successful, AVL2108_ERROR_* otherwise
 */
u16 avl2108_diseqc_switch_mode(struct dvb_frontend *fe)
{
	struct avl2108_state *state = fe->demodulator_priv;
	u16 ret = AVL2108_OK;
	u32 x1;
	switch (state->diseqc_status)
	{
		case DISEQC_STATUS_MOD:
		case DISEQC_STATUS_TONE:
			ret |= avl2108_i2c_read32(state, REG_DISEQC_TX_ST, &x1);
			if (((x1 & 0x00000040) >> 6) != 1)
				ret |= AVL2108_ERROR_PREV;
			break;
		case DISEQC_STATUS_CONTINUOUS:
		case DISEQC_STATUS_INIT:
			break;
		default:
			ret |= AVL2108_ERROR_GENERIC;
			break;
	}
	dprintk(10, "Leaving %s() with status %u\n", __func__, ret);
	return ret;
}

/**
 * @brief
 * @param fe A ptr to a DVB API struct dvb_frontend
 * @return Returns AVL2108_OK if successful, AVL2108_ERROR_* otherwise
 */
u16 avl2108_diseqc_send_mod_data(struct dvb_frontend *fe, const u8 *buf, u8 size)
{
	struct avl2108_state *state = fe->demodulator_priv;
	u16 ret = AVL2108_OK;
	u32 x1, x2;
	u8 buf_tmp[8];
	if (size > 8)
		ret = AVL2108_ERROR_MEM;
	else
	{
		ret = avl2108_diseqc_switch_mode(fe);
		if (ret == AVL2108_OK)
		{
			/* Reset rx_fifo */
			ret |= avl2108_i2c_read32(state, REG_DISEQC_RX_CTRL, &x2);
			ret |= avl2108_i2c_write32(state, REG_DISEQC_RX_CTRL, (x2 | 0x01));
			ret |= avl2108_i2c_write32(state, REG_DISEQC_RX_CTRL, (x2 & 0xfffffffe));
			ret |= avl2108_i2c_read32(state, REG_DISEQC_TX_CTRL, &x1);
			x1 &= 0xfffffff8; //set to modulation mode and put it to FIFO load mode
			ret |= avl2108_i2c_write32(state, REG_DISEQC_TX_CTRL, x1);
			/* Trunk address */
			format_addr(REG_DISEQC_TX_FIFO_MAP, buf_tmp);
			buf_tmp[3] = 0;
			buf_tmp[4] = 0;
			buf_tmp[5] = 0;
			for (x2 = 0; x2 < size; x2++)
			{
				buf_tmp[6] = buf[x2];
				ret |= avl2108_i2c_write(state, buf_tmp, 7);
			}
			x1 |= (1 << 2); //start fifo transmit.
			ret |= avl2108_i2c_write32(state, REG_DISEQC_TX_CTRL, x1);
			if (ret == AVL2108_OK)
				state->diseqc_status = DISEQC_STATUS_MOD;
		}
	}
	dprintk(10, "%s(): ret: %u\n", __func__, ret);
	return ret;
}

/**
 * @brief
 * @param fe A ptr to a DVB API struct dvb_frontend
 * @return Returns AVL2108_OK if successful, AVL2108_ERROR_* otherwise
 */
u16 avl2108_diseqc_get_tx_status(struct dvb_frontend *fe, struct avl2108_diseqc_tx_status *pTxStatus)
{
	struct avl2108_state *state = fe->demodulator_priv;
	u16 ret = AVL2108_OK;
	u32 x1;
	if ((state->diseqc_status == DISEQC_STATUS_MOD) ||
			(state->diseqc_status == DISEQC_STATUS_TONE))
	{
		ret = avl2108_i2c_read32(state, REG_DISEQC_TX_ST, &x1);
		pTxStatus->tx_done = (u8)((x1 & 0x00000040) >> 6);
		pTxStatus->tx_fifo_cnt = (u8)((x1 & 0x0000003c) >> 2);
	}
	else
		ret = AVL2108_ERROR_GENERIC;
	dprintk(10, "Leaving %s() with status %u\n", __func__, ret);
	return ret;
}

/**
 * @brief Send DiSEqC message
 * @return 0 = success
 */
static int avl2108_send_diseqc_msg(struct dvb_frontend *fe, struct dvb_diseqc_master_cmd *d)
{
	u16 ret = AVL2108_OK;
	struct avl2108_diseqc_tx_status tx_status;
	int cnt = 100;
	if ((d->msg_len < 3) || (d->msg_len > 6))
		return -EINVAL;
	ret = avl2108_diseqc_send_mod_data(fe, d->msg, d->msg_len);
	if (ret != AVL2108_OK)
		eprintk("%s(): Output %u modulation bytes: fail!\n", __func__, d->msg_len);
	else
	{
		do
		{
			ret = avl2108_diseqc_get_tx_status(fe, &tx_status);
			if (tx_status.tx_done == 1)
				break;
			msleep(/* 10 */ 3);
			cnt--;
			if (cnt == 0)
				break;
		}
		/* Wait until operation finishes */
		while (tx_status.tx_done != 1);
		/* msleep(100); */
		if (cnt == 0)
			dprintk(20, "%s(): cnt= %d, r= 0x%x, tx_status.tx_done = %d\n",
					__func__, cnt, ret, tx_status.tx_done);
		if (ret != AVL2108_OK)
			dprintk(20, "%s(): Output %u modulation bytes fail!\n", __func__, d->msg_len);
		else
			dprintk(20, "%s(): Output %u modulation bytes: success!\n", __func__, d->msg_len);
	}
	return 0;
}

/**
 * @brief Send Mini-DiSEqC burst also called Tone-burst.
 * It's used for switching between satellite A and satellite B
 * @param burst The satellite to which we want to switch: SEC_MINI_A or SEC_MINI_B
 * @return 0 = success
 */
static int avl2108_diseqc_send_burst(struct dvb_frontend *fe, fe_sec_mini_cmd_t burst)
{
	struct avl2108_state *state = fe->demodulator_priv;
	u16 ret;
	u32 x1;
	u8 buf[8];
	struct avl2108_diseqc_tx_status tx_status;
	ret = avl2108_diseqc_switch_mode(fe);
	if (ret != AVL2108_OK)
	{
		eprintk("%s(): Tone-burst failed!\n", __func__);
		return 0;
	}
	/* No data in the FIFO */
	ret = avl2108_i2c_read32(state, REG_DISEQC_TX_CTRL, &x1);
	x1 &= 0xfffffff8; /* Put it into the FIFO load mode */
	if (burst == SEC_MINI_A)
		x1 |= 0x01;
	else
		x1 |= 0x02;
	ret |= avl2108_i2c_write32(state, REG_DISEQC_TX_CTRL, x1);
	/* Trunk address */
	format_addr(REG_DISEQC_TX_FIFO_MAP, buf);
	buf[3] = 0;
	buf[4] = 0;
	buf[5] = 0;
	buf[6] = 1;
	ret |= avl2108_i2c_write(state, buf, 7);
	x1 |= (1 << 2); /* Start fifo transmit */
	ret |= avl2108_i2c_write32(state, REG_DISEQC_TX_CTRL, x1);
	if (ret != AVL2108_OK)
	{
		eprintk("%s(): Tone-burst failed!\n", __func__);
		return 0;
	}
	else
		state->diseqc_status = DISEQC_STATUS_TONE;
	ret = avl2108_diseqc_get_tx_status(fe, &tx_status);
	if (ret != AVL2108_OK)
		eprintk("%s(): Tune-burst sent failed: %d\n", __func__, tx_status.tx_done);
	else
		dprintk(10, "%s(): Tune-burst sent successfully: %d\n", __func__, tx_status.tx_done);
	return 0;
}

/**
 * @brief Set/Unset the tone. This is used for selecting a High (22 KHz) or a Low (0 KHz) freq
 * @param tone The to which we want to use: SEC_TONE_ON or SEC_TONE_OFF
 * @return 0 = success
 */
int avl2108_set_tone(struct dvb_frontend *fe, fe_sec_tone_mode_t tone)
{
	struct avl2108_state *state = fe->demodulator_priv;
	u16 ret;
	u32 x1;
	dprintk(10, "%s >", __func__);
	if (tone == SEC_TONE_ON)
	{
		/*avl2108_diseqc_start_cont(fe);*/
		ret = avl2108_diseqc_switch_mode(fe);
		if (ret != AVL2108_OK)
		{
			return 0;
		}
		avl2108_i2c_read32(state, REG_DISEQC_TX_CTRL, &x1);
		x1 &= 0xfffffff8;
		x1 |= 0x03;
		avl2108_i2c_write32(state, REG_DISEQC_TX_CTRL, x1);
		x1 |= (1 << 10);
		ret = avl2108_i2c_write32(state, REG_DISEQC_TX_CTRL, x1);
		if (ret == AVL2108_OK)
			state->diseqc_status = DISEQC_STATUS_CONTINUOUS;
	}
	else
	{
		/*avl2108_diseqc_stop_cont(fe);*/
		if (state->diseqc_status == DISEQC_STATUS_CONTINUOUS)
		{
			avl2108_i2c_read32(state, REG_DISEQC_TX_CTRL, &x1);
			x1 &= 0xfffff3ff;
			avl2108_i2c_write32(state, REG_DISEQC_TX_CTRL, x1);
		}
	}
	return 0;
}

/*****************************
 * Read status/stats
 *****************************/

/**
 * @brief Reset the regs used for gathering stats
 * @param fe A ptr to a DVB API struct dvb_frontend
 * @return Returns AVL2108_OK if successful, AVL2108_ERROR_* otherwise
 */
u16 avl2108_reset_stats(struct dvb_frontend *fe)
{
	struct avl2108_state *state = fe->demodulator_priv;
	u8 func_mode;
	u16 ret;
	ret = avl2108_get_mode(state, &func_mode);
	if (func_mode == FUNC_MODE_DEMOD)
		ret |= avl2108_send_op(DEMOD_OP_RESET_BERPER, state);
	else
		ret = AVL2108_ERROR_GENERIC;
	return ret;
}

/**
 * @brief Check the lock status of the demod. The only status we can have is if we have lock or not
 * We cannot get nothing in-between like FE_HAS_CARRIER | FE_HAS_VITERBI | FE_HAS_SYNC
 * @param fe A ptr to a DVB API struct dvb_frontend
 * @return Always returns 0
 */
static int avl2108_read_status(struct dvb_frontend *fe, fe_status_t *status)
{
	struct avl2108_state *state = fe->demodulator_priv;
	u16 lock_status = 5;
	*status = 0;
	/* Check only once if we have lock and return immediately, do not wait */
	avl2108_i2c_read16(state, REG_FEC_LOCK, &lock_status);
	if (lock_status != 1)
	{
		*status = 0;
		eprintk("Could not get lock status!\n");
	}
	else
	{
		*status |= FE_HAS_SIGNAL | FE_HAS_CARRIER | FE_HAS_VITERBI | FE_HAS_SYNC | FE_HAS_LOCK;
		dprintk(50, "%s(): Service locked!!!\n", __func__);
	}
	return 0;
}

/**
 * @brief Get Bit error rate
 * @param fe A ptr to a DVB API struct dvb_frontend
 * @param ber User-space reads this value
 * @return Always returns 0
 */
static int avl2108_read_ber(struct dvb_frontend *fe, u32 *ber)
{
	struct avl2108_state *state = fe->demodulator_priv;
	u16 ret = AVL2108_OK;
	ret = avl2108_i2c_read32(state, REG_BER, ber);
	if (ret != AVL2108_OK)
	{
		dprintk(1, "Could not get BER\n");
		*ber = 0;
	}
	else
		*ber /= 1000000000;
	return 0;
}

/**
 * @brief Get signal strength
 * @param fe A ptr to a DVB API struct dvb_frontend
 * @param signal_strength User-space reads this value
 * @return 0 if successful, -1 otherwise
 */
static int avl2108_read_signal_strength(struct dvb_frontend *fe, u16 *signal_strength)
{
	struct avl2108_state *state = fe->demodulator_priv;
	u16 ret = AVL2108_OK;
	u32 data, pol;
	ret = avl2108_i2c_read32(state, REG_AAGC, &data);
	ret |= avl2108_i2c_read32(state, REG_RF_AGC_POL, &pol);
	if (ret == AVL2108_OK)
	{
		data += 0x800000;
		data &= 0xffffff;
		*signal_strength = (u16)(data >> 8);
		return 0;
	}
	else
	{
		*signal_strength = 0;
		dprintk(30, "Could not get signal strength");
	}
	return -1;
}

/**
 * @brief Get Signal-to-noise ratio
 * @param fe A ptr to a DVB API struct dvb_frontend
 * @param snr User-space reads this value
 * @return Always returns 0
 */
static int avl2108_read_snr(struct dvb_frontend *fe, u16 *snr)
{
	struct avl2108_state *state = fe->demodulator_priv;
	u16 ret;
	u32 r_snr;
	ret = avl2108_i2c_read32(state, REG_SNR_DB, &r_snr);
	if (ret != AVL2108_OK)
	{
		dprintk(30, "Could not get SNR\n");
		*snr = 0;
	}
	else
	{
		/* FIXME: The demod returns the snr in dB * 100
		 * I convert it to the range [0 - 65535] = [0% - 100%],
		 * but this is not the way for doing it :( */
		if (((r_snr) >= 0) && ((r_snr) <= 2236))
			*snr = r_snr * 65535 / 2236;
		else
			*snr = 0;
	}
	return 0;
}

/**
 * @brief Supported by demod???
 * @param fe A ptr to a DVB API struct dvb_frontend
 */
static int avl2108_read_ucblocks(struct dvb_frontend *fe, u32 *ucblocks)
{
	*ucblocks = 0;
	return 0;
}

/*****************************
 * Frontend ops
 * (except status/stats gathering)
 *****************************/

/**
 * @brief Free the main struct avl2108_state
 * @param fe A ptr to a DVB API struct dvb_frontend
 */
static void avl2108_release(struct dvb_frontend *fe)
{
	struct avl2108_state *state = fe->demodulator_priv;
	dprintk(10, "%s()\n", __func__);
	kfree(state);
}

static struct dvb_frontend_ops avl2108_ops;

/**
 * @brief Entry point to the driver. Inits the state struct used everywhere
 * @param config A ptr to struct avl2108_config that is filled outside
 * @param A ptr to struct i2c_adapter that knows everything regarding the i2c
 * including our slot number.
 * @return A ptr to a DVB API struct dvb_frontend
 */
struct dvb_frontend *avl2108_attach(const struct avl2108_config *config,
									struct i2c_adapter *i2c)
{
	struct avl2108_state *state = NULL;
	struct avl2108_ver_info version;
	/* Allocate memory for the internal state */
	state = kmalloc(sizeof(struct avl2108_state), GFP_KERNEL);
	if (state == NULL)
	{
		eprintk("Unable to kmalloc\n");
		return NULL;
	}
	/* Setup the state used everywhere */
	memset(state, 0, sizeof(struct avl2108_state));
	state->config = config;
	state->i2c = i2c;
	state->boot_done = 0;
	state->diseqc_status = DISEQC_STATUS_UNINIT;
#ifdef UFS922
	{
		int ret;
		u8 b0[] = { 0x00 };
		u8 b1[] = { 0x00 };
		struct i2c_msg msg[] =
		{
			{.addr = config->demod_address, .flags = 0, .buf = b0, .len = 1},
			{.addr = config->demod_address, .flags = I2C_M_RD, .buf = b1, .len = 1}
		};
		ret = i2c_transfer(state->i2c, msg, 2);
		if (ret != 2)
		{
			printk("%s(): i2c-error: %i\n", __func__, ret);
			kfree(state);
			return NULL;
		}
		if (b1[0] == 0x99)
		{
			printk("avl2108: Detected SP2237\n");
		}
		else
			printk("avl2108: No SP2237\n");
	}
#endif
	/* Get version and patch (debug only) number */
	if (avl2108_get_version(state, &version) != AVL2108_OK)
	{
		eprintk("Could not get version");
		kfree(state);
		return NULL;
	}
	else
	{
		printk("AVL2108: Chip version: %u.%u.%u\n",
			   version.major, version.minor, version.build);
		dprintk(10, "Patch version: %u.%u.%u\n",
				version.patch_major, version.patch_minor, version.patch_build);
	}
	state->equipment.demod_i2c_repeater_send = avl2108_i2c_repeater_send;
	state->equipment.demod_i2c_repeater_recv = avl2108_i2c_repeater_recv;
	state->equipment.demod_i2c_write = avl2108_i2c_write;
	state->equipment.demod_i2c_write16 = avl2108_i2c_write16;
	state->equipment.demod_i2c_read16 = avl2108_i2c_read16;
	state->equipment.demod_send_op = avl2108_send_op;
	state->equipment.demod_get_op_status = avl2108_get_op_status;
	if (config->usedTuner == cTUNER_EXT_STV6306)
	{
		printk("using external control (no tuner fw; tuner stv6306)\n");
		stv6306_attach(&state->frontend, state, &state->equipment, 0 /* internal = false */, i2c);
	}
	else if (config->usedTuner == cTUNER_EXT_STV6110A)
	{
		printk("using external control (no tuner fw; tuner stv6110A)\n");
		/* fixme make mclk configurable via platform */
		stv6110a_attach(&state->frontend, state, &state->equipment, 16, state->config->max_lpf);
	}
	else
	{
		printk("using internal control (tuner fw)\n");
		stv6306_attach(&state->frontend, state, &state->equipment, 1 /* internal = true */, i2c);
	}
	if (config->usedLNB == cLNB_LNBH221)
		state->lnb_priv = lnbh221_attach(state->config->lnb, &state->equipment);
	else if (config->usedLNB == cLNB_PIO)
		state->lnb_priv = lnb_pio_attach(state->config->lnb, &state->equipment);
	memcpy(&state->frontend.ops, &avl2108_ops, sizeof(struct dvb_frontend_ops));
	state->frontend.demodulator_priv = state;
	return &state->frontend;
}

/**
 * @brief Initialise or wake up device
 * Power config will reset and load initial firmware if required
 * @param fe A ptr to a DVB API struct dvb_frontend
 * @return Returns 0 if successful, -1 otherwise
 */
static int avl2108_initfe(struct dvb_frontend *fe)
{
	struct avl2108_state *state = fe->demodulator_priv;
	u16 ret;
	if (state->boot_done)
	{
		dprintk(10, "Boot procedure already done. Skipping it.\n");
		return 0;
	}
	ret = avl2108_setup_pll(state, (const struct avl2108_pllconf *)(pll_conf + state->config->pll_config));
	if (ret != AVL2108_OK)
	{
		eprintk("PLL initialization failed!");
		return -1;
	}
	mdelay(1);
	ret = avl2108_load_firmware(fe);
	if (ret != AVL2108_OK)
	{
		eprintk("Demod firmware load failed!");
		return -1;
	}
	/* Wait for the demod to boot */
	mdelay(100);
	ret = avl2108_get_demod_status(fe);
	if (ret != AVL2108_OK)
	{
		eprintk("Boot failed!\n");
		return -1;
	}
	ret = avl2108_demod_init(fe);
	if (ret != AVL2108_OK)
	{
		eprintk("Demod Initialization failed!\n");
		return -1;
	}
	if ((state->config->usedTuner != cTUNER_EXT_STV6306) &&
			(state->config->usedTuner != cTUNER_EXT_STV6110A))
	{
		if (state->equipment.tuner_load_fw)
		{
			ret = state->equipment.tuner_load_fw(fe);
			if (ret != AVL2108_OK)
			{
				eprintk("Tuner firmware load failed!");
				return -1;
			}
		}
	}
	/* tuner init start */
	ret = avl2108_i2c_write16(state, REG_TUNER_SLAVE_ADDR, state->config->tuner_address);
	if (state->config->usedTuner == cTUNER_EXT_STV6306)
	{
		/* Use external control */
		ret |= avl2108_i2c_write16(state, REG_TUNER_USE_INTERNAL_CTRL, 0);
		ret |= avl2108_i2c_write16(state, REG_TUNER_LPF_MARGIN_100KHZ, 0);
		ret |= avl2108_i2c_write16(state, REG_TUNER_MAX_LPF_100KHZ, state->config->max_lpf);
	}
	else if (state->config->usedTuner == cTUNER_EXT_STV6110A)
	{
		/* Use external control */
		ret |= avl2108_i2c_write16(state, REG_TUNER_USE_INTERNAL_CTRL, 0);
		ret |= avl2108_i2c_write16(state, REG_TUNER_LPF_MARGIN_100KHZ, 0);
		ret |= avl2108_i2c_write16(state, REG_TUNER_MAX_LPF_100KHZ, state->config->max_lpf);
	}
	else
	{
		/* use internal control requires extra fw */
		ret |= avl2108_i2c_write16(state, REG_TUNER_USE_INTERNAL_CTRL, 1);
	}
	if (state->equipment.tuner_init)
		ret |= state->equipment.tuner_init(fe);
	ret |= avl2108_i2c_repeater_init(state->config->i2c_speed_khz, state);
	if (ret != AVL2108_OK)
	{
		eprintk("Tuner initialization failed!\n");
		return -1;
	}
	/* tuner init end */
	ret = avl2108_diseqc_init(fe);
	if (ret != AVL2108_OK)
	{
		eprintk("Diseqc initialization failed!\n");
		return -1;
	}
	dprintk(10, "AVL2108: Demod and tuner successfully initialized!\n");
	state->boot_done = 1;
	return 0;
}

/**
 * @brief Put device to sleep
 * @param fe A ptr to a DVB API struct dvb_frontend
 * @return Returns 0 if successful, -1 otherwise
 */
static int avl2108_sleep(struct dvb_frontend *fe)
{
	struct avl2108_state *state = fe->demodulator_priv;
	u16 ret;
	dprintk(10, "%s()\n", __func__);
	/* TODO: Is this ok? */
	ret = avl2108_cpu_halt(fe);
	if (ret != AVL2108_OK)
	{
		eprintk("Tuner cpu halt failed!\n");
		return -1;
	}
	return 0;
}

#if DVB_API_VERSION >= 5
static int avl2108_set_property(struct dvb_frontend *fe, struct dtv_property *tvp)
{
	dprintk(20, "%s()\n", __func__);
	return 0;
}

static int avl2108_get_property(struct dvb_frontend *fe, struct dtv_property *tvp)
{
	/* get delivery system info */
	if (tvp->cmd == DTV_DELIVERY_SYSTEM)
	{
		switch (tvp->u.data)
		{
			case SYS_DVBS2:
			case SYS_DVBS:
			case SYS_DSS:
				break;
			default:
				return -EINVAL;
		}
	}
	dprintk(20, "%s()\n", __func__);
	return 0;
}
#endif

#if DVB_API_VERSION < 5
static struct dvbfe_info dvbs_info =
{
	.name = "Availink AVL2108 DVB-S2",
	.delivery = DVBFE_DELSYS_DVBS,
	.delsys = {
		.dvbs.modulation = DVBFE_MOD_QPSK,
		.dvbs.fec = DVBFE_FEC_1_2 | DVBFE_FEC_2_3 |
		DVBFE_FEC_3_4 | DVBFE_FEC_4_5 |
		DVBFE_FEC_5_6 | DVBFE_FEC_6_7 | DVBFE_FEC_7_8 | DVBFE_FEC_AUTO
	},

	.frequency_min = 950000,
	.frequency_max = 2150000,
	.frequency_step = 1011,
	.frequency_tolerance = 5000,
	.symbol_rate_min = 800000,
	.symbol_rate_max = 50000000
};

static const struct dvbfe_info dvbs2_info =
{
	.name = "Availink AVL2108 DVB-S2",
	.delivery = DVBFE_DELSYS_DVBS2,
	.delsys = {
		.dvbs2.modulation = DVBFE_MOD_QPSK | DVBFE_MOD_8PSK,

		/* TODO: Review these */
		.dvbs2.fec = DVBFE_FEC_1_4 | DVBFE_FEC_1_3 |
		DVBFE_FEC_2_5 | DVBFE_FEC_1_2 |
		DVBFE_FEC_3_5 | DVBFE_FEC_2_3 |
		DVBFE_FEC_3_4 | DVBFE_FEC_4_5 |
		DVBFE_FEC_5_6 | DVBFE_FEC_8_9 | DVBFE_FEC_9_10,

	},

	.frequency_min = 950000,
	.frequency_max = 2150000,
	.frequency_step = 0,
	.symbol_rate_min = 800000,
	.symbol_rate_max = 50000000,
	.symbol_rate_tolerance = 0
};

static int avl2108_get_info(struct dvb_frontend *fe, struct dvbfe_info *fe_info)
{
	dprintk(10, "%s\n", __FUNCTION__);
	switch (fe_info->delivery)
	{
		case DVBFE_DELSYS_DVBS:
			dprintk(10, "%s(DVBS)\n", __FUNCTION__);
			memcpy(fe_info, &dvbs_info, sizeof(dvbs_info));
			break;
		case DVBFE_DELSYS_DVBS2:
			dprintk(10, "%s(DVBS2)\n", __FUNCTION__);
			memcpy(fe_info, &dvbs2_info, sizeof(dvbs2_info));
			break;
		default:
			printk("%s() invalid arg\n", __FUNCTION__);
			return -EINVAL;
	}
	return 0;
}
#endif

/**
 * @brief dvb-core told us to tune, the tv property cache will be complete,
 * it's safe for us to pull values and use them for tuning purposes.
 * The setting procedure is divided in two steps: the lock setting that sets only the frequency,
 * wait 150ms, and then set what availink calls the 'channel' setting that sets symbol rate,
 * inversion, delivery system, pilot, roll-off, FEC.
 * The reason behing the 'channel' setting is that all these parameters, except the symbol rate,
 * can be calculated automatically by the demod.
 */
static int avl2108_set_frontend(struct dvb_frontend *fe, struct dvb_frontend_parameters *p)
{
	struct avl2108_state *state = fe->demodulator_priv;
#if DVB_API_VERSION >= 5
	struct dtv_frontend_properties *c = &fe->dtv_property_cache;
	fe_rolloff_t rolloff = ROLLOFF_35;
#else
	struct dvbfe_params newfe;
	enum dvbfe_rolloff rolloff = DVBFE_ROLLOFF_35;
#endif
	struct avl2108_tuning tuning;
	u8 func_mode;
	u16 cnt, lock_status = 0, ret = 0;
	u32 freq, max_time;
#if DVB_API_VERSION >= 5
	dprintk(1, "%s: delivery system	= %d\n", __func__, c->delivery_system);
	dprintk(1, "%s: modulation 		= %d\n", __func__, c->modulation);
	dprintk(1, "%s: frequency 		= %d\n", __func__, c->frequency);
	dprintk(1, "%s: symbol_rate 		= %d\n", __func__, c->symbol_rate);
	dprintk(1, "%s: inversion 		= %d\n", __func__, c->inversion);
	dprintk(1, "%s: rolloff 		= %d\n", __func__, c->rolloff);
#else
	newfe.frequency = p->frequency;
	if (p->u.qpsk.fec_inner > FEC_AUTO)
	{
		newfe.delivery = DVBFE_DELSYS_DVBS2;
	}
	else
	{
		newfe.delivery = DVBFE_DELSYS_DVBS;
	}
	newfe.delsys.dvbs.symbol_rate = p->u.qpsk.symbol_rate;
	if (ret != 0)
	{
		eprintk("failed to create new qpsk values\n");
		return ret;
	}
	if (p->u.qpsk.fec_inner > FEC_AUTO)
	{
		switch (p->inversion & 0xc)
		{
			default: // unknown rolloff
			case 0: // 0.35
				rolloff = DVBFE_ROLLOFF_35;
				break;
			case 4: // 0.25
				rolloff = DVBFE_ROLLOFF_25;
				break;
			case 8: // 0.20
				rolloff = DVBFE_ROLLOFF_20;
				break;
		}
	}
	dprintk(1, "%s: delivery system	= %d\n", __func__, newfe.delivery);
	dprintk(1, "%s: frequency 		= %d\n", __func__, newfe.frequency);
	dprintk(1, "%s: symbol_rate 		= %d\n", __func__, newfe.delsys.dvbs.symbol_rate);
	dprintk(1, "%s: rolloff 		= %d\n", __func__, rolloff);
#endif
	/* Halt CPU to improve tuner's locking speed */
	ret = avl2108_cpu_halt(fe);
	if (ret != AVL2108_OK)
	{
		eprintk("Tuner cpu halt failed!\n");
		return -1;
	}
	/* Tuner lock */
#if DVB_API_VERSION >= 5
	freq = c->frequency / 100;
#else
	freq = newfe.frequency / 100;
#endif
#if DVB_API_VERSION >= 5
	ret = state->equipment.tuner_lock(fe, freq, c->symbol_rate, state->config->lpf);
#else
	ret = state->equipment.tuner_lock(fe, freq, newfe.delsys.dvbs.symbol_rate, state->config->lpf);
#endif
	if (ret != AVL2108_OK)
	{
		eprintk("Tuner set failed!\n");
		return -1;
	}
	/* Wait for tuner locking */
	/* msleep(150);*/
	/* Check lock status */
	ret = state->equipment.tuner_lock_status(fe);
	if (ret != AVL2108_OK)
	{
		eprintk("Failed while checking lock status!\n");
		return -1;
	}
	dprintk(1, "%s(): Tuner successfully set!\n", __func__);
	/* Be sure that we are in demod status, if not, set it */
	avl2108_get_mode(state, &func_mode);
	if (func_mode != FUNC_MODE_DEMOD)
	{
		dprintk(1, "Functional mode is not set for demodulation, changing it");
		avl2108_set_functional_mode(fe);
	}
	/* Channel lock */
#if DVB_API_VERSION >= 5
	tuning.symbol_rate = c->symbol_rate;
#else
	tuning.symbol_rate = newfe.delsys.dvbs.symbol_rate;
#endif
	tuning.iq_swap = state->config->iq_swap;
	tuning.auto_iq_swap = state->config->auto_iq_swap;
#if DVB_API_VERSION >= 5
	if (c->delivery_system == SYS_DVBS)
#else
	if (newfe.delivery == DVBFE_DELSYS_DVBS)
#endif
		tuning.delsys = 0x00; /* Set DVB-S */
	else
		tuning.delsys = 0x01; /* Set DVB-S2 */
	tuning.lock_mode = state->config->lock_mode;
	ret = avl2108_channel_lock(fe, &tuning);
	if (ret != AVL2108_OK)
	{
		eprintk("Channel set failed!\n");
		return -1;
	}
	/* Wait a bit more when we have slow symbol rates */
#if DVB_API_VERSION >= 5
	if (c->symbol_rate < 5000000)
		max_time = 1000; /* Max waiting time: 1000ms */
	else if (c->symbol_rate < 10000000)
		max_time = 600; /* Max waiting time: 600ms */
	else
		max_time = 250; /* Max waiting time: 250ms */
#else
	if (newfe.delsys.dvbs.symbol_rate < 5000000)
		max_time = 1000; /* Max waiting time: 1000ms */
	else if (newfe.delsys.dvbs.symbol_rate < 10000000)
		max_time = 600; /* Max waiting time: 600ms */
	else
		max_time = 250; /* Max waiting time: 250ms */
#endif
	cnt = max_time / 10;
	do
	{
		if (avl2108_i2c_read16(state, REG_FEC_LOCK, &lock_status) != AVL2108_OK)
			dprintk(20, "%s(): Could not get lock status. Retrying %d more times\n",
					__func__, cnt);
		if (lock_status == 1)
			break;
		msleep(/* 10 */ 3); /* Wait 10ms for demod to lock the channel */
		dprintk(20, "%s(): Waiting for lock (tuner %d): %d\n", __func__, state->config->tuner_no, cnt);
	}
	while (--cnt);
	if (cnt == 0)
	{
		eprintk("%s(): Time out!\n", __func__);
		return 0;
	}
	dprintk(1, "%s(): Service locked!!! (tuner %d)\n", __func__, state->config->tuner_no);
	ret = avl2108_reset_stats(fe);
	if (ret != AVL2108_OK)
		eprintk("Could not reset stats!\n");
	return ret;
}

static struct dvb_frontend_ops avl2108_ops =
{
	.info = {
		.name = "Availink AVL2108 DVB-S2",
		.type = FE_QPSK,
		.frequency_min = 950000,
		.frequency_max = 2150000,
		.frequency_stepsize = 1011, /* kHz for QPSK frontends */
		.frequency_tolerance = 5000,
		.symbol_rate_min = 800000, /* Min = 800K */
		.symbol_rate_max = 50000000, /* Max = 50M */
		.caps = FE_CAN_INVERSION_AUTO |
		FE_CAN_FEC_1_2 | FE_CAN_FEC_2_3 | FE_CAN_FEC_3_4 |
		FE_CAN_FEC_4_5 | FE_CAN_FEC_5_6 | FE_CAN_FEC_6_7 |
		FE_CAN_FEC_7_8 | FE_CAN_FEC_8_9 | FE_CAN_FEC_AUTO |
		FE_CAN_QPSK | FE_CAN_RECOVER /*| FE_CAN_MUTE_TS*/
	},

	.init = avl2108_initfe,
	.release = avl2108_release,
	.sleep = avl2108_sleep,
	.read_status = avl2108_read_status,
	.read_ber = avl2108_read_ber,
	.read_signal_strength = avl2108_read_signal_strength,
	.read_snr = avl2108_read_snr,
	.read_ucblocks = avl2108_read_ucblocks,
	.set_tone = avl2108_set_tone,
	.set_voltage = avl2108_set_voltage,
	.diseqc_send_master_cmd = avl2108_send_diseqc_msg,
	.diseqc_send_burst = avl2108_diseqc_send_burst,

#if DVB_API_VERSION >= 5
	.set_property = avl2108_set_property,
	.get_property = avl2108_get_property,
	.set_frontend = avl2108_set_frontend,
#else
	.set_frontend = avl2108_set_frontend,
	.get_info = avl2108_get_info,
#endif
};

void avl2108_register_frontend(struct dvb_adapter *dvb_adap)
{
	struct dvb_frontend *frontend;
	struct avl2108_config *cfg;
	struct stpio_pin *pin;
	int i;
	printk("%s\n", __func__);
#ifdef ATEVIO7500
	/* fixme: I think we should move this to st-merger.c */
	/* set the muxer pin otherwise startci output will
	 * not properly passed to tsmerger.
	 */
	pin = stpio_request_pin(5, 3, "tuner2_mux", STPIO_OUT);
	stpio_set_pin(pin, 0);
	pin = stpio_request_pin(5, 4, "tuner2_mux", STPIO_OUT);
	stpio_set_pin(pin, 0);
#endif
	for (i = 0; i < numFrontends; i++)
	{
		struct avl_private_data_s *priv = (struct avl_private_data_s *) frontendList[i].private;
		cfg = kmalloc(sizeof(struct avl2108_config), GFP_KERNEL);
		if (cfg == NULL)
		{
			printk("avl2108: error malloc\n");
			return;
		}
		cfg->tuner_no = i + 1;
		cfg->tuner_enable_pin = stpio_request_pin(frontendList[i].tuner_enable[0],
												  frontendList[i].tuner_enable[1],
												  "tun_enab",
												  STPIO_OUT);
		printk("tuner_enable_pin %p\n", cfg->tuner_enable_pin);
		if (cfg->tuner_enable_pin == NULL)
		{
			printk("%s: failed\n", __func__);
			return;
		}
		stpio_set_pin(cfg->tuner_enable_pin, !frontendList[i].tuner_enable[2]);
		stpio_set_pin(cfg->tuner_enable_pin, frontendList[i].tuner_enable[2]);
		msleep(250);
		cfg->tuner_active_lh = frontendList[i].tuner_enable[2];
		cfg->demod_address = frontendList[i].demod_i2c;
		cfg->tuner_address = frontendList[i].tuner_i2c;
		cfg->ref_freq = priv->ref_freq; /*< Reference clock in kHz units */
		cfg->demod_freq = priv->demod_freq;/*< Demod clock in 10kHz units */
		cfg->fec_freq = priv->fec_freq; /*< FEC clock in 10kHz units */
		cfg->mpeg_freq = priv->mpeg_freq; /*< MPEG clock in 10kHz units */
		cfg->i2c_speed_khz = priv->i2c_speed_khz;
		cfg->agc_polarization = priv->agc_polarization;
		cfg->mpeg_mode = priv->mpeg_mode;
		cfg->mpeg_serial = priv->mpeg_serial;
		cfg->mpeg_clk_mode = priv->mpeg_clk_mode;
		cfg->pll_config = priv->pll_config;
		cfg->lpf = priv->lpf;
		cfg->max_lpf = priv->max_lpf;
		cfg->lock_mode = priv->lock_mode;
		cfg->iq_swap = priv->iq_swap;
		cfg->auto_iq_swap = priv->auto_iq_swap;
		cfg->agc_ref = priv->agc_ref;
		cfg->usedTuner = priv->usedTuner;
		cfg->usedLNB = priv->usedLNB;
		memcpy(cfg->lnb, frontendList[i].lnb, sizeof(cfg->lnb));
		frontend = avl2108_attach(cfg, i2c_get_adapter(frontendList[i].i2c_bus));
		if (frontend == NULL)
		{
			printk("avl2108: avl2108_attach failed\n");
			if (cfg->tuner_enable_pin)
				stpio_free_pin(cfg->tuner_enable_pin);
			kfree(cfg);
			return;
		}
		if (dvb_register_frontend(dvb_adap, frontend))
		{
			printk("%s: Frontend registration failed !\n", __FUNCTION__);
			if (frontend->ops.release)
				frontend->ops.release(frontend);
			return;
		}
	} /* for */
	return;
}
EXPORT_SYMBOL(avl2108_register_frontend);

/* ******************************* */
/* platform device functions */
/* ******************************* */

static int avl2108_probe(struct platform_device *pdev)
{
	struct platform_frontend_s *plat_data = pdev->dev.platform_data;
	int i;
	printk("%s >\n", __func__);
	numFrontends = plat_data->numFrontends;
	frontendList = kmalloc(numFrontends * sizeof(struct platform_frontend_config_s), GFP_KERNEL);
	memcpy(frontendList, plat_data->frontendList, numFrontends * sizeof(struct platform_frontend_config_s));
	for (i = 0; i < numFrontends; i++)
	{
		printk("found frontend \"%s\" in platform config\n", frontendList[i].name);
	}
	printk("%s <\n", __func__);
	return 0;
}

static int avl2108_remove(struct platform_device *pdev)
{
	struct plat_tuner_data *plat_data = pdev->dev.platform_data;
	return 0;
}

static struct platform_driver avl2108_driver =
{
	.probe = avl2108_probe,
	.remove = avl2108_remove,
	.driver = {
		.name = "avl2108",
		.owner = THIS_MODULE,
	},
};

/* ******************************* */
/* module functions */
/* ******************************* */

int __init avl2108_init(void)
{
	int ret;
	printk("%s >\n", __func__);
	ret = platform_driver_register(&avl2108_driver);
	printk("%s < %d\n", __func__, ret);
	return ret;
}

static void avl2108_cleanup(void)
{
	printk("%s >\n", __func__);
}

module_param(paramDebug, short, 0644);
MODULE_PARM_DESC(paramDebug, "Activates frontend debugging (default:0)");

MODULE_DESCRIPTION("DVB Frontend module for demod Availink avl2108 and tuner Sharp stv6306 or stv6110A");

MODULE_AUTHOR("Pedro Aguilar");
MODULE_LICENSE("GPL");

EXPORT_SYMBOL(avl2108_set_tone);
EXPORT_SYMBOL(avl2108_set_voltage);

module_init(avl2108_init);
module_exit(avl2108_cleanup);

