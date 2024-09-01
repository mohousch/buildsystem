/*
 STB0899 Multistandard Frontend driver
 Copyright (C) Manu Abraham (abraham.manu@gmail.com)

 Copyright (C) ST Microelectronics

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>

#include <linux/interrupt.h>

#include <asm/io.h>

#include <linux/dvb/frontend.h>
#include "dvb_frontend.h"

#include "stb0899_drv.h"
#include "stb0899_priv.h"
#include "stb0899_reg.h"

#define ISL6405_SR 0x80

/* SR = 0 */
#define ISL6405_OLF1 0x01
#define ISL6405_EN1 0x02
#define ISL6405_VSEL1 0x04
#define ISL6405_LLC1 0x08
#define ISL6405_ENT1 0x10
#define ISL6405_ISEL1 0x20
#define ISL6405_DCL 0x40

/* SR = 1 */
#define ISL6405_OLF2 0x01
#define ISL6405_OTF 0x02
#define ISL6405_EN2 0x04
#define ISL6405_VSEL2 0x08
#define ISL6405_LLC2 0x10
#define ISL6405_ENT2 0x20
#define ISL6405_ISEL2 0x40

static struct stpio_pin *pin_tx_diseq;
static struct stpio_pin *pin_rx_diseq;

static unsigned int verbose = 0;

static unsigned char isl6405_init_sr1 = 0;//0
static unsigned char isl6405_vol_sr1 = 0; //0
static unsigned char isl6405_tone_sr1 = 0; //0

static unsigned char bska_init = 0;

/* C/N in dB/10, NIRM/NIRL */
static const struct stb0899_tab stb0899_cn_tab[] =
{
	{ 200, 2600 },
	{ 190, 2700 },
	{ 180, 2860 },
	{ 170, 3020 },
	{ 160, 3210 },
	{ 150, 3440 },
	{ 140, 3710 },
	{ 130, 4010 },
	{ 120, 4360 },
	{ 110, 4740 },
	{ 100, 5190 },
	{ 90, 5670 },
	{ 80, 6200 },
	{ 70, 6770 },
	{ 60, 7360 },
	{ 50, 7970 },
	{ 40, 8250 },
	{ 30, 9000 },
	{ 20, 9450 },
	{ 15, 9600 },
};

/* DVB-S AGCIQ_VALUE vs. signal level in dBm/10.
 * As measured, connected to a modulator.
 * -8.0 to -50.0 dBm directly connected,
 * -52.0 to -74.8 with extra attenuation.
 * Cut-off to AGCIQ_VALUE = 0x80 below -74.8dBm.
 * Crude linear extrapolation below -84.8dBm and above -8.0dBm.
 */
static const struct stb0899_tab stb0899_dvbsrf_tab[] =
{
	{ -950, -128 },
	{ -748, -94 },
	{ -745, -92 },
	{ -735, -90 },
	{ -720, -87 },
	{ -670, -77 },
	{ -640, -70 },
	{ -610, -62 },
	{ -600, -60 },
	{ -590, -56 },
	{ -560, -41 },
	{ -540, -25 },
	{ -530, -17 },
	{ -520, -11 },
	{ -500, 1 },
	{ -490, 6 },
	{ -480, 10 },
	{ -440, 22 },
	{ -420, 27 },
	{ -400, 31 },
	{ -380, 34 },
	{ -340, 40 },
	{ -320, 43 },
	{ -280, 48 },
	{ -250, 52 },
	{ -230, 55 },
	{ -180, 61 },
	{ -140, 66 },
	{ -90, 73 },
	{ -80, 74 },
	{ 500, 127 }
};

/* DVB-S2 IF_AGC_GAIN vs. signal level in dBm/10.
 * As measured, connected to a modulator.
 * -8.0 to -50.1 dBm directly connected,
 * -53.0 to -76.6 with extra attenuation.
 * Cut-off to IF_AGC_GAIN = 0x3fff below -76.6dBm.
 * Crude linear extrapolation below -76.6dBm and above -8.0dBm.
 */
static const struct stb0899_tab stb0899_dvbs2rf_tab[] =
{
	{ 700, 0 },
	{ -80, 3217 },
	{ -150, 3893 },
	{ -190, 4217 },
	{ -240, 4621 },
	{ -280, 4945 },
	{ -320, 5273 },
	{ -350, 5545 },
	{ -370, 5741 },
	{ -410, 6147 },
	{ -450, 6671 },
	{ -490, 7413 },
	{ -501, 7665 },
	{ -530, 8767 },
	{ -560, 10219 },
	{ -580, 10939 },
	{ -590, 11518 },
	{ -600, 11723 },
	{ -650, 12659 },
	{ -690, 13219 },
	{ -730, 13645 },
	{ -750, 13909 },
	{ -766, 14153 },
	{ -999, 16383 }
};

/* DVB-S2 Es/N0 quant in dB/100 vs read value * 100*/
static struct stb0899_tab stb0899_quant_tab[] =
{
	{ 0, 0 },
	{ 0, 100 },
	{ 600, 200 },
	{ 950, 299 },
	{ 1200, 398 },
	{ 1400, 501 },
	{ 1560, 603 },
	{ 1690, 700 },
	{ 1810, 804 },
	{ 1910, 902 },
	{ 2000, 1000 },
	{ 2080, 1096 },
	{ 2160, 1202 },
	{ 2230, 1303 },
	{ 2350, 1496 },
	{ 2410, 1603 },
	{ 2460, 1698 },
	{ 2510, 1799 },
	{ 2600, 1995 },
	{ 2650, 2113 },
	{ 2690, 2213 },
	{ 2720, 2291 },
	{ 2760, 2399 },
	{ 2800, 2512 },
	{ 2860, 2692 },
	{ 2930, 2917 },
	{ 2960, 3020 },
	{ 3010, 3199 },
	{ 3040, 3311 },
	{ 3060, 3388 },
	{ 3120, 3631 },
	{ 3190, 3936 },
	{ 3400, 5012 },
	{ 3610, 6383 },
	{ 3800, 7943 },
	{ 4210, 12735 },
	{ 4500, 17783 },
	{ 4690, 22131 },
	{ 4810, 25410 }
};

/* DVB-S2 Es/N0 estimate in dB/100 vs read value */
static struct stb0899_tab stb0899_est_tab[] =
{
	{ 0, 0 },
	{ 0, 1 },
	{ 301, 2 },
	{ 1204, 16 },
	{ 1806, 64 },
	{ 2408, 256 },
	{ 2709, 512 },
	{ 3010, 1023 },
	{ 3311, 2046 },
	{ 3612, 4093 },
	{ 3823, 6653 },
	{ 3913, 8185 },
	{ 4010, 10233 },
	{ 4107, 12794 },
	{ 4214, 16368 },
	{ 4266, 18450 },
	{ 4311, 20464 },
	{ 4353, 22542 },
	{ 4391, 24604 },
	{ 4425, 26607 },
	{ 4457, 28642 },
	{ 4487, 30690 },
	{ 4515, 32734 },
	{ 4612, 40926 },
	{ 4692, 49204 },
	{ 4816, 65464 },
	{ 4913, 81846 },
	{ 4993, 98401 },
	{ 5060, 114815 },
	{ 5118, 131220 },
	{ 5200, 158489 },
	{ 5300, 199526 },
	{ 5400, 251189 },
	{ 5500, 316228 },
	{ 5600, 398107 },
	{ 5720, 524807 },
	{ 5721, 526017 },
};

/*
* diseqc pwm by plfreebox@gmail.com
*/
#define DEBUG_DISEQC_PWM

#ifdef DEBUG_DISEQC_PWM
#define debug_diseqc_pwm(args...) printk(args)
#else
#define debug_diseqc_pwm(args...)
#endif

unsigned long pwm_registers;

#define PWM0_VAL ( pwm_registers + 0x00 )
#define PWM1_VAL ( pwm_registers + 0x04 )
#define PWM0_CPT_VAL ( pwm_registers + 0x10 )
#define PWM1_CPT_VAL ( pwm_registers + 0x14 )
#define PWM0_CMP_VAL ( pwm_registers + 0x20 )
#define PWM1_CMP_VAL ( pwm_registers + 0x24 )
#define PWM0_CPT_EDGE ( pwm_registers + 0x30 )
#define PWM1_CPT_EDGE ( pwm_registers + 0x34 )
#define PWM0_CMP_OUT_VAL ( pwm_registers + 0x40 )
#define PWM1_CMP_OUT_VAL ( pwm_registers + 0x44 )
#define PWM_CTRL ( pwm_registers + 0x50 )
#define PWM_INT_EN ( pwm_registers + 0x54 )
#define PWM_INT_STA ( pwm_registers + 0x58 )
#define PWM_INT_ACK ( pwm_registers + 0x5C )
#define PWM_CNT PWM ( pwm_registers + 0x60 )
#define PWM_CPT_CMP_CNT ( pwm_registers + 0x64 )

struct stpio_pin *pin_tx_diseqc1;
struct stpio_pin *pin_tx_diseqc2;

static volatile unsigned char pwm_diseqc_buf1[200];
static volatile unsigned char pwm_diseqc_buf1_len = 0;
static volatile unsigned char pwm_diseqc_buf1_pos = 0;

static volatile unsigned char pwm_diseqc_buf2[200];
static volatile unsigned char pwm_diseqc_buf2_len = 0;
static volatile unsigned char pwm_diseqc_buf2_pos = 0;

static irqreturn_t pwm_diseqc_irq(int irq, void *dev_id)
{
	writel(0x001, PWM_INT_ACK);
	if (pwm_diseqc_buf1_len == 0) stpio_set_pin(pin_tx_diseqc1, 0);
	if (pwm_diseqc_buf2_len == 0) stpio_set_pin(pin_tx_diseqc2, 0);
	if ((pwm_diseqc_buf1_len == 0) && (pwm_diseqc_buf2_len == 0))
	{
		writel(0x000, PWM_INT_EN);
		return IRQ_HANDLED;
	}
	if (pwm_diseqc_buf1_len > 0)
	{
		if (pwm_diseqc_buf1[pwm_diseqc_buf1_pos] == 1)
			stpio_set_pin(pin_tx_diseqc1, 1);
		else
			stpio_set_pin(pin_tx_diseqc1, 0);
		pwm_diseqc_buf1_pos = pwm_diseqc_buf1_pos + 1;
		pwm_diseqc_buf1_len = pwm_diseqc_buf1_len - 1;
	}
	if (pwm_diseqc_buf2_len > 0)
	{
		if (pwm_diseqc_buf2[pwm_diseqc_buf2_pos] == 1)
			stpio_set_pin(pin_tx_diseqc2, 1);
		else
			stpio_set_pin(pin_tx_diseqc2, 0);
		pwm_diseqc_buf2_pos = pwm_diseqc_buf2_pos + 1;
		pwm_diseqc_buf2_len = pwm_diseqc_buf2_len - 1;
	}
	return IRQ_HANDLED;
}

static int pwm_wait_diseqc1_idle(int timeout)
{
	unsigned long start = jiffies;
	int status;
	while (1)
	{
		if (pwm_diseqc_buf1_len == 0) break;
		if (jiffies - start > timeout)
		{
			debug_diseqc_pwm("%s: timeout!!\n", __FUNCTION__);
			return -ETIMEDOUT;
		}
		msleep(10);
	};
	return 0;
}

