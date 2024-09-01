/*
 * @brief avl6222.c
 *
 * based on code from avl2108 from:
 *
 * @author Pedro Aguilar <pedro@duolabs.com>
 *
 * @brief Availink avl6222 - DVBS/S2 Satellite demod driver with LG TDQA-S601F tuner
 *
 * Copyright (C) 2009-2010 Duolabs Spa
 *               2011 adapted by konfetti for use with ufs913
 *
 * based on avl6222 code from:
 * @author Ramon-Tomislav Rebersak <ramon.rebersak@gmail.com>
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
#include <linux/proc_fs.h>
#include <linux/stm/pio.h>
#include <linux/proc_fs.h>
#include "dvb_frontend.h"
#include "avl6222.h"
#include "avl6222_reg.h"
#include "avl6222_platform.h"
#include "frontend_platform.h"
#include "socket.h"
#include "tuner.h"
#include "lnb.h"

short paramDebug = 0;  // debug print level is zero as default (0=nothing, 1= errors, 10=some detail, 20=more detail, 50=open/close functions, 100=all)
#if defined TAGDEBUG
#undef TAGDEBUG
#endif
#define TAGDEBUG "[avl6222] "

//---------------------------------------------------------------------

static int lockError = 0;
static short demodLock = 0;
static short tunerLock = 0;
short useOriginTimings = 1;
short initDone = 0;

//---------------------------------------------------------------------

const struct avl6222_pllconf pll_conf[] =
{
	{ 503,  1, 7, 4, 2,  4000, 11200, 16800, 25200 },
	{ 447,  1, 7, 4, 2,  4500, 11200, 16800, 25200 },
	{ 503,  4, 7, 4, 2, 10000, 11200, 16800, 25200 },
	{ 503,  7, 7, 4, 2, 16000, 11200, 16800, 25200 },
	{ 111,  2, 7, 4, 2, 27000, 11200, 16800, 25200 }
};

const unsigned short pll_array_size = sizeof(pll_conf) / sizeof(struct avl6222_pllconf);

//---------------------------------------------------------------------

/* saved platform config */
static int numConfigs = 0;
static struct platform_frontend_config_s *frontend_cfg = NULL;

#define cMaxSockets 4
static u8 numSockets = 0;
static struct socket_s socketList[cMaxSockets];

/*---------------------------------------------------------------------
 * I2C functions
 *---------------------------------------------------------------------*/
static u16 avl6222_i2c_writereg(struct avl6222_state *state, u8 *data, u16 *size)
{
	struct i2c_msg msg =
	{ .addr = state->config->demod_address,
	  .flags = 0, .buf = data, .len = *size
	};
	int err;
#if 1
//for debug, if we only want to see tuner values
//if ((data[*size - 1] == 0x02) && (data[*size - 2] == 0xc0))
	{
		u8 i;
		u8 dstr[512];
		dstr[0] = '\0';
		for (i = 0; i < *size; i++)
		{
			sprintf(dstr, "%s 0x%02x", dstr, data[i]);
		}
		dprintk(200, "%s(): %u b: %s\n", __func__, *size, dstr);
	}
#endif

#if defined(optimized_debug)
	if (state->config->demod_address == 0x0d)
	{
		int try;
		printk("[I2C]\t");
		for (try = 0; try < *size; try++)
		{
			printk("%02X ", data[try]);
			if (((try + 1) % 16) == 0)
			{
				printk("\n[I2C]\t");
			}
		}
		printk("\n");
	}
#endif
	if ((err = i2c_transfer(state->i2c, &msg, 1)) != 1)
	{
		dprintk(1, "%s: Error: %i, size %d\n", __func__, err, *size);
		return AVL6222_ERROR_I2C; //return -EREMOTEIO;
	}
	return AVL6222_OK;
}

//---------------------------------------------------------------------

static u16 avl6222_i2c_readreg(struct avl6222_state *state, u8 *data, u16 *size)
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
		dprintk(1, "%s: Error: unsupported data size %d\n", __func__, *size);
		return AVL6222_ERROR_I2C;
	}

#if defined(optimized_debug)
	if (state->config->demod_address == 0x0d)
	{
		int try;
		printk("[I2C]\t");
		for (try = 0; try < *size; try++)
		{
			printk("%02X ", data[try]);
			if (((try + 1) % 16) == 0)
			{
				printk("\n[I2C]\t");
			}
		}
		printk("\n");
	}
#endif
	if ((ret = i2c_transfer(state->i2c, &msg, 1)) != 1)
	{
		dprintk(1, "%s: Error: %i\n", __func__, ret);
		return AVL6222_ERROR_I2C; //return -EREMOTEIO;
	}
	if (*size == 2)
	{
		data[0] = res2[0];
		data[1] = res2[1];
		dprintk(200, "%s(): 0x%02x 0x%02x\n", __func__, res2[0], res2[1]);
	}
	else
	{
		data[0] = res4[0];
		data[1] = res4[1];
		data[2] = res4[2];
		data[3] = res4[3];
		dprintk(200, "%s(): 0x%02x 0x%02x 0x%02x 0x%02x\n", __func__, res4[0], res4[1], res4[2], res4[3]);
	}
#if defined(optimized_debug)
	if (state->config->demod_address == 0x0d)
	{
		int try;
		printk("[I2C]\t");
		for (try = 0; try < *size; try++)
		{
			printk("%02X ", data[try]);
			if (((try + 1) % 16) == 0)
			{
				printk("\n[I2C]\t");
			}
		}
		printk("\n");
	}
#endif
	return AVL6222_OK;
}

//---------------------------------------------------------------------

static u16 avl6222_i2c_read(struct avl6222_state *state, u32 offset, u8 *buf, u16 buf_size)
{
	u16 ret;
	u8 buf_tmp[3];
	u16 x1 = 3,
	    x2 = 0;
	u16 size;

	format_addr(offset, buf_tmp);
	ret = avl6222_i2c_writereg(state, buf_tmp, &x1);
	if (ret == AVL6222_OK)
	{
		if (buf_size & 1)
		{
			size = buf_size - 1;
		}
		else
		{
			size = buf_size;
		}
		while (size > I2C_MAX_READ)
		{
			x1 = I2C_MAX_READ;
			ret |= avl6222_i2c_readreg(state, buf + x2, &x1);
			x2 += I2C_MAX_READ;
			size -= I2C_MAX_READ;
		}

		if (size != 0)
		{
			ret |= avl6222_i2c_readreg(state, buf + x2, &size);
		}
		if (buf_size & 1)
		{
			x1 = 2;
			ret |= avl6222_i2c_readreg(state, buf_tmp, &x1);
			buf[buf_size - 1] = buf_tmp[0];
		}
	}
	return ret;
}

//---------------------------------------------------------------------

static u16 avl6222_i2c_write(void *_state, u8 *buf, u16 buf_size)
{
	struct avl6222_state *state = (struct avl6222_state *) _state;
	u8 buf_tmp[5], *x3;
	u16 ret = 0, x1, x2 = 0, tmp;
	u16 size;
	u32 addr;

	if (buf_size < 3)
	{
		return (AVL6222_ERROR_GENERIC);
	}
	/* Actual data size */
	buf_size -= 3;
	/* Dump address */
	addr = buf[0];
	addr = addr << 8;
	addr += buf[1];
	addr = addr << 8;
	addr += buf[2];

	if (buf_size & 1)
	{
		size = buf_size - 1;
	}
	else
	{
		size = buf_size;
	}
	tmp = (I2C_MAX_WRITE - 3) & 0xfffe;  /* How many bytes data we can transfer every time */

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
		ret |= avl6222_i2c_writereg(state, buf + x2, &x1);
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
	ret |= avl6222_i2c_writereg(state, buf + x2, &x1);
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
		ret |= avl6222_i2c_writereg(state, buf_tmp, &x1);
		x1 = 2;
		ret |= avl6222_i2c_readreg(state, buf_tmp + 3, &x1);
		buf_tmp[3] = buf[x2 + 3];
		x1 = 5;
		ret |= avl6222_i2c_writereg(state, buf_tmp, &x1);
	}
	return ret;
}

//---------------------------------------------------------------------

static u16 avl6222_i2c_read16(void *_state, u32 addr, u16 *data)
{
	struct avl6222_state *state = (struct avl6222_state *) _state;
	u16 ret;
	u8 buf[2];

	ret = avl6222_i2c_read(state, addr, buf, 2);
	if (ret == AVL6222_OK)
	{
		*data = extract_16(buf);
	}
	return ret;
}

//---------------------------------------------------------------------

static u16 avl6222_i2c_read32(struct avl6222_state *state, u32 addr, u32 *data)
{
	u16 ret;
	u8 buf[4];

	ret = avl6222_i2c_read(state, addr, buf, 4);
	if (ret == AVL6222_OK)
	{
		*data = extract_32(buf);
	}
	return ret;
}

//---------------------------------------------------------------------

static u16 avl6222_i2c_write16(void *_state, u32 addr, u16 data)
{
	struct avl6222_state *state = (struct avl6222_state *) _state;
	u16 ret;
	u8 buf[5], *p;

	format_addr(addr, buf);
	p = buf + 3;
	format_16(data, p);
	ret = avl6222_i2c_write(state, buf, 5);
	return ret;
}

//---------------------------------------------------------------------

static u16 avl6222_i2c_write32(struct avl6222_state *state, u32 addr, u32 data)
{
	u16 ret;
	u8 buf[7], *p;

	format_addr(addr, buf);
	p = buf + 3;
	format_32(data, p);
	ret = avl6222_i2c_write(state, buf, 7);
	return ret;
}

/*---------------------------------------------------------------------
 * I2C Repeater functions
 *---------------------------------------------------------------------*/

