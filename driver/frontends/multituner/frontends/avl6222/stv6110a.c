/*
 * @brief stv6110a.c
 *
 * @author adapted by konfetti
 *
 *
 * Copyright (C) 2011 duckbox
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/firmware.h>
#include <linux/platform_device.h>
#include <linux/dvb/version.h>
#include <linux/version.h>

#include "equipment.h"
#include "avl6222.h"
#include "frontend_platform.h"  // for dprintk definition

extern short paramDebug;
#if defined TAGDEBUG
#undef TAGDEBUG
#endif
#define TAGDEBUG "[stv6110a] "

#define RSTV6110_CTRL1   0
#define RSTV6110_CTRL2   1
#define RSTV6110_TUNING1 2
#define RSTV6110_TUNING2 3
#define RSTV6110_CTRL3   4
#define RSTV6110_STAT1   5
#define RSTV6110_STAT2   6
#define RSTV6110_STAT3   7

#define RSTV6110_MAX     8

struct stv6110_state
{
	struct equipment_s equipment;
	u8                 regs[RSTV6110_MAX];  /* stv6110a tuner register saves */
	u32                mclk;  /* stv6110a masterclock setting */
	u32                max_lpf;
};

static int PowOf2(int number)
{
	int i;
	int result = 1;

	for (i = 0; i < number; i++)
	{
		result *= 2;
	}
	return (result);
}

int absolute(int x)
{
	if (x > 0)
	{
		return x;
	}
	else
	{
		return -1 * x;
	}
}

u32 stv6110_commit(struct dvb_frontend *fe)
{
	struct stv6110_state *state = fe->tuner_priv;
	u16 res;
	u8  TunerReg[9];
	u8  ucTemp[2];
	u8  i;

	dprintk(100, "%s > (%p %p)\n", __func__, fe->tuner_priv, fe->demodulator_priv);

	TunerReg[0] = RSTV6110_CTRL1; /* address of the start register */
	for (i = 0; i < 8 ; i++)
	{
		TunerReg[i + 1] = state->regs[i];
	}
	dprintk(200, "%s 1. send\n", __func__);
	res = state->equipment.demod_i2c_repeater_send(fe->demodulator_priv, TunerReg, 9);
	if (AVL6222_OK != res)
	{
		return (res);
	}
	ucTemp[0] = RSTV6110_STAT1;
	/* ucTemp[1] = 0x04;  start VCO Auto Calibration */
	ucTemp[1] = 0x07;  /* ufs913 */

	dprintk(200, "%s 2. send\n", __func__);
	res = state->equipment.demod_i2c_repeater_send(fe->demodulator_priv, ucTemp, 2);

	if (AVL6222_OK != res)
	{
		return (res);
	}
	msleep(10); /* wait 10ms for VCO Calibration */
	ucTemp[0] = RSTV6110_STAT1;
	ucTemp[1] = 0x02; /* Start LPF auto calibration*/
	dprintk(200, "%s 3. send\n", __func__);

	res = state->equipment.demod_i2c_repeater_send(fe->demodulator_priv, ucTemp, 2);
	if (AVL6222_OK != res)
	{
		return (res);
	}
	msleep(10); /* wait 10ms for LPF Calibration */
	ucTemp[0] = RSTV6110_CTRL3;
	ucTemp[1] = state->regs[RSTV6110_CTRL3] | 0x40;  /* calibration done, desactivate the calibration Clock */

	dprintk(200, "%s 4. send\n", __func__);
	res = state->equipment.demod_i2c_repeater_send(fe->demodulator_priv, ucTemp, 2);
	dprintk(100, "%s < res %d\n", __func__, res);
	return (res);
}