static int pwm_send_diseqc1_burst(struct dvb_frontend *fe, fe_sec_mini_cmd_t burst)
{
	int i, j;
	if (pwm_wait_diseqc1_idle(100) < 0)
		return -ETIMEDOUT;
	pwm_diseqc_buf1_pos = 1;
	pwm_diseqc_buf1_len = 0;
	//dodanie pustego przrwania dla wyrownia czasu przepelnienia licznika
	pwm_diseqc_buf1_len = pwm_diseqc_buf1_len + 1;
	pwm_diseqc_buf1[pwm_diseqc_buf1_len] = 0;
	switch (burst)
	{
		case SEC_MINI_A:
			debug_diseqc_pwm("%s Tone=A\n", __FUNCTION__);
			for (i = 0; i < 8; i++)
			{
				pwm_diseqc_buf1_len = pwm_diseqc_buf1_len + 1;
				pwm_diseqc_buf1[pwm_diseqc_buf1_len] = 1;
				pwm_diseqc_buf1_len = pwm_diseqc_buf1_len + 1;
				pwm_diseqc_buf1[pwm_diseqc_buf1_len] = 1;
				pwm_diseqc_buf1_len = pwm_diseqc_buf1_len + 1;
				pwm_diseqc_buf1[pwm_diseqc_buf1_len] = 1;
			}
			pwm_diseqc_buf1_len = pwm_diseqc_buf1_len + 1;
			pwm_diseqc_buf1[pwm_diseqc_buf1_len] = 1;
			break;
		case SEC_MINI_B:
			debug_diseqc_pwm("%s Tone=B\n", __FUNCTION__);
			for (i = 0; i < 8; i++)
			{
				pwm_diseqc_buf1_len = pwm_diseqc_buf1_len + 1;
				pwm_diseqc_buf1[pwm_diseqc_buf1_len] = 1;
				pwm_diseqc_buf1_len = pwm_diseqc_buf1_len + 1;
				pwm_diseqc_buf1[pwm_diseqc_buf1_len] = 0;
				pwm_diseqc_buf1_len = pwm_diseqc_buf1_len + 1;
				pwm_diseqc_buf1[pwm_diseqc_buf1_len] = 0;
			}
			pwm_diseqc_buf1_len = pwm_diseqc_buf1_len + 1;
			pwm_diseqc_buf1[pwm_diseqc_buf1_len] = 1;
			break;
	}
	pwm_diseqc_buf1_len = pwm_diseqc_buf1_len + 1;
	pwm_diseqc_buf1[pwm_diseqc_buf1_len] = 0;
	writel(0x001, PWM_INT_EN);
	if (pwm_wait_diseqc1_idle(100) < 0)
		return -ETIMEDOUT;
	return 0;
}

static int pwm_diseqc1_send_msg(struct dvb_frontend *fe,
								struct dvb_diseqc_master_cmd *m)
{
	int i, j;
	debug_diseqc_pwm("%s: msg len %x\n", __FUNCTION__, m->msg_len);
	if (pwm_wait_diseqc1_idle(100) < 0)
		return -ETIMEDOUT;
	pwm_diseqc_buf1_pos = 1;
	pwm_diseqc_buf1_len = 0;
	//dodanie pustego przrwania dla wyrownia czasu przepelnienia licznika
	pwm_diseqc_buf1_len = pwm_diseqc_buf1_len + 1;
	pwm_diseqc_buf1[pwm_diseqc_buf1_len] = 0;
	for (j = 0; j < m->msg_len; j++)
	{
		unsigned char byte = m->msg[j];
		unsigned char parity = 0;
		for (i = 0; i < 8; i++)
		{
			if ((byte & 128) == 128)
			{
				//diseq 1
				parity = parity + 1;
				pwm_diseqc_buf1_len = pwm_diseqc_buf1_len + 1;
				pwm_diseqc_buf1[pwm_diseqc_buf1_len] = 1;
				pwm_diseqc_buf1_len = pwm_diseqc_buf1_len + 1;
				pwm_diseqc_buf1[pwm_diseqc_buf1_len] = 0;
				pwm_diseqc_buf1_len = pwm_diseqc_buf1_len + 1;
				pwm_diseqc_buf1[pwm_diseqc_buf1_len] = 0;
			}
			else
			{
				//diseq 0
				pwm_diseqc_buf1_len = pwm_diseqc_buf1_len + 1;
				pwm_diseqc_buf1[pwm_diseqc_buf1_len] = 1;
				pwm_diseqc_buf1_len = pwm_diseqc_buf1_len + 1;
				pwm_diseqc_buf1[pwm_diseqc_buf1_len] = 1;
				pwm_diseqc_buf1_len = pwm_diseqc_buf1_len + 1;
				pwm_diseqc_buf1[pwm_diseqc_buf1_len] = 0;
			}
			byte = byte << 1;
		}
		if ((parity & 1) == 1)
		{
			//diseq 0
			pwm_diseqc_buf1_len = pwm_diseqc_buf1_len + 1;
			pwm_diseqc_buf1[pwm_diseqc_buf1_len] = 1;
			pwm_diseqc_buf1_len = pwm_diseqc_buf1_len + 1;
			pwm_diseqc_buf1[pwm_diseqc_buf1_len] = 1;
			pwm_diseqc_buf1_len = pwm_diseqc_buf1_len + 1;
			pwm_diseqc_buf1[pwm_diseqc_buf1_len] = 0;
		}
		else
		{
			//diseq 1
			pwm_diseqc_buf1_len = pwm_diseqc_buf1_len + 1;
			pwm_diseqc_buf1[pwm_diseqc_buf1_len] = 1;
			pwm_diseqc_buf1_len = pwm_diseqc_buf1_len + 1;
			pwm_diseqc_buf1[pwm_diseqc_buf1_len] = 0;
			pwm_diseqc_buf1_len = pwm_diseqc_buf1_len + 1;
			pwm_diseqc_buf1[pwm_diseqc_buf1_len] = 0;
		}
	}
	pwm_diseqc_buf1_len = pwm_diseqc_buf1_len + 1;
	pwm_diseqc_buf1[pwm_diseqc_buf1_len] = 0;
	writel(0x001, PWM_INT_EN);
	if (pwm_wait_diseqc1_idle(100) < 0)
		return -ETIMEDOUT;
	return 0;
}

int pwm_diseqc_init(void)
{
	debug_diseqc_pwm("PWM Diseqc Init\n");
	pwm_registers = (unsigned long) ioremap(0x18010000, 0x100);
	pin_tx_diseqc1 = stpio_request_pin(5, 5, "pin_tx_diseqc1", STPIO_OUT);
	if (pin_tx_diseqc1 == NULL)
	{
		debug_diseqc_pwm("FAIL : request pin 5 5\n");
		goto err;
	}
	stpio_set_pin(pin_tx_diseqc1, 0);
	pin_tx_diseqc2 = stpio_request_pin(2, 5, "pin_tx_diseqc2", STPIO_OUT);
	if (pin_tx_diseqc2 == NULL)
	{
		debug_diseqc_pwm("FAIL : request pin 2 5\n");
		goto err;
	}
	stpio_set_pin(pin_tx_diseqc2, 0);
	if (request_irq(126, pwm_diseqc_irq, IRQF_DISABLED, "timer_pwm", NULL))
	{
		debug_diseqc_pwm("FAIL : request irq pwm\n");
		goto err;
	}
	//500us=2000hz
	//27000000/2000=/256=52
	//100us=10000hz
	//27000000/10000=2700=10
	//reg=52;//500us
	//reg=10;//100us
	//reg=52;
	//reg=(reg&0x0f)+((reg&0xf0)<<(11-4))+PWM_CTRL_PWM_EN;
	//debug("reg div = 0x%x\n", reg);
	writel(0x1a04, PWM_CTRL); //generowanie przerwania co 500us
	writel(0x000, PWM0_VAL);
	writel(0x000, PWM_INT_EN);
	pwm_diseqc_buf1_pos = 1;
	pwm_diseqc_buf1_len = 0;
	pwm_diseqc_buf2_pos = 1;
	pwm_diseqc_buf2_len = 0;
	debug_diseqc_pwm("PWM Diseqc Init : OK\n");
	return 0;
err:
	iounmap(pwm_registers);
	return -ENODEV;
}

/*
* diseqc sti7100
*/

static unsigned long diseqc_registers;
static volatile int diseqc_int_flag;

#define DSQ_TX_EN ( diseqc_registers + 0x00 )
#define DSQ_TX_MSG_CFG ( diseqc_registers + 0x04 )/* 00 [2:8] */
#define DSQ_TX_PRESCALER ( diseqc_registers + 0x08 )/* 2 */
#define DSQ_TX_SUBCARR_DIV ( diseqc_registers + 0x0C )/* 1511 */
#define DSQ_TX_SILENCE_PER ( diseqc_registers + 0x10 )/* ?? 1000 */
#define DSQ_TX_DATA_BUF ( diseqc_registers + 0x14 )
#define DSQ_TX_SYMBOL_PER ( diseqc_registers + 0x18 )/* 33 */
#define DSQ_TX_SYMBOL0_ONTIME ( diseqc_registers + 0x1c )/* 11 */
#define DSQ_TX_SYMBOL1_ONTIME ( diseqc_registers + 0x20 )/* 11 */
#define DSQ_TX_SW_RST ( diseqc_registers + 0x24 )
#define DSQ_TX_START ( diseqc_registers + 0x28 )

#define DSQ_TX_INT_EN ( diseqc_registers + 0x2c )
#define DSQ_TX_INT_STA ( diseqc_registers + 0x30 )
#define DSQ_TX_STA ( diseqc_registers + 0x34 )
#define DSQ_TX_CLR_INT_STA ( diseqc_registers + 0x38 )
#define DSQ_TX_AGC ( diseqc_registers + 0x40 )

static int stm_wait_diseqc_idle(int timeout)
{
	unsigned long start = jiffies;
	int status;
	////printk ("%s\n", __FUNCTION__);
	while ((status = readl(DSQ_TX_STA)) & 0x8)
	{
		if (jiffies - start > timeout)
		{
			printk("%s: timeout!!\n", __FUNCTION__);
			return -ETIMEDOUT;
		}
		msleep(10);
	};
	return 0;
}

/*
 SEC_MINI_A = 0x00 = Unmodulated
 SEC_MINI_B = 0xff = Modulated
*/

static int stm_send_diseqc_burst(struct dvb_frontend *fe, fe_sec_mini_cmd_t burst)
{
	printk("%s\n", __FUNCTION__);
	if (stm_wait_diseqc_idle(100) < 0)
		return -ETIMEDOUT;
	diseqc_int_flag = 0;
	switch (burst)
	{
		//bit 0 type_burst_tone
		//bit 1 modulated tone
		//bit 9-15 tone_len
		case SEC_MINI_A:
			printk("!!!!!!! A !!!!!!\n");
			writel(0x01, DSQ_TX_MSG_CFG);
			writel(0x01 | (0x09 << 9), DSQ_TX_MSG_CFG);
			break;
		case SEC_MINI_B:
			printk("!!!!!!! B !!!!!!\n");
			writel(0x03, DSQ_TX_MSG_CFG);
			writel(0x03 | (0x09 << 9), DSQ_TX_MSG_CFG);
			break;
	}
	writel(1, DSQ_TX_EN);
	writel(0x9, DSQ_TX_CLR_INT_STA);
	writel(1, DSQ_TX_START);
	writel(0x9, DSQ_TX_INT_EN);
	while (!diseqc_int_flag)
	{
		printk("%s: waiting for interrupt\n", __FUNCTION__);
		msleep(10);
	}
	writel(0, DSQ_TX_EN);
	return 0;
}

static int stm_diseqc_send_msg(struct dvb_frontend *fe,
							   struct dvb_diseqc_master_cmd *m)
{
	int n, i, j;
	int buffer;
	printk("%s: entered %x\n", __FUNCTION__, m->msg_len);
	writel(22, DSQ_TX_SILENCE_PER);
	writel(0, DSQ_TX_MSG_CFG);
	writel((m->msg_len) << 2, DSQ_TX_MSG_CFG);
	diseqc_int_flag = 0;
	for (n = 0; n < m->msg_len; n += 4)
	{
		buffer = 0;
		for (j = 0; j < 4; j++)
		{
			unsigned char c = m->msg[j + n];
			unsigned char d = 0;
			for (i = 0; i < 8; i++)
			{
				if (i > 4)
					d |= (c & (1 << i)) >> (2 * i - 7);
				else
					d |= (c & (1 << i)) << (7 - 2 * i);
			}
			buffer |= d << (j * 8);
		}
		writel(buffer, DSQ_TX_DATA_BUF);
	}
	writel(1, DSQ_TX_EN);
	writel(0x9, DSQ_TX_CLR_INT_STA);
	writel(1, DSQ_TX_START);
	writel(0x9, DSQ_TX_INT_EN);
	while (!diseqc_int_flag)
	{
		printk("%s: waiting for interrupt\n", __FUNCTION__);
		msleep(10);
	}
	writel(0, DSQ_TX_EN);
	return 0;
}

static irqreturn_t stm_diseqc_irq(int irq, void *dev_id, struct pt_regs *regs)
{
	unsigned int status;
	status = readl(DSQ_TX_INT_STA);
	printk("%s: %x\n", __FUNCTION__, status);
	/* TX Ready */
	if (status & 0x8)
	{
		writel(0x8, DSQ_TX_CLR_INT_STA);
		writel(readl(DSQ_TX_INT_EN) & ~ 0x8, DSQ_TX_INT_EN);
		diseqc_int_flag = 1;
	}
	return IRQ_HANDLED;
}