static u16 avl6222_i2c_repeater_get_status(struct avl6222_state *state)
{
	u16 ret;
	u8 buf[2];

	ret = avl6222_i2c_read(state, REG_I2C_CMD + I2C_CMD_LEN - 2 /* this is 0x0416 */, buf, 2);
	if (ret == AVL6222_OK)
	{
		if (buf[1] != 0)
		{
			ret = AVL6222_ERROR_PREV;
		}
	}
	return ret;
}

//---------------------------------------------------------------------

static u16 avl6222_i2c_repeater_exec(struct avl6222_state *state, u8 *buf, u8 size)
{
	u16 ret = AVL6222_OK;
	u32 i = 0;
	u32 cnt = 20;

	dprintk(100, "%s >\n", __func__);

	if (useOriginTimings == 0)
	{
		cnt = 60;
	}
	while (avl6222_i2c_repeater_get_status(state) != AVL6222_OK)
	{
		if (cnt < i++)
		{
			ret = AVL6222_ERROR_PREV;
			break;
		}
		if (useOriginTimings == 1)
		{
			msleep(10);
		}
		else
		{
			msleep(/* 10*/ /* 5 */ 3);
		}
	}
	if (ret == AVL6222_OK)
	{
		ret = avl6222_i2c_write(state, buf, size);
	}
	dprintk(100, "%s < status = %u, useOriginTimings = %d\n", __func__, ret, useOriginTimings);
	return ret;
}

//---------------------------------------------------------------------

static u16 avl6222_i2c_repeater_send(void *_state, u8 *buf, u16 size)
{
	struct avl6222_state *state = (struct avl6222_state *) _state;
	u8 tmp_buf[I2C_CMD_LEN + 3];
	u16 i, j;
	u16 cmd_size;

	memset(tmp_buf, 0, I2C_CMD_LEN + 3);

	if (size > I2C_CMD_LEN - 3)
	{
		return AVL6222_ERROR_GENERIC;
	}
	cmd_size = ((size + 3) % 2) + 3 + size;
	format_addr(REG_I2C_CMD + I2C_CMD_LEN - cmd_size, tmp_buf);

	i = 3 + ((3 + size) % 2);	  /* skip one byte if the size +3 is odd */

	for (j = 0; j < size; j++)
	{
		tmp_buf[i++] = buf[j];
	}
	tmp_buf[i++] = (u8)size;
	tmp_buf[i++] = state->config->tuner_address;
	tmp_buf[i++] = I2C_WRITE;
	return avl6222_i2c_repeater_exec(state, tmp_buf, (u8)(cmd_size + 3));
}

//---------------------------------------------------------------------

static u16 avl6222_i2c_repeater_recv(void *_state, u8 *buf, u16 size)
{
	struct avl6222_state *state = (struct avl6222_state *) _state;
	u16 ret = AVL6222_OK;
	u16 timeout = 0;
	u32 cnt = 100;
	u8 tmp_buf[I2C_RSP_LEN];

	if (size > I2C_RSP_LEN)
	{
		return AVL6222_ERROR_GENERIC;
	}
	format_addr(REG_I2C_CMD + I2C_CMD_LEN - 4, tmp_buf);
	tmp_buf[3] = 0x0;
	tmp_buf[4] = (u8)size;
	tmp_buf[5] = state->config->tuner_address;
	tmp_buf[6] = I2C_READ;

	ret = avl6222_i2c_repeater_exec(state, tmp_buf, 7);
	if (ret == AVL6222_OK)
	{
		while (avl6222_i2c_repeater_get_status(state) != AVL6222_OK)
		{
			if ((++timeout) >= cnt)
			{
				ret = AVL6222_ERROR_TIMEOUT;
				return ret;
			}
			msleep(10);
		}
		ret = avl6222_i2c_read(state, REG_I2C_RSP, buf, size);
	}
	dprintk(100, "%s < status = %u\n", __func__, ret);
	return ret;
}

//---------------------------------------------------------------------

static u16 avl6222_i2c_repeater_init(u16 bus_clk, struct avl6222_state *state)
{
	u8 buf[5];
	u16 ret;

	ret = avl6222_i2c_write16(state, REG_I2C_SPEED_KHZ, bus_clk);
	format_addr(REG_I2C_CMD + I2C_CMD_LEN - 2, buf);
	buf[3] = 0x01;
	buf[4] = I2C_INIT;
	ret |= avl6222_i2c_repeater_exec(state, buf, 5);
	dprintk(100, "%s < status = %u\n", __func__, ret);
	return ret;
}

/*---------------------------------------------------------------------
 * Demodulator functions
 *---------------------------------------------------------------------*/

static u16 avl6222_get_op_status(void *_state)
{
	struct avl6222_state *state = (struct avl6222_state *) _state;
	u16 ret;
	u8 buf[2];

	ret = avl6222_i2c_read(state, REG_RX_CMD, buf, 2);
	if (ret == AVL6222_OK)
	{
		if (buf[1] != 0)
		{
			ret = AVL6222_ERROR_PREV;
		}
	}
	dprintk(100, "%s < status = buf[0]: 0x%02x, buf[1]: 0x%02x\n", __func__, buf[0], buf[1]);
	return ret;
}

//---------------------------------------------------------------------

static u16 avl6222_send_op(u8 ucOpCmd, void *_state)
{
	struct avl6222_state *state = (struct avl6222_state *) _state;
	u16 ret;
	u8 buf[2];
	u16 x1;

	ret = avl6222_get_op_status(state);
	if (ret == AVL6222_OK)
	{
		buf[0] = 0;
		buf[1] = ucOpCmd;
		x1 = extract_16(buf);
		ret |= avl6222_i2c_write16(state, REG_RX_CMD, x1);
	}
	dprintk(100, "%s < status = %u\n", __func__, ret);
	return ret;
}

//---------------------------------------------------------------------

u16 avl6222_cpu_halt(struct dvb_frontend *fe)
{
	struct avl6222_state *state = fe->demodulator_priv;
	u16 ret;

	ret = avl6222_send_op(DEMOD_OP_HALT, state);

	dprintk(100, "%s < status = %u\n", __func__, ret);
	return ret;
}

//---------------------------------------------------------------------

static u16 avl6222_setup_pll(struct avl6222_state *state, const struct avl6222_pllconf *pll_ptr)
{
	u16 ret = 0;

	dprintk(100, "%s >\n", __func__);

	ret = avl6222_i2c_write32(state, PLL_R1, pll_ptr->m_r1);
	ret |= avl6222_i2c_write32(state, PLL_R6, pll_ptr->m_r1);
	ret |= avl6222_i2c_write32(state, PLL_R2, pll_ptr->m_r2);
	ret |= avl6222_i2c_write32(state, PLL_R3, pll_ptr->m_r3);
	ret |= avl6222_i2c_write32(state, PLL_R4, pll_ptr->m_r4);
	ret |= avl6222_i2c_write32(state, PLL_R5, pll_ptr->m_r5);
	//ret |= avl6222_i2c_write32(state, PLL_SOFTVALUE_EN,   1);
	ret |= avl6222_i2c_write32(state, REG_RESET, 0);

	/* Reset */
	ret |= avl6222_i2c_write32(state, REG_RESET, 1);
	return ret;
}
//---------------------------------------------------------------------

static u16 avl6222_setup_pll_2(struct avl6222_state *state, const struct avl6222_pllconf *pll_ptr)
{
	u16 ret = 0;

	dprintk(100, "%s >\n", __func__);
	ret = avl6222_i2c_write32(state, PLL_R1, pll_ptr->m_r1);
	ret |= avl6222_i2c_write32(state, PLL_R6, pll_ptr->m_r1);
	ret |= avl6222_i2c_write32(state, PLL_R2, pll_ptr->m_r2);
	ret |= avl6222_i2c_write32(state, PLL_R3, pll_ptr->m_r3);
	ret |= avl6222_i2c_write32(state, PLL_R4, pll_ptr->m_r4);
	ret |= avl6222_i2c_write32(state, PLL_R5, pll_ptr->m_r5);
	ret |= avl6222_i2c_write32(state, PLL_SOFTVALUE_EN,   1);
	ret |= avl6222_i2c_write32(state, REG_RESET, 0);
	/* Reset */
	ret |= avl6222_i2c_write32(state, REG_RESET, 1);
	return ret;
}
//---------------------------------------------------------------------

static int avl6222_load_firmware(struct dvb_frontend *fe)
{
	struct avl6222_state *state = fe->demodulator_priv;
	const struct firmware *fw;
	u8 *buffer;
	u32 buf_size, data_size;
	u32 i = 4;
	u16 ret;
	int fw_ret;

	dprintk(100, "%s >\n", __func__);
	ret = avl6222_i2c_write32(state, REG_CORE_RESET_B, 0);
	dprintk(10, "%s(): Uploading demod firmware (%s)...\n", __func__, AVL6222_DEMOD_FW);
	fw_ret = request_firmware(&fw, AVL6222_DEMOD_FW, &state->i2c->dev);
	if (fw_ret)
	{
		printk("%s(): Firmware request failed. Timeout or file not found \n", __func__);
		return AVL6222_ERROR_GENERIC;
	}
	dprintk(20, "%s: firmware request successful\n", __func__);
	data_size = extract_32(fw->data);
	buffer = kmalloc(fw->size, GFP_KERNEL);
	memcpy(buffer, fw->data, fw->size);
	dprintk(20, "%s: writing firmware... (data_size %d)\n", __func__, data_size);

	while (i < data_size)
	{
		buf_size = extract_32(fw->data + i);
		i += 4;

		/* need write access here, which is permitted under stm24 */
		ret |= avl6222_i2c_write(state, &buffer[i + 1], (u16)(buf_size + 3));
		i += 4 + buf_size;
	}
	ret |= avl6222_i2c_write32(state, 0x00000000, 0x00003ffc);
	ret |= avl6222_i2c_write16(state, core_ready_word_addr, 0x0000);
	ret |= avl6222_i2c_write32(state, error_msg_addr, 0x00000000);
	ret |= avl6222_i2c_write32(state, error_msg_addr + 4, 0x00000000);
	ret |= avl6222_i2c_write32(state, REG_CORE_RESET_B, 1);
	kfree(buffer);
	dprintk(100, "%s < status = %u\n", __func__, ret);
	return ret;
}