u16 stv6110_tuner_lock(struct dvb_frontend *fe, u32 frequency, u32 srate, u32 _lfp)
{
	struct stv6110_state *state = fe->tuner_priv;
	u32                  P, Presc, r, divider;
	int                  pVal, pCalc, pCalcOpt, rDiv, rDivOpt;
	u32                  ret;
	u32                  lpf;

	dprintk(100, "%s > freq=%d kHz, mclk=%d MHz, srate = %d\n", __func__, frequency, state->mclk, srate);

	/* set_frequency */
	state->regs[RSTV6110_CTRL1] = (state->regs[RSTV6110_CTRL1] & 0x07) + ((state->mclk - 16) << 3);

	if (frequency <= 10230)
	{
		P = 1;
		Presc = 0;
	}
	else if (frequency <= 13000)
	{
		P = 1;
		Presc = 1;
	}
	else if (frequency <= 20460)
	{
		P = 0;
		Presc = 0;
	}
	else
	{
		P = 0;
		Presc = 1;
	}
	pVal = (int) PowOf2(P + 1) * 10;
	pCalcOpt = 1000;
	rDivOpt = 0;
	for (rDiv = 0; rDiv <= 3; rDiv++)
	{
		pCalc = (state->mclk * 10) / (PowOf2(rDiv + 1));
		if ((absolute(pCalc - pVal)) < (absolute(pCalcOpt - pVal)))
		{
			rDivOpt = rDiv;
		}
		pCalcOpt = (state->mclk * 10) / (PowOf2(rDivOpt + 1));
	}
	r = PowOf2(rDivOpt + 1);
	divider = (frequency * 100 * r * PowOf2(P + 1) * 10) / (state->mclk * 1000);
	divider = (divider + 5) / 10;
	state->regs[RSTV6110_TUNING1] = divider & 0xff;
	state->regs[RSTV6110_TUNING2] = (P << 4) + (Presc << 5) + (rDivOpt << 6) + ((divider >> 8) & 0x0f);
	/* end set_frequency */
	/* setlpf */
	lpf = (state->max_lpf * 10) / 100;
	if (lpf < 5)
	{
		lpf = 5;
	}
	else if (lpf > 36)
	{
		lpf = 36;
	}
	state->regs[RSTV6110_CTRL3] &= (~0x40);  /* Activate the calibration Clock */
	state->regs[RSTV6110_CTRL3] = (state->regs[RSTV6110_CTRL3] & 0xe0) + (lpf - 5);  /* Set the LPF value */
	/* end setlpf */
	state->regs[RSTV6110_CTRL2] = (state->regs[RSTV6110_CTRL2] & 0xf0) + 2 / 2; /* bbgain */
	ret = stv6110_commit(fe);
	dprintk(100, "%s < status = %u\n", __func__, ret);
	return ret;
}

u16 stv6110_tuner_lock_status(struct dvb_frontend *fe)
{
	struct stv6110_state *state = fe->tuner_priv;
	u16                  res;
	u8                   ucTemp;

	dprintk(100, "%s >\n", __func__);
	ucTemp = RSTV6110_STAT1;
	res = state->equipment.demod_i2c_repeater_send(fe->demodulator_priv, &ucTemp, 1);

	if (AVL6222_OK != res)
	{
		return (res);
	}
	res = state->equipment.demod_i2c_repeater_recv(fe->demodulator_priv, &ucTemp, 1);

	if (AVL6222_OK == res)
	{
		if (1 == (ucTemp & 0x01))
		{
			res = AVL6222_OK;
		}
		else
		{
			res = AVL6222_ERROR_GENERIC;
		}
	}
	dprintk(20, "%s: lock status: %u, buf: 0x%X\n", __func__, res, ucTemp);
	return res;
}

u16 stv6110_tuner_init(struct dvb_frontend *fe)
{
	struct stv6110_state *state = fe->tuner_priv;
	u16 ret = 0;
	u8 buf0[] = { 0x07, 0x11, 0xdc, 0x85, 0x17, 0x01, 0xe6, 0x1e };

	dprintk(100, "%s(): >\n", __func__);
	memcpy(state->regs, buf0, 8);
	dprintk(100, "%s < status = %u\n", __func__, ret);
	return ret;
}

int stv6110a_attach(struct dvb_frontend *fe, void *demod_priv, struct equipment_s *equipment, u32 mclk, u32 max_lpf)
{
	struct stv6110_state *state = kmalloc(sizeof(struct stv6110_state), GFP_KERNEL);

	fe->tuner_priv = state;
	state->equipment = *equipment;
	state->mclk = mclk;
	state->max_lpf = max_lpf;

	equipment->tuner_load_fw     = NULL;
	equipment->tuner_init        = stv6110_tuner_init;
	equipment->tuner_lock        = stv6110_tuner_lock;
	equipment->tuner_lock_status = stv6110_tuner_lock_status;
	return 0;
}
// vim:ts=4