#define CLOCKGEN_PLL1_CFG (0xb9213000 + 0x24)
#define CONFIG_SH_EXTERNAL_CLOCK 27000000

static int get_pll_freq(unsigned long addr)
{
	unsigned long freq, data, ndiv, pdiv, mdiv;
	data = readl(addr);
	mdiv = (data >> 0) & 0xff;
	ndiv = (data >> 8) & 0xff;
	pdiv = (data >> 16) & 0x7;
	freq = (((2 * (CONFIG_SH_EXTERNAL_CLOCK / 1000) * ndiv) / mdiv) /
			(1 << pdiv)) * 1000;
	return freq;
}

/* 0x18068000 */
int stm_diseqc_init(void)
{
	int f;
	diseqc_registers = ioremap(0x18068000, 0x100);
	if (request_irq(0x81, stm_diseqc_irq, IRQF_DISABLED, "Diseqc", NULL)) goto err1;
	stpio_request_pin(5, 5, "DiseqC", STPIO_ALT_OUT);
	f = get_pll_freq(CLOCKGEN_PLL1_CFG);
	f = ((f / 4) / 44000) / 2;
	writel(2, DSQ_TX_PRESCALER);
	writel(f, DSQ_TX_SUBCARR_DIV);
	writel(33, DSQ_TX_SYMBOL_PER);
	writel(22, DSQ_TX_SYMBOL0_ONTIME);
	writel(11, DSQ_TX_SYMBOL1_ONTIME);
	writel(0, DSQ_TX_SW_RST);
	writel(0, DSQ_TX_EN);
	writel(0, DSQ_TX_INT_EN);
	return 0;
err1:
	iounmap(diseqc_registers);
	return -ENODEV;
}

static int _stb0899_read_reg(struct stb0899_state *state, unsigned int reg)
{
	int ret;
	u8 b0[] = { reg >> 8, reg & 0xff };
	u8 buf;
	struct i2c_msg msg[] =
	{
		{
			.addr = state->config->demod_address,
			.flags = 0,
			.buf = b0,
			.len = 2
		}, {
			.addr = state->config->demod_address,
			.flags = I2C_M_RD,
			.buf = &buf,
			.len = 1
		}
	};
	ret = i2c_transfer(state->i2c, msg, 2);
	if (ret != 2)
	{
		if (ret != -ERESTARTSYS)
			dprintk(state->verbose, FE_ERROR, 1,
					"Read error, Reg=[0x%02x], Status=%d",
					reg, ret);
		return ret < 0 ? ret : -EREMOTEIO;
	}
	if (unlikely(*state->verbose >= FE_DEBUGREG))
		dprintk(state->verbose, FE_ERROR, 1, "Reg=[0x%02x], data=%02x",
				reg, buf);
	return (unsigned int)buf;
}

int stb0899_read_reg(struct stb0899_state *state, unsigned int reg)
{
	int result;
	result = _stb0899_read_reg(state, reg);
	/*
	 * Bug ID 9:
	 * access to 0xf2xx/0xf6xx
	 * must be followed by read from 0xf2ff/0xf6ff.
	 */
	if ((reg != 0xf2ff) && (reg != 0xf6ff) &&
			(((reg & 0xff00) == 0xf200) || ((reg & 0xff00) == 0xf600)))
		_stb0899_read_reg(state, (reg | 0x00ff));
	return result;
}

u32 _stb0899_read_s2reg(struct stb0899_state *state,
						u32 stb0899_i2cdev,
						u32 stb0899_base_addr,
						u16 stb0899_reg_offset)
{
	int status;
	u32 data;
	u8 buf[7] = { 0 };
	u16 tmpaddr;
	u8 buf_0[] =
	{
		GETBYTE(stb0899_i2cdev, BYTE1), /* 0xf3 S2 Base Address (MSB) */
		GETBYTE(stb0899_i2cdev, BYTE0), /* 0xfc S2 Base Address (LSB) */
		GETBYTE(stb0899_base_addr, BYTE0), /* 0x00 Base Address (LSB) */
		GETBYTE(stb0899_base_addr, BYTE1), /* 0x04 Base Address (LSB) */
		GETBYTE(stb0899_base_addr, BYTE2), /* 0x00 Base Address (MSB) */
		GETBYTE(stb0899_base_addr, BYTE3), /* 0x00 Base Address (MSB) */
	};
	u8 buf_1[] =
	{
		0x00, /* 0xf3 Reg Offset */
		0x00, /* 0x44 Reg Offset */
	};
	struct i2c_msg msg_0 =
	{
		.addr = state->config->demod_address,
		.flags = 0,
		.buf = buf_0,
		.len = 6
	};
	struct i2c_msg msg_1 =
	{
		.addr = state->config->demod_address,
		.flags = 0,
		.buf = buf_1,
		.len = 2
	};
	struct i2c_msg msg_r =
	{
		.addr = state->config->demod_address,
		.flags = I2C_M_RD,
		.buf = buf,
		.len = 4
	};
	tmpaddr = stb0899_reg_offset & 0xff00;
	if (!(stb0899_reg_offset & 0x8))
		tmpaddr = stb0899_reg_offset | 0x20;
	buf_1[0] = GETBYTE(tmpaddr, BYTE1);
	buf_1[1] = GETBYTE(tmpaddr, BYTE0);
	status = i2c_transfer(state->i2c, &msg_0, 1);
	if (status < 1)
	{
		if (status != -ERESTARTSYS)
			printk(KERN_ERR "%s ERR(1), Device=[0x%04x], Base address=[0x%08x], Offset=[0x%04x], Status=%d\n",
				   __func__, stb0899_i2cdev, stb0899_base_addr, stb0899_reg_offset, status);
		goto err;
	}
	/* Dummy */
	status = i2c_transfer(state->i2c, &msg_1, 1);
	if (status < 1)
		goto err;
	status = i2c_transfer(state->i2c, &msg_r, 1);
	if (status < 1)
		goto err;
	buf_1[0] = GETBYTE(stb0899_reg_offset, BYTE1);
	buf_1[1] = GETBYTE(stb0899_reg_offset, BYTE0);
	/* Actual */
	status = i2c_transfer(state->i2c, &msg_1, 1);
	if (status < 1)
	{
		if (status != -ERESTARTSYS)
			printk(KERN_ERR "%s ERR(2), Device=[0x%04x], Base address=[0x%08x], Offset=[0x%04x], Status=%d\n",
				   __func__, stb0899_i2cdev, stb0899_base_addr, stb0899_reg_offset, status);
		goto err;
	}
	status = i2c_transfer(state->i2c, &msg_r, 1);
	if (status < 1)
	{
		if (status != -ERESTARTSYS)
			printk(KERN_ERR "%s ERR(3), Device=[0x%04x], Base address=[0x%08x], Offset=[0x%04x], Status=%d\n",
				   __func__, stb0899_i2cdev, stb0899_base_addr, stb0899_reg_offset, status);
		return status < 0 ? status : -EREMOTEIO;
	}
	data = MAKEWORD32(buf[3], buf[2], buf[1], buf[0]);
	if (unlikely(*state->verbose >= FE_DEBUGREG))
		printk(KERN_DEBUG "%s Device=[0x%04x], Base address=[0x%08x], Offset=[0x%04x], Data=[0x%08x]\n",
			   __func__, stb0899_i2cdev, stb0899_base_addr, stb0899_reg_offset, data);
	return data;
err:
	return status < 0 ? status : -EREMOTEIO;
}

int stb0899_write_s2reg(struct stb0899_state *state,
						u32 stb0899_i2cdev,
						u32 stb0899_base_addr,
						u16 stb0899_reg_offset,
						u32 stb0899_data)
{
	int status;
	/* Base Address Setup */
	u8 buf_0[] =
	{
		GETBYTE(stb0899_i2cdev, BYTE1), /* 0xf3 S2 Base Address (MSB) */
		GETBYTE(stb0899_i2cdev, BYTE0), /* 0xfc S2 Base Address (LSB) */
		GETBYTE(stb0899_base_addr, BYTE0), /* 0x00 Base Address (LSB) */
		GETBYTE(stb0899_base_addr, BYTE1), /* 0x04 Base Address (LSB) */
		GETBYTE(stb0899_base_addr, BYTE2), /* 0x00 Base Address (MSB) */
		GETBYTE(stb0899_base_addr, BYTE3), /* 0x00 Base Address (MSB) */
	};
	u8 buf_1[] =
	{
		0x00, /* 0xf3 Reg Offset */
		0x00, /* 0x44 Reg Offset */
		0x00, /* data */
		0x00, /* data */
		0x00, /* data */
		0x00, /* data */
	};
	struct i2c_msg msg_0 =
	{
		.addr = state->config->demod_address,
		.flags = 0,
		.buf = buf_0,
		.len = 6
	};
	struct i2c_msg msg_1 =
	{
		.addr = state->config->demod_address,
		.flags = 0,
		.buf = buf_1,
		.len = 6
	};
	buf_1[0] = GETBYTE(stb0899_reg_offset, BYTE1);
	buf_1[1] = GETBYTE(stb0899_reg_offset, BYTE0);
	buf_1[2] = GETBYTE(stb0899_data, BYTE0);
	buf_1[3] = GETBYTE(stb0899_data, BYTE1);
	buf_1[4] = GETBYTE(stb0899_data, BYTE2);
	buf_1[5] = GETBYTE(stb0899_data, BYTE3);
	if (unlikely(*state->verbose >= FE_DEBUGREG))
		printk(KERN_DEBUG "%s Device=[0x%04x], Base Address=[0x%08x], Offset=[0x%04x], Data=[0x%08x]\n",
			   __func__, stb0899_i2cdev, stb0899_base_addr, stb0899_reg_offset, stb0899_data);
	status = i2c_transfer(state->i2c, &msg_0, 1);
	if (unlikely(status < 1))
	{
		if (status != -ERESTARTSYS)
			printk(KERN_ERR "%s ERR (1), Device=[0x%04x], Base Address=[0x%08x], Offset=[0x%04x], Data=[0x%08x], status=%d\n",
				   __func__, stb0899_i2cdev, stb0899_base_addr, stb0899_reg_offset, stb0899_data, status);
		goto err;
	}
	status = i2c_transfer(state->i2c, &msg_1, 1);
	if (unlikely(status < 1))
	{
		if (status != -ERESTARTSYS)
			printk(KERN_ERR "%s ERR (2), Device=[0x%04x], Base Address=[0x%08x], Offset=[0x%04x], Data=[0x%08x], status=%d\n",
				   __func__, stb0899_i2cdev, stb0899_base_addr, stb0899_reg_offset, stb0899_data, status);
		return status < 0 ? status : -EREMOTEIO;
	}
	return 0;
err:
	return status < 0 ? status : -EREMOTEIO;
}

int stb0899_read_regs(struct stb0899_state *state, unsigned int reg, u8 *buf, u32 count)
{
	int status;
	u8 b0[] = { reg >> 8, reg & 0xff };
	struct i2c_msg msg[] =
	{
		{
			.addr = state->config->demod_address,
			.flags = 0,
			.buf = b0,
			.len = 2
		}, {
			.addr = state->config->demod_address,
			.flags = I2C_M_RD,
			.buf = buf,
			.len = count
		}
	};
	status = i2c_transfer(state->i2c, msg, 2);
	if (status != 2)
	{
		if (status != -ERESTARTSYS)
			printk(KERN_ERR "%s Read error, Reg=[0x%04x], Count=%u, Status=%d\n",
				   __func__, reg, (u32)count, status);
		goto err;
	}
	/*
	 * Bug ID 9:
	 * access to 0xf2xx/0xf6xx
	 * must be followed by read from 0xf2ff/0xf6ff.
	 */
	if ((reg != 0xf2ff) && (reg != 0xf6ff) &&
			(((reg & 0xff00) == 0xf200) || ((reg & 0xff00) == 0xf600)))
		_stb0899_read_reg(state, (reg | 0x00ff));
	if (unlikely(*state->verbose >= FE_DEBUGREG))
	{
		int i;
		printk(KERN_DEBUG "%s [0x%04x]:", __func__, reg);
		for (i = 0; i < count; i++)
		{
			printk(" %02x", buf[i]);
		}
		printk("\n");
	}
	return 0;
err:
	return status < 0 ? status : -EREMOTEIO;
}