//---------------------------------------------------------------------

static int avl6222_get_demod_status(struct dvb_frontend *fe)
{
	struct avl6222_state *state = fe->demodulator_priv;
	u16 r;
	u8  buf[2];
	u32 x1 = 0;

	r = avl6222_i2c_read32(state, REG_CORE_RESET_B, &x1);
	r |= avl6222_i2c_read16(state, REG_CORE_RDY_WORD, (u16 *)buf);

	dprintk(20, "Demod status = 0x%02X%02X\n", buf[0], buf[1]);

	if ((AVL6222_OK == r))
	{
		if ((x1 == 0) || (buf[0] != 0x5a) || (buf[1] != 0xa5))
		{
			r = AVL6222_ERROR_GENERIC;
		}
	}
	return r;
}

//---------------------------------------------------------------------

u16 avl6222_get_version(struct avl6222_state *state, struct avl6222_ver_info *version)
{
	u32 tmp;
	u8 buf[4];
	u16 ret;

	ret = avl6222_i2c_read32(state, REG_ROM_VER, &tmp);
	if (ret == AVL6222_OK)
	{
		format_32(tmp, buf);
		version->major = buf[0];
		version->minor = buf[1];
		version->build = buf[2];
		version->build = ((u16)((version->build) << 8)) + buf[3];

		ret = avl6222_i2c_read32(state, REG_PATCH_VER, &tmp);
		if (ret == AVL6222_OK)
		{
			format_32(tmp, buf);
			version->patch_major = buf[0];
			version->patch_minor = buf[1];
			version->patch_build = buf[2];
			version->patch_build = ((u16)((version->patch_build) << 8)) + buf[3];
		}
	}
	dprintk(100, "%s < status = %u\n", __func__, ret);
	return ret;
}

//---------------------------------------------------------------------

static u16 avl6222_get_mode(struct avl6222_state *state, u8 *pFunctionalMode)
{
	u16 tmp;
	u16 ret = avl6222_i2c_read16(state, rc_functional_mode_addr, &tmp);
	*pFunctionalMode = (u8)(tmp & 0x0001);
	return ret;
}

//---------------------------------------------------------------------

static u16 avl6222_channel_lock(struct dvb_frontend *fe, struct avl6222_tuning *tuning)
{
	struct avl6222_state *state = fe->demodulator_priv;
	u16 ret = AVL6222_OK;
	u32 autoIQ_Detect;
	u16 Standard;
	u8 func_mode;

	dprintk(20, "%s: iq_swap %d, auto_iq %d, symbol_rate %d\n", __func__,
		tuning->iq_swap,
		tuning->auto_iq_swap,
		tuning->symbol_rate);

	ret |= avl6222_get_mode(state, &func_mode);
	if (func_mode == FUNC_MODE_DEMOD)
	{
		if ((tuning->symbol_rate > 800000) && (tuning->symbol_rate < 60000000))
		{
			if (tuning->lock_mode == LOCK_MODE_ADAPTIVE)
			{
				ret |= avl6222_i2c_write16(state, REG_LOCK_MODE, 1);

				if (tuning->symbol_rate < 3000000)
				{
					ret |= avl6222_i2c_write16(state, REG_CARRIER_FREQ_HALF_RANGE_MHZ, 300);
				}
				else
				{
					ret |= avl6222_i2c_write16(state, REG_CARRIER_FREQ_HALF_RANGE_MHZ, 500);
				}
			}
			else
			{
				ret |= avl6222_i2c_write16(state, REG_LOCK_MODE, 0);
			}
			ret |= avl6222_i2c_write32(state, REG_SPEC_INV, tuning->iq_swap);
			Standard = (u16)(tuning->delsys);
			autoIQ_Detect = tuning->auto_iq_swap;

			if ((Standard == CI_FLAG_DVBS2_UNDEF) || (autoIQ_Detect == 1))
			{
				Standard = 0x14;
			}
			ret |= avl6222_i2c_write16(state, REG_DECODE_MODE, Standard);
			ret |= avl6222_i2c_write16(state, REG_IQ_SWAP_MODE, (u16)autoIQ_Detect);
			ret |= avl6222_i2c_write32(state, REG_SRATE_HZ, tuning->symbol_rate);
			ret |= avl6222_send_op(DEMOD_OP_INIT_GO, state);
		}
		else
		{
			ret = AVL6222_ERROR_GENERIC;
		}
	}
	else
	{
		ret = AVL6222_ERROR_GENERIC;
	}
	dprintk(100, "%s < status = %u\n", __func__, ret);
	return ret;
}

//---------------------------------------------------------------------

static int avl6222_demod_init(struct dvb_frontend *fe)
{
	struct avl6222_state *state = fe->demodulator_priv;
	u16 ret = 0;

	/* Set clk to match the PLL */
	ret = avl6222_i2c_write16(state, REG_DMD_CLK_MHZ, state->config->demod_freq);
	ret |= avl6222_i2c_write16(state, REG_FEC_CLK_MHZ, state->config->fec_freq);
	ret |= avl6222_i2c_write16(state, REG_MPEG_CLK_MHZ, state->config->mpeg_freq);
	ret |= avl6222_i2c_write32(state, REG_FORMAT, 1);

	dprintk(10, "%s: demod_freq %d, fec_freq %d, mpeg_freq %d\n", __func__,
		state->config->demod_freq,
		state->config->fec_freq,
		state->config->mpeg_freq);

	/* Set AGC polarization */
	ret |= avl6222_i2c_write32(state, REG_RF_AGC_POL, state->config->agc_polarization);
	ret |= avl6222_i2c_write32(state, REG_RGAGC_TRI_ENB, state->config->agc_tri);
	dprintk(10, "%s: agc_pol %d\n", __func__, state->config->agc_polarization);
	/* Set MPEG data */
	ret |= avl6222_i2c_write32(state, REG_MPEG_MODE, state->config->mpeg_mode);
	ret |= avl6222_i2c_write16(state, REG_MPEG_SERIAL, state->config->mpeg_serial);
	ret |= avl6222_i2c_write16(state, REG_MPEG_POS_EDGE, state->config->mpeg_clk_mode);
	dprintk(10, "%s: mpeg_mode %d, mpeg_serial %d, mpeg_clk_mode %d, mpeg_tri 0x%x\n", __func__,
		state->config->mpeg_mode,
		state->config->mpeg_serial,
		state->config->mpeg_clk_mode,
		state->config->mpeg_tri);

	/* Enable MPEG output */
	ret = avl6222_i2c_write32(state, rc_mpeg_bus_tri_enb, state->config->mpeg_tri);
	dprintk(100, "%s < status = %u\n", __func__, ret);
	return ret;
}

u16 avl6222_save_config(struct dvb_frontend *fe, u32 *buf32, u16 *buf16)
{
	struct avl6222_state *state = fe->demodulator_priv;
	u16 ret;

	ret = avl6222_i2c_read32(state, REG_RF_AGC_POL, buf32++);
	ret |= avl6222_i2c_read32(state, REG_MPEG_ERR_SIGNAL_POL, buf32++);
	ret |= avl6222_i2c_read32(state, REG_MPEG_MODE, buf32++);
	ret |= avl6222_i2c_read32(state, REG_MPEG_SERIAL_OUT_SEL, buf32++);
	ret |= avl6222_i2c_read32(state, REG_MPEG_SERIAL_BIT_SEQ, buf32++);
	ret |= avl6222_i2c_read32(state, REG_SPEC_INV, buf32++);
	ret |= avl6222_i2c_read32(state, REG_ALPHA, buf32++);
	ret |= avl6222_i2c_read32(state, REG_ALPHA_SETTING, buf32++);
	ret |= avl6222_i2c_read16(state, REG_TUNER_SLAVE_ADDR, buf16++);
	ret |= avl6222_i2c_read16(state, REG_TUNER_LPF_MARGIN_100KHZ, buf16++);
	ret |= avl6222_i2c_read16(state, REG_TUNER_MAX_LPF_100KHZ, buf16++);
	ret |= avl6222_i2c_read16(state, REG_TUNER_LPF_100KHZ, buf16++);
	ret |= avl6222_i2c_read16(state, REG_AAGC_REF, buf16++);
	ret |= avl6222_i2c_read16(state, REG_MPEG_POS_EDGE, buf16++);
	ret |= avl6222_i2c_read16(state, REG_MPEG_SERIAL, buf16++);
	ret |= avl6222_i2c_read16(state, REG_MPEG_SERIAL_CLK_N, buf16++);
	ret |= avl6222_i2c_read16(state, REG_MPEG_SERIAL_CLK_D, buf16++);
	ret |= avl6222_i2c_read16(state, REG_ERR_MODE_CTRL, buf16++);
	ret |= avl6222_i2c_read16(state, REG_CARRIER_FREQ_HALF_RANGE_MHZ, buf16++);
	ret |= avl6222_i2c_read16(state, 1536, buf16++);
	ret |= avl6222_i2c_read16(state, REG_MPEG_PERSISTENT_CLK_MODE, buf16++);
	dprintk(100, "%s < status = %u\n", __func__, ret);
	return ret;
}

//---------------------------------------------------------------------

u16 avl6222_restore_config(struct dvb_frontend *fe, u32 *buf32, u16 *buf16)
{
	struct avl6222_state *state = fe->demodulator_priv;
	u16 ret;

	ret = avl6222_i2c_write32(state, REG_RF_AGC_POL, *buf32++);
	ret |= avl6222_i2c_write32(state, REG_MPEG_ERR_SIGNAL_POL, *buf32++);
	ret |= avl6222_i2c_write32(state, REG_MPEG_MODE, *buf32++);
	ret |= avl6222_i2c_write32(state, REG_MPEG_SERIAL_OUT_SEL, *buf32++);
	ret |= avl6222_i2c_write32(state, REG_MPEG_SERIAL_BIT_SEQ, *buf32++);
	ret |= avl6222_i2c_write32(state, REG_SPEC_INV, *buf32++);
	ret |= avl6222_i2c_write32(state, REG_ALPHA, *buf32++);
	ret |= avl6222_i2c_write32(state, REG_ALPHA_SETTING, *buf32++);
	ret |= avl6222_i2c_write16(state, REG_TUNER_SLAVE_ADDR, *buf16++);
	ret |= avl6222_i2c_write16(state, REG_TUNER_LPF_MARGIN_100KHZ, *buf16++);
	ret |= avl6222_i2c_write16(state, REG_TUNER_MAX_LPF_100KHZ, *buf16++);
	ret |= avl6222_i2c_write16(state, REG_TUNER_LPF_100KHZ, *buf16++);
	ret |= avl6222_i2c_write16(state, REG_AAGC_REF, *buf16++);
	ret |= avl6222_i2c_write16(state, REG_MPEG_POS_EDGE, *buf16++);
	ret |= avl6222_i2c_write16(state, REG_MPEG_SERIAL, *buf16++);
	ret |= avl6222_i2c_write16(state, REG_MPEG_SERIAL_CLK_N, *buf16++);
	ret |= avl6222_i2c_write16(state, REG_MPEG_SERIAL_CLK_D, *buf16++);
	ret |= avl6222_i2c_write16(state, REG_ERR_MODE_CTRL, *buf16++);
	ret |= avl6222_i2c_write16(state, REG_CARRIER_FREQ_HALF_RANGE_MHZ, *buf16++);
	ret |= avl6222_i2c_write16(state, 1536, *buf16++);
	ret |= avl6222_i2c_write16(state, REG_MPEG_PERSISTENT_CLK_MODE, *buf16++);
	dprintk(100, "%s < status = %u\n", __func__, ret);
	return ret;
}

//---------------------------------------------------------------------

u16 avl6222_set_functional_mode(struct avl6222_state *state, u8 mode)
{
	u16 ret = 0;

	if (mode == FUNC_MODE_DEMOD)
	{
		ret = avl6222_i2c_write16(state, 0x2642, 400);
	}
	ret |= avl6222_i2c_write16(state, rc_iq_mode_addr, 0);
	return ret;
}

//---------------------------------------------------------------------

int avl6222_set_voltage(struct dvb_frontend *fe, fe_sec_voltage_t voltage)
{
	struct avl6222_state *state = fe->demodulator_priv;
	int ret = 0;

	dprintk(10, "%s(%p, %d)\n", __func__, fe, voltage);
	mutex_lock(&state->lock);
	ret = state->equipment.lnb_set_voltage(state->lnb_priv, fe, voltage);
	mutex_unlock(&state->lock);
	return ret;
}
EXPORT_SYMBOL(avl6222_set_voltage);

/*---------------------------------------------------------------------
 * DiSEqC, LNB, Tone functions
 *---------------------------------------------------------------------*/

/**
 * @brief Initialise DiSEqC
 * @param fe A ptr to a DVB API struct dvb_frontend
 * @return Returns AVL6222_OK if successful, AVL6222_ERROR_* otherwise
 */
static int avl6222_diseqc_init(struct dvb_frontend *fe)
{
	struct avl6222_state *state = fe->demodulator_priv;
	u16 ret = AVL6222_OK;
	u32 x1;
	u32 diseqc_info;

	dprintk(100, "%s >\n", __func__);

#if defined(UFS913)
	//ufs913 uses envelope mode
	diseqc_info = DISEQC_RX_150 | DISEQC_WAVEFORM_ENVELOPE | DISEQC_TX_GAP_15;
#else
	diseqc_info = DISEQC_RX_150 | DISEQC_WAVEFORM_NORMAL | DISEQC_TX_GAP_15;
#endif

	dprintk(10, "%s: diseqc_info = %d\n", __func__, diseqc_info);

	ret |= avl6222_i2c_write32(state, REG_DISEQC_SRST, 1);
	ret |= avl6222_i2c_write32(state, REG_DISEQC_SAMP_FRAC_N, 200);  /* 2M = 200 * 10kHz */
	ret |= avl6222_i2c_write32(state, REG_DISEQC_SAMP_FRAC_D, state->config->demod_freq);
	ret |= avl6222_i2c_write32(state, REG_DISEQC_TONE_FRAC_N, ((DISEQC_TONE_FREQ) << 1));
	ret |= avl6222_i2c_write32(state, REG_DISEQC_TONE_FRAC_D, state->config->demod_freq * 10);
	/* Initialize the tx_control */
	ret |= avl6222_i2c_read32(state, REG_DISEQC_TX_CTRL, &x1);
	x1 &= 0x00000300;
	x1 |= 0x20;  /* Reset tx_fifo */
	x1 |= ((u32)(diseqc_info & 0x000F) << 6);
	x1 |= ((u32)((diseqc_info & 0x0F00) >> 8) << 4);
	x1 |= (1 << 3);  /* Enable tx gap */

	dprintk(10, "%s: x1 = 0x%x\n", __func__, x1);

	ret |= avl6222_i2c_write32(state, REG_DISEQC_TX_CTRL, x1);  //ufs913 diff
	x1 &= ~(0x20);	/* Release tx_fifo reset */

	dprintk(10, "%s: x1 = 0x%x\n", __func__, x1);
	ret |= avl6222_i2c_write32(state, REG_DISEQC_TX_CTRL, x1);  //ufs913 diff

	/* Initialize the rx_control */
	x1 = ((u32)((diseqc_info & 0x0F00) >> 8) << 2);
	x1 |= (1 << 1); 	/* Activate the receiver */
	x1 |= (1 << 3);	 /* Envelop high when tone present */

	dprintk(10, "%s: x1 = 0x%x\n", __func__, x1);
	ret |= avl6222_i2c_write32(state, REG_DISEQC_RX_CTRL, x1);  //ufs913 diff

	x1 = (u32)((diseqc_info & 0xF000) >> 12);
	dprintk(10, "%s: x1 = 0x%x\n", __func__, x1);
	ret |= avl6222_i2c_write32(state, REG_DISEQC_RX_MSG_TMR, x1);
	ret |= avl6222_i2c_write32(state, REG_DISEQC_SRST, 0);
	if (ret == AVL6222_OK)
	{
		state->diseqc_status = DISEQC_STATUS_INIT;
	}
	dprintk(100, "%s < status = %u\n", __func__, ret);
	return 0;
}

//---------------------------------------------------------------------

/**
 * @brief Check if it is safe to switch Diseqc operation mode
 * @param fe A ptr to a DVB API struct dvb_frontend
 * @return Returns AVL6222_OK if successful, AVL6222_ERROR_* otherwise
 */
u16 avl6222_diseqc_switch_mode(struct dvb_frontend *fe)
{
	struct avl6222_state *state = fe->demodulator_priv;
	u16 ret = AVL6222_OK;
	u32 x1;

	switch (state->diseqc_status)
	{
		case DISEQC_STATUS_MOD:
		case DISEQC_STATUS_TONE:
		{
			ret |= avl6222_i2c_read32(state, REG_DISEQC_TX_ST, &x1);
			if (((x1 & 0x00000040) >> 6) != 1)
			{
				ret |= AVL6222_ERROR_PREV;
			}
			break;
		}
		case DISEQC_STATUS_CONTINUOUS:
		case DISEQC_STATUS_INIT:
		{
			break;
		}
		default:
		{
			ret |= AVL6222_ERROR_GENERIC;
			break;
		}
	}
	dprintk(100, "%s < status= %u\n", __func__, ret);
	return ret;
}

//---------------------------------------------------------------------

/**
 * @brief
 * @param fe A ptr to a DVB API struct dvb_frontend
 * @return Returns AVL6222_OK if successful, AVL6222_ERROR_* otherwise
 */
u16 avl6222_diseqc_send_mod_data(struct dvb_frontend *fe, const u8 *buf, u8 size)
{
	struct avl6222_state *state = fe->demodulator_priv;
	u16 ret = AVL6222_OK;
	u32 x1, x2;
	u8 buf_tmp[8];

	if (size > 8)
	{
		ret = AVL6222_ERROR_MEM;
	}
	else
	{
		ret = avl6222_diseqc_switch_mode(fe);
		if (ret == AVL6222_OK)
		{
			/* Reset rx_fifo */
			ret |= avl6222_i2c_read32(state, REG_DISEQC_RX_CTRL, &x2);
			ret |= avl6222_i2c_write32(state, REG_DISEQC_RX_CTRL, (x2 | 0x01));
			ret |= avl6222_i2c_write32(state, REG_DISEQC_RX_CTRL, (x2 & 0xfffffffe));

			ret |= avl6222_i2c_read32(state, REG_DISEQC_TX_CTRL, &x1);
			x1 &= 0xfffffff8;  //set to modulation mode and put it to FIFO load mode
			ret |= avl6222_i2c_write32(state, REG_DISEQC_TX_CTRL, x1);
			/* Trunk address */
			format_addr(REG_DISEQC_TX_FIFO_MAP, buf_tmp);
			buf_tmp[3] = 0;
			buf_tmp[4] = 0;
			buf_tmp[5] = 0;
			for (x2 = 0; x2 < size; x2++)
			{
				buf_tmp[6] = buf[x2];
				ret |= avl6222_i2c_write(state, buf_tmp, 7);
			}
			x1 |= (1 << 2);  //start fifo transmit.
			ret |= avl6222_i2c_write32(state, REG_DISEQC_TX_CTRL, x1);
			if (ret == AVL6222_OK)
			{
				state->diseqc_status = DISEQC_STATUS_MOD;
			}
		}
	}
	dprintk(100, "%s: < ret = %u\n", __func__, ret);
	return ret;
}