int stb0899_write_regs(struct stb0899_state *state, unsigned int reg, u8 *data, u32 count)
{
	int ret;
	u8 buf[2 + count];
	struct i2c_msg i2c_msg =
	{
		.addr = state->config->demod_address,
		.flags = 0,
		.buf = buf,
		.len = 2 + count
	};
	buf[0] = reg >> 8;
	buf[1] = reg & 0xff;
	memcpy(&buf[2], data, count);
	if (unlikely(*state->verbose >= FE_DEBUGREG))
	{
		int i;
		printk(KERN_DEBUG "%s [0x%04x]:", __func__, reg);
		for (i = 0; i < count; i++)
			printk(" %02x", data[i]);
		printk("\n");
	}
	ret = i2c_transfer(state->i2c, &i2c_msg, 1);
	/*
	 * Bug ID 9:
	 * access to 0xf2xx/0xf6xx
	 * must be followed by read from 0xf2ff/0xf6ff.
	 */
	if ((((reg & 0xff00) == 0xf200) || ((reg & 0xff00) == 0xf600)))
		stb0899_read_reg(state, (reg | 0x00ff));
	if (ret != 1)
	{
		if (ret != -ERESTARTSYS)
			dprintk(state->verbose, FE_ERROR, 1, "Reg=[0x%04x], Data=[0x%02x ...], Count=%u, Status=%d",
					reg, data[0], count, ret);
		return ret < 0 ? ret : -EREMOTEIO;
	}
	return 0;
}

int stb0899_write_reg(struct stb0899_state *state, unsigned int reg, u8 data)
{
	return stb0899_write_regs(state, reg, &data, 1);
}

/*
 * stb0899_get_mclk
 * Get STB0899 master clock frequency
 * ExtClk: external clock frequency (Hz)
 */
static u32 stb0899_get_mclk(struct stb0899_state *state)
{
	u32 mclk = 0, div = 0;
	div = stb0899_read_reg(state, STB0899_NCOARSE);
	mclk = (div + 1) * state->config->xtal_freq / 6;
	dprintk(state->verbose, FE_DEBUG, 1, "div=%d, mclk=%d", div, mclk);
	return mclk;
}

/*
 * stb0899_set_mclk
 * Set STB0899 master Clock frequency
 * Mclk: demodulator master clock
 * ExtClk: external clock frequency (Hz)
 */
static void stb0899_set_mclk(struct stb0899_state *state, u32 Mclk)
{
	struct stb0899_internal *internal = &state->internal;
	u8 mdiv = 0;
	dprintk(state->verbose, FE_DEBUG, 1, "state->config=%p", state->config);
	mdiv = ((6 * Mclk) / state->config->xtal_freq) - 1;
	dprintk(state->verbose, FE_DEBUG, 1, "mdiv=%d", mdiv);
	stb0899_write_reg(state, STB0899_NCOARSE, mdiv);
	internal->master_clk = stb0899_get_mclk(state);
	dprintk(state->verbose, FE_DEBUG, 1, "MasterCLOCK=%d", internal->master_clk);
}

static int stb0899_postproc(struct stb0899_state *state, u8 ctl, int enable)
{
	struct stb0899_config *config = state->config;
	const struct stb0899_postproc *postproc = config->postproc;
	/* post process event */
	if (postproc)
	{
		if (postproc[ctl].gpio == 0)
		{
			return 0;
		}
		if (enable)
		{
			if (postproc[ctl].level == STB0899_GPIOPULLUP)
				stb0899_write_reg(state, postproc[ctl].gpio, 0x02);
			else
				stb0899_write_reg(state, postproc[ctl].gpio, 0x82);
		}
		else
		{
			if (postproc[ctl].level == STB0899_GPIOPULLUP)
				stb0899_write_reg(state, postproc[ctl].gpio, 0x82);
			else
				stb0899_write_reg(state, postproc[ctl].gpio, 0x02);
		}
	}
	return 0;
}

static void stb0899_release(struct dvb_frontend *fe)
{
	struct stb0899_state *state = fe->demodulator_priv;
	dprintk(state->verbose, FE_DEBUG, 1, "Release Frontend");
	/* post process event */
	stb0899_postproc(state, STB0899_POSTPROC_GPIO_POWER, 0);
	kfree(state);
}

/*
 * stb0899_get_alpha
 * return: rolloff
 */
static int stb0899_get_alpha(struct stb0899_state *state)
{
	u8 mode_coeff;
	mode_coeff = stb0899_read_reg(state, STB0899_DEMOD);
	if (STB0899_GETFIELD(MODECOEFF, mode_coeff) == 1)
		return 20;
	else
		return 35;
}

/*
 * stb0899_init_calc
 */
static void stb0899_init_calc(struct stb0899_state *state)
{
	struct stb0899_internal *internal = &state->internal;
	int master_clk;
	u8 agc[2];
	u8 agc1cn;
	u32 reg;
	/* Read registers (in burst mode) */
	agc1cn = stb0899_read_reg(state, STB0899_AGC1CN);
	stb0899_read_regs(state, STB0899_AGC1REF, agc, 2); /* AGC1R and AGC2O */
	/* Initial calculations */
	master_clk = stb0899_get_mclk(state);
	internal->t_agc1 = 0;
	internal->t_agc2 = 0;
	internal->master_clk = master_clk;
	internal->mclk = master_clk / 65536L;
	internal->rolloff = stb0899_get_alpha(state);
	/* DVBS2 Initial calculations */
	/* Set AGC value to the middle */
	internal->agc_gain = 8154;
	reg = STB0899_READ_S2REG(STB0899_S2DEMOD, IF_AGC_CNTRL);
	STB0899_SETFIELD_VAL(IF_GAIN_INIT, reg, internal->agc_gain);
	stb0899_write_s2reg(state, STB0899_S2DEMOD, STB0899_BASE_IF_AGC_CNTRL, STB0899_OFF0_IF_AGC_CNTRL, reg);
	reg = STB0899_READ_S2REG(STB0899_S2DEMOD, RRC_ALPHA);
	internal->rrc_alpha = STB0899_GETFIELD(RRC_ALPHA, reg);
	internal->center_freq = 0;
	internal->av_frame_coarse = 10;
	internal->av_frame_fine = 20;
	internal->step_size = 2;
	/*
	 if ((pParams->SpectralInv == FE_IQ_NORMAL) || (pParams->SpectralInv == FE_IQ_AUTO))
	 pParams->IQLocked = 0;
	 else
	 pParams->IQLocked = 1;
	*/
}

static int stb0899_wait_diseqc_fifo_empty(struct stb0899_state *state, int timeout)
{
	u8 reg = 0;
	unsigned long start = jiffies;
	while (1)
	{
		reg = stb0899_read_reg(state, STB0899_DISSTATUS);
		if (!STB0899_GETFIELD(FIFOFULL, reg))
			break;
		if ((jiffies - start) > timeout)
		{
			dprintk(state->verbose, FE_ERROR, 1, "timed out !!");
			return -ETIMEDOUT;
		}
	}
	return 0;
}

static int stb0899_send_diseqc_msg(struct dvb_frontend *fe, struct dvb_diseqc_master_cmd *cmd)
{
	struct stb0899_state *state = fe->demodulator_priv;
	u8 reg, i;
	if (cmd->msg_len > 8)
		return -EINVAL;
	stm_diseqc_send_msg(fe, cmd);
	//soft_diseqc_send_msg (fe,cmd);
	/*
	 // enable FIFO precharge
	 reg = stb0899_read_reg(state, STB0899_DISCNTRL1);
	 STB0899_SETFIELD_VAL(DISPRECHARGE, reg, 1);
	 stb0899_write_reg(state, STB0899_DISCNTRL1, reg);
	 for (i = 0; i < cmd->msg_len; i++) {
	 // wait for FIFO empty
	 if (stb0899_wait_diseqc_fifo_empty(state, 10) < 0)
	 return -ETIMEDOUT;

	 stb0899_write_reg(state, STB0899_DISFIFO, cmd->msg[i]);
	 }
	 reg = stb0899_read_reg(state, STB0899_DISCNTRL1);
	 STB0899_SETFIELD_VAL(DISPRECHARGE, reg, 0);
	 stb0899_write_reg(state, STB0899_DISCNTRL1, reg);
	*/
	return 0;
}

static int stb0899_wait_diseqc_rxidle(struct stb0899_state *state, int timeout)
{
	u8 reg = 0;
	unsigned long start = jiffies;
	while (!STB0899_GETFIELD(RXEND, reg))
	{
		reg = stb0899_read_reg(state, STB0899_DISRX_ST0);
		if (jiffies - start > timeout)
		{
			dprintk(state->verbose, FE_ERROR, 1, "timed out!!");
			return -ETIMEDOUT;
		}
		msleep(10);
	}
	return 0;
}

static int stb0899_recv_slave_reply(struct dvb_frontend *fe, struct dvb_diseqc_slave_reply *reply)
{
	struct stb0899_state *state = fe->demodulator_priv;
	u8 reg, length = 0, i;
	int result;
	if (stb0899_wait_diseqc_rxidle(state, 100) < 0)
		return -ETIMEDOUT;
	reg = stb0899_read_reg(state, STB0899_DISRX_ST0);
	if (STB0899_GETFIELD(RXEND, reg))
	{
		reg = stb0899_read_reg(state, STB0899_DISRX_ST1);
		length = STB0899_GETFIELD(FIFOBYTENBR, reg);
		if (length > sizeof(reply->msg))
		{
			result = -EOVERFLOW;
			goto exit;
		}
		reply->msg_len = length;
		/* extract data */
		for (i = 0; i < length; i++)
			reply->msg[i] = stb0899_read_reg(state, STB0899_DISFIFO);
	}
	return 0;
exit:
	return result;
}

static int stb0899_wait_diseqc_txidle(struct stb0899_state *state, int timeout)
{
	u8 reg = 0;
	unsigned long start = jiffies;
	while (!STB0899_GETFIELD(TXIDLE, reg))
	{
		reg = stb0899_read_reg(state, STB0899_DISSTATUS);
		if (jiffies - start > timeout)
		{
			dprintk(state->verbose, FE_ERROR, 1, "timed out!!");
			return -ETIMEDOUT;
		}
		msleep(10);
	}
	return 0;
}

static int stb0899_send_diseqc_burst(struct dvb_frontend *fe, fe_sec_mini_cmd_t burst)
{
	struct stb0899_state *state = fe->demodulator_priv;
	u8 reg, old_state;
	stm_send_diseqc_burst(fe, burst);
	/*
	 // wait for diseqc idle
	 if (stb0899_wait_diseqc_txidle(state, 100) < 0)
	 return -ETIMEDOUT;

	 reg = stb0899_read_reg(state, STB0899_DISCNTRL1);
	 old_state = reg;
	 // set to burst mode
	 STB0899_SETFIELD_VAL(DISEQCMODE, reg, 0x03);
	 STB0899_SETFIELD_VAL(DISPRECHARGE, reg, 0x01);
	 stb0899_write_reg(state, STB0899_DISCNTRL1, reg);
	 switch (burst) {
	 case SEC_MINI_A:
	 // unmodulated
	 stb0899_write_reg(state, STB0899_DISFIFO, 0x00);
	 break;
	 case SEC_MINI_B:
	 // modulated
	 stb0899_write_reg(state, STB0899_DISFIFO, 0xff);
	 break;
	 }
	 reg = stb0899_read_reg(state, STB0899_DISCNTRL1);
	 STB0899_SETFIELD_VAL(DISPRECHARGE, reg, 0x00);
	 stb0899_write_reg(state, STB0899_DISCNTRL1, reg);
	 // wait for diseqc idle
	 if (stb0899_wait_diseqc_txidle(state, 100) < 0)
	 return -ETIMEDOUT;

	 // restore state
	 stb0899_write_reg(state, STB0899_DISCNTRL1, old_state);
	*/
	return 0;
}