//---------------------------------------------------------------------

/**
 * @brief
 * @param fe A ptr to a DVB API struct dvb_frontend
 * @return Returns AVL6222_OK if successful, AVL6222_ERROR_* otherwise
 */
u16 avl6222_diseqc_get_tx_status(struct dvb_frontend *fe, struct avl6222_diseqc_tx_status *pTxStatus)
{
	struct avl6222_state *state = fe->demodulator_priv;
	u16 ret = AVL6222_OK;
	u32 x1;

	if ((state->diseqc_status == DISEQC_STATUS_MOD)
	||  (state->diseqc_status == DISEQC_STATUS_TONE))
	{
		ret = avl6222_i2c_read32(state, REG_DISEQC_TX_ST, &x1);
		pTxStatus->tx_done = (u8)((x1 & 0x00000040) >> 6);
		pTxStatus->tx_fifo_cnt = (u8)((x1 & 0x0000003c) >> 2);
	}
	else
	{
		ret = AVL6222_ERROR_GENERIC;
	}
	dprintk(100, "%s < status %u\n", __func__, ret);
	return ret;
}

//---------------------------------------------------------------------

/**
 * @brief Send DiSEqC message
 * @return 0 = success
 */
static int avl6222_send_diseqc_msg(struct dvb_frontend *fe, struct dvb_diseqc_master_cmd *d)
{
	u16 ret = AVL6222_OK;
	struct avl6222_diseqc_tx_status tx_status;
	int cnt = 100;

	if (useOriginTimings == 0)
	{
		cnt = 200;
	}
	if ((d->msg_len < 3) || (d->msg_len > 6))
	{
		return -EINVAL;
	}
	ret = avl6222_diseqc_send_mod_data(fe, d->msg, d->msg_len);
	if (ret != AVL6222_OK)
	{
		dprintk(1, "%s: Output %u modulation bytes: fail!\n", __func__, d->msg_len);
	}
	else
	{
		msleep(55);
		do
		{
			ret = avl6222_diseqc_get_tx_status(fe, &tx_status);
			if (tx_status.tx_done == 1)
			{
				break;
			}
			if (useOriginTimings == 1)
			{
				msleep(10);
			}
			else
			{
				msleep(/* 10 */ /* 3 */ 4);
			}
			if (--cnt == 0)
			{
				break;
			}
		}
		while (tx_status.tx_done != 1); /* Wait until operation finishes */

		/* msleep(100); */
		if (cnt == 0)
		{
			lockError |= cError7;
			printk("%s(): error cnt= %d, r= 0x%x, tx_status.tx_done = %d\n",
			       __func__, cnt, ret, tx_status.tx_done);
		}
		if (ret != AVL6222_OK)
		{
			dprintk(1, "%s: Output %u modulation bytes fail!\n", __func__, d->msg_len);
		}
		else
		{
			dprintk(20, "%s: Output %u modulation bytes: success!\n", __func__, d->msg_len);
		}
	}
	return 0;
}

//---------------------------------------------------------------------

/**
 * @brief Send Mini-DiSEqC burst also called Tone-burst.
 * It is used for switching between satellite A and satellite B
 * @param burst The satellite to which we want to switch: SEC_MINI_A or SEC_MINI_B
 * @return 0 = success
 */
static int avl6222_diseqc_send_burst(struct dvb_frontend *fe, fe_sec_mini_cmd_t burst)
{
	struct avl6222_state *state = fe->demodulator_priv;
	u16 ret;
	u32 x1;
	u8 buf[8];
	struct avl6222_diseqc_tx_status tx_status;

	ret = avl6222_diseqc_switch_mode(fe);
	if (ret != AVL6222_OK)
	{
		dprintk(1, "%s: Tone-burst failed!\n", __func__);
		return 0;
	}

	/* No data in the FIFO */
	ret = avl6222_i2c_read32(state, REG_DISEQC_TX_CTRL, &x1);
	x1 &= 0xfffffff8;   /* Put it into the FIFO load mode */
	if (burst == SEC_MINI_A)
	{
		x1 |= 0x01;
	}
	else
	{
		x1 |= 0x02;
	}
	ret |= avl6222_i2c_write32(state, REG_DISEQC_TX_CTRL, x1);
	/* Trunk address */
	format_addr(REG_DISEQC_TX_FIFO_MAP, buf);
	buf[3] = 0;
	buf[4] = 0;
	buf[5] = 0;
	buf[6] = 1;

	ret |= avl6222_i2c_write(state, buf, 7);

	x1 |= (1 << 2); /* Start fifo transmit */
	ret |= avl6222_i2c_write32(state, REG_DISEQC_TX_CTRL, x1);
	if (ret != AVL6222_OK)
	{
		dprintk(1, "%s: Tone-burst failed!\n", __func__);
		return 0;
	}
	else
	{
		state->diseqc_status = DISEQC_STATUS_TONE;
	}
	ret = avl6222_diseqc_get_tx_status(fe, &tx_status);
	if (ret != AVL6222_OK)
	{
		dprintk(1, "%s: Tune-burst sent failed: %d\n", __func__, tx_status.tx_done);
	}
	else
	{
		dprintk(10, "%s: Tune-burst sent successfully: %d\n", __func__, tx_status.tx_done);
	}
	return 0;
}

//---------------------------------------------------------------------

/**
 * @brief Set/Unset the tone. This is used for selecting a High (22 KHz) or a Low (0 KHz) freq
 * @param tone to which we want to use: SEC_TONE_ON or SEC_TONE_OFF
 * @return 0 = success
 */
int avl6222_set_tone(struct dvb_frontend *fe, fe_sec_tone_mode_t tone)
{
	struct avl6222_state *state = fe->demodulator_priv;
	u16 ret;
	u32 x1;

	dprintk(10, "%s >\n", __func__);

	if (tone == SEC_TONE_ON)
	{
		/*avl6222_diseqc_start_cont(fe);*/
		ret = avl6222_diseqc_switch_mode(fe);
		if (ret != AVL6222_OK)
		{
			return 0;
		}
		avl6222_i2c_read32(state, REG_DISEQC_TX_CTRL, &x1);
		x1 &= 0xfffffff8;
		x1 |= 0x03;
		avl6222_i2c_write32(state, REG_DISEQC_TX_CTRL, x1);
		x1 |= (1 << 10);
		ret = avl6222_i2c_write32(state, REG_DISEQC_TX_CTRL, x1);
		if (ret == AVL6222_OK)
		{
			state->diseqc_status = DISEQC_STATUS_CONTINUOUS;
		}
	}
	else
	{
		/*avl6222_diseqc_stop_cont(fe);*/
		if (state->diseqc_status == DISEQC_STATUS_CONTINUOUS)
		{
			avl6222_i2c_read32(state, REG_DISEQC_TX_CTRL, &x1);
			x1 &= 0xfffff3ff;
			avl6222_i2c_write32(state, REG_DISEQC_TX_CTRL, x1);
		}
	}
	return 0;
}
EXPORT_SYMBOL(avl6222_set_tone);

/*---------------------------------------------------------------------
 * Status functions
 *---------------------------------------------------------------------*/

/**
 * @brief Reset the regs used for gathering stats
 * @param fe A ptr to a DVB API struct dvb_frontend
 * @return Returns AVL6222_OK if successful, AVL6222_ERROR_* otherwise
 */
u16 avl6222_reset_stats(struct dvb_frontend *fe)
{
	struct avl6222_state *state = fe->demodulator_priv;
	u8 func_mode;
	u16 ret;

	ret = avl6222_get_mode(state, &func_mode);

	if (func_mode == FUNC_MODE_DEMOD)
	{
		ret |= avl6222_send_op(DEMOD_OP_RESET_BERPER, state);
	}
	else
	{
		ret = AVL6222_ERROR_GENERIC;
	}
	return ret;
}

//---------------------------------------------------------------------

/**
 * @brief Check the lock status of the demod. The only status we can have is if we have lock or not
 * We cannot get nothing in-between like FE_HAS_CARRIER | FE_HAS_VITERBI | FE_HAS_SYNC
 * @param fe A ptr to a DVB API struct dvb_frontend
 * @return Always returns 0
 */
static int avl6222_read_status(struct dvb_frontend *fe, fe_status_t *status)
{
	struct avl6222_state *state = fe->demodulator_priv;
	u16 lock_status = 5;

	*status = 0;

	/* Check only once if we have lock and return immediately, do not wait */
	avl6222_i2c_read16(state, REG_FEC_LOCK, &lock_status);

	if (lock_status == 0) /* the orig app on ufs913 gots not 1 on success! */
	{
		*status = 0;
		dprintk(1, "%s: Could not get lock status!\n", __func__);
	}
	else
	{
		*status |= FE_HAS_SIGNAL | FE_HAS_CARRIER | FE_HAS_VITERBI | FE_HAS_SYNC | FE_HAS_LOCK;
		dprintk(50, "%s: Service locked!!!\n", __func__);
	}
	return 0;
}

//---------------------------------------------------------------------

/**
 * @brief Get Bit error rate
 * @param fe A ptr to a DVB API struct dvb_frontend
 * @param ber User-space reads this value
 * @return Always returns 0
 */
static int avl6222_read_ber(struct dvb_frontend *fe, u32 *ber)
{
	struct avl6222_state *state = fe->demodulator_priv;
	u16 ret = AVL6222_OK;

	ret = avl6222_i2c_read32(state, REG_BER, ber);
	if (ret != AVL6222_OK)
	{
		dprintk(1, "Could not get BER\n");
		*ber = 0;
	}
	else
	{
		*ber /= 1000000000;
	}
	return 0;
}

//---------------------------------------------------------------------

/**
 * @brief Get signal strength
 * @param fe A ptr to a DVB API struct dvb_frontend
 * @param signal_strength User-space reads this value
 * @return 0 if successful, -1 otherwise
 */
static int avl6222_read_signal_strength(struct dvb_frontend *fe, u16 *signal_strength)
{
	struct avl6222_state *state = fe->demodulator_priv;
	u16 ret = AVL6222_OK;
	u32 data;

	ret = avl6222_i2c_read32(state, REG_AAGC, &data);

	if (ret == AVL6222_OK)
	{
		data += 0x800000;
		data &= 0xffffff;
		*signal_strength = (u16)(data >> 8);
		return 0;
	}
	else
	{
		*signal_strength = 0;
		dprintk(1, "Could not get signal strength\n");
	}
	return -1;
}

//---------------------------------------------------------------------

/**
 * @brief Get Signal-to-noise ratio
 * @param fe A ptr to a DVB API struct dvb_frontend
 * @param snr User-space reads this value
 * @return Always returns 0
 */
static int avl6222_read_snr(struct dvb_frontend *fe, u16 *snr)
{
	struct avl6222_state *state = fe->demodulator_priv;
	u16 ret;
	u32 r_snr;

	ret = avl6222_i2c_read32(state, REG_SNR_DB, &r_snr);
	if (ret != AVL6222_OK)
	{
		dprintk(1, "Could not get SNR\n");
		*snr = 0;
	}
	else
	{
		/* FIXME: The demod returns the snr in dB * 100
		 * I convert it to the range [0 - 65535] = [0% - 100%],
		 * but this is not the way for doing it :( */
		if (((r_snr) >= 0) && ((r_snr) <= 2236))
		{
			*snr = r_snr * 65535 / 2236;
		}
		else
		{
			*snr = 0;
		}
	}
	return 0;
}

//---------------------------------------------------------------------

/**
 * @brief Supported by demod???
 * @param fe A ptr to a DVB API struct dvb_frontend
 */
static int avl6222_read_ucblocks(struct dvb_frontend *fe, u32 *ucblocks)
{
	*ucblocks = 0;
	return 0;
}

/*---------------------------------------------------------------------
 * Frontend ops
 *---------------------------------------------------------------------*/

/**
 * @brief Free the main struct avl6222_state
 * @param fe A ptr to a DVB API struct dvb_frontend
 */
static void avl6222_release(struct dvb_frontend *fe)
{
	struct avl6222_state *state = fe->demodulator_priv;
	dprintk(100, "%s <>\n", __func__);
	kfree(state);
}

//---------------------------------------------------------------------

static struct dvb_frontend_ops avl6222_ops;


//---------------------------------------------------------------------

/**
 * @brief Initialise or wake up device
 * Power config will reset and load initial firmware if required
 * @param fe A ptr to a DVB API struct dvb_frontend
 * @return Returns 0 if successful, -1 otherwise
 */
static int avl6222_initfe(struct dvb_frontend *fe)
{
	struct avl6222_state *state = fe->demodulator_priv;
	u16 ret;

	if (state->boot_done)
	{
		dprintk(10, "Boot procedure already done. Skipping it.\n");
		return 0;
	}
	ret = avl6222_setup_pll_2(state, (const struct avl6222_pllconf *)(pll_conf + state->config->pll_config));

	if (ret != AVL6222_OK)
	{
		dprintk(1, "%s: PLL initialization failed!\n",__func__);
		return -1;
	}
	mdelay(1);

	ret = avl6222_load_firmware(fe);
	if (ret != AVL6222_OK)
	{
		dprintk(1, "%s: Demod firmware load failed!\n", __func__);
		return -1;
	}

	/* Wait for the demod to boot */
	mdelay(100);
	ret = avl6222_get_demod_status(fe);
	if (ret != AVL6222_OK)
	{
		dprintk(1, "%s: Demodulator boot failed!\n", __func__);
		return -1;
	}
	ret = avl6222_demod_init(fe);
	if (ret != AVL6222_OK)
	{
		dprintk(1, "%s: Demod Initialization failed!\n", __func__);
		return -1;
	}

	/* tuner init start */
	ret = avl6222_i2c_write16(state, REG_TUNER_SLAVE_ADDR, state->config->tuner_address);

	/* Sharp QM1D1C0042 specific settings
	 * ->no it isn't the settings are also needed for stv6110a !!! so
	 * they are needed for external tuner.
	 */
	ret |= avl6222_i2c_write16(state, REG_TUNER_USE_INTERNAL_CTRL, 0); /* Use external control */
	ret |= avl6222_i2c_write16(state, REG_TUNER_LPF_MARGIN_100KHZ, 0);
	ret |= avl6222_i2c_write16(state, REG_TUNER_MAX_LPF_100KHZ, state->config->max_lpf);
	ret |= avl6222_i2c_repeater_init(state->config->i2c_speed_khz, state);
	ret |= avl6222_i2c_write32(state, 0x640160, 0xF3);
	mdelay(5);
	ret |= avl6222_i2c_write32(state, 0x640160, 0xD3);
	mdelay(5);
	ret |= state->equipment.tuner_init(fe);
	if (ret != AVL6222_OK)
	{
		dprintk(1, "%s: Tuner initialization failed!\n");
		return -1;
	}
	/* tuner init end */
	ret = avl6222_diseqc_init(fe);
	if (ret != AVL6222_OK)
	{
		dprintk(1, "%s: DiSEqC initialization failed!\n");
		return -1;
	}
	dprintk(10, "Demodulator and tuner successfully initialized.\n");
	state->boot_done = 1;
	initDone = 1;
	return 0;
}

//---------------------------------------------------------------------

/**
 * @brief Put device to sleep
 * @param fe A ptr to a DVB API struct dvb_frontend
 * @return Returns 0 if successful, -1 otherwise
 */
static int avl6222_sleep(struct dvb_frontend *fe)
{
	struct avl6222_state *state = fe->demodulator_priv;
	u16 ret;

	dprintk(10, "%s()\n", __func__);
	//ret = avl6222_send_op(DEMOD_OP_SLEEP, state);
	return 0;
}

//---------------------------------------------------------------------
static int avl6222_wake(struct dvb_frontend *fe)
{
	struct avl6222_state *state = fe->demodulator_priv;
	u16 ret;

	dprintk(10, "%s()\n", __func__);
	ret = avl6222_send_op(DEMOD_OP_WAKE, state);
	return 0;
}


//---------------------------------------------------------------------

static int avl6222_set_property(struct dvb_frontend *fe, struct dtv_property *tvp)
{
	dprintk(20, "%s()\n", __func__);
	return 0;
}

//---------------------------------------------------------------------

static int avl6222_get_property(struct dvb_frontend *fe, struct dtv_property *tvp)
{
	/* get delivery system info */
	if (tvp->cmd == DTV_DELIVERY_SYSTEM)
	{
		switch (tvp->u.data)
		{
			case SYS_DVBS2:
			{
				dprintk(10, "%s: DVB-S2 delivery system selected\n", __func__);
				break;
			}
			case SYS_DVBS:
			{
				dprintk(10, "%s: DVB-S delivery system selected\n", __func__);
				break;
			}
			case SYS_DSS:
			{
				dprintk(10, "%s: DSS delivery system selected\n", __func__);
				break;
			}
			default:
			{
				return -EINVAL;
			}
		}
	}
	dprintk(20, "%s()\n", __func__);
	return 0;
}

/*---------------------------------------------------------------------
 * ITuner
 *---------------------------------------------------------------------*/

/**
 * @brief dvb-core told us to tune, the tv property cache will be complete,
 * it's safe for us to pull values and use them for tuning purposes.
 * The setting procedure is divided in two steps: the lock setting that sets only the frequency,
 * wait 150ms, and then set what availink calls the 'channel' setting that sets symbol rate,
 * inversion, delivery system, pilot, roll-off, FEC.
 * The reason behing the 'channel' setting is that all these parameters, except the symbol rate,
 * can be calculated automatically by the demod.
 */