static int stb0899_diseqc_init(struct stb0899_state *state)
{
	struct dvb_diseqc_master_cmd tx_data;
	u8 f22_tx, f22_rx, reg;
	u32 mclk, tx_freq = 22000;
	tx_data.msg[0] = 0xe2;
	tx_data.msg_len = 3;
	reg = stb0899_read_reg(state, STB0899_DISCNTRL2);
	STB0899_SETFIELD_VAL(ONECHIP_TRX, reg, 0);
	stb0899_write_reg(state, STB0899_DISCNTRL2, reg);
	/* disable Tx spy */
	reg = stb0899_read_reg(state, STB0899_DISCNTRL1);
	STB0899_SETFIELD_VAL(DISEQCRESET, reg, 1);
	stb0899_write_reg(state, STB0899_DISCNTRL1, reg);
	reg = stb0899_read_reg(state, STB0899_DISCNTRL1);
	STB0899_SETFIELD_VAL(DISEQCRESET, reg, 0);
	stb0899_write_reg(state, STB0899_DISCNTRL1, reg);
	mclk = stb0899_get_mclk(state);
	f22_tx = mclk / (tx_freq * 32);
	stb0899_write_reg(state, STB0899_DISF22, f22_tx); /* DiSEqC Tx freq */
	state->rx_freq = 20000;
	f22_rx = mclk / (state->rx_freq * 32);
	return 0;
}

static int stb0899_sleep(struct dvb_frontend *fe)
{
	struct stb0899_state *state = fe->demodulator_priv;
	dprintk(state->verbose, FE_DEBUG, 1, "Going to Sleep .. (Really tired .. :-))");
	/* post process event */
	stb0899_postproc(state, STB0899_POSTPROC_GPIO_POWER, 0);
	return 0;
}

static int stb0899_wakeup(struct dvb_frontend *fe)
{
	int rc;
	struct stb0899_state *state = fe->demodulator_priv;
	if ((rc = stb0899_write_reg(state, STB0899_SYNTCTRL, STB0899_SELOSCI)))
		return rc;
	/* Activate all clocks; DVB-S2 registers are inaccessible otherwise. */
	if ((rc = stb0899_write_reg(state, STB0899_STOPCLK1, 0x00)))
		return rc;
	if ((rc = stb0899_write_reg(state, STB0899_STOPCLK2, 0x00)))
		return rc;
	/* post process event */
	stb0899_postproc(state, STB0899_POSTPROC_GPIO_POWER, 1);
	return 0;
}

static int stb0899_init(struct dvb_frontend *fe)
{
	int i;
	struct stb0899_state *state = fe->demodulator_priv;
	struct stb0899_config *config = state->config;
	int ret;
	u8 b;
	struct i2c_msg msg = {.addr = 0x08, .flags = 0,
			   .buf = &b,
				.len = 1
	};
	dprintk(state->verbose, FE_DEBUG, 1, "Initializing STB0899 ... ");
	//--------------------------------------------------------------------------
	if (bska_init == 0)
	{
		bska_init = 1;
		pin_rx_diseq = stpio_request_pin(5, 4, "pin_rx_diseq", STPIO_IN);
		stm_diseqc_init();
		b = 0x10;
		ret = i2c_transfer(state->i2c, &msg, 1);
		if (ret = !1) printk("Error:ISL6423 SR1\n");
		b = 0x2c;
		//24 - int22khz-extmod problemy z obsluga przelacznikow
		//2c - ext22khz
		ret = i2c_transfer(state->i2c, &msg, 1);
		if (ret = !1) printk("Error:ISL6423 SR2\n");
		b = 0x5b; //0x49;//4a-635mA 4b-800mA 5b-dynamic_limit
		//0x59 - 515mA + dynamic limit
		ret = i2c_transfer(state->i2c, &msg, 1);
		if (ret = !1) printk("Error:ISL6423 SR3\n");
	}
	//--------------------------------------------------------------------------
//	mutex_init(&state->search_lock);
	/* init device */
	dprintk(state->verbose, FE_DEBUG, 1, "init device");
	for (i = 0; config->init_dev[i].address != 0xffff; i++)
		stb0899_write_reg(state, config->init_dev[i].address, config->init_dev[i].data);
	dprintk(state->verbose, FE_DEBUG, 1, "init S2 demod");
	/* init S2 demod */
	for (i = 0; config->init_s2_demod[i].offset != 0xffff; i++)
		stb0899_write_s2reg(state, STB0899_S2DEMOD,
							config->init_s2_demod[i].base_address,
							config->init_s2_demod[i].offset,
							config->init_s2_demod[i].data);
	dprintk(state->verbose, FE_DEBUG, 1, "init S1 demod");
	/* init S1 demod */
	for (i = 0; config->init_s1_demod[i].address != 0xffff; i++)
		stb0899_write_reg(state, config->init_s1_demod[i].address, config->init_s1_demod[i].data);
	dprintk(state->verbose, FE_DEBUG, 1, "init S2 FEC");
	/* init S2 fec */
	for (i = 0; config->init_s2_fec[i].offset != 0xffff; i++)
		stb0899_write_s2reg(state, STB0899_S2FEC,
							config->init_s2_fec[i].base_address,
							config->init_s2_fec[i].offset,
							config->init_s2_fec[i].data);
	dprintk(state->verbose, FE_DEBUG, 1, "init TST");
	/* init test */
	for (i = 0; config->init_tst[i].address != 0xffff; i++)
		stb0899_write_reg(state, config->init_tst[i].address, config->init_tst[i].data);
	stb0899_init_calc(state);
	stb0899_diseqc_init(state);
	/* post process event */
	stb0899_postproc(state, STB0899_POSTPROC_GPIO_POWER, 1);
	return 0;
}

static int stb0899_table_lookup(const struct stb0899_tab *tab, int max, int val)
{
	int res = 0;
	int min = 0, med;
	if (val < tab[min].read)
		res = tab[min].real;
	else if (val >= tab[max].read)
		res = tab[max].real;
	else
	{
		while ((max - min) > 1)
		{
			med = (max + min) / 2;
			if (val >= tab[min].read && val < tab[med].read)
				max = med;
			else
				min = med;
		}
		res = ((val - tab[min].read) *
			   (tab[max].real - tab[min].real) /
			   (tab[max].read - tab[min].read)) +
			  tab[min].real;
	}
	return res;
}

static int stb0899_read_signal_strength(struct dvb_frontend *fe, u16 *strength)
{
	struct stb0899_state *state = fe->demodulator_priv;
	struct stb0899_internal *internal = &state->internal;
	int val;
	u32 reg;
	switch (state->delsys)
	{
		case SYS_DVBS:
		case SYS_DSS:
			if (internal->lock)
			{
				reg = stb0899_read_reg(state, STB0899_VSTATUS);
				if (STB0899_GETFIELD(VSTATUS_LOCKEDVIT, reg))
				{
					reg = stb0899_read_reg(state, STB0899_AGCIQIN);
					val = (s32)(s8)STB0899_GETFIELD(AGCIQVALUE, reg);
					*strength = stb0899_table_lookup(stb0899_dvbsrf_tab, ARRAY_SIZE(stb0899_dvbsrf_tab) - 1, val);
					*strength += 750;
					const int MIN_STRENGTH_DVBS = 0;
					const int MAX_STRENGTH_DVBS = 680;
					if (*strength < MIN_STRENGTH_DVBS)
						*strength = 0;
					else if (*strength > MAX_STRENGTH_DVBS)
						*strength = 0xFFFF;
					else
						*strength = (*strength - MIN_STRENGTH_DVBS) * 0xFFFF / (MAX_STRENGTH_DVBS - MIN_STRENGTH_DVBS);
					dprintk(state->verbose, FE_DEBUG, 1, "AGCIQVALUE = 0x%02x, C = %d * 0.1 dBm",
							val & 0xff, *strength);
				}
			}
			break;
		case SYS_DVBS2:
			if (internal->lock)
			{
//			reg = STB0899_READ_S2REG(STB0899_DEMOD, IF_AGC_GAIN);
				reg = _stb0899_read_s2reg(state, 0xf3fc, 0x00000000, 0xf30c);
				val = STB0899_GETFIELD(IF_AGC_GAIN, reg);
				*strength = stb0899_table_lookup(stb0899_dvbs2rf_tab, ARRAY_SIZE(stb0899_dvbs2rf_tab) - 1, val);
				*strength += 750;
				const int MIN_STRENGTH_DVBS2 = 150;
				const int MAX_STRENGTH_DVBS2 = 600;
				if (*strength < MIN_STRENGTH_DVBS2)
					*strength = 0;
				else if (*strength > MAX_STRENGTH_DVBS2)
					*strength = 0xFFFF;
				else
					*strength = (*strength - MIN_STRENGTH_DVBS2) * 0xFFFF / (MAX_STRENGTH_DVBS2 - MIN_STRENGTH_DVBS2);
			}
			break;
		default:
			dprintk(state->verbose, FE_DEBUG, 1, "Unsupported delivery system");
			return -EINVAL;
	}
	return 0;
}

static int stb0899_read_snr(struct dvb_frontend *fe, u16 *snr)
{
	struct stb0899_state *state = fe->demodulator_priv;
	struct stb0899_internal *internal = &state->internal;
	unsigned int val, quant, quantn = -1, est, estn = -1;
	u8 buf[2];
	u32 reg;
	reg = stb0899_read_reg(state, STB0899_VSTATUS);
	switch (state->delsys)
	{
		case SYS_DVBS:
		case SYS_DSS:
			if (internal->lock)
			{
				if (STB0899_GETFIELD(VSTATUS_LOCKEDVIT, reg))
				{
					stb0899_read_regs(state, STB0899_NIRM, buf, 2);
					val = MAKEWORD16(buf[0], buf[1]);
					*snr = stb0899_table_lookup(stb0899_cn_tab, ARRAY_SIZE(stb0899_cn_tab) - 1, val);
					const int MIN_SNR_DVBS = 0;
					const int MAX_SNR_DVBS = 200;
					if (*snr < MIN_SNR_DVBS)
						*snr = 0;
					else if (*snr > MAX_SNR_DVBS)
						*snr = 0xFFFF;
					else
						*snr = (*snr - MIN_SNR_DVBS) * 0xFFFF / (MAX_SNR_DVBS - MIN_SNR_DVBS);
					dprintk(state->verbose, FE_DEBUG, 1, "NIR = 0x%02x%02x = %u, C/N = %d * 0.1 dBm\n",
							buf[0], buf[1], val, *snr);
				}
			}
			break;
		case SYS_DVBS2:
			if (internal->lock)
			{
				reg = STB0899_READ_S2REG(STB0899_S2DEMOD, UWP_CNTRL1);
				quant = STB0899_GETFIELD(UWP_ESN0_QUANT, reg);
				reg = STB0899_READ_S2REG(STB0899_S2DEMOD, UWP_STAT2);
				est = STB0899_GETFIELD(ESN0_EST, reg);
				if (est == 1)
					val = 301; /* C/N = 30.1 dB */
				else if (est == 2)
					val = 270; /* C/N = 27.0 dB */
				else
				{
					/* quantn = 100 * log(quant^2) */
					quantn = stb0899_table_lookup(stb0899_quant_tab, ARRAY_SIZE(stb0899_quant_tab) - 1, quant * 100);
					/* estn = 100 * log(est) */
					estn = stb0899_table_lookup(stb0899_est_tab, ARRAY_SIZE(stb0899_est_tab) - 1, est);
					/* snr(dBm/10) = -10*(log(est)-log(quant^2)) => snr(dBm/10) = (100*log(quant^2)-100*log(est))/10 */
					val = (quantn - estn) / 10;
				}
				*snr = val;
				const int MIN_SNR_DVBS2 = 79;
				const int MAX_SNR_DVBS2 = 130;
				if (*snr < MIN_SNR_DVBS2)
					*snr = 0;
				else if (*snr > MAX_SNR_DVBS2)
					*snr = 0xFFFF;
				else
					*snr = (*snr - MIN_SNR_DVBS2) * 0xFFFF / (MAX_SNR_DVBS2 - MIN_SNR_DVBS2);
				dprintk(state->verbose, FE_DEBUG, 1, "Es/N0 quant = %d (%d) estimate = %u (%d), C/N = %d * 0.1 dBm",
						quant, quantn, est, estn, val);
			}
			break;
		default:
			dprintk(state->verbose, FE_DEBUG, 1, "Unsupported delivery system");
			return -EINVAL;
	}
	return 0;
}

static int stb0899_read_status(struct dvb_frontend *fe, enum fe_status *status)
{
	struct stb0899_state *state = fe->demodulator_priv;
	struct stb0899_internal *internal = &state->internal;
	u8 reg;
	*status = 0;
	switch (state->delsys)
	{
		case SYS_DVBS:
		case SYS_DSS:
			dprintk(state->verbose, FE_DEBUG, 1, "Delivery system DVB-S/DSS");
			if (internal->lock)
			{
				reg = stb0899_read_reg(state, STB0899_VSTATUS);
				if (STB0899_GETFIELD(VSTATUS_LOCKEDVIT, reg))
				{
					dprintk(state->verbose, FE_DEBUG, 1, "--------> FE_HAS_CARRIER | FE_HAS_LOCK");
					*status |= FE_HAS_SIGNAL | FE_HAS_CARRIER | FE_HAS_LOCK;
					reg = stb0899_read_reg(state, STB0899_PLPARM);
					if (STB0899_GETFIELD(VITCURPUN, reg))
					{
						dprintk(state->verbose, FE_DEBUG, 1, "--------> FE_HAS_VITERBI | FE_HAS_SYNC");
						*status |= FE_HAS_VITERBI | FE_HAS_SYNC;
						/* post process event */
						stb0899_postproc(state, STB0899_POSTPROC_GPIO_LOCK, 1);
					}
				}
			}
			break;
		case SYS_DVBS2:
			dprintk(state->verbose, FE_DEBUG, 1, "Delivery system DVB-S2");
			if (internal->lock)
			{
				reg = STB0899_READ_S2REG(STB0899_S2DEMOD, DMD_STAT2);
				if (STB0899_GETFIELD(UWP_LOCK, reg) && STB0899_GETFIELD(CSM_LOCK, reg))
				{
					*status |= FE_HAS_SIGNAL | FE_HAS_CARRIER;
					dprintk(state->verbose, FE_DEBUG, 1,
							"UWP & CSM Lock ! ---> DVB-S2 FE_HAS_CARRIER");
					reg = stb0899_read_reg(state, STB0899_CFGPDELSTATUS1);
					if (STB0899_GETFIELD(CFGPDELSTATUS_LOCK, reg))
					{
						*status |= FE_HAS_LOCK;
						dprintk(state->verbose, FE_DEBUG, 1,
								"Packet Delineator Locked ! -----> DVB-S2 FE_HAS_LOCK");
					}
					if (STB0899_GETFIELD(CONTINUOUS_STREAM, reg))
					{
						*status |= FE_HAS_VITERBI;
						dprintk(state->verbose, FE_DEBUG, 1,
								"Packet Delineator found VITERBI ! -----> DVB-S2 FE_HAS_VITERBI");
					}
					if (STB0899_GETFIELD(ACCEPTED_STREAM, reg))
					{
						*status |= FE_HAS_SYNC;
						dprintk(state->verbose, FE_DEBUG, 1,
								"Packet Delineator found SYNC ! -----> DVB-S2 FE_HAS_SYNC");
						/* post process event */
						stb0899_postproc(state, STB0899_POSTPROC_GPIO_LOCK, 1);
					}
				}
			}
			break;
		default:
			dprintk(state->verbose, FE_DEBUG, 1, "Unsupported delivery system");
			return -EINVAL;
	}
	return 0;
}

/*
 * stb0899_get_error
 * viterbi error for DVB-S/DSS
 * packet error for DVB-S2
 * Bit Error Rate or Packet Error Rate * 10 ^ 7
 */
static int stb0899_read_ber(struct dvb_frontend *fe, u32 *ber)
{
	struct stb0899_state *state = fe->demodulator_priv;
	struct stb0899_internal *internal = &state->internal;
	u8 lsb, msb;
	u32 i;
	*ber = 0;
	switch (state->delsys)
	{
		case SYS_DVBS:
		case SYS_DSS:
			if (internal->lock)
			{
				/* average 5 BER values */
				for (i = 0; i < 5; i++)
				{
					msleep(100);
					lsb = stb0899_read_reg(state, STB0899_ECNT1L);
					msb = stb0899_read_reg(state, STB0899_ECNT1M);
					*ber += MAKEWORD16(msb, lsb);
					//printk("!!!!!!! BER:%d",*ber);
				}
				*ber /= 5;
				/* Viterbi Check */
				if (STB0899_GETFIELD(VSTATUS_PRFVIT, internal->v_status))
				{
					/* Error Rate */
					*ber *= 9766;
					/* ber = ber * 10 ^ 7 */
					*ber /= (-1 + (1 << (2 * STB0899_GETFIELD(NOE, internal->err_ctrl))));
					*ber /= 8;
				}
			}
			break;
		case SYS_DVBS2:
			if (internal->lock)
			{
				lsb = stb0899_read_reg(state, STB0899_ECNT2L);
				msb = stb0899_read_reg(state, STB0899_ECNT2M);
				*ber += MAKEWORD16(msb, lsb);
			}
			break;
		default:
			dprintk(state->verbose, FE_DEBUG, 1, "Unsupported delivery system");
			return -EINVAL;
	}
	return 0;
}

enum { VOLTAGE_13 = 1, VOLTAGE_18 = 0 };
enum { VOLTAGE_ON = 1, VOLTAGE_OFF = 0 };

static int stb0899_set_voltage(struct dvb_frontend *fe, fe_sec_voltage_t voltage)
{
	struct stb0899_state *state = fe->demodulator_priv;
	int ret;
	u8 b;
	struct i2c_msg msg = {.addr = 0x08, .flags = 0,
			   .buf = &b,
				.len = 1
	};
	switch (voltage)
	{
		case SEC_VOLTAGE_13:
			printk("BSKA SEC_VOLTAGE_13 POL:V\n");
			b = 0x70;
			ret = i2c_transfer(state->i2c, &msg, 1);
			if (ret = !1) printk("stb0899_set_voltage:SEC_VOLTAGE_13 Error ISL6423\n");
			break;
		case SEC_VOLTAGE_18:
			printk("BSKA SEC_VOLTAGE_18 POL:H\n");
			b = 0x72;
			ret = i2c_transfer(state->i2c, &msg, 1);
			if (ret = !1) printk("stb0899_set_voltage:SEC_VOLTAGE_18 Error ISL6423\n");
			break;
		case SEC_VOLTAGE_OFF:
			printk("BSKA SEC_VOLTAGE_OFF\n");
			b = 0x60;
			ret = i2c_transfer(state->i2c, &msg, 1);
			if (ret = !1) printk("stb0899_set_voltage:SEC_VOLTAGE_OFF Error ISL6423\n");
			break;
		default:
			return -EINVAL;
	}
	return 0;
}

static int stb0899_set_tone(struct dvb_frontend *fe, fe_sec_tone_mode_t tone)
{
	struct stb0899_state *state = fe->demodulator_priv;
	struct stb0899_internal *internal = &state->internal;
	u8 div, reg;
	int ret;
	u8 b;
	struct i2c_msg msg = {.addr = 0x08, .flags = 0,
			   .buf = &b,
				.len = 1
	};
	switch (tone)
	{
		case SEC_TONE_ON:
			printk("SEC_TONE_ON BSKA_DISEQ_STI7100\n");
			b = 0x34; //int 22khz enable
			ret = i2c_transfer(state->i2c, &msg, 1);
			if (ret = !1) printk("Error:ISL6423 SR2\n");
			break;
		case SEC_TONE_OFF:
			printk("SEC_TONE_OFF BSKA_DISEQ_STI7100\n");
			b = 0x2c; //2c-ext22khz 24-int22khz extmod
			ret = i2c_transfer(state->i2c, &msg, 1);
			if (ret = !1) printk("Error:ISL6423 SR2\n");
			break;
		default:
			return -EINVAL;
	}
	return 0;
}

int stb0899_i2c_gate_ctrl(struct dvb_frontend *fe, int enable)
{
	int i2c_stat;
	struct stb0899_state *state = fe->demodulator_priv;
// poprawka dla niedzialjacych glowic z komunikatem strojenie nieudane
	i2c_stat = 0x48 | 0x80;
	if (stb0899_write_reg(state, STB0899_I2CRPT, i2c_stat) < 0)
		goto err;
	return 0;
err:
	dprintk(state->verbose, FE_ERROR, 1, "I2C Repeater control failed");
	return -EREMOTEIO;
}

static inline void CONVERT32(u32 x, char *str)
{
	*str++ = (x >> 24) & 0xff;
	*str++ = (x >> 16) & 0xff;
	*str++ = (x >> 8) & 0xff;
	*str++ = (x >> 0) & 0xff;
	*str = '\0';
}

int stb0899_get_dev_id(struct stb0899_state *state)
{
	u8 chip_id, release;
	u16 id;
	u32 demod_ver = 0, fec_ver = 0;
	char demod_str[5] = { 0 };
	char fec_str[5] = { 0 };
	id = stb0899_read_reg(state, STB0899_DEV_ID);
	dprintk(state->verbose, FE_DEBUG, 1, "ID reg=[0x%02x]", id);
	chip_id = STB0899_GETFIELD(CHIP_ID, id);
	release = STB0899_GETFIELD(CHIP_REL, id);
	dprintk(state->verbose, FE_ERROR, 1, "Device ID=[%d], Release=[%d]",
			chip_id, release);
	CONVERT32(STB0899_READ_S2REG(STB0899_S2DEMOD, DMD_CORE_ID), (char *)&demod_str);
	demod_ver = STB0899_READ_S2REG(STB0899_S2DEMOD, DMD_VERSION_ID);
	dprintk(state->verbose, FE_ERROR, 1, "Demodulator Core ID=[%s], Version=[%d]", (char *) &demod_str, demod_ver);
	CONVERT32(STB0899_READ_S2REG(STB0899_S2FEC, FEC_CORE_ID_REG), (char *)&fec_str);
	fec_ver = STB0899_READ_S2REG(STB0899_S2FEC, FEC_VER_ID_REG);
	if (!(chip_id > 0))
	{
		dprintk(state->verbose, FE_ERROR, 1, "couldn't find a STB 0899");
		return -ENODEV;
	}
	dprintk(state->verbose, FE_ERROR, 1, "FEC Core ID=[%s], Version=[%d]", (char *) &fec_str, fec_ver);
	return 0;
}