static int avl6222_set_frontend(struct dvb_frontend *fe, struct dvb_frontend_parameters *p)
{
	struct avl6222_state *state = fe->demodulator_priv;
	struct dtv_frontend_properties *c = &fe->dtv_property_cache;
	struct avl6222_tuning tuning;
	u8 func_mode;
	u16 cnt, lock_status = 0, ret = 0;
	u32 freq, max_time;

	mutex_lock(&state->lock);

	lockError = 0;
	demodLock = 0;
	tunerLock = 0;

	dprintk(20, "%s: delivery system	= %d\n", __func__, c->delivery_system);
	dprintk(20, "%s: modulation      = %d\n", __func__, c->modulation);
	dprintk(20, "%s: frequency       = %d\n", __func__, c->frequency);
	dprintk(20, "%s: symbol_rate     = %d\n", __func__, c->symbol_rate);
	dprintk(20, "%s: inversion       = %d\n", __func__, c->inversion);
	dprintk(20, "%s: rolloff         = %d\n", __func__, c->rolloff);
	dprintk(20, "%s: pilot           = %d\n", __func__, c->pilot);
	cnt = 10;

	do
	{
		/* Halt CPU to improve tuner's locking speed */
		ret = avl6222_cpu_halt(fe);

		if (ret != AVL6222_OK)
		{
			dprintk(1, "%s: failed to halt cpu %d\n", __func__, cnt);
			msleep(50);
			continue;
		}
		else
		{
			break;
		}
	}
	while (--cnt);

	/* Tuner lock */
	freq = c->frequency / 100;

	ret = state->equipment.tuner_lock(fe, freq, c->symbol_rate, state->config->lpf);
	if (ret != AVL6222_OK)
	{
		lockError |= cError2;
		dprintk(1, "%s: Tuner set failed!\n", __func__);
		mutex_unlock(&state->lock);
		return -1;
	}
	/* Wait for tuner locking */
	max_time = 150; /* Max waiting time: 150ms */

	if (useOriginTimings == 1)
	{
		cnt = max_time / 10;
	}
	else
	{
		cnt = max_time / 5;
	}
	do
	{
		/* Check lock status */
		ret = state->equipment.tuner_lock_status(fe);

		if (ret == AVL6222_ERROR_PREV)
		{
			if (useOriginTimings == 1)
			{
				msleep(10);  /* Wait 10ms for tuner to lock the channel */
			}
			else
			{
				msleep(5);  /* Wait 10ms for tuner to lock the channel */
			}
			dprintk(10, "%s(): Waiting for tuner lock: %d\n", __func__, cnt);
			continue;
		}
		else
		{
			if (ret != AVL6222_OK)
			{
				lockError |= cError3;
				dprintk(1, "%s: Failed while checking lock status!\n", __func__);
				mutex_unlock(&state->lock);
				return -1;
			}
			else
			{
				break;
			}
		}
	}
	while (--cnt);

	dprintk(20, "%s: Tuner successfully set!\n", __func__);

	/* Be sure that we are in demod status, if not, set it */
	avl6222_get_mode(state, &func_mode);
	if (func_mode != FUNC_MODE_DEMOD)
	{
		lockError |= cError4;
		dprintk(1, "Functional mode is not set for demodulation, changing it");
		avl6222_set_functional_mode(state, FUNC_MODE_DEMOD);
	}

	/* fixme: next one is different in SDK */

	/* Channel lock */
	tuning.symbol_rate = c->symbol_rate;
	tuning.iq_swap = state->config->iq_swap;
	tuning.auto_iq_swap = state->config->auto_iq_swap;
	if (c->delivery_system == SYS_DVBS)
	{
		tuning.delsys = 0x00; /* Set DVB-S */
	}
	else
	{
		tuning.delsys = 0x01; /* Set DVB-S2 */
	}
	tuning.lock_mode = state->config->lock_mode;

	ret = avl6222_channel_lock(fe, &tuning);
	if (ret != AVL6222_OK)
	{
		lockError |= cError5;
		dprintk(1, "%s: Channel set failed!\n", __func__);
		mutex_unlock(&state->lock);
		return -1;
	}
	cnt = 20;
	do
	{
		ret = avl6222_get_op_status(state);

		if (ret == AVL6222_OK)
		{
			break;
		}

		mdelay(10);
	}
	while (--cnt);

	tunerLock = 1;

	/* Wait a bit more when we have low symbol rates */
	if (c->symbol_rate < 5000000)
	{
		max_time = 5000 * 2; /* Max waiting time: 5000ms */
	}
	else if (c->symbol_rate < 10000000)
	{
		max_time = 600 * 2; /* Max waiting time: 600ms */
	}
	else
	{
		max_time = 250 * 2; /* Max waiting time: 250ms */
	}
	if (useOriginTimings == 1)
	{
		cnt = max_time / 10;
	}
	else
	{
		cnt = max_time / /* 10 */ 5;
	}
	do
	{
		if (avl6222_i2c_read16(state, REG_FEC_LOCK, &lock_status) != AVL6222_OK)
		{
			dprintk(20, "%s: Could not get lock status. Retrying %d more times\n", __func__, cnt);
		}
		if (lock_status == 1)
		{
			break;
		}
		if (useOriginTimings == 1)
		{
			msleep(10);  /* Wait 10ms for demod to lock the channel */
		}
		else
		{
			msleep(/* 10 */ 4);  /* Wait 4ms for demod to lock the channel */ //changed from 20 to 4 by Andreas Kircher
		}

		dprintk(20, "%s: Waiting for lock (tuner %d) c: %d\n", __func__, state->config->tuner_no, cnt);
	}
	while (--cnt);

	if (cnt == 0)
	{
		lockError |= cError1;
		dprintk(1, "%s: Time out (tuner %d, 0x%02x) !\n", __func__, state->config->tuner_no, state->config->demod_address);
		mutex_unlock(&state->lock);
		return -ETIMEDOUT;
	}
	dprintk(1, "%s: Service locked!!! (tuner %d)\n", __func__, state->config->tuner_no);

	ret = avl6222_reset_stats(fe);
	if (ret != AVL6222_OK)
	{
		lockError |= cError6;
		dprintk(1, "%s: Could not reset stats!\n", __func__);
	}
	else
	{
		demodLock = 1;
	}
	mutex_unlock(&state->lock);
	return ret;
}

/**
 * @brief Entry point to the driver. Inits the state struct used everywhere
 * @param config A ptr to struct avl6222_config that is filled outside
 * @param A ptr to struct i2c_adapter that knows everything regarding the i2c
 *		  including our slot number.
 * @return A ptr to a DVB API struct dvb_frontend
 */
struct dvb_frontend *avl6222_attach(struct avl6222_config *config, struct i2c_adapter *i2c)
{
	struct avl6222_state *state = NULL;
	struct avl6222_ver_info version;

	dprintk(100, "%s: > \n", __func__);

	/* Allocate memory for the internal state */
	state = kmalloc(sizeof(struct avl6222_state), GFP_KERNEL);
	if (state == NULL)
	{
		dprintk(1, "%s: Unable to kmalloc\n", __func__);
		return NULL;
	}

	/* Setup the state used everywhere */
	memset(state, 0, sizeof(struct avl6222_state));

	state->config = config;
	state->i2c = i2c;
	state->boot_done = 0;
	state->diseqc_status = DISEQC_STATUS_UNINIT;

	/* Get version and patch (debug only) number */
	if (avl6222_get_version(state, &version) != AVL6222_OK)
	{
		dprintk(1, "Could not get version");
		kfree(state);
		return NULL;
	}
	else
	{
		dprintk(20, "AVL6222: Chip version: %u.%u.%u\n", version.major, version.minor, version.build);
		dprintk(30, "Patch version: %u.%u.%u\n", version.patch_major, version.patch_minor, version.patch_build);
	}

	state->equipment.demod_i2c_repeater_send = avl6222_i2c_repeater_send;
	state->equipment.demod_i2c_repeater_recv = avl6222_i2c_repeater_recv;
	state->equipment.demod_i2c_write         = avl6222_i2c_write;
	state->equipment.demod_i2c_write16       = avl6222_i2c_write16;
	state->equipment.demod_i2c_read16        = avl6222_i2c_read16;
	state->equipment.demod_send_op           = avl6222_send_op;
	state->equipment.demod_get_op_status     = avl6222_get_op_status;

	stv6110a_attach(&state->frontend, state, &state->equipment, 16, state->config->max_lpf);

	if (config->usedLNB == cLNB_LNBH221)
	{
		state->lnb_priv = lnbh221_attach(state->config->lnb, &state->equipment);
	}
	else if (config->usedLNB == cLNB_PIO)
	{
		state->lnb_priv = lnb_pio_attach(state->config->lnb, &state->equipment);
	}
	else if (config->usedLNB == cLNB_A8293)
	{
		state->lnb_priv = lnb_a8293_attach(state->config->lnb, &state->equipment);
	}
	mutex_init(&state->lock);
	mutex_init(&state->protect);
	memcpy(&state->frontend.ops, &avl6222_ops, sizeof(struct dvb_frontend_ops));
	state->frontend.demodulator_priv = state;
	dprintk(100, "%s: < \n", __func__);
	return &state->frontend;
}

static struct dvb_frontend_ops avl6222_ops =
{
	.info =
	{
		.name = "Availink AVL6222 DVB-S2",
		.type = FE_QPSK,
		.frequency_min       =  950000, /* 950 MHz */
		.frequency_max       = 2150000, /* 2150 MHz */
		.frequency_stepsize  = 1011, /* 1011 kHz for QPSK frontends */
		.frequency_tolerance = 5000, /* 5000 kHz */
		.symbol_rate_min     = 800000,  /* 800 kSym/s */
		.symbol_rate_max     = 60000000,  /* 50 MSym/s */
		.caps                = FE_CAN_INVERSION_AUTO
		                     | FE_CAN_FEC_1_2
		                     | FE_CAN_FEC_2_3
		                     | FE_CAN_FEC_3_4
		                     | FE_CAN_FEC_4_5
		                     | FE_CAN_FEC_5_6
		                     | FE_CAN_FEC_6_7
		                     | FE_CAN_FEC_7_8
		                     | FE_CAN_FEC_8_9
		                     | FE_CAN_FEC_AUTO
		                     | FE_CAN_QPSK
		                     | FE_CAN_RECOVER
		                     | FE_CAN_2G_MODULATION
		                     | FE_CAN_QAM_16
		                     | FE_CAN_QAM_32
		                     | FE_CAN_QAM_64
		                     | FE_CAN_QAM_AUTO
		                     | FE_CAN_TRANSMISSION_MODE_AUTO
		                     | FE_CAN_GUARD_INTERVAL_AUTO
		                     | FE_CAN_HIERARCHY_AUTO
	},
	.init                   = avl6222_initfe,
	.release                = avl6222_release,
	.sleep                  = avl6222_sleep,
	.read_status            = avl6222_read_status,
	.read_ber               = avl6222_read_ber,
	.read_signal_strength   = avl6222_read_signal_strength,
	.read_snr               = avl6222_read_snr,
	.read_ucblocks          = avl6222_read_ucblocks,
	.set_tone               = avl6222_set_tone,
	.set_voltage            = avl6222_set_voltage,
	.diseqc_send_master_cmd = avl6222_send_diseqc_msg,
	.diseqc_send_burst      = avl6222_diseqc_send_burst,
	.set_property           = avl6222_set_property,
	.get_property           = avl6222_get_property,
	.set_frontend           = avl6222_set_frontend,
};