static void stb0899_set_delivery(struct stb0899_state *state)
{
	u8 reg;
	u8 stop_clk[2];
	stop_clk[0] = stb0899_read_reg(state, STB0899_STOPCLK1);
	stop_clk[1] = stb0899_read_reg(state, STB0899_STOPCLK2);
	switch (state->delsys)
	{
		case SYS_DVBS:
			dprintk(state->verbose, FE_DEBUG, 1, "Delivery System -- DVB-S");
			/* FECM/Viterbi ON */
			reg = stb0899_read_reg(state, STB0899_FECM);
			STB0899_SETFIELD_VAL(FECM_RSVD0, reg, 0);
			STB0899_SETFIELD_VAL(FECM_VITERBI_ON, reg, 1);
			stb0899_write_reg(state, STB0899_FECM, reg);
			stb0899_write_reg(state, STB0899_RSULC, 0xb1);
			stb0899_write_reg(state, STB0899_TSULC, 0x40);
			stb0899_write_reg(state, STB0899_RSLLC, 0x42);
			stb0899_write_reg(state, STB0899_TSLPL, 0x12);
			reg = stb0899_read_reg(state, STB0899_TSTRES);
			STB0899_SETFIELD_VAL(FRESLDPC, reg, 1);
			stb0899_write_reg(state, STB0899_TSTRES, reg);
			STB0899_SETFIELD_VAL(STOP_CHK8PSK, stop_clk[0], 1);
			STB0899_SETFIELD_VAL(STOP_CKFEC108, stop_clk[0], 1);
			STB0899_SETFIELD_VAL(STOP_CKFEC216, stop_clk[0], 1);
			STB0899_SETFIELD_VAL(STOP_CKPKDLIN108, stop_clk[1], 1);
			STB0899_SETFIELD_VAL(STOP_CKPKDLIN216, stop_clk[1], 1);
			STB0899_SETFIELD_VAL(STOP_CKINTBUF216, stop_clk[0], 1);
			STB0899_SETFIELD_VAL(STOP_CKCORE216, stop_clk[0], 0);
			STB0899_SETFIELD_VAL(STOP_CKS2DMD108, stop_clk[1], 1);
			break;
		case SYS_DVBS2:
			/* FECM/Viterbi OFF */
			reg = stb0899_read_reg(state, STB0899_FECM);
			STB0899_SETFIELD_VAL(FECM_RSVD0, reg, 0);
			STB0899_SETFIELD_VAL(FECM_VITERBI_ON, reg, 0);
			stb0899_write_reg(state, STB0899_FECM, reg);
			stb0899_write_reg(state, STB0899_RSULC, 0xb1);
			stb0899_write_reg(state, STB0899_TSULC, 0x42);
			stb0899_write_reg(state, STB0899_RSLLC, 0x40);
			stb0899_write_reg(state, STB0899_TSLPL, 0x02);
			reg = stb0899_read_reg(state, STB0899_TSTRES);
			STB0899_SETFIELD_VAL(FRESLDPC, reg, 0);
			stb0899_write_reg(state, STB0899_TSTRES, reg);
			STB0899_SETFIELD_VAL(STOP_CHK8PSK, stop_clk[0], 1);
			STB0899_SETFIELD_VAL(STOP_CKFEC108, stop_clk[0], 0);
			STB0899_SETFIELD_VAL(STOP_CKFEC216, stop_clk[0], 0);
			STB0899_SETFIELD_VAL(STOP_CKPKDLIN108, stop_clk[1], 0);
			STB0899_SETFIELD_VAL(STOP_CKPKDLIN216, stop_clk[1], 0);
			STB0899_SETFIELD_VAL(STOP_CKINTBUF216, stop_clk[0], 0);
			STB0899_SETFIELD_VAL(STOP_CKCORE216, stop_clk[0], 0);
			STB0899_SETFIELD_VAL(STOP_CKS2DMD108, stop_clk[1], 0);
			break;
		case SYS_DSS:
			/* FECM/Viterbi ON */
			reg = stb0899_read_reg(state, STB0899_FECM);
			STB0899_SETFIELD_VAL(FECM_RSVD0, reg, 1);
			STB0899_SETFIELD_VAL(FECM_VITERBI_ON, reg, 1);
			stb0899_write_reg(state, STB0899_FECM, reg);
			stb0899_write_reg(state, STB0899_RSULC, 0xa1);
			stb0899_write_reg(state, STB0899_TSULC, 0x61);
			stb0899_write_reg(state, STB0899_RSLLC, 0x42);
			reg = stb0899_read_reg(state, STB0899_TSTRES);
			STB0899_SETFIELD_VAL(FRESLDPC, reg, 1);
			stb0899_write_reg(state, STB0899_TSTRES, reg);
			STB0899_SETFIELD_VAL(STOP_CHK8PSK, stop_clk[0], 1);
			STB0899_SETFIELD_VAL(STOP_CKFEC108, stop_clk[0], 1);
			STB0899_SETFIELD_VAL(STOP_CKFEC216, stop_clk[0], 1);
			STB0899_SETFIELD_VAL(STOP_CKPKDLIN108, stop_clk[1], 1);
			STB0899_SETFIELD_VAL(STOP_CKPKDLIN216, stop_clk[1], 1);
			STB0899_SETFIELD_VAL(STOP_CKCORE216, stop_clk[0], 0);
			STB0899_SETFIELD_VAL(STOP_CKS2DMD108, stop_clk[1], 1);
			break;
		default:
			dprintk(state->verbose, FE_ERROR, 1, "Unsupported delivery system");
			break;
	}
	STB0899_SETFIELD_VAL(STOP_CKADCI108, stop_clk[0], 0);
	stb0899_write_regs(state, STB0899_STOPCLK1, stop_clk, 2);
}

/*
 * stb0899_set_iterations
 * set the LDPC iteration scale function
 */
static void stb0899_set_iterations(struct stb0899_state *state)
{
	struct stb0899_internal *internal = &state->internal;
	struct stb0899_config *config = state->config;
	s32 iter_scale;
//	u32 reg;
	iter_scale = 17 * (internal->master_clk / 1000);
	iter_scale += 410000;
//	iter_scale /= (internal->srate / 1000000);
//	iter_scale /= 1000;
	iter_scale /= (internal->srate / 1000);
	if (iter_scale > config->ldpc_max_iter)
		iter_scale = config->ldpc_max_iter;
//	reg = STB0899_READ_S2REG(STB0899_S2DEMOD, MAX_ITER);
//	STB0899_SETFIELD_VAL(MAX_ITERATIONS, reg, iter_scale);
//	stb0899_write_s2reg(state, STB0899_S2DEMOD, STB0899_BASE_MAX_ITER, STB0899_OFF0_MAX_ITER, reg);
	stb0899_write_s2reg(state, STB0899_S2DEMOD, STB0899_BASE_MAX_ITER, STB0899_OFF0_MAX_ITER, iter_scale);
	stb0899_write_s2reg(state, STB0899_S2DEMOD, STB0899_BASE_ITER_SCALE, STB0899_OFF0_ITER_SCALE, iter_scale);
//	bug SR30000 - patch tBox Team
//	stb0899_write_s2reg(state, STB0899_S2FEC, STB0899_BASE_MAX_ITER, STB0899_OFF0_MAX_ITER, iter_scale);
//	stb0899_write_s2reg(state, STB0899_S2FEC, STB0899_BASE_ITER_SCALE, STB0899_OFF0_ITER_SCALE, iter_scale);
}

static int stb0899_set_property(struct dvb_frontend *fe, struct dtv_property *tvp)
{
	struct stb0899_state *state = fe->demodulator_priv;
	dprintk(state->verbose, FE_ERROR, 1, "%s(..)\n", __func__);
	return 0;
}

static int stb0899_get_property(struct dvb_frontend *fe, struct dtv_property *tvp)
{
	struct stb0899_state *state = fe->demodulator_priv;
	dprintk(state->verbose, FE_ERROR, 1, "%s(..)\n", __func__);
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
	return 0;
}

static enum dvbfe_search stb0899_search(struct dvb_frontend *fe, struct dvb_frontend_parameters *p)
{
	struct stb0899_state *state = fe->demodulator_priv;
	struct stb0899_params *i_params = &state->params;
	struct stb0899_internal *internal = &state->internal;
	struct stb0899_config *config = state->config;
	struct dtv_frontend_properties *c = &fe->dtv_property_cache;
	u32 SearchRange, gain;
	i_params->freq = c->frequency;
	i_params->srate = c->symbol_rate;
	state->delsys = c->delivery_system;
	dprintk(state->verbose, FE_DEBUG, 1, "delivery system=%d", state->delsys);
	SearchRange = 10000000;
	dprintk(state->verbose, FE_DEBUG, 1, "Frequency=%d, Srate=%d", i_params->freq, i_params->srate);
	/* checking Search Range is meaningless for a fixed 3 Mhz */
	if (INRANGE(i_params->srate, 1000000, 45000000))
	{
		dprintk(state->verbose, FE_DEBUG, 1, "Parameters IN RANGE");
		stb0899_set_delivery(state);
		if (state->config->tuner_set_rfsiggain)
		{
			if (internal->srate > 15000000)
				gain = 8; /* 15Mb < srate < 45Mb, gain = 8dB */
			else if (internal->srate > 5000000)
				gain = 12; /* 5Mb < srate < 15Mb, gain = 12dB */
			else
				gain = 14; /* 1Mb < srate < 5Mb, gain = 14db */
			state->config->tuner_set_rfsiggain(fe, gain);
		}
		if (i_params->srate <= 5000000)
			stb0899_set_mclk(state, config->lo_clk);
		else
			stb0899_set_mclk(state, config->hi_clk);
		switch (state->delsys)
		{
			case SYS_DVBS:
			case SYS_DSS:
				dprintk(state->verbose, FE_DEBUG, 1, "DVB-S delivery system");
				internal->freq = i_params->freq;
				internal->srate = i_params->srate;
				/*
				 * search = user search range +
				 * 500Khz +
				 * 2 * Tuner_step_size +
				 * 10% of the symbol rate
				 */
				internal->srch_range = SearchRange + 1500000 + (i_params->srate / 5);
				internal->derot_percent = 30;
				/* What to do for tuners having no bandwidth setup ? */
				/* enable tuner I/O */
				stb0899_i2c_gate_ctrl(&state->frontend, 1);
				if (state->config->tuner_set_bandwidth)
					state->config->tuner_set_bandwidth(fe, (13 * (stb0899_carr_width(state) + SearchRange)) / 10);
				if (state->config->tuner_get_bandwidth)
					state->config->tuner_get_bandwidth(fe, &internal->tuner_bw);
				/* disable tuner I/O */
				stb0899_i2c_gate_ctrl(&state->frontend, 0);
				/* Set DVB-S1 AGC */
				stb0899_write_reg(state, STB0899_AGCRFCFG, 0x11);
				/* Run the search algorithm */
				dprintk(state->verbose, FE_DEBUG, 1, "running DVB-S search algo ..");
				if (stb0899_dvbs_algo(state) == RANGEOK)
				{
					internal->lock = 1;
					dprintk(state->verbose, FE_DEBUG, 1,
							"-------------------------------------> DVB-S LOCK !");
//				stb0899_write_reg(state, STB0899_ERRCTRL1, 0x3d); /* Viterbi Errors */
//				internal->v_status = stb0899_read_reg(state, STB0899_VSTATUS);
//				internal->err_ctrl = stb0899_read_reg(state, STB0899_ERRCTRL1);
//				dprintk(state->verbose, FE_DEBUG, 1, "VSTATUS=0x%02x", internal->v_status);
//				dprintk(state->verbose, FE_DEBUG, 1, "ERR_CTRL=0x%02x", internal->err_ctrl);
					return DVBFE_ALGO_SEARCH_SUCCESS;
				}
				else
				{
					internal->lock = 0;
					return DVBFE_ALGO_SEARCH_FAILED;
				}
				break;
			case SYS_DVBS2:
				internal->freq = i_params->freq;
				internal->srate = i_params->srate;
				internal->srch_range = SearchRange;
				/* enable tuner I/O */
				stb0899_i2c_gate_ctrl(&state->frontend, 1);
				if (state->config->tuner_set_bandwidth)
					state->config->tuner_set_bandwidth(fe, (stb0899_carr_width(state) + SearchRange));
				if (state->config->tuner_get_bandwidth)
					state->config->tuner_get_bandwidth(fe, &internal->tuner_bw);
				/* disable tuner I/O */
				stb0899_i2c_gate_ctrl(&state->frontend, 0);
//			pParams->SpectralInv = pSearch->IQ_Inversion;
				/* Set DVB-S2 AGC */
				stb0899_write_reg(state, STB0899_AGCRFCFG, 0x1c);
				/* Set IterScale =f(MCLK,SYMB) */
				stb0899_set_iterations(state);
				/* Run the search algorithm */
				dprintk(state->verbose, FE_DEBUG, 1, "running DVB-S2 search algo ..");
				if (stb0899_dvbs2_algo(state) == DVBS2_FEC_LOCK)
				{
					internal->lock = 1;
					dprintk(state->verbose, FE_DEBUG, 1,
							"-------------------------------------> DVB-S2 LOCK !");
//				stb0899_write_reg(state, STB0899_ERRCTRL1, 0xb6); /* Packet Errors */
//				internal->v_status = stb0899_read_reg(state, STB0899_VSTATUS);
//				internal->err_ctrl = stb0899_read_reg(state, STB0899_ERRCTRL1);
					return DVBFE_ALGO_SEARCH_SUCCESS;
				}
				else
				{
					internal->lock = 0;
					return DVBFE_ALGO_SEARCH_FAILED;
				}
				break;
			default:
				dprintk(state->verbose, FE_ERROR, 1, "Unsupported delivery system");
				return DVBFE_ALGO_SEARCH_INVALID;
		}
	}
	return DVBFE_ALGO_SEARCH_ERROR;
}

#if 0
static enum stb0899_status stb0899_track_carrier(struct stb0899_state *state)
{
	u8 reg;
	reg = stb0899_read_reg(state, STB0899_DSTATUS);
	dprintk(state->verbose, FE_DEBUG, 1, "--------------------> STB0899_DSTATUS=[0x%02x]", reg);
	if (STB0899_GETFIELD(CARRIER_FOUND, reg))
	{
		dprintk(state->verbose, FE_DEBUG, 1, "-------------> CARRIEROK !");
		return CARRIEROK;
	}
	else
	{
		dprintk(state->verbose, FE_DEBUG, 1, "-------------> NOCARRIER !");
		return NOCARRIER;
	}
	return NOCARRIER;
}

static enum stb0899_status stb0899_get_ifagc(struct stb0899_state *state)
{
	u8 reg;
	reg = STB0899_READ_S2REG(STB0899_S2DEMOD, DMD_STATUS);
	dprintk(state->verbose, FE_DEBUG, 1, "DMD_STATUS=[0x%02x]", reg);
	if (STB0899_GETFIELD(IF_AGC_LOCK, reg))
	{
		dprintk(state->verbose, FE_DEBUG, 1, "------------->IF AGC LOCKED !");
		return AGC1OK;
	}
	else
	{
		dprintk(state->verbose, FE_DEBUG, 1, "------------->IF AGC LOCK LOST !");
		return NOAGC1;
	}
	return NOAGC1;
}

static int stb0899_get_s1fec(struct stb0899_internal *internal, enum fe_code_rate *fec)
{
	switch (internal->fecrate)
	{
		case STB0899_FEC_1_2:
			*fec = FEC_1_2;
			break;
		case STB0899_FEC_2_3:
			*fec = FEC_2_3;
			break;
		case STB0899_FEC_3_4:
			*fec = FEC_3_4;
			break;
		case STB0899_FEC_5_6:
			*fec = FEC_5_6;
			break;
		case STB0899_FEC_6_7:
			*fec = FEC_6_7;
			break;
		case STB0899_FEC_7_8:
			*fec = FEC_7_8;
			break;
		default:
			return -EINVAL;
	}
	return 0;
}

static int stb0899_get_modcod(struct stb0899_internal *internal, struct dvbs2_params *params)
{
	switch (internal->modcod)
	{
		case STB0899_DUMMY_PLF:
			params->modulation = DVBFE_MOD_NONE;
			params->fec = DVBFE_FEC_NONE;
			break;
		case STB0899_QPSK_14:
			params->modulation = DVBFE_MOD_QPSK;
			params->fec = DVBFE_FEC_1_4;
			break;
		case STB0899_QPSK_13:
			params->modulation = DVBFE_MOD_QPSK;
			params->fec = DVBFE_FEC_1_3;
			break;
		case STB0899_QPSK_25:
			params->modulation = DVBFE_MOD_QPSK;
			params->fec = DVBFE_FEC_2_5;
			break;
		case STB0899_QPSK_12:
			params->modulation = DVBFE_MOD_QPSK;
			params->fec = DVBFE_FEC_1_2;
			break;
		case STB0899_QPSK_35:
			params->modulation = DVBFE_MOD_QPSK;
			params->fec = DVBFE_FEC_3_5;
			break;
		case STB0899_QPSK_23:
			params->modulation = DVBFE_MOD_QPSK;
			params->fec = DVBFE_FEC_2_3;
			break;
		case STB0899_QPSK_34:
			params->modulation = DVBFE_MOD_QPSK;
			params->fec = DVBFE_FEC_3_4;
			break;
		case STB0899_QPSK_45:
			params->modulation = DVBFE_MOD_QPSK;
			params->fec = DVBFE_FEC_4_5;
			break;
		case STB0899_QPSK_56:
			params->modulation = DVBFE_MOD_QPSK;
			params->fec = DVBFE_FEC_5_6;
			break;
		case STB0899_QPSK_89:
			params->modulation = DVBFE_MOD_QPSK;
			params->fec = DVBFE_FEC_8_9;
			break;
		case STB0899_QPSK_910:
			params->modulation = DVBFE_MOD_QPSK;
			params->fec = DVBFE_FEC_9_10;
			break;
		case STB0899_8PSK_35:
			params->modulation = DVBFE_MOD_8PSK;
			params->fec = DVBFE_FEC_3_5;
			break;
		case STB0899_8PSK_23:
			params->modulation = DVBFE_MOD_8PSK;
			params->fec = DVBFE_FEC_2_3;
			break;
		case STB0899_8PSK_34:
			params->modulation = DVBFE_MOD_8PSK;
			params->fec = DVBFE_FEC_3_4;
			break;
		case STB0899_8PSK_56:
			params->modulation = DVBFE_MOD_8PSK;
			params->fec = DVBFE_FEC_5_6;
			break;
		case STB0899_8PSK_89:
			params->modulation = DVBFE_MOD_8PSK;
			params->fec = DVBFE_FEC_8_9;
			break;
		case STB0899_8PSK_910:
			params->modulation = DVBFE_MOD_8PSK;
			params->fec = DVBFE_FEC_9_10;
			break;
		case STB0899_16APSK_23:
			params->modulation = DVBFE_MOD_16APSK;
			params->fec = DVBFE_FEC_2_3;
			break;
		case STB0899_16APSK_34:
			params->modulation = DVBFE_MOD_16APSK;
			params->fec = DVBFE_FEC_3_4;
			break;
		case STB0899_16APSK_45:
			params->modulation = DVBFE_MOD_16APSK;
			params->fec = DVBFE_FEC_4_5;
			break;
		case STB0899_16APSK_56:
			params->modulation = DVBFE_MOD_16APSK;
			params->fec = DVBFE_FEC_5_6;
			break;
		case STB0899_16APSK_89:
			params->modulation = DVBFE_MOD_16APSK;
			params->fec = DVBFE_FEC_8_9;
			break;
		case STB0899_16APSK_910:
			params->modulation = DVBFE_MOD_16APSK;
			params->fec = DVBFE_FEC_9_10;
			break;
		case STB0899_32APSK_34:
			params->modulation = DVBFE_MOD_32APSK;
			params->fec = DVBFE_FEC_3_4;
			break;
		case STB0899_32APSK_45:
			params->modulation = DVBFE_MOD_32APSK;
			params->fec = DVBFE_FEC_4_5;
			break;
		case STB0899_32APSK_56:
			params->modulation = DVBFE_MOD_32APSK;
			params->fec = DVBFE_FEC_5_6;
			break;
		case STB0899_32APSK_89:
			params->modulation = DVBFE_MOD_32APSK;
			params->fec = DVBFE_FEC_8_9;
			break;
		case STB0899_32APSK_910:
			params->modulation = DVBFE_MOD_32APSK;
			params->fec = DVBFE_FEC_9_10;
			break;
		default:
			return -EINVAL;
	}
	return 0;
}
#endif
/*
 * stb0899_track
 * periodically check the signal level against a specified
 * threshold level and perform derotator centering.
 * called once we have a lock from a successful search
 * event.
 *
 * Will be called periodically called to maintain the
 * lock.
 *
 * Will be used to get parameters as well as info from
 * the decoded baseband header
 *
 * Once a new lock has established, the internal state
 * frequency (internal->freq) is updated
 */
static int stb0899_track(struct dvb_frontend *fe, struct dvb_frontend_parameters *p)
{
#if 0
	u32 lock_lost;
	struct stb0899_state *state = fe->demodulator_priv;
	struct stb0899_internal *internal = &state->internal;
	switch (state->delsys)
	{
		case DVBFE_DELSYS_DVBS:
			dprintk(state->verbose, FE_DEBUG, 1, "Tracking DVB-S state");
			if (stb0899_track_carrier(state) == CARRIEROK)
			{
				params->frequency = internal->freq;
				params->inversion = internal->inversion;
				params->delivery = state->delsys;
				params->delsys.dvbs.symbol_rate = internal->srate;
				params->delsys.dvbs.modulation = DVBFE_MOD_QPSK;
				stb0899_get_s1fec(internal, &params->delsys.dvbs.fec);
			}
			break;
		case DVBFE_DELSYS_DSS:
			dprintk(state->verbose, FE_DEBUG, 1, "Tracking DSS state");
			if (stb0899_track_carrier(state) == CARRIEROK)
			{
				params->frequency = internal->freq;
				params->inversion = internal->inversion;
				params->delivery = state->delsys;
				params->delsys.dss.symbol_rate = internal->srate;
				params->delsys.dss.modulation = DVBFE_MOD_QPSK;
				stb0899_get_s1fec(internal, &params->delsys.dss.fec);
			}
			break;
		case DVBFE_DELSYS_DVBS2:
			dprintk(state->verbose, FE_DEBUG, 1, "Tracking DVB-S2 state");
			if (stb0899_get_ifagc(state) == AGC1OK)
			{
				params->frequency = internal->freq;
				params->inversion = internal->inversion;
				params->delivery = state->delsys;
				params->delsys.dvbs2.symbol_rate = internal->srate;
				stb0899_get_modcod(internal, &params->delsys.dvbs2);
				params->delsys.dvbs2.rolloff = internal->rolloff;
				params->delsys.dvbs2.matype_1 = stb0899_read_reg(state, STB0899_MATSTRL);
				params->delsys.dvbs2.matype_2 = stb0899_read_reg(state, STB0899_MATSTRM);
				params->delsys.dvbs2.upl_1 = stb0899_read_reg(state, STB0899_UPLSTRL);
				params->delsys.dvbs2.upl_2 = stb0899_read_reg(state, STB0899_UPLSTRM);
				params->delsys.dvbs2.dfl_1 = stb0899_read_reg(state, STB0899_DFLSTRL);
				params->delsys.dvbs2.dfl_2 = stb0899_read_reg(state, STB0899_DFLSTRM);
				params->delsys.dvbs2.sync = stb0899_read_reg(state, STB0899_SYNCSTR);
				params->delsys.dvbs2.syncd_1 = stb0899_read_reg(state, STB0899_SYNCDSTRL);
				params->delsys.dvbs2.syncd_2 = stb0899_read_reg(state, STB0899_SYNCDSTRM);
			}
			lock_lost = STB0899_READ_S2REG(STB0899_S2DEMOD, LOCK_LOST);
			dprintk(state->verbose, FE_DEBUG, 1, "Lock Lost=[0x%02x]\n", lock_lost);
			if (STB0899_GETFIELD(LOCK_LOST, lock_lost))
				dprintk(state->verbose, FE_ERROR, 1, "Demodulator LOST LOCK !\n");
			break;
		default:
			dprintk(state->verbose, FE_ERROR, 1, "Unsupported delivery system");
			return -EINVAL;
	}
//	*delay = HZ/10;
#endif
	return 0;
}

static int stb0899_get_frontend(struct dvb_frontend *fe, struct dvb_frontend_parameters *p)
{
	struct stb0899_state *state = fe->demodulator_priv;
	struct stb0899_internal *internal = &state->internal;
	dprintk(state->verbose, FE_DEBUG, 1, "Get params");
	p->u.qpsk.symbol_rate = internal->srate;
	return 0;
}

static enum dvbfe_algo stb0899_frontend_algo(struct dvb_frontend *fe)
{
	return DVBFE_ALGO_CUSTOM;
}

static struct dvb_frontend_ops stb0899_ops =
{

	.info = {
		.name = "STB0899 Multistandard",
		.type = FE_QPSK, /* with old API */
		.frequency_min = 950000,
		.frequency_max = 2150000,
		.frequency_stepsize = 0,
		.frequency_tolerance = 0,
		.symbol_rate_min = 1000000,
		.symbol_rate_max = 45000000,

		.caps = FE_CAN_INVERSION_AUTO |
		FE_CAN_FEC_AUTO |
		FE_CAN_2G_MODULATION |
		FE_CAN_QPSK
	},

	.release = stb0899_release,
	.init = stb0899_init,
	.sleep = stb0899_sleep,

	.i2c_gate_ctrl = stb0899_i2c_gate_ctrl,

	.set_property = stb0899_set_property,
	.get_property = stb0899_get_property,
	.get_frontend_algo = stb0899_frontend_algo,
	.search = stb0899_search,
	.track = stb0899_track,
	.get_frontend = stb0899_get_frontend,

	.read_status = stb0899_read_status,
	.read_snr = stb0899_read_snr,
	.read_signal_strength = stb0899_read_signal_strength,
	.read_ber = stb0899_read_ber,

	.set_voltage = stb0899_set_voltage,
	.set_tone = stb0899_set_tone,

	.diseqc_send_master_cmd = stb0899_send_diseqc_msg,
	.diseqc_recv_slave_reply = stb0899_recv_slave_reply,
	.diseqc_send_burst = stb0899_send_diseqc_burst,
};

struct dvb_frontend *stb0899_attach(struct stb0899_config *config, struct i2c_adapter *i2c)
{
	struct stb0899_state *state = NULL;
	enum stb0899_inversion inversion;
	state = kzalloc(sizeof(struct stb0899_state), GFP_KERNEL);
	if (state == NULL)
		goto error;
	inversion = config->inversion;
	state->verbose = &verbose;
	state->config = config;
	state->i2c = i2c;
	state->frontend.ops = stb0899_ops;
	state->frontend.demodulator_priv = state;
	state->internal.inversion = inversion;
	stb0899_wakeup(&state->frontend);
	if (stb0899_get_dev_id(state) == -ENODEV)
	{
		printk("%s: Exiting .. !\n", __func__);
		goto error;
	}
	printk("%s: Attaching STB0899 \n", __func__);
	return &state->frontend;
error:
	kfree(state);
	return NULL;
}
EXPORT_SYMBOL(stb0899_attach);