static void avl6222_register_frontend(struct dvb_adapter *dvb_adap, struct socket_s *socket)
{
	struct dvb_frontend *frontend;
	struct avl6222_config *cfg;
	struct avl_private_data_s *priv;

	dprintk(100, "%s\n", __func__);

	if (numSockets + 1 == cMaxSockets)
	{
		dprintk(1, "Error: maximum number of sockets reached; cannot register\n");
		return;
	}

	socketList[numSockets] = *socket;
	numSockets++;

	priv = (struct avl_private_data_s *)frontend_cfg[numSockets - 1].private;

	cfg = kmalloc(sizeof(struct avl6222_config), GFP_KERNEL);

	if (cfg == NULL)
	{
		dprintk(1, "%s: malloc error\n", __func__);
		return;
	}
	cfg->tuner_no = numSockets;
	cfg->tuner_enable_pin = stpio_request_pin(socket->tuner_enable[0], socket->tuner_enable[1], "FE_ENABLE", STPIO_OUT);
	cfg->tuner_active_lh = socket->tuner_enable[2];
	cfg->demod_address = frontend_cfg[numSockets - 1].demod_i2c;
	cfg->tuner_address = frontend_cfg[numSockets - 1].tuner_i2c;

	printk("--->>> cfg->demod_address=0x%x, cfg->tuner_address=0x%x\n", cfg->demod_address, cfg->tuner_address);

	cfg->demod_freq       = priv->demod_freq;  /*< Demod clock in 10kHz units */
	cfg->fec_freq         = priv->fec_freq;	   /*< FEC clock in 10kHz units */
	cfg->mpeg_freq        = priv->mpeg_freq;   /*< MPEG clock in 10kHz units */
	cfg->i2c_speed_khz    = priv->i2c_speed_khz;
	cfg->agc_polarization = priv->agc_polarization;
	cfg->mpeg_mode        = priv->mpeg_mode;
	cfg->mpeg_serial      = priv->mpeg_serial;
	cfg->mpeg_clk_mode    = priv->mpeg_clk_mode;
	cfg->mpeg_tri         = priv->mpeg_tri;
	cfg->pll_config       = priv->pll_config;
	cfg->lpf              = priv->lpf;
	cfg->max_lpf          = priv->max_lpf;
	cfg->lock_mode        = priv->lock_mode;
	cfg->iq_swap          = priv->iq_swap;
	cfg->auto_iq_swap     = priv->auto_iq_swap;
	cfg->agc_tri          = priv->agc_tri;
	cfg->usedTuner        = priv->usedTuner;
	cfg->usedLNB          = priv->usedLNB;

	memcpy(cfg->lnb, socket->lnb, sizeof(cfg->lnb));

	frontend = avl6222_attach(cfg, i2c_get_adapter(socket->i2c_bus));

	if (frontend == NULL)
	{
		dprintk(1, "Function avl6222_attach failed\n");

		if (cfg->tuner_enable_pin)
		{
			stpio_free_pin(cfg->tuner_enable_pin);
		}
		kfree(cfg);
		return;
	}
	if (dvb_register_frontend(dvb_adap, frontend))
	{
		dprintk(1, "%s: Frontend registration failed !\n", __func__);
		if (frontend->ops.release)
		{
			frontend->ops.release(frontend);
		}
		return;
	}
	return;
}

static int avl6222_demod_detect(struct socket_s *socket, struct frontend_s *frontend)
{
	u8 b0[] = { 0x00 };
	u8 b1[] = { 0x00 };
	struct avl_private_data_s *priv;
	struct stpio_pin *pin = NULL;

	struct i2c_msg msg[] =
	{
		{ .addr = frontend_cfg[numSockets].demod_i2c, .flags = 0,        .buf = b0, .len = 1 },
		{ .addr = frontend_cfg[numSockets].demod_i2c, .flags = I2C_M_RD, .buf = b1, .len = 1 }
	};

	pin = stpio_request_pin(socket->tuner_enable[0], socket->tuner_enable[1], (numSockets == 0) ? "FE1_ENABLE" : "FE2_ENABLE", STPIO_OUT);

	dprintk(100, "%s > %s: i2c-%d addr 0x%x\n", __func__, socket->name, socket->i2c_bus, frontend_cfg[numSockets].demod_i2c);

	if (pin != NULL)
	{
		struct avl6222_state state;
		int ret;

		stpio_set_pin(pin, socket->tuner_enable[2]);
		msleep(50);
		stpio_set_pin(pin, !socket->tuner_enable[2]);
		msleep(50);
		stpio_set_pin(pin, socket->tuner_enable[2]);
		msleep(250);

		state.i2c = i2c_get_adapter(socket->i2c_bus);
		dprintk(20, "%s: state.i2c name = %s, i2c_bus = 0x%x\n", __func__, state.i2c->name, socket->i2c_bus);
		ret = i2c_transfer(state.i2c, msg, 2);

		if (ret != 2)
		{
			dprintk(1, "%s: i2c-error: %i\n", __func__, ret);
			stpio_free_pin(pin);
			return -EREMOTEIO;
		}
		dprintk(20, "%s: i2c detect reply = 0x%02X\n", __func__, b1[0]);
		priv = (struct avl_private_data_s *)frontend_cfg[numSockets].private;

		/* so now lets us try to init the demod ... */
		state.config = kmalloc(sizeof(struct avl6222_config), GFP_KERNEL);
		state.config->demod_address = frontend_cfg[numSockets].demod_i2c;
		state.config->pll_config = priv->pll_config;
		state.frontend.demodulator_priv = &state;

		ret = avl6222_setup_pll(&state, (const struct avl6222_pllconf *)(pll_conf + state.config->pll_config));
		mdelay(1);
		ret |= avl6222_load_firmware(&state.frontend);
		mdelay(100);
		ret |= avl6222_get_demod_status(&state.frontend);
		kfree(state.config);
		if (ret != AVL6222_OK)
		{
			dprintk(1, "%s: Failed to detect avl6222 0x%x\n", __func__, b1[0]);
			stpio_free_pin(pin);
			return -1;
		}
		dprintk(20, "%s: Detected avl6222\n", __func__);
	}
	else
	{
		dprintk(1, "%s: Failed to allocate pio pin (%d,%d)\n", __func__, socket->tuner_enable[0], socket->tuner_enable[1]);
		return 1;
	}
	stpio_free_pin(pin);
	dprintk(100, "%s <\n", __func__);
	return 0;
}

//---------------------------------------------------------------------

static int avl6222_demod_attach(struct dvb_adapter *adapter, struct socket_s *socket, struct frontend_s *frontend)
{
	dprintk(100, "%s >\n", __func__);

	avl6222_register_frontend(adapter, socket);

	dprintk(100, "%s <\n", __func__);
	return 0;
}

/* ******************************* */
/* platform device functions       */
/* ******************************* */

static int avl6222_probe(struct platform_device *pdev)
{
	struct platform_frontend_s *plat_data = pdev->dev.platform_data;
	struct frontend_s frontend;
	int i;

	dprintk(100, "%s >\n", __func__);

	frontend_cfg = kmalloc(plat_data->numConfigs * sizeof(struct platform_frontend_config_s), GFP_KERNEL);
	memcpy(frontend_cfg, plat_data->config, plat_data->numConfigs * sizeof(struct platform_frontend_config_s));

	for (i = 0; i < plat_data->numConfigs; i++)
	{
		dprintk(20,"Found frontend[%d] \"%s\" in platform config\n", i, frontend_cfg[i].name);
	}
	numConfigs = plat_data->numConfigs;

	frontend.demod_detect = avl6222_demod_detect;
	frontend.demod_attach = avl6222_demod_attach;
	frontend.name         = "avl6222";

	if (socket_register_frontend(&frontend) < 0)
	{
		dprintk(1, "Failed to register frontend\n");
	}
	dprintk(100, "%s <\n", __func__);
	return 0;
}

//---------------------------------------------------------------------

static int avl6222_remove(struct platform_device *pdev)
{
	return 0;
}

//---------------------------------------------------------------------

static struct platform_driver avl6222_driver =
{
	.probe  = avl6222_probe,
	.remove = avl6222_remove,
	.driver	=
	{
		.name  = "avl6222",
		.owner = THIS_MODULE,
	},
};

/* ******************************* */
/* module functions                */
/* ******************************* */

int __init avl6222_init(void)
{
	int ret;

	dprintk(100, "%s >\n", __func__);
	ret = platform_driver_register(&avl6222_driver);
	dprintk(100, "%s < (ret = %d)\n", __func__, ret);
	return ret;
}

static void avl6222_cleanup(void)
{
	dprintk(100, "%s <>\n", __func__);
}

module_init(avl6222_init);
module_exit(avl6222_cleanup);

module_param(paramDebug, short, 0644);
MODULE_PARM_DESC(paramDebug, "Activates frontend debugging (default:0)");

MODULE_DESCRIPTION("Availink AVL6222 Demod");
MODULE_AUTHOR("TDT");
MODULE_VERSION("1.0");
MODULE_LICENSE("GPL");
// vim:ts=4
