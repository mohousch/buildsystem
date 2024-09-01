
/*****************************************************************************/
/* COPYRIGHT (C) 2009 STMicroelectronics - All Rights Reserved */
/* ST makes no warranty express or implied including but not limited to, */
/* any warranty of */
/* */
/* (i) merchantability or fitness for a particular purpose and/or */
/* (ii) requirements, for a particular purpose in relation to the LICENSED */
/* MATERIALS, which is provided "AS IS", WITH ALL FAULTS. ST does not */
/* represent or warrant that the LICENSED MATERIALS provided here */
/* under is free of infringement of any third party patents, */
/* copyrights,trade secrets or other intellectual property rights. */
/* ALL WARRANTIES, CONDITIONS OR OTHER TERMS IMPLIED BY LAW ARE */
/* EXCLUDED TO THE FULLEST EXTENT PERMITTED BY LAW */
/* */
/*****************************************************************************/
/**
 @File d3501.c
 @brief

*/

//YWDRIVER_MODI lwj add begin
/*****************************************************************************
* Copyright (C)2004 Ali Corporation. All Rights Reserved.
*
* File: This file contains S3501 DVBS2 basic function in LLD.
*
* Description: Header file in LLD.
* History:
* Date Athor Version Reason
* ============ ========= ======= =================
* 1. 05/18/2006 Berg Ver 0.1 Create file for S3501 DVBS2 project
* 2. 06/30/2007 Dietel Add synchronous mechanism support
* 3. 07/03/2007 Dietel Add autoscan support ,Clean external macro
* 4. 07/03/2007 Dietel Not using VBV_BUFFER

* 5. 03/18/2008 Carcy Liu Open EQ, only mask EQ when 1/4, 1/3, 2/5 code rate.
 -> key word is "EQ_ENABLE".

* 6. 03/18/2008 Carcy Liu Add description about I2C through. Note:
 M3501 support I2C through function, but it should be
 careful when open & close I2C through function, otherwise
 the I2C slave may be "deadlock". Please do it as bellow.
 1, open I2C through function.
 2, wait 50ms at least.
 3, configure tuner
 4, wait 50ms at least.
 5, close I2C through function.
 6, wait 50ms at least.
 -> key word is "I2C_THROUGH_EN"

* 7. 03/21/2008 Carcy Liu Disable HBCD disable function, HBCD is DVBS2 H8PSK
 option mode, if enable HBCD, firmware will spend
 more time to check HBCD when channel change.
 Suggest that disable HBCD.
 -> key word is "HBCD_DISABLE"
* 8. 04/04/2008 Carcy Liu Update power control
 -> key word is "power control"

* 9. 04/18/2008 Carcy Liu Update code rate information.

* 10. 04/21/2008 Carcy Liu Add configure ZL10037's base band gain "G_BB_GAIN"

* 11. 04/23/2008 Carcy Liu Software MUST send a reset pulse to the M3501 reset pin
 before it perform a channel change. This is because a bug
 is found in M3501 power saving logic which may cause
 abnormal large power consumption if it is not reset before
 a channel change is performed. It has been proven that
 after reset this bug is cleared.
 Key word "m3501_reset"

* 12. 04/28/2008 Carcy Liu Show how to get LDPC code and other information:
 (All information show in the function)
 nim_s3501_reg_get_code_rate(UINT8* code_rate)
 nim_s3501_reg_get_map_type(UINT8*map_type)
 nim_s3501_reg_get_work_mode(UINT8*work_mode)
 nim_s3501_reg_get_roll_off(UINT8* roll_off)
 1, use nim_s3501_reg_get_work_mode to get work mode: dvbs or dvbs2
 // Work Mode
 // 0x0: DVB-S
 // 0x1: DVB-S2
 // 0x2: DVB-S2 HBC
 2, use nim_s3501_reg_get_map_type to get modulate type: QPSK/8SPK/16APSK
 // Map type:
 // 0x0: HBCD.
 // 0x1: BPSK
 // 0x2: QPSK
 // 0x3: 8PSK
 // 0x4: 16APSK
 // 0x5: 32APSK
 3, use nim_s3501_reg_get_code_rate to get LDPC code rate.
 // Code rate list
 // for DVBS:
 // 0x0: 1/2,
 // 0x1: 2/3,
 // 0x2: 3/4,
 // 0x3: 5/6,
 // 0x4: 6/7,
 // 0x5: 7/8.
 // For DVBS2 :
 // 0x0: 1/4 ,
 // 0x1: 1/3 ,
 // 0x2: 2/5 ,
 // 0x3: 1/2 ,
 // 0x4: 3/5 ,
 // 0x5: 2/3 ,
 // 0x6: 3/4 ,
 // 0x7: 4/5 ,
 // 0x8: 5/6 ,
 // 0x9: 8/9 ,
 // 0xa: 9/10.
 Key word "ldpc_code"

* 13. 04/28/2008 Carcy Liu Add "nim_s3501_hw_init()" in m3501_reset().
 Make sure hardware reset successfully.
 Key word "nim_s3501_hw_init"

* 14. 2008-5-27 Carcy Liu a. Add dynamic power control: nim_s3501_dynamic_power()
 This function should be called every some seconds. In fact,
 MPEG host chip should get signal quality and intensity every
 some second, You can add nim_s3501_dynamic_power to it's tail.
 Every time when MPEG host chip get signal quality, it monitor
 and control M3501's power at the same time.

 b. Add clock control in channel change, if channel lock,
 readback workmode, if workmode is DVBS, then slow down DVBS2
 clock to save power.

 c. Add description for get signal quality and get signal intensity
 c1: get signal quality : nim_s3501_get_SNR
 c2: get signal intensity: nim_s3501_get_AGC

 d: nim_s3501_get_BER and nim_s3501_get_PER.
 For DVBS2, BER is from BCH, it's meaning is very different with
 from viterbi in DVBS. So we use packet error rate (PER) for
 nim_s3501_get_BER when work in DVBS2.

 Key word : "power_ctrl"

* 15. 2008-12-2 Douglass Yan a. Delete some local static variable
 b. Change attach function name
 c. add variable to private structure to save qpsk addr
 d. Update local function parameters num

* 16.2008-12-11 Douglass Yan a. Delete all shared variable for support Dual S3501

* 17.2008-12-16 Douglass Yan a. Delete maco REVERT_POLAR

*****************************************************************************/

#include <linux/version.h>
#include <linux/bitops.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/dvb/version.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,23)
# include <linux/stpio.h>
#else
# include <linux/stm/pio.h>
#endif

#include "dvbdev.h"
#include "dmxdev.h"
#include "dvb_frontend.h"

#include "D3501_ext.h"
#include "D3501.h"

extern int debug_fe7162;
#define _DEBUG if (debug_fe7162)

struct dvb_d3501_fe_state
{
	struct dvb_frontend frontend;
	struct nim_device spark_nimdev;
	struct stpio_pin *fe_lnb_13_18;
	struct stpio_pin *fe_lnb_on_off;
};

#define I2C_ERROR_BASE -200

#define ERR_I2C_SCL_LOCK (I2C_ERROR_BASE - 1) /* I2C SCL be locked */
#define ERR_I2C_SDA_LOCK (I2C_ERROR_BASE - 2) /* I2C SDA be locked */
#define ERR_I2C_NO_ACK (I2C_ERROR_BASE - 3) /* I2C slave no ack */

#define S3501_ERR_I2C_NO_ACK ERR_I2C_NO_ACK
#define T_CTSK OSAL_T_CTSK

extern INT32 FFT_energy_1024[1024];

#if 0 //we don't need the tuner name
/* Name for the tuner, the last character must be Number for index */
static char nim_s3501_name[3][HLD_MAX_NAME_SIZE] =
{
	"NIM_S3501_0", "NIM_S3501_1", "NIM_S3501_2"
};
#endif

static const UINT8 ssi_clock_tab[] =
{
	98, 90, 83, 77, 72, 67, 60, 54, 50
};

static const UINT8 MAP_BETA_ACTIVE_BUF[32] =
{
	0x00, // //index 0, do not use
	0x01, // 1/4 of QPSK //1
	0x01, // 1/3 //2
	0x01, // 2/5 //3
	0x01, // 1/2 //4
	0x01, // 3/5 //5
	0x01, // 2/3 //6
	0x01, // 3/4 //7
	0x01, // 4/5 //8
	0x01, // 5/6 //9
	0x01, // 8/9 //a
	0x01, // 9/10 //b
	0x01, // 3/5 of 8PSK //c
	0x01, // 2/3 //d
	0x01, // 3/4 //e
	0x01, // 5/6 //f
	0x01, // 8/9 //10
	0x01, // 9/10 //11
	0x01, // 2/3 of 16APSK //12
	0x01, // 3/4 //13
	0x01, // 4/5 //14
	0x01, // 5/6 //15
	0x01, // 8/9 //16
	0x01, // 9/10 //17
	0x01, // for 32 APSK, dont use
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01
};

static const UINT8 MAP_BETA_BUF[32] =
{
	188, // 205, // index 0, do not use
	188, // 205, // 1/4 of QPSK //1
	190, // 230, // 1/3 //2
	205, // 205, // 2/5 //3
	205, // 205, // 1/2 //4
	180, // 180, // 3/5 //5
	180, // 180, // 2/3 //6
	180, // 180, // 3/4 //7
	155, // 180, // 4/5 //8
	168, // 180, // 5/6 //9
	150, // 155, // 8/9 //a
	150, // 155, // 9/10 //b
	180, // 180, // 3/5 of 8PSK //c
	180, // 180, // 2/3 //d
	170, // 180, // 3/4 //e
	180, // 155, // 5/6 //f
	150, // 155, // 8/9 //10
	150, // 155, // 9/10 //11
	180, // 205, // 2/3 of 16APSK //12
	180, // 180, // 3/4 //13
	180, // 180, // 4/5 //14
	170, // 155, // 5/6 //15
	155, // 155, // 8/9 //16
	155, // 155, // 9/10 //17
	155, //--------------------- for 32 APSK, dont use
	155, 155, 155, 155, 155, 155, 155
};

static const UINT16 DEMAP_NOISE[32] =
{
	0x00, // index 0, do not use
	0x16b, // 1/4 of QPSK //1
	0x1d5, // 1/3 //2
	0x246, // 2/5 //3
	0x311, // 1/2 //4
	0x413, // 3/5 //5
	0x4fa, // 2/3 //6
	0x62b, // 3/4 //7
	0x729, // 4/5 //8
	0x80c, // 5/6 //9
	0xa2a, // 8/9 //a
	0xab2, // 9/10 //b
	0x8a9, // 3/5 of 8PSK //c
	0xb31, // 2/3 //d
	0xf1d, // 3/4 //e
	0x1501, // 5/6 //f
	0x1ca5, // 8/9 //10
	0x1e91, // 9/10 //11
	0x133b, // 2/3 of 16APSK //12
	0x199a, // 3/4 //13
	0x1f08, // 4/5 //14
	0x234f, // 5/6 //15
	0x2fa1, // 8/9 //16
	0x3291, // 9/10 //17
	0x00, // for 32 APSK, dont use
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const UINT8 snr_tab[177] =
{
	0, 1, 3, 4, 5, 7, 8, \
	9, 10, 11, 13, 14, 15, 16, 17, \
	18, 19, 20, 21, 23, 24, 25, 26, \
	26, 27, 28, 29, 30, 31, 32, 33, \
	34, 35, 35, 36, 37, 38, 39, 39, \
	40, 41, 42, 42, 43, 44, 45, 45, \
	46, 47, 47, 48, 49, 49, 50, 51, \
	51, 52, 52, 53, 54, 54, 55, 55, \
	56, 57, 57, 58, 58, 59, 59, 60, \
	61, 61, 62, 62, 63, 63, 64, 64, \
	65, 66, 66, 67, 67, 68, 68, 69, \
	69, 70, 70, 71, 72, 72, 73, 73, \
	74, 74, 75, 76, 76, 77, 77, 78, \
	79, 79, 80, 80, 81, 82, 82, 83, \
	84, 84, 85, 86, 86, 87, 88, 89, \
	89, 90, 91, 92, 92, 93, 94, 95, \
	96, 96, 97, 98, 99, 100, 101, 102, \
	102, 103, 104, 105, 106, 107, 108, 109, \
	110, 111, 113, 114, 115, 116, 117, 118, \
	119, 120, 122, 123, 124, 125, 127, 128, \
	129, 131, 132, 133, 135, 136, 138, 139, \
	141, 142, 144, 145, 147, 148, 150, 152, \
	153, 155
};

//internal function
////static INT32 nim_s3501_open(struct nim_device *dev);
////static INT32 nim_s3501_close(struct nim_device *dev);
////static INT32 nim_s3501_ioctl(struct nim_device *dev, INT32 cmd, UINT32 param);
////static INT32 nim_s3501_set_polar(struct nim_device *dev, UINT8 polar);
////static INT32 nim_s3501_set_12v(struct nim_device *dev, UINT8 flag);
////static INT32 nim_s3501_channel_change(struct nim_device *dev, UINT32 freq, UINT32 sym, UINT8 fec);
////static INT32 nim_s3501_channel_search(struct nim_device *dev, UINT32 CRNum);
////static INT32 nim_s3501_DiSEqC_operate(struct nim_device *dev, UINT32 mode, UINT8 *cmd, UINT8 cnt);
////static INT32 nim_s3501_DiSEqC2X_operate(struct nim_device *dev, UINT32 mode, UINT8 *cmd, UINT8 cnt, UINT8 *rt_value, UINT8 *rt_cnt);
////static INT32 nim_s3501_get_lock(struct nim_device *dev, UINT8 *lock);//remove
////static INT32 nim_s3501_get_freq(struct nim_device *dev, UINT32 *freq);
//static INT32 nim_s3501_get_symbol_rate(struct nim_device *dev, UINT32 *sym_rate);
//static INT32 nim_s3501_get_code_rate(struct nim_device *dev, UINT8 *code_rate);
static INT32 nim_s3501_get_AGC(struct nim_device *dev, UINT16 *agc);
static INT32 nim_s3501_get_SNR(struct nim_device *dev, UINT16 *snr);
static INT32 nim_s3501_get_BER(struct nim_device *dev, UINT32 *RsUbc);
////static INT32 nim_s3501_get_PER(struct nim_device *dev, UINT32 *RsUbc);//question
static INT32 nim_s3501_get_LDPC(struct nim_device *dev, UINT32 *RsUbc);
//static YW_ErrorType_T demod_d3501_ScanFreq(U8 Handle);

static INT32 nim_s3501_get_type(struct nim_device *dev);
static INT32 nim_s3501_reg_get_map_type(struct nim_device *dev, UINT8 *map_type);
static INT32 nim_s3501_reg_get_work_mode(struct nim_device *dev, UINT8 *work_mode);
static INT32 nim_s3501_reg_get_iqswap_flag(struct nim_device *dev, UINT8 *iqswap_flag);
static INT32 nim_s3501_reg_get_roll_off(struct nim_device *dev, UINT8 *roll_off);

static UINT8 nim_s3501_get_SNR_index(struct nim_device *dev);
static INT32 nim_s3501_set_ts_mode(struct nim_device *dev, UINT8 work_mode, UINT8 map_type, UINT8 code_rate, UINT32 Rs,
								   UINT8 channel_change_flag);
static INT32 nim_s3501_get_bit_rate(struct nim_device *dev, UINT8 work_mode, UINT8 map_type, UINT8 code_rate, UINT32 Rs, UINT8 *bit_rate);
static INT32 nim_s3501_set_ssi_clk(struct nim_device *dev, UINT8 bit_rate);
////static INT32 nim_s3501_set_dmy_format(struct nim_device *dev); //bentao add for M3501B in 20100113.
////static INT32 nim_s3501_ioctl_ext(struct nim_device *dev, INT32 cmd, void *param_list);
static INT32 nim_s3501_dynamic_power(struct nim_device *dev, UINT8 snr);
static INT32 nim_s3501_get_type(struct nim_device *dev);
////static INT32 nim_s3501_set_err(struct nim_device *dev);
static INT32 nim_s3501_get_err(struct nim_device *dev);
static INT32 nim_s3501_clear_err(struct nim_device *dev);
static INT32 nim_s3501_i2c_open(struct nim_device *dev);
static INT32 nim_s3501_i2c_close(struct nim_device *dev);
////static INT32 nim_s3501_ext_lnb_config(struct nim_device *dev, struct QPSK_TUNER_CONFIG_API *ptrQPSK_Tuner);//lwj remove
static INT32 nim_s3501_interrupt_mask_clean(struct nim_device *dev);
static INT32 nim_s3501_sym_config(struct nim_device *dev, UINT32 sym);
static INT32 nim_s3501_adc_setting(struct nim_device *dev);
static INT32 nim_s3501_set_hw_timeout(struct nim_device *dev, UINT8 time_thr);
static INT32 nim_s3501_agc1_ctrl(struct nim_device *dev, UINT8 low_sym, UINT8 s_Case);
static INT32 nim_s3501_freq_offset_set(struct nim_device *dev, UINT8 low_sym, UINT32 *s_Freq);
static INT32 nim_s3501_freq_offset_reset(struct nim_device *dev, UINT8 low_sym);

static INT32 nim_s3501_cr_setting(struct nim_device *dev, UINT8 s_Case);
static INT32 nim_s3501_ldpc_setting(struct nim_device *dev, UINT8 s_Case, UINT8 c_ldpc, UINT8 c_fec);
static INT32 nim_s3501_hw_init(struct nim_device *dev);
static INT32 nim_s3501_demod_ctrl(struct nim_device *dev, UINT8 c_Value);
static INT32 nim_s3501_hbcd_timeout(struct nim_device *dev, UINT8 s_Case);
static INT32 nim_s3501_set_acq_workmode(struct nim_device *dev, UINT8 s_Case);
static INT32 nim_s3501_set_FC_Search_Range(struct nim_device *dev, UINT8 s_Case, UINT32 rs);
static INT32 nim_s3501_RS_Search_Range(struct nim_device *dev, UINT8 s_Case, UINT32 rs);

static INT32 nim_s3501_TR_CR_Setting(struct nim_device *dev, UINT8 s_Case);
//static INT32 nim_s3501_task_init(struct nim_device *dev);//question
//static void nim_s3501_task(UINT32 param1, UINT32 param2);
static INT32 nim_s3501_set_phase_noise(struct nim_device *dev);
static INT32 nim_s3501_get_new_BER(struct nim_device *dev, UINT32 *ber);
static INT32 nim_s3501_get_new_PER(struct nim_device *dev, UINT32 *per);
//static INT32 nim_s3501_get_phase_error(struct nim_device *dev, INT32 *phase_error);
//static void nim_s3501_set_demap_noise(struct nim_device *dev);
//static INT32 nim_s3501_s2_get_BER(struct nim_device *dev, UINT32 *RsUbc);
//static INT32 nim_change_ts_gap(struct nim_device *dev, UINT8 gap);//lwj don't know this

INT32 nim_vz7306_status(U8 tuner_id, BOOL *lock);
INT32 nim_vz7306_init(U8 *tuner_id, struct QPSK_TUNER_CONFIG_EXT *ptrTuner_Config);
INT32 nim_vz7306_control(U8 tuner_id, UINT32 freq, UINT32 sym);

static int d3501_initition(struct nim_device *dev, struct i2c_adapter *i2c);
static int d3501_term(struct nim_device *dev);

#define SUCCESS 0
#define ERR_FAILED -1
#define ERR_TIME_OUT -3 /* Waiting time out */

#if 0
INT32 nim_reg_read(struct nim_device *dev, UINT8 bMemAdr, UINT8 *pData, UINT8 bLen)
{
	INT32 err;
	UINT8 chip_adr;//= m3501_ext_dm_config.i2c_base_addr;
	UINT32 i2c_type_id;// = m3501_ext_dm_config.i2c_type_id;
	struct nim_s3501_private *priv = (struct nim_s3501_private *) dev->priv;
	chip_adr = priv->ext_dm_config.i2c_base_addr;
	i2c_type_id = priv->ext_dm_config.i2c_type_id;
	pData[0] = bMemAdr;
	//NIM_MUTEX_ENTER(priv); //
	err = i2c_write_read(i2c_type_id, chip_adr, pData, 1, bLen);
	if (err)
	{
		if (priv->ul_status.m_pfn_reset_s3501)
		{
			priv->ul_status.m_pfn_reset_s3501((priv->tuner_id + 1) << 16);
			msleep(100);
			priv->t_Param.t_i2c_err_flag = 0x01;
			//nim_s3501_open(dev);
		}
		nim_s3501_set_err(dev);
		printk("s3501 i2c read error = %d,chip_adr=0x%x,bMemAdr=0x%x,I2C_FOR_S3501 = %d,TaskID=%d\n", -err, chip_adr, pData[0],
			   i2c_type_id, osal_task_get_current_id());
	}
	else
	{
		if (priv->t_Param.t_i2c_err_flag)
		{
			priv->t_Param.t_i2c_err_flag = 0x00;
			//nim_s3501_open(dev);
		}
	}
	//NIM_MUTEX_LEAVE(priv); //
	return err;
}
#else
INT32 nim_reg_read(struct nim_device *dev, UINT8 bMemAdr, UINT8 *pData, UINT8 bLen)
{
	int ret;
	struct nim_s3501_private *priv_mem;
	u8 b0[] = { bMemAdr };
	struct i2c_msg msg[] =
	{
		{ .addr = dev->base_addr, .flags = 0, .buf = b0, .len = 1 },
		{ .addr = dev->base_addr, .flags = I2C_M_RD, .buf = pData, .len = bLen }
	};
	priv_mem = (struct nim_s3501_private *)dev->priv;
	ret = i2c_transfer(priv_mem->i2c_adap, msg, 2);
	if (ret != 2)
	{
		if (ret != -ERESTARTSYS)
			printk("Read error, Reg=[0x%02x], Status=%d\n", bMemAdr, ret);
		return ret < 0 ? ret : -EREMOTEIO;
	}
	return ret;
}
#endif

#if 0
INT32 nim_reg_write(struct nim_device *dev, UINT8 bMemAdr, UINT8 *pData, UINT8 bLen)
{
	int err;
	//UINT8 aa = 1;
	UINT8 i, buffer[8];
	UINT8 chip_adr ;//= m3501_ext_dm_config.i2c_base_addr;
	UINT32 i2c_type_id;// = m3501_ext_dm_config.i2c_type_id;
	struct nim_s3501_private *priv = (struct nim_s3501_private *) dev->priv;
	chip_adr = priv->ext_dm_config.i2c_base_addr;
	i2c_type_id = priv->ext_dm_config.i2c_type_id;
	if (bLen > 7)
	{
		nim_s3501_set_err(dev);
		return ERR_FAILUE;
	}
	//printk("offset: %x len: %x\n",bMemAdr,bLen);
	buffer[0] = bMemAdr;
	for (i = 0; i < bLen; i++)
	{
		buffer[i + 1] = pData[i];
	}
	NIM_MUTEX_ENTER(priv);
	err = i2c_write(i2c_type_id, chip_adr, buffer, bLen + 1);
	NIM_MUTEX_LEAVE(priv);
	if (err != 0)
	{
		if (priv->ul_status.m_pfn_reset_s3501)
		{
			priv->ul_status.m_pfn_reset_s3501((priv->tuner_id + 1) << 16);
			msleep(100);
			priv->t_Param.t_i2c_err_flag = 0x01;
			//nim_s3501_open(dev);
		}
		nim_s3501_set_err(dev);
		printk("s3501 i2c write error = %d,chip_adr=0x%x,bMemAdr=0x%x,I2C_FOR_S3501 = %d,TaskID=%d\n", -err, chip_adr, bMemAdr,
			   i2c_type_id, osal_task_get_current_id());
	}
	else
	{
		if (priv->t_Param.t_i2c_err_flag)
		{
			priv->t_Param.t_i2c_err_flag = 0x00;
			nim_s3501_open(dev);
		}
	}
	return err;
}
#else
INT32 nim_reg_write(struct nim_device *dev, UINT8 bMemAdr, UINT8 *pData, UINT8 bLen)
{
	int ret;
	u8 buf[1 + bLen];
	struct nim_s3501_private *priv_mem;
	struct i2c_msg i2c_msg = { .addr = dev->base_addr, .flags = 0, .buf = buf, .len = 1 + bLen };
#if defined(NIM_S3501_DEBUG)
	int i;
	for (i = 0; i < bLen; i++)
	{
		printk("%02x ", pData[i]);
	}
	printk("\n");
#endif /* NIM_S3501_DEBUG */
	priv_mem = (struct nim_s3501_private *)dev->priv;
	buf[0] = bMemAdr;
	memcpy(&buf[1], pData, bLen);
	ret = i2c_transfer(priv_mem->i2c_adap, &i2c_msg, 1);
	if (ret != 1)
	{
		if (ret != -ERESTARTSYS)
			printk("Reg=[0x%04x], Data=[0x%02x ...], Count=%u, Status=%d\n",
				   bMemAdr, pData[0], bLen, ret);
		return ret < 0 ? ret : -EREMOTEIO;
	}
	return ret;
}

#endif

//#ifdef NIM_3501_FUNC_EXT
#if 0
void reg_read_verification(struct nim_device *dev)
{
	UINT8 ver_data;
	INT32 i, j, k;
	static UINT8 m_reg_list[192] =
	{
		0x11, 0x00, 0x00, 0x0f, 0x00, 0x5a, 0x50, 0x2f, 0x48, 0x3f, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x83, 0x00, 0x00, 0xec, 0x17, 0x1e,
		0x17, 0xec, 0x6b, 0x8e, 0xb1, 0x28, 0xba, 0x0c, 0x60, 0x00, 0x03, 0x05, 0x00, 0x20, 0x10, 0x04, 0x00, 0x00, 0xe3, 0x88, 0x4f, 0x2d,
		0x68, 0x06, 0x5a, 0x10, 0x77, 0xe5, 0x24, 0xaa, 0x45, 0x87, 0x51, 0x52, 0xba, 0x46, 0x80, 0x3e, 0x25, 0x14, 0x28, 0x3f, 0xc0, 0x00,
		0x02, 0x83, 0x00, 0x00, 0x00, 0x7f, 0x98, 0x48, 0x38, 0x58, 0xc2, 0x12, 0xd2, 0x00, 0x10, 0x27, 0x60, 0xea, 0x71, 0x00, 0x00, 0x32,
		0x10, 0x08, 0x00, 0x73, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0xb0, 0x30, 0x30, 0x59, 0x33, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x27, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x88, 0x50, 0x5a, 0x57, 0x0b, 0x45, 0xc6, 0x41, 0x3c, 0x04, 0x20, 0x70,
		0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x35, 0xa0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0xca, 0x7c, 0x21, 0x04, 0xb4, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00
	};
	for (i = 0; i <= 0xc7; i++)
	{
		if ((0x03 == i) ||
				(0x04 == i) ||
				(0x0b == i) ||
				(0x0c == i) ||
				(0x11 == i) ||
				(0x12 == i) ||
				(0x26 == i) ||
				(0x27 == i) ||
				(0x45 == i) ||
				(0x46 == i) ||
				(0x55 == i) ||
				(0x56 == i) ||
				(0x5a == i) ||
				(0x68 == i) ||
				(0x69 == i) ||
				(0x6a == i) ||
				(0x6b == i) ||
				(0x6c == i) ||
				(0x6d == i) ||
				(0x6e == i) ||
				(0x6f == i) ||
				(0x72 == i) ||
				(0x73 == i) ||
				(0x7d == i) ||
				(0x86 == i) ||
				(0x87 == i) ||
				(0x88 == i) ||
				(0x89 == i) ||
				(0x8a == i) ||
				(0x8b == i) ||
				(0x8c == i) ||
				(0x8d == i) ||
				(0x9d == i) ||
				(0xa3 == i) ||
				(0xa4 == i) ||
				(0xa5 == i) ||
				(0xa6 == i) ||
				(0xaa == i) ||
				(0xab == i) ||
				(0xac == i) ||
				(0xb2 == i) ||
				(0xb1 == i) ||
				(0xb4 == i) ||
				(0xba == i) ||
				(0xbb == i) ||
				(0xbc == i) ||
				(0xc0 == i) ||
				(0xc1 == i) ||
				(0xc4 == i) ||
				(0xc2 == i) ||
				(0xc3 == i))
		{
		}
		else
		{
			//for (j = 0; j < 1000; j++)
			{
				nim_reg_read(dev, i, &ver_data, 1);
				/*for (k = 0; k < 1000; k++)
				{
				 j = j;
				}*/
				if (ver_data != m_reg_list[i])
				{
					printk(" read reg number :0x%x ; ver_data :0x%x ,reg_list_data : 0x%x\n", i, ver_data, m_reg_list[i]);
				}
			}
		}
	}
	printk("finish register read test\n");
	return;
}

void reg_write_verification(struct nim_device *dev)
{
	UINT8 ver_data;
	UINT8 write_data;
	INT32 i;
	for (i = 0x0; i <= 0xc7; i++)
	{
		if ((0x00 == i) ||
				(0xb1 == i) ||
				(0x04 == i) ||
				(0x0b == i) ||
				(0x0c == i) ||
				(0x11 == i) ||
				(0x12 == i) ||
				(0x26 == i) ||
				(0x27 == i) ||
				(0x45 == i) ||
				(0x46 == i) ||
				(0x55 == i) ||
				(0x56 == i) ||
				(0x5a == i) ||
				(0x68 == i) ||
				(0x69 == i) ||
				(0x6a == i) ||
				(0x6b == i) ||
				(0x6c == i) ||
				(0x6d == i) ||
				(0x6e == i) ||
				(0x6f == i) ||
				(0x72 == i) ||
				(0x73 == i) ||
				(0x7d == i) ||
				(0x86 == i) ||
				(0x87 == i) ||
				(0x88 == i) ||
				(0x89 == i) ||
				(0x8a == i) ||
				(0x8b == i) ||
				(0x8c == i) ||
				(0x8d == i) ||
				(0x9d == i) ||
				(0xa3 == i) ||
				(0xa4 == i) ||
				(0xa5 == i) ||
				(0xa6 == i) ||
				(0xaa == i) ||
				(0xab == i) ||
				(0xac == i) ||
				(0xb2 == i) ||
				(0xb3 == i) ||
				(0xb4 == i) ||
				(0xba == i) ||
				(0xbb == i) ||
				(0xbc == i) ||
				(0xc2 == i) ||
				(0xc3 == i) ||
				(0xc4 == i))
		{
		}
		else
		{
			printk(" i=%d\n", i);
			write_data = 0x5a;
			nim_reg_write(dev, i, &write_data, 1);
			nim_reg_read(dev, i, &ver_data, 1);
			if (ver_data != write_data)
			{
				printk(" write reg number :0x%x ; write data : %x; ver_data :0x%x\n", i, write_data, ver_data);
			}
			write_data = 0xa6;
			nim_reg_write(dev, i, &write_data, 1);
			nim_reg_read(dev, i, &ver_data, 1);
			if (ver_data != write_data)
			{
				printk(" write reg number :0x%x ; write data : %x; ver_data :0x%x\n", i, write_data, ver_data);
			}
		}
	}
	printk("reg_write_verfication all finish\n");
	return;
}
#endif

/*****************************************************************************
* INT32 nim_s3501_attach (struct QPSK_TUNER_CONFIG_API * ptrQPSK_Tuner)
* Description: S3501 initialization
*
* Arguments:
* none
*
* Return Value: INT32
*****************************************************************************/
//__attribute__((section(".reuse")))
#if 0
INT32 nim_s3501_attach(struct QPSK_TUNER_CONFIG_API *ptrQPSK_Tuner)
{
	struct nim_device *dev;
	struct nim_s3501_private *priv_mem;
	static unsigned char nim_dev_num = 0;
	if (ptrQPSK_Tuner == NULL)
	{
		printk("Tuner Configuration API structure is NULL!/n");
		return ERR_NO_DEV;
	}
	if (nim_dev_num > 2)
	{
		printk("Can not support three or more S3501 !/n");
		return ERR_NO_DEV;
	}
	dev = (struct nim_device *) dev_alloc(nim_s3501_name[nim_dev_num], HLD_DEV_TYPE_NIM, sizeof(struct nim_device));
	if (dev == NULL)
	{
		printk("Error: Alloc nim device error!\n");
		return ERR_NO_MEM;
	}
	priv_mem = (struct nim_s3501_private *) comm_malloc(sizeof(struct nim_s3501_private));
	if (priv_mem == NULL)
	{
		dev_free(dev);
		printk("Alloc nim device prive memory error!/n");
		return ERR_NO_MEM;
	}
	comm_memset(priv_mem, 0, sizeof(struct nim_s3501_private));
	dev->priv = (void *) priv_mem;
	//diseqc state init
	dev->diseqc_info.diseqc_type = 0;
	dev->diseqc_info.diseqc_port = 0;
	dev->diseqc_info.diseqc_k22 = 0;
	if ((ptrQPSK_Tuner->config_data.QPSK_Config & M3501_POLAR_REVERT) == M3501_POLAR_REVERT) //bit4: polarity revert.
		dev->diseqc_info.diseqc_polar = LNB_POL_V;
	else //default usage, not revert.
		dev->diseqc_info.diseqc_polar = LNB_POL_H;
	dev->diseqc_typex = 0;
	dev->diseqc_portx = 0;
	/* Function point init */
	dev->base_addr = ptrQPSK_Tuner->ext_dm_config.i2c_base_addr;
	dev->init = nim_s3501_attach;
	dev->open = nim_s3501_open;
	dev->stop = nim_s3501_close;
	dev->do_ioctl = nim_s3501_ioctl;
	dev->set_polar = nim_s3501_set_polar;
	dev->set_12v = nim_s3501_set_12v;
	//dev->channel_change = nim_s3501_channel_change;
	dev->do_ioctl_ext = nim_s3501_ioctl_ext;
	dev->channel_search = nim_s3501_channel_search;
	dev->DiSEqC_operate = nim_s3501_DiSEqC_operate;
	dev->DiSEqC2X_operate = nim_s3501_DiSEqC2X_operate;
	dev->get_lock = nim_s3501_get_lock;
	dev->get_freq = nim_s3501_get_freq;
	dev->get_sym = nim_s3501_get_symbol_rate;
	dev->get_FEC = nim_s3501_get_code_rate;
	dev->get_AGC = nim_s3501_get_AGC;
	dev->get_SNR = nim_s3501_get_SNR;
	//dev->get_BER = nim_s3501_get_PER;
	dev->get_BER = nim_s3501_get_BER;
	dev->get_fft_result = nim_s3501_get_fft_result;
	dev->get_ver_infor = NULL;//nim_s3501_get_ver_infor;
	/* tuner configuration function */
	priv_mem->nim_Tuner_Init = ptrQPSK_Tuner->nim_Tuner_Init;
	priv_mem->nim_Tuner_Control = ptrQPSK_Tuner->nim_Tuner_Control;
	priv_mem->nim_Tuner_Status = ptrQPSK_Tuner->nim_Tuner_Status;
	priv_mem->i2c_type_id = ptrQPSK_Tuner->tuner_config.i2c_type_id;
	priv_mem->Tuner_Config_Data.QPSK_Config = ptrQPSK_Tuner->config_data.QPSK_Config;
	priv_mem->ext_dm_config.i2c_type_id = ptrQPSK_Tuner->ext_dm_config.i2c_type_id;
	priv_mem->ext_dm_config.i2c_base_addr = ptrQPSK_Tuner->ext_dm_config.i2c_base_addr;
	priv_mem->ul_status.m_enable_dvbs2_hbcd_mode = 0;
	priv_mem->ul_status.m_dvbs2_hbcd_enable_value = 0x7f;
	priv_mem->ul_status.nim_s3501_sema = OSAL_INVALID_ID;
	priv_mem->ul_status.s3501_autoscan_stop_flag = 0;
	priv_mem->ul_status.s3501_chanscan_stop_flag = 0;
	priv_mem->ul_status.old_ber = 0;
	priv_mem->ul_status.old_per = 0;
	priv_mem->ul_status.m_hw_timeout_thr = 0;
	priv_mem->ul_status.old_ldpc_ite_num = 0;
	priv_mem->ul_status.c_RS = 0;
	priv_mem->ul_status.phase_err_check_status = 0;
	priv_mem->ul_status.s3501d_lock_status = NIM_LOCK_STUS_NORMAL;
	priv_mem->ul_status.m_s3501_type = 0x00;
	priv_mem->ul_status.m_setting_freq = 123;
	priv_mem->ul_status.m_Err_Cnts = 0x00;
	priv_mem->tsk_status.m_lock_flag = NIM_LOCK_STUS_NORMAL;
//// priv_mem->tsk_status.m_task_id = 0x00;
	priv_mem->t_Param.t_aver_snr = -1;
	priv_mem->t_Param.t_last_iter = -1;
	priv_mem->t_Param.t_last_snr = -1;
	priv_mem->t_Param.t_snr_state = 0;
	priv_mem->t_Param.t_snr_thre1 = 256;
	priv_mem->t_Param.t_snr_thre2 = 256;
	priv_mem->t_Param.t_snr_thre3 = 256;
	priv_mem->t_Param.t_dynamic_power_en = 0;
	priv_mem->t_Param.t_phase_noise_detected = 0;
	priv_mem->t_Param.t_reg_setting_switch = 0x0f;
	priv_mem->t_Param.t_i2c_err_flag = 0x00;
	priv_mem->flag_id = OSAL_INVALID_ID;
	/* Add this device to queue */
	if (dev_register(dev) != SUCCESS)
	{
		printk("Error: Register nim device error!\n");
		comm_free(priv_mem);
		dev_free(dev);
		return ERR_NO_DEV;
	}
	nim_dev_num++;
	priv_mem->ul_status.nim_s3501_sema = NIM_MUTEX_CREATE(1);
	if (nim_s3501_i2c_open(dev))
		return S3501_ERR_I2C_NO_ACK;
	// Initial the QPSK Tuner
	if (priv_mem->nim_Tuner_Init != NULL)
	{
		printk(" Initial the Tuner \n");
		if (((struct nim_s3501_private *) dev->priv)->nim_Tuner_Init(&priv_mem->tuner_id, &(ptrQPSK_Tuner->tuner_config)) != SUCCESS)
		{
			printk("Error: Init Tuner Failure!\n");
			if (nim_s3501_i2c_close(dev))
				return S3501_ERR_I2C_NO_ACK;
			return ERR_NO_DEV;
		}
	}
	if (nim_s3501_i2c_close(dev))
		return S3501_ERR_I2C_NO_ACK;
	nim_s3501_ext_lnb_config(dev, ptrQPSK_Tuner);
	nim_s3501_get_type(dev);
	if (priv_mem->ul_status.m_s3501_type == NIM_CHIP_ID_M3501A && // Chip 3501A
			(priv_mem->Tuner_Config_Data.QPSK_Config & 0xc0) == M3501_2BIT_MODE) //TS 2bit mode
	{
		//for M3606+M3501A full nim ssi-2bit patch, auto change to 1bit mode.
		priv_mem->Tuner_Config_Data.QPSK_Config &= 0x3f; // set to TS 1 bit mode
		//libc_printk("M3501A SSI 2bit mode, auto change to 1bit mode\n");
	}
	ptrQPSK_Tuner->device_type = priv_mem->ul_status.m_s3501_type;
	return SUCCESS;
}
#endif //lwj remove
INT32 nim_s3501_hw_check(struct nim_device *dev)
{
	UINT8 data = 0;
	nim_reg_read(dev, RA3_CHIP_ID + 0x01, &data, 1);
	if (data != 0x35)
		return ERR_FAILED;
	else
		return SUCCESS;
}
#if 0 //lwj remove this
static void nim_s3501_set_demap_noise(struct nim_device *dev)
{
	UINT8 data, noise_index;
	UINT16 est_noise;
	//int i;
	// for (i=0;i<32;i++){
	// activate noise
	nim_reg_read(dev, RD0_DEMAP_NOISE_RPT + 2, &data, 1);
	data &= 0xfc;
	nim_reg_write(dev, RD0_DEMAP_NOISE_RPT + 2, &data, 1);
	// set noise_index
	noise_index = 0x0c; // 8psk,3/5.
	nim_reg_write(dev, RD0_DEMAP_NOISE_RPT + 1, &noise_index, 1);
	// set noise
	est_noise = DEMAP_NOISE[noise_index];
	data = est_noise & 0xff;
	nim_reg_write(dev, RD0_DEMAP_NOISE_RPT, &data, 1);
	data = (est_noise >> 8) & 0x3f;
	data |= 0xc0;
	nim_reg_write(dev, RD0_DEMAP_NOISE_RPT + 1, &data, 1);
	// }
}
#endif
void nim_s3501_after_reset_set_param(struct nim_device *dev)
{
	UINT8 data;//,data1;
	int i;
	struct nim_s3501_private *priv = (struct nim_s3501_private *) dev->priv;
	nim_s3501_demod_ctrl(dev, NIM_DEMOD_CTRL_0X91);
	nim_s3501_interrupt_mask_clean(dev);
	data = 0x50; ////0x2B; let AGC lock, try to modify tuner's gain
	nim_reg_write(dev, R0A_AGC1_LCK_CMD, &data, 1);
	if (priv->ul_status.m_s3501_type == NIM_CHIP_ID_M3501B)
	{
		// disable dummy function
		data = 0x00;
		nim_reg_write(dev, RD8_TS_OUT_SETTING, &data, 1);
		//----------------------------------------------------
		// Set demap beta
		for (i = 0; i < 32; i++)
		{
			data = i;
			nim_reg_write(dev, R9C_DEMAP_BETA + 0x02, &data, 1);
			data = MAP_BETA_ACTIVE_BUF[i];
			nim_reg_write(dev, R9C_DEMAP_BETA, &data, 1);
			data = 0x04;
			nim_reg_write(dev, R9C_DEMAP_BETA + 0x03, &data, 1);
			data = MAP_BETA_BUF[i];
			nim_reg_write(dev, R9C_DEMAP_BETA, &data, 1);
			data = 0x03;
			nim_reg_write(dev, R9C_DEMAP_BETA + 0x03, &data, 1);
			// printk (" !!-----set map par %d OK\n",i);
		}
	}
	return ;
}
#define OSAL_INVALID_ID 0

#if 1 //lwj remove , we have another function replace these

/*****************************************************************************
* INT32 nim_s3501_open(struct nim_device *dev)
* Description: S3501 open
*
* Arguments:
* Parameter1: struct nim_device *dev
*
* Return Value: INT32
*****************************************************************************/
static INT32 nim_s3501_open(struct nim_device *dev)
{
	INT32 ret;
	//struct nim_s3501_private *priv = (struct nim_s3501_private *) dev->priv;
	_DEBUG
	printk(" Enter fuction nim_s3501_open\n");
	nim_s3501_set_acq_workmode(dev, NIM_OPTR_HW_OPEN);
	ret = nim_s3501_hw_check(dev);
	if (ret != SUCCESS)
		return ret;
	ret = nim_s3501_hw_init(dev);
	nim_s3501_after_reset_set_param(dev);
	nim_s3501_hbcd_timeout(dev, NIM_OPTR_HW_OPEN);
//	nim_s3501_task_init(dev);//question
#ifdef CHANNEL_CHANGE_ASYNC
	if (priv->flag_id == OSAL_INVALID_ID)
		priv->flag_id = NIM_FLAG_CREATE(0);
#endif
	//printk(" Leave fuction nim_s3501_open\n");
	return SUCCESS;
}

/*****************************************************************************
* INT32 nim_s3501_close(struct nim_device *dev)
* Description: S3501 close
*
* Arguments:
* Parameter1: struct nim_device *dev
*
* Return Value: INT32
*****************************************************************************/
static INT32 nim_s3501_close(struct nim_device *dev)
{
	//UINT8 data,ver_data;
	//struct nim_s3501_private *priv = (struct nim_s3501_private *) dev->priv;////
	nim_s3501_demod_ctrl(dev, NIM_DEMOD_CTRL_0X90);
	nim_s3501_set_acq_workmode(dev, NIM_OPTR_HW_CLOSE);
	//NIM_MUTEX_DELETE(priv->ul_status.nim_s3501_sema);//question
#ifdef CHANNEL_CHANGE_ASYNC
	NIM_FLAG_DEL(priv->flag_id);
#endif
	return SUCCESS;
}

#if 0
static INT32 nim_s3501_tuner_lock(struct nim_device *dev, UINT8 *tun_lock)
{
	struct nim_s3501_private *priv = (struct nim_s3501_private *) dev->priv;
	//UINT8 data;
	/* Setup tuner */
	////NIM_MUTEX_ENTER(priv);
	priv->nim_Tuner_Status(priv->tuner_id, tun_lock);
	////NIM_MUTEX_LEAVE(priv);
	return SUCCESS;
}
#endif /* 0 */
#endif
/*****************************************************************************
* INT32 nim_s3501_set_polar(struct nim_device *dev, UINT8 polar)
* Description: S3501 set polarization
*
* Arguments:
* Parameter1: struct nim_device *dev
* Parameter2: UINT8 pol
*
* Return Value: void
*****************************************************************************/
#if 1 //lwj remove question
//#define REVERT_POLAR
#define NIM_PORLAR_HORIZONTAL 0x00
#define NIM_PORLAR_VERTICAL 0x01

U32 nim_s3501_set_polar(struct nim_device *dev, U8 polar)
{
	UINT8 data = 0;
	struct nim_s3501_private *priv;
	priv = (struct nim_s3501_private *) dev->priv;
	nim_reg_read(dev, R7C_DISEQC_CTRL, &data, 1);
	if ((priv->Tuner_Config_Data.QPSK_Config & M3501_POLAR_REVERT) == 0x00) //not exist H/V polarity revert.
	{
		if (NIM_PORLAR_HORIZONTAL == polar)
		{
			data &= 0xBF;
			//printk("nim_s3501_set_polar CR5C is 00\n");
		}
		else if (NIM_PORLAR_VERTICAL == polar)
		{
			data |= 0x40;
			//printk("nim_s3501_set_polar CR5C is 40\n");
		}
		else
		{
			//printk("nim_s3501_set_polar error\n");
			return 1;
		}
	}
	else//exist H/V polarity revert.
	{
		if (NIM_PORLAR_HORIZONTAL == polar)
		{
			data |= 0x40;
			//printk("nim_s3501_set_polar CR5C is 40\n");
		}
		else if (NIM_PORLAR_VERTICAL == polar)
		{
			data &= 0xBF;
			//printk("nim_s3501_set_polar CR5C is 00\n");
		}
		else
		{
			//printk("nim_s3501_set_polar error\n");
			return 1;
		}
	}
	nim_reg_write(dev, R7C_DISEQC_CTRL, &data, 1);
	return SUCCESS;
}
/*****************************************************************************
* INT32 nim_s3501_set_12v(struct nim_device *dev, UINT8 flag)
* Description: S3501 set LNB votage 12V enable or not
*
* Arguments:
* Parameter1: struct nim_device *dev
* Parameter2: UINT8 flag
*
* Return Value: SUCCESS
*****************************************************************************/
INT32 nim_s3501_set_12v(struct nim_device *dev, UINT8 flag)
{
	return SUCCESS;
}
#endif

static INT32 nim_s3501_set_ssi_clk(struct nim_device *dev, UINT8 bit_rate)
{
	UINT8 data;
	if (bit_rate >= ssi_clock_tab[0])
	{
		// use 135M SSI debug
		nim_reg_read(dev, RDF_TS_OUT_DVBS2, &data, 1);
		data = (data & 0x0f) | 0xf0;
		nim_reg_write(dev, RDF_TS_OUT_DVBS2, &data, 1);
		nim_reg_read(dev, RCE_TS_FMT_CLK, &data, 1);
		data = (data & 0xf3) | 0x08; //135M SSI CLK.
		nim_reg_write(dev, RCE_TS_FMT_CLK, &data, 1);
	}
	else if (bit_rate > ssi_clock_tab[1])
	{
		// use 98M SSI debug
		data = 0x1d; //
		nim_reg_write(dev, RDF_TS_OUT_DVBS2, &data, 1);
	}
	else if (bit_rate > ssi_clock_tab[2])
	{
		// use 90M SSI debug
		data = 0xfd;
		nim_reg_write(dev, RDF_TS_OUT_DVBS2, &data, 1);
		nim_reg_read(dev, RCE_TS_FMT_CLK, &data, 1);
		data = (data & 0xf3) | 0x04; //90M SSI CLK.
		nim_reg_write(dev, RCE_TS_FMT_CLK, &data, 1);
	}
	else if (bit_rate > ssi_clock_tab[3])
	{
		// use 83M SSI debug
		data = 0x0d; //
		nim_reg_write(dev, RDF_TS_OUT_DVBS2, &data, 1);
	}
	else if (bit_rate > ssi_clock_tab[4])
	{
		// use 77M SSI debug
		data = 0x2d; //
		nim_reg_write(dev, RDF_TS_OUT_DVBS2, &data, 1);
	}
	else if (bit_rate > ssi_clock_tab[5])
	{
		// use 72M SSI debug
		data = 0xfd;
		nim_reg_write(dev, RDF_TS_OUT_DVBS2, &data, 1);
		nim_reg_read(dev, RCE_TS_FMT_CLK, &data, 1);
		data = (data & 0xf3) | 0x0c; //72M SSI CLK.
		nim_reg_write(dev, RCE_TS_FMT_CLK, &data, 1);
	}
	else if (bit_rate > ssi_clock_tab[6])
	{
		// use 67M SSI debug
		data = 0x3d;
		nim_reg_write(dev, RDF_TS_OUT_DVBS2, &data, 1);
	}
	else if (bit_rate > ssi_clock_tab[7])
	{
		// use 60M SSI debug
		data = 0x4d;
		nim_reg_write(dev, RDF_TS_OUT_DVBS2, &data, 1);
	}
	else
	{
		// use 54M SSI debug
		data = 0xfd;
		nim_reg_write(dev, RDF_TS_OUT_DVBS2, &data, 1);
		nim_reg_read(dev, RCE_TS_FMT_CLK, &data, 1);
		data = (data & 0xf3) | 0x00; //54M SSI CLK.
		nim_reg_write(dev, RCE_TS_FMT_CLK, &data, 1);
	}
	////printk("clock setting is: %02x \n", data);
	return SUCCESS;
}

static INT32 nim_set_ts_rs(struct nim_device *dev, UINT32 Rs)
{
	UINT8 data;
	UINT32 temp;
	Rs = ((Rs << 10) + 500) / 1000;
	temp = (Rs * 204 + 94) / 188; // rs *2 *204/188
	temp = temp + 1024; // add 1M symbol rate for range.
	data = temp & 0xff;
	nim_reg_write(dev, RDD_TS_OUT_DVBS, &data, 1);
	data = (temp >> 8) & 0xff;
	nim_reg_write(dev, RDD_TS_OUT_DVBS + 0x01, &data, 1);
	nim_reg_read(dev, RDC_EQ_DBG_TS_CFG, &data, 1);
	data = data | 0x10;
	nim_reg_write(dev, RDC_EQ_DBG_TS_CFG, &data, 1);
	////printk("xxx set rs from register file........... \n");
	return SUCCESS;
}

// TS_SSI_SPI_SEL = reg_crd8[0];
// TS_OUT_MODE = reg_crd8[2:1];
// MOCLK_PHASE_OUT = reg_crd8[3];
// TS_SPI_SEL = reg_crd8[4];
// SSI_DEBUG = reg_crd8[5];
// MOCLK_PHASE_SEL = reg_crd8[6];

#ifdef NIM_3501_FUNC_EXT
static INT32 nim_invert_moclk_phase(struct nim_device *dev)
{
	UINT8 data;
	nim_reg_read(dev, RD8_TS_OUT_SETTING, &data, 1);
	data = data ^ 0x08;
	nim_reg_write(dev, RD8_TS_OUT_SETTING, &data, 1);
	return SUCCESS;
}

static INT32 nim_open_ssi_debug(struct nim_device *dev)
{
	UINT8 data;
	nim_reg_read(dev, RD8_TS_OUT_SETTING, &data, 1);
	data = data | 0x20;
	nim_reg_write(dev, RD8_TS_OUT_SETTING, &data, 1);
	return SUCCESS;
}

static INT32 nim_close_ssi_debug(struct nim_device *dev)
{
	UINT8 data;
	nim_reg_read(dev, RD8_TS_OUT_SETTING, &data, 1);
	data = data & 0xdf;
	nim_reg_write(dev, RD8_TS_OUT_SETTING, &data, 1);
	return SUCCESS;
}

static INT32 nim_open_ts_dummy(struct nim_device *dev)
{
	UINT8 data;
	nim_reg_read(dev, RD8_TS_OUT_SETTING, &data, 1);
	data = data | 0x10;
	nim_reg_write(dev, RD8_TS_OUT_SETTING, &data, 1);
	return SUCCESS;
}
#endif

static INT32 nim_close_ts_dummy(struct nim_device *dev)
{
	UINT8 data;
	//struct nim_s3501_private* priv = (struct nim_s3501_private *)dev->priv;
	data = 0x00;
	nim_reg_write(dev, RD8_TS_OUT_SETTING, &data, 1);
	data = 0x40;
	nim_reg_write(dev, RAD_TSOUT_SYMB + 0x01, &data, 1);
	nim_reg_read(dev, RDC_EQ_DBG_TS_CFG, &data, 1);
	data = data & 0x1f;
	nim_reg_write(dev, RDC_EQ_DBG_TS_CFG, &data, 1);
	data = 0xff; // ssi tx debug close
	nim_reg_write(dev, RDF_TS_OUT_DVBS2, &data, 1);
	return SUCCESS;
}
#if 0 ////
#ifdef NIM_3501_FUNC_EXT
static INT32 nim_s3501_set_dmy_format(struct nim_device *dev)
{
	//SW config dummy packet format.
	UINT8 data, tmp;
	//enable ECO_TS_DYM_HEAD_EN = bf[7];
	nim_reg_read(dev, RBF_S2_FEC_DBG, &data, 1);
	data = data | 0x80;
	nim_reg_write(dev, RBF_S2_FEC_DBG, &data, 1);
	//enable ECO_TS_DYM_HEAD_0 = {a2[7:3],a1[7:5]} = 47;
	nim_reg_read(dev, RA0_RXADC_REG + 0x02, &data, 1);
	tmp = TS_DYM_HEAD0 & 0xf8;
	data = (data & 0x07) | tmp;
	nim_reg_write(dev, RA0_RXADC_REG + 0x02, &data, 1);
	nim_reg_read(dev, RA0_RXADC_REG + 0x01, &data, 1);
	tmp = TS_DYM_HEAD0 & 0x07;
	tmp = (tmp << 5) & 0xff;
	data = (data & 0x1f) | tmp;
	nim_reg_write(dev, RA0_RXADC_REG + 0x01, &data, 1);
	//enable ECO_TS_DYM_HEAD_1 = {ce[7:4],a9[1:0],a8[1:0]} = 1f;
	nim_reg_read(dev, RCE_TS_FMT_CLK, &data, 1);
	tmp = TS_DYM_HEAD1 & 0xf0;
	data = (data & 0x0f) | tmp;
	nim_reg_write(dev, RCE_TS_FMT_CLK, &data, 1);
	nim_reg_read(dev, RA9_M180_CLK_DCHAN, &data, 1);
	tmp = TS_DYM_HEAD1 & 0x0c;
	tmp = (tmp >> 2) & 0xff;
	data = (data & 0xfc) | tmp;
	nim_reg_write(dev, RA9_M180_CLK_DCHAN, &data, 1);
	nim_reg_read(dev, RA8_M90_CLK_DCHAN, &data, 1);
	tmp = TS_DYM_HEAD1 & 0x03;
	data = (data & 0xfc) | tmp;
	nim_reg_write(dev, RA8_M90_CLK_DCHAN, &data, 1);
	//enable ECO_TS_DYM_HEAD_2 = {c1[7:2],cc[7:6]} = ff;
	nim_reg_read(dev, RC1_DVBS2_FEC_LDPC, &data, 1);
	tmp = TS_DYM_HEAD2 & 0xfc;
	data = (data & 0x03) | tmp;
	nim_reg_write(dev, RC1_DVBS2_FEC_LDPC, &data, 1);
	nim_reg_read(dev, RCC_STRAP_PIN_CLOCK, &data, 1);
	tmp = TS_DYM_HEAD2 & 0x03;
	tmp = (tmp << 6) & 0xff;
	data = (data & 0x3f) | tmp;
	nim_reg_write(dev, RCC_STRAP_PIN_CLOCK, &data, 1);
	//enable ECO_TS_DYM_HEAD_3 = {cf[7:1],c0[1]} = 10;
	nim_reg_read(dev, RCE_TS_FMT_CLK + 0x01, &data, 1);
	tmp = TS_DYM_HEAD3 & 0xfe;
	data = (data & 0x01) | tmp;
	nim_reg_write(dev, RCE_TS_FMT_CLK + 0x01, &data, 1);
	nim_reg_read(dev, RC0_BIST_LDPC_REG, &data, 1);
	tmp = TS_DYM_HEAD3 & 0x01;
	tmp = (tmp << 1) & 0xff;
	data = (data & 0xfd) | tmp;
	nim_reg_write(dev, RC0_BIST_LDPC_REG, &data, 1);
	//enable ECO_TS_DYM_HEAD_4 = {9f[7:4],92[7:4]} = 00;
	nim_reg_read(dev, R9C_DEMAP_BETA + 0x03, &data, 1);
	tmp = TS_DYM_HEAD4 & 0xf0;
	data = (data & 0x0f) | tmp;
	nim_reg_write(dev, R9C_DEMAP_BETA + 0x03, &data, 1);
	nim_reg_read(dev, R90_DISEQC_CLK_RATIO + 0x02, &data, 1);
	tmp = TS_DYM_HEAD4 & 0x0f;
	tmp = (tmp << 4) & 0xff;
	data = (data & 0x0f) | tmp;
	nim_reg_write(dev, R90_DISEQC_CLK_RATIO + 0x02, &data, 1);
	return SUCCESS;
}
#endif
#endif

#if 0
static INT32 nim_change_ts_gap(struct nim_device *dev, UINT8 gap)
{
	// 0 4 8 16
	// 188 192 196 204
	// 00 01 10 11
	UINT8 data, temp;
	if (gap == 0)
	{
		temp = 0x00;
		printk("Set ts gap 0....\n");
	}
	else if (gap == 4)
	{
		temp = 1 << 1;
		printk("Set ts gap 4....\n");
	}
	else if (gap == 8)
	{
		temp = 2 << 1;
		printk("Set ts gap 8....\n");
	}
	else if (gap == 16)
	{
		temp = 3 << 1;
		printk("Set ts gap 16....\n");
	}
	else
	{
		temp = 3 << 1;
		printk("Nim error: wrong ts gap setting....\n");
	}
	nim_reg_read(dev, RD8_TS_OUT_SETTING, &data, 1);
	data = (data & 0xf9) | temp;
	nim_reg_write(dev, RD8_TS_OUT_SETTING, &data, 1);
	return SUCCESS;
}
#endif

static INT32 nim_s3501_get_bit_rate(struct nim_device *dev, UINT8 work_mode, UINT8 map_type, UINT8 code_rate, UINT32 Rs, UINT8 *bit_rate)
{
	UINT8 data;
	UINT32 temp;
	struct nim_s3501_private *priv = (struct nim_s3501_private *) dev->priv;
	if (work_mode != M3501_DVBS2_MODE) // DVBS mode
	{
		if (code_rate == 0)
			temp = (Rs * 2 * 1 + 1000) / 2000;
		else if (code_rate == 1)
			temp = (Rs * 2 * 2 + 1500) / 3000;
		else if (code_rate == 2)
			temp = (Rs * 2 * 3 + 2000) / 4000;
		else if (code_rate == 3)
			temp = (Rs * 2 * 5 + 3000) / 6000;
		else
			temp = (Rs * 2 * 7 + 4000) / 8000;
		if (temp > 254)
			data = 255;
		else
			data = temp + 1; // add 1 M for margin
		*bit_rate = data;
		////printk("xxx dvbs bit_rate is %d \n", *bit_rate);
		return SUCCESS;
	}
	else //DVBS2 mode
	{
		if (code_rate == 0)
			temp = (Rs * 1 + 2000) / 4000;
		else if (code_rate == 1)
			temp = (Rs * 1 + 1500) / 3000;
		else if (code_rate == 2)
			temp = (Rs * 2 + 2500) / 5000;
		else if (code_rate == 3)
			temp = (Rs * 1 + 1000) / 2000;
		else if (code_rate == 4)
			temp = (Rs * 3 + 2500) / 5000;
		else if (code_rate == 5)
			temp = (Rs * 2 + 1500) / 3000;
		else if (code_rate == 6)
			temp = (Rs * 3 + 2000) / 4000;
		else if (code_rate == 7)
			temp = (Rs * 4 + 2500) / 5000;
		else if (code_rate == 8)
			temp = (Rs * 5 + 3000) / 6000;
		else if (code_rate == 9)
			temp = (Rs * 8 + 4500) / 9000;
		else
			temp = (Rs * 9 + 5000) / 10000;
		if (map_type == 2)
			temp = temp * 2;
		else if (map_type == 3)
			temp = temp * 3;
		else if (map_type == 4)
			temp = temp * 4;
		else
		{
			YWOSTRACE((YWOS_TRACE_ERROR, "Map type error: %02x \n", map_type));
		}
		if ((priv->Tuner_Config_Data.QPSK_Config & M3501_USE_188_MODE) != M3501_USE_188_MODE)
		{
			temp = (temp * 204 + 94) / 188;
		}
		if (temp > 200)
			data = 200;
		else
			data = temp;
		YWOSTRACE((YWOS_TRACE_INFO, "Code rate is: %02x \n", code_rate));
		YWOSTRACE((YWOS_TRACE_INFO, "Map type is: %02x \n", map_type));
		data += 1; // Add 1M
		*bit_rate = data;
		YWOSTRACE((YWOS_TRACE_INFO, "xxx dvbs2 bit_rate is %d \n", *bit_rate));
		return SUCCESS;
	}
}

static INT32 nim_s3501_open_ci_plus(struct nim_device *dev, UINT8 *ci_plus_flag)
{
	UINT8 data;
	struct nim_s3501_private *priv = (struct nim_s3501_private *) dev->priv;
	// For CI plus test.
	if (priv->ul_status.m_s3501_sub_type == NIM_CHIP_SUB_ID_S3501D)
		data = 0x06;
	else
		data = 0x02; // symbol period from reg, 2 cycle
	nim_reg_write(dev, RAD_TSOUT_SYMB, &data, 1);
	YWOSTRACE((YWOS_TRACE_INFO, "open ci plus enable REG_ad = %02x \n", data));
	nim_reg_read(dev, RAD_TSOUT_SYMB + 0x01, &data, 1);
	data = data | 0x80; // enable symbol period from reg
	nim_reg_write(dev, RAD_TSOUT_SYMB + 0x01, &data, 1);
	nim_reg_read(dev, RDC_EQ_DBG_TS_CFG, &data, 1);
	data = data | 0xe0;
	nim_reg_write(dev, RDC_EQ_DBG_TS_CFG, &data, 1);
	nim_reg_read(dev, RDF_TS_OUT_DVBS2, &data, 1);
	data = (data & 0xfc) | 0x01;
	nim_reg_write(dev, RDF_TS_OUT_DVBS2, &data, 1);
	*ci_plus_flag = 1;
	return SUCCESS;
}

static INT32 nim_s3501_set_ts_mode(struct nim_device *dev, UINT8 work_mode, UINT8 map_type, UINT8 code_rate, UINT32 Rs,
								   UINT8 channel_change_flag)
{
	UINT8 data;
	UINT8 bit_rate;
	//UINT32 temp;
	struct nim_s3501_private *priv = (struct nim_s3501_private *) dev->priv;
	UINT8 ci_plus_flag = 0;
	if (priv->ul_status.m_s3501_type == NIM_CHIP_ID_M3501B)
	{
		nim_reg_read(dev, RC0_BIST_LDPC_REG, &data, 1);
		data = data | 0xc0; //1//enable 1bit mode
		nim_reg_write(dev, RC0_BIST_LDPC_REG, &data, 1);
		bit_rate = 0xff;
		nim_s3501_get_bit_rate(dev, work_mode, map_type, code_rate, Rs, &bit_rate);
		/****************TS output config 8bit mode begin*****************************************/
		if ((priv->Tuner_Config_Data.QPSK_Config & 0xc0) == M3501_8BIT_MODE) //8bit mode
		{
			//printk("xxx S3501D output SPI mode \n");
			//printk("xxx work mode is %02x \n", work_mode);
			////printk("xxx bit rate kkk is %d \n", bit_rate);
			if (work_mode != M3501_DVBS2_MODE)// DVBS mode
			{
				if (((bit_rate >= 98) || (bit_rate <= ssi_clock_tab[8])) && channel_change_flag)
				{
					// USE SPI dummy, no SSI debug mode.
					if ((priv->Tuner_Config_Data.QPSK_Config & M3501_USE_188_MODE) == M3501_USE_188_MODE)
						data = 0x10;
					else
						data = 0x16;
					nim_reg_write(dev, RD8_TS_OUT_SETTING, &data, 1);
					////printk("DVBS USE SPI normal dummy, no SSI debug mode \n");
				}
				else
				{
					// USE SSI debug mode with dummy enable.
					nim_reg_read(dev, RDC_EQ_DBG_TS_CFG, &data, 1);
					data = data | 0x20;
					nim_reg_write(dev, RDC_EQ_DBG_TS_CFG, &data, 1);
					nim_s3501_set_ssi_clk(dev, bit_rate);
					if ((priv->Tuner_Config_Data.QPSK_Config & M3501_USE_188_MODE) == M3501_USE_188_MODE)
						data = 0x21;
					else
						data = 0x27;
					nim_reg_write(dev, RD8_TS_OUT_SETTING, &data, 1);
					////printk("DVBS USE SSI debug mode \n");
				}
			}
			else // DVBS2 mode
			{
				// If >98, M3602 need configure DMX clock phase:
				// 0xb8012000 == 0x......AB -> 0x......AA
				if (((bit_rate <= 98) || (bit_rate >= ssi_clock_tab[8])) && channel_change_flag)
				{
					// USE normal SPI
					//#ifdef USE_188_MODE
					if ((priv->Tuner_Config_Data.QPSK_Config & M3501_USE_188_MODE) == M3501_USE_188_MODE)
						data = 0x10;
					else
						data = 0x16; // SPI dummy, no SSI debug mode.
					nim_reg_write(dev, RD8_TS_OUT_SETTING, &data, 1);
					////printk("DVBS2 Enter normal SPI mode, not use SSI debug..\n");
				}
				else
				{
					//use ssi debug to output 8bit spi mode
					nim_s3501_open_ci_plus(dev, &ci_plus_flag);
					nim_s3501_set_ssi_clk(dev, bit_rate);
					//#ifdef USE_188_MODE
					if ((priv->Tuner_Config_Data.QPSK_Config & M3501_USE_188_MODE) == M3501_USE_188_MODE)
						data = 0x21;
					else
						data = 0x27; // enable SSI debug
					nim_reg_write(dev, RD8_TS_OUT_SETTING, &data, 1);
					////printk("DVBS2 Enter SSI debug..\n");
				}
			}
		}
		/****************TS output config 8bit mode end*****************************************/
		/****************TS output config 1bit mode begin*****************************************/
		else if ((priv->Tuner_Config_Data.QPSK_Config & 0xc0) == M3501_1BIT_MODE) //SSI mode
		{
			//////SSI mode
			//printk("xxx S3501D output SSI mode \n");
			//printk("xxx work mode is %02x \n", work_mode);
			if (priv->ul_status.m_s3501_sub_type == NIM_CHIP_SUB_ID_S3501D)
			{
				data = 0x60;
				nim_reg_write(dev, RAD_TSOUT_SYMB + 0x01, &data, 1);
			}
			nim_s3501_set_ssi_clk(dev, bit_rate);
			if (work_mode != M3501_DVBS2_MODE)
			{
				//DVBS mode
				nim_reg_read(dev, RDC_EQ_DBG_TS_CFG, &data, 1);
				data = data | 0x20; //add for ssi_clk change point
				nim_reg_write(dev, RDC_EQ_DBG_TS_CFG, &data, 1);
				////printk("DVBS USE SSI mode \n");
			}
			else
			{
				// DVBS2 mode
				/// If >98, M3602 need configure DMX clock phase:
				// 0xb8012000 == 0x......AB -> 0x......AA
				if (bit_rate < 50)
				{
					//don't need open ci plus function in low bit rate.
					printk("Low bit rate, Close CI plus......................... \n");
				}
				else
				{
					nim_s3501_open_ci_plus(dev, &ci_plus_flag);
				}
				printk("DVBS2 USE SSI mode \n");
			}
			if ((priv->Tuner_Config_Data.QPSK_Config & M3501_USE_188_MODE) == M3501_USE_188_MODE)
				data = 0x01;
			else
				data = 0x07; // enable SSI debug
			nim_reg_write(dev, RD8_TS_OUT_SETTING, &data, 1);
		}
		/****************TS output config 1bit mode end*****************************************/
		/****************TS output config 2bit mode begin*****************************************/
		else if ((priv->Tuner_Config_Data.QPSK_Config & 0xc0) == M3501_2BIT_MODE) //TS 2bit mode
		{
			//TS 2bit mode
			//S3501D's 2bit is very different from M3501B----------------------------------------
			if (priv->ul_status.m_s3501_sub_type == NIM_CHIP_SUB_ID_S3501D)
			{
				if ((priv->Tuner_Config_Data.QPSK_Config & M3501_USE_188_MODE) == M3501_USE_188_MODE)
					data = 0x10;
				else
					data = 0x16;
				nim_reg_write(dev, RD8_TS_OUT_SETTING, &data, 1);
				data = 0x00;
				nim_reg_write(dev, RAD_TSOUT_SYMB + 0x01, &data, 1);
				YWOSTRACE((YWOS_TRACE_INFO, "S3501D Enter 2bit Mode============= \n"));
			}
			else
			{
				//TS 2bit mode
				//ECO_SSI_2B_EN = cr9f[3]
				nim_reg_read(dev, R9C_DEMAP_BETA + 0x03, &data, 1);
				data = data | 0x08;
				nim_reg_write(dev, R9C_DEMAP_BETA + 0x03, &data, 1);
				//ECO_SSI_SEL_2B = crc0[3]
				nim_reg_read(dev, RC0_BIST_LDPC_REG, &data, 1);
				data = data | 0x08; //for 2bit mode
				nim_reg_write(dev, RC0_BIST_LDPC_REG, &data, 1);
				nim_s3501_set_ssi_clk(dev, bit_rate);
				if (work_mode != M3501_DVBS2_MODE)// DVBS mode
				{
					nim_reg_read(dev, RDC_EQ_DBG_TS_CFG, &data, 1);
					data = data | 0x20; //add for ssi_clk change point
					nim_reg_write(dev, RDC_EQ_DBG_TS_CFG, &data, 1);
					YWOSTRACE((YWOS_TRACE_INFO, "Enter DVBS 2bit mode \n"));
				}
				else // DVBS2 mode
				{
					// For CI plus test.
					if (bit_rate < 50)
					{
						//don't need open ci plus function in low bit rate.
						YWOSTRACE((YWOS_TRACE_INFO, "Low bit rate, Close CI plus......................... \n"));
					}
					else
					{
						nim_s3501_open_ci_plus(dev, &ci_plus_flag);
					}
					YWOSTRACE((YWOS_TRACE_INFO, "Enter DVBS2 2bit mode..\n"));
				}
				//no matter bit_rate all use ssi_debug mode
				if ((priv->Tuner_Config_Data.QPSK_Config & M3501_USE_188_MODE) == M3501_USE_188_MODE)
					data = 0x29; //0x21 question
				else
					data = 0x2F; //0x27 question // enable SSI debug
				nim_reg_write(dev, RD8_TS_OUT_SETTING, &data, 1);
			}//end if M3501B 2bit
		}
		/****************TS output config 2bit mode end*****************************************/
		/****************TS output config 4bit mode begin*****************************************/
		else if ((priv->Tuner_Config_Data.QPSK_Config & 0xc0) == M3501_4BIT_MODE) //4//4bit mode
		{
			//TS 4bit mode
			//S3501D's 4bit is very different from M3501B----------------------------------------
			if (priv->ul_status.m_s3501_sub_type == NIM_CHIP_SUB_ID_S3501D)
			{
				if ((priv->Tuner_Config_Data.QPSK_Config & M3501_USE_188_MODE) == M3501_USE_188_MODE)
					data = 0x10;
				else
					data = 0x16; //moclk interv for 4bit.
				nim_reg_write(dev, RD8_TS_OUT_SETTING, &data, 1);
				data = 0x20;
				nim_reg_write(dev, RAD_TSOUT_SYMB + 0x01, &data, 1);
			}
			else
			{
				//TS 4bit mode
				//ECO_SSI_2B_EN = cr9f[3]
				nim_reg_read(dev, R9C_DEMAP_BETA + 0x03, &data, 1);
				data = data | 0x08;
				nim_reg_write(dev, R9C_DEMAP_BETA + 0x03, &data, 1);
				//ECO_SSI_SEL_2B = crc0[3]
				nim_reg_read(dev, RC0_BIST_LDPC_REG, &data, 1);
				data = data & 0xf7; //for 4bit mode
				nim_reg_write(dev, RC0_BIST_LDPC_REG, &data, 1);
				nim_s3501_set_ssi_clk(dev, bit_rate);
				if (work_mode != M3501_DVBS2_MODE)// DVBS mode
				{
					nim_reg_read(dev, RDC_EQ_DBG_TS_CFG, &data, 1);
					data = data | 0x20; //add for ssi_clk change point
					nim_reg_write(dev, RDC_EQ_DBG_TS_CFG, &data, 1);
					//printk("M3501B Enter DVBS 4bit mode \n");
				}
				else // DVBS2 mode
				{
					// For CI plus test.
					if (bit_rate < 50)
					{
						//don't need open ci plus function in low bit rate.
						printk("Low bit rate, Close CI plus......................... \n");
					}
					else
					{
						nim_s3501_open_ci_plus(dev, &ci_plus_flag);
					}
					//printk("M3501B Enter DVBS2 4bit mode..\n");
				}
				//no matter bit_rate all use ssi_debug mode
				if ((priv->Tuner_Config_Data.QPSK_Config & M3501_USE_188_MODE) == M3501_USE_188_MODE)
					data = 0x21;
				else
					data = 0x27; // enable SSI debug
				nim_reg_write(dev, RD8_TS_OUT_SETTING, &data, 1);
			} //end if M3501B 4bit
		}
		if (ci_plus_flag)
		{
			//RST fsm
			data = 0x91;
			nim_reg_write(dev, R00_CTRL, &data, 1);
			data = 0x51;
			nim_reg_write(dev, R00_CTRL, &data, 1);
			//-------------------------------------------------------------------
			//-------------------------------------------------------------------
		}
		/****************TS output config 4bit mode end*****************************************/
		//end M3501B config.
	}
	else
	{
		//M3501A
		if ((priv->Tuner_Config_Data.QPSK_Config & 0xc0) == M3501_1BIT_MODE)
			data = 0x60;
		else if ((priv->Tuner_Config_Data.QPSK_Config & 0xc0) == M3501_2BIT_MODE)
			data = 0x00;
		else if ((priv->Tuner_Config_Data.QPSK_Config & 0xc0) == M3501_4BIT_MODE)
			data = 0x20;
		else if ((priv->Tuner_Config_Data.QPSK_Config & 0xc0) == M3501_8BIT_MODE)
			data = 0x40;
		else
			data = 0x40;
		nim_reg_write(dev, RAD_TSOUT_SYMB + 0x01, &data, 1);
	}
	return SUCCESS;
}
#if 0
#ifdef NIM_3501_FUNC_EXT
static INT32 nim_s3501_get_phase_error(struct nim_device *dev, INT32 *phase_error)
{
	UINT8 rdata = 0;
	UINT8 data = 0;
	nim_reg_read(dev, RC0_BIST_LDPC_REG + 4, &data, 1);
	if (data & 0x80)
	{
		data &= 0x7f;
		nim_reg_write(dev, RC0_BIST_LDPC_REG + 4, &data, 1);
	}
	nim_reg_read(dev, RC0_BIST_LDPC_REG, &data, 1);
	if ((data & 0x04) == 0)
	{
		nim_reg_read(dev, RC0_BIST_LDPC_REG + 5, &rdata, 1);
		data |= 0x04;
		nim_reg_write(dev, RC0_BIST_LDPC_REG, &data, 1);
		if (rdata & 0x80)
			*phase_error = rdata - 256;
		else
			*phase_error = rdata; // phase error is signed!!!
		printk("phase error is %d\n", (*phase_error));
		return SUCCESS; // means phase error measured.
	}
	else
	{
		*phase_error = 0;
		return ERR_FAILUE; // means that phase error is not ready
	}
}
#endif
#endif
static INT32 nim_s3501_set_phase_noise(struct nim_device *dev)
{
	UINT32 debug_time, debug_time_thre, i;
	UINT8 data, verdata, sdat;
	UINT16 snr;
	UINT32 ber, per;
	UINT32 min_per, max_per;
	UINT32 per_buf[4] =
	{
		0, 0, 0, 0
	}; // 0~3: per_ba~per_8a
	UINT32 buf_index = 0;
	struct nim_s3501_private *priv = (struct nim_s3501_private *) dev->priv;
	_DEBUG
	printk(" Eenter function set_phase_noise\n");
	sdat = 0xba;
	nim_reg_write(dev, RB5_CR_PRS_TRA, &sdat, 1);
	debug_time = 0;
	debug_time_thre = 4;
	sdat = 0xba;
	data = 0x00;
	nim_reg_write(dev, R74_PKT_STA_NUM, &data, 1);
	data = 0x10; // M3501B need at least 0x10, or else read 0 only.
	nim_reg_write(dev, R74_PKT_STA_NUM + 0x01, &data, 1);
	for (debug_time = 0; debug_time < debug_time_thre; debug_time++)
	{
		nim_reg_read(dev, R02_IERR, &data, 1);
		if ((data & 0x02) == 0)
		{
			break;
		}
		data = 0x80;
		nim_reg_write(dev, R76_BIT_ERR + 0x02, &data, 1);
		nim_reg_read(dev, R76_BIT_ERR + 0x02, &verdata, 1);
		if (verdata != data)
		{
			_DEBUG
			printk("!!! RESET BER ERROR!\n");
		}
		data = 0x80;
		nim_reg_write(dev, R79_PKT_ERR + 0x01, &data, 1);
		nim_reg_read(dev, R79_PKT_ERR + 0x01, &verdata, 1);
		if (verdata != data)
		{
			_DEBUG
			printk("!!! RESET PER ERROR!\n");
		}
		//comm_delay(100);
		udelay(100);
		nim_s3501_get_SNR(dev, &snr);
		nim_s3501_get_new_BER(dev, &ber);
		nim_s3501_get_new_PER(dev, &per);
		_DEBUG
		printk("--- snr/ber/per = %d/%d/%d\n", snr, ber, per);
		// if (per_buf[buf_index] < per)
		per_buf[buf_index] = per;
		sdat = sdat - 0x10;
		buf_index ++;
		nim_reg_write(dev, RB5_CR_PRS_TRA, &sdat, 1);
		if ((per_buf[buf_index - 2] == 0) && (buf_index >= 2))
			break;
	}
	// }
	min_per = 0;
	max_per = 0;
	for (i = 0; i < buf_index; i++)
	{
		_DEBUG
		printk("per_buf[%d] = 0x%x\n", i, per_buf[i]);
		per_buf[i] >>= 4;
		if (per_buf[i] < per_buf[min_per])
			min_per = i;
		if (per_buf[i] > per_buf[max_per])
		{
			max_per = i;
			if (i > 1)
			{
				break; // if phase noise exist, wider BW should have miner per. At threshod, wider BW may cause unlock, per=0 when unlock
			}
		}
	}
	if (min_per <= max_per)
	{
		priv->t_Param.t_phase_noise_detected = 0;
		_DEBUG
		printk("No phase noise detected!\n");
	}
	else
	{
		priv->t_Param.t_phase_noise_detected = 1;
		_DEBUG
		printk("Phase noise detected!\n");
		data = 0x42;
		nim_reg_write(dev, R33_CR_CTRL + 0x04, &data, 1);
	}
	_DEBUG
	printk("min_per is %d, max_per is %d\n", min_per, max_per);
	if ((min_per < buf_index - 1) && (per_buf[min_per] == per_buf[min_per + 1]))
	{
		if ((per_buf[min_per] > 0) || (snr < 0x10) || (priv->t_Param.t_phase_noise_detected == 0))
			sdat = 0xba - min_per * 0x10;
		else
		{
			sdat = 0xaa - min_per * 0x10;
		}
		nim_reg_write(dev, RB5_CR_PRS_TRA, &sdat, 1);
	}
	else
	{
		sdat = 0xba - min_per * 0x10;
		nim_reg_write(dev, RB5_CR_PRS_TRA, &sdat, 1);
	}
	_DEBUG
	printk("--------------------EXIT set_phase_noise, REG_b5 = 0x%x\n", sdat);
	data = 0x10;
	nim_reg_write(dev, R74_PKT_STA_NUM, &data, 1);
	data = 0x27;
	nim_reg_write(dev, R74_PKT_STA_NUM + 0x01, &data, 1);
	return SUCCESS;
}

#if 1
#define sys_ms_cnt (SYS_CPU_CLOCK/2000)

static INT32 nim_s3501_waiting_channel_lock(/*TUNER_ScanTaskParam_T *Inst,*/
	struct nim_device *dev, UINT32 freq, UINT32 sym)
{
	UINT32 timeout, locktimes = 200;
	UINT32 tempFreq;
	UINT32 Rs;
	////UINT32 starttime = 0;
	////UINT32 endtime;
	UINT8 work_mode, map_type, iqswap_flag, roll_off;
	////UINT8 intindex;
	UINT8 intdata;
	UINT8 code_rate;
	UINT8 data;
	//U32 TuneStartTime;
	struct nim_s3501_private *priv = (struct nim_s3501_private *) dev->priv;
	UINT8 channel_change_flag = 1; //bentao add for judge channel chang or soft_search in set_ts_mode
	timeout = 0;
	if (sym > 40000)
		locktimes = 204;
	else if (sym < 2000)
		locktimes = 2000;
	else if (sym < 4000)
		locktimes = 1604 - sym / 40;
	else if (sym < 6500)
		locktimes = 1004 - sym / 60;
	else
		locktimes = 604 - sym / 100;
	if (priv->ul_status.m_s3501_type == NIM_CHIP_ID_M3501B)
		locktimes *= 2; //lwj change *3 to *2
	else
		locktimes *= 2;
	//TuneStartTime = YWOS_TimeNow();
	locktimes = locktimes / 5; //lwj add
	//printk("locktimes = %d\n", locktimes);
	while (priv->ul_status.s3501_chanscan_stop_flag == 0)
	{
		//if(Inst->ForceSearchTerm)
		//{
		// return SUCCESS;
		//}
		timeout ++ ;
		if (locktimes < timeout) // hardware timeout
		{
			priv->t_Param.phase_noise_detect_finish = 1;
			nim_s3501_clear_int(dev);
			YWOSTRACE((YWOS_TRACE_INFO, " ####### timeout \n"));
			priv->ul_status.s3501_chanscan_stop_flag = 0;
			if (priv->ul_status.m_s3501_type == NIM_CHIP_ID_M3501B)
				priv->tsk_status.m_lock_flag = NIM_LOCK_STUS_SETTING;
			return ERR_FAILED;
		}
		nim_reg_read(dev, R02_IERR, &intdata, 1);
		_DEBUG
		printk("###########R02_IERR intdata = 0x%x\n", intdata);
		//intdata |= 0x02;
		data = 0x02;
		if (0 != (intdata & data))
		{
			tempFreq = freq;
			nim_s3501_reg_get_freq(dev, &tempFreq);
			YWOSTRACE((YWOS_TRACE_INFO, " ######## Freq is %d\n", (LNB_LOACL_FREQ - tempFreq)));
			nim_s3501_reg_get_symbol_rate(dev, &Rs);
			YWOSTRACE((YWOS_TRACE_INFO, " ####### Rs is %d\n", Rs));
			nim_s3501_reg_get_code_rate(dev, &code_rate);
			YWOSTRACE((YWOS_TRACE_INFO, " code_rate is %d\n", code_rate));
			nim_s3501_reg_get_work_mode(dev, &work_mode);
			YWOSTRACE((YWOS_TRACE_INFO, " work_mode is %d\n", work_mode));
			nim_s3501_reg_get_roll_off(dev, &roll_off);
			YWOSTRACE((YWOS_TRACE_INFO, " roll_off is %d\n", roll_off));
			nim_s3501_reg_get_iqswap_flag(dev, &iqswap_flag);
			YWOSTRACE((YWOS_TRACE_INFO, " iqswap_flag is %d\n", iqswap_flag));
			nim_s3501_reg_get_map_type(dev, &map_type);
			YWOSTRACE((YWOS_TRACE_INFO, " map_type is %d\n", map_type));
			if ((priv->ul_status.m_enable_dvbs2_hbcd_mode == 0) && ((map_type == 0) || (map_type == 5)))
			{
				YWOSTRACE((YWOS_TRACE_INFO, " Demod Error: wrong map_type is %d\n", map_type));
			}
			else
			{
				_DEBUG
				printk(" lock chanel \n");
				//printk("[%dms]\n",YWOS_TimeNow()-TuneStartTime);
				//printk(" lock time is %d ms, %d times\n", (endtime - starttime)/sys_ms_cnt,timeout );
				if (work_mode == M3501_DVBS2_MODE)
				{
					data = 0x52;
					nim_reg_write(dev, R33_CR_CTRL + 0x04, &data, 1);
					data = 0xba;
					nim_reg_write(dev, RB5_CR_PRS_TRA, &data, 1);
				} // amy modify for QPSK 2010-3-2
				if (priv->ul_status.m_s3501_type == NIM_CHIP_ID_M3501B)
				{
					if (work_mode != M3501_DVBS2_MODE)// not in DVBS2 mode, key word: power_ctrl
					{
						//slow down S2 clock
						data = 0x77;
						nim_reg_write(dev, R5B_ACQ_WORK_MODE, &data, 1);
						priv->ul_status.phase_err_check_status = 1000;
					}
					priv->tsk_status.m_lock_flag = NIM_LOCK_STUS_CLEAR;
					nim_s3501_set_ts_mode(dev, work_mode, map_type, code_rate, Rs, channel_change_flag);
					//endtime = 0;
					//printk(" channel lock time is %d ms\n", (endtime - starttime)/sys_ms_cnt);
					//open ts
					/*nim_reg_read(dev,RAF_TSOUT_PAD,&data,1);
					data = data & 0xef; // ts open
					nim_reg_write(dev,RAF_TSOUT_PAD, &data, 1);*/
					//ECO_TS_EN = reg_cr9e[7], after dmy config successfully, enable it.
					nim_reg_read(dev, R9C_DEMAP_BETA + 0x02, &data, 1);
					data = data | 0x80;
					nim_reg_write(dev, R9C_DEMAP_BETA + 0x02, &data, 1);
					/*
					 //open ts
					 nim_reg_read(dev,RAF_TSOUT_PAD,&data,1);
					 data = data & 0xef; // ts open
					 nim_reg_write(dev,RAF_TSOUT_PAD, &data, 1);
					*/
					nim_reg_read(dev, R02_IERR, &data, 1);
					data = data & 0x02;
					if (0 != data)
					{
						//Inst->Status = TUNER_STATUS_LOCKED;
						priv->bLock = TRUE;
						priv->ul_status.s3501d_lock_status = NIM_LOCK_STUS_NORMAL; //Carcy modify for unstable lock
						if ((work_mode == M3501_DVBS2_MODE) && (map_type == 3))
							nim_s3501_set_phase_noise(dev);
					}
					else
					{
						priv->ul_status.s3501d_lock_status = NIM_LOCK_STUS_SETTING; //Carcy
					}
				}
				else
				{
					//M3501A
					if (work_mode != M3501_DVBS2_MODE)// not in DVBS2 mode, key word: power_ctrl
					{
						// slow down S2 clock
						data = 0x77;
						nim_reg_write(dev, R5B_ACQ_WORK_MODE, &data, 1);
					}
					else
					{
						if (map_type == 3)
						{
							// S2,8PSK
							nim_s3501_set_phase_noise(dev);
						}
					}
				}
				priv->t_Param.phase_noise_detect_finish = 1;
				if ((work_mode == M3501_DVBS2_MODE) && (map_type == 3) && (priv->t_Param.t_phase_noise_detected == 0))
				{
					// S2, 8PSK
					if (code_rate == 4)
					{
						// coderate3/5
						priv->t_Param.t_snr_thre1 = 30;
						priv->t_Param.t_snr_thre2 = 45;
						priv->t_Param.t_snr_thre3 = 85;
					}
					else if ((code_rate == 5) || (code_rate == 6))
					{
						// coderate2/3,3/4
						priv->t_Param.t_snr_thre1 = 35;
						priv->t_Param.t_snr_thre2 = 55;
					}
					else if (code_rate == 8)
					{
						// coderate5/6
						priv->t_Param.t_snr_thre1 = 55;
						priv->t_Param.t_snr_thre2 = 65;
					}
					else if (code_rate == 9)
					{
						// coderate8/9
						priv->t_Param.t_snr_thre1 = 75;
					}
					else
					{
						// coderate9/10
						priv->t_Param.t_snr_thre1 = 80;
					}
				}
				if ((work_mode == M3501_DVBS2_MODE) && (map_type <= 3)) //only s2 need dynamic power
					priv->t_Param.t_dynamic_power_en = 1;
				/* Keep current frequency.*/
				priv->ul_status.m_CurFreq = tempFreq;
				nim_s3501_clear_int(dev);
				priv->ul_status.s3501_chanscan_stop_flag = 0;
				return SUCCESS;
			}
		}
		else
		{
			//if(Inst->ForceSearchTerm) //lwj add
			//{
			// return SUCCESS;
			//}
			if (priv->ul_status.s3501_chanscan_stop_flag)
			{
				priv->ul_status.s3501_chanscan_stop_flag = 0;
				if (priv->ul_status.m_s3501_type == NIM_CHIP_ID_M3501B)
					priv->tsk_status.m_lock_flag = NIM_LOCK_STUS_SETTING;
				return ERR_FAILED;
			}
			//udelay(200);
			msleep(10); //lwj change 200us to 10ms
		}
	}
	priv->ul_status.s3501_chanscan_stop_flag = 0;
	return SUCCESS;
}
#endif
void nim_s3501_clear_int(struct nim_device *dev)
{
	UINT8 data;
	UINT8 rdata;
	//clear the int
	//CR02
	data = 0x00;
	nim_reg_write(dev, R02_IERR, &data, 1);
	nim_reg_write(dev, R04_STATUS, &data, 1);
	nim_reg_read(dev, R00_CTRL, &rdata, 1);
	data = (rdata | 0x10);
	nim_s3501_demod_ctrl(dev, data);
	//printk(" enter nim_s3501_clear_int\n");
}

#if 0//lwj remove diseqc operate

/*****************************************************************************
* INT32 nim_s3501_DiSEqC_operate(struct nim_device *dev, UINT32 mode, UINT8* cmd, UINT8 cnt)
*
* defines DiSEqC operations
*
* Arguments:
* Parameter1: struct nim_device *dev
* Parameter2: UINT32 mode
* Parameter3: UINT8* cmd
* Parameter4: UINT8 cnt
*
* Return Value: void
*****************************************************************************/
#define NIM_DISEQC_MODE_22KOFF 0 /* 22kHz off */
#define NIM_DISEQC_MODE_22KON 1 /* 22kHz on */
#define NIM_DISEQC_MODE_BURST0 2 /* Burst mode, on for 12.5mS = 0 */
#define NIM_DISEQC_MODE_BURST1 3 /* Burst mode, modulated 1:2 for 12.5mS = 1 */
#define NIM_DISEQC_MODE_BYTES 4 /* Modulated with bytes from DISEQC INSTR */
#define NIM_DISEQC_MODE_ENVELOP_ON 5 /* Envelop enable*/
#define NIM_DISEQC_MODE_ENVELOP_OFF 6 /* Envelop disable, out put 22K wave form*/
#define NIM_DISEQC_MODE_OTHERS 7 /* Undefined mode */
#define NIM_DISEQC_MODE_BYTES_EXT_STEP1 8 /*Split NIM_DISEQC_MODE_BYTES to 2 steps to improve the speed,*/
#define NIM_DISEQC_MODE_BYTES_EXT_STEP2 9 /*(30ms--->17ms) to fit some SPEC */

static INT32 nim_s3501_DiSEqC_operate(struct nim_device *dev, UINT32 mode, UINT8 *cmd, UINT8 cnt)
{
	UINT8 data, temp;
	UINT16 timeout, timer;
	UINT8 i;
	struct nim_s3501_private *priv = (struct nim_s3501_private *) dev->priv;
	printk("mode = 0x%d\n", mode);
	if (priv->ul_status.m_s3501_type == NIM_CHIP_ID_M3501B)
		data = (CRYSTAL_FREQ * 90 / 135);
	else
		data = (CRYSTAL_FREQ * 99 / 135);
	nim_reg_write(dev, R7C_DISEQC_CTRL + 0x14, &data, 1);
	switch (mode)
	{
		case NIM_DISEQC_MODE_22KOFF:
		case NIM_DISEQC_MODE_22KON:
			nim_reg_read(dev, R7C_DISEQC_CTRL, &data, 1);
			data = ((data & 0xF8) | mode);
			nim_reg_write(dev, R7C_DISEQC_CTRL, &data, 1);
			break;
		case NIM_DISEQC_MODE_BURST0:
			nim_reg_read(dev, R7C_DISEQC_CTRL, &data, 1);
			//tone burst 0
			temp = 0x02;
			data = ((data & 0xF8) | temp);
			nim_reg_write(dev, R7C_DISEQC_CTRL, &data, 1);
			msleep(16);
			break;
		case NIM_DISEQC_MODE_BURST1:
			nim_reg_read(dev, R7C_DISEQC_CTRL, &data, 1);
			//tone bust 1
			temp = 0x03;
			data = ((data & 0xF8) | temp);
			nim_reg_write(dev, R7C_DISEQC_CTRL, &data, 1);
			msleep(16);
			break;
		case NIM_DISEQC_MODE_BYTES:
			//diseqc init for mode byte every time :test
			nim_reg_read(dev, R7C_DISEQC_CTRL, &data, 1);
			data = ((data & 0xF8) | 0x00);
			nim_reg_write(dev, R7C_DISEQC_CTRL, &data, 1);
			//write the writed data count
			nim_reg_read(dev, R7C_DISEQC_CTRL, &data, 1);
			temp = cnt - 1;
			data = ((data & 0xC7) | (temp << 3));
			nim_reg_write(dev, R7C_DISEQC_CTRL, &data, 1);
			//write the data
			for (i = 0; i < cnt; i++)
			{
				nim_reg_write(dev, (i + 0x7E), cmd + i, 1);
			}
			//clear the interupt
			nim_reg_read(dev, R7C_DISEQC_CTRL + 0x01, &data, 1);
			data &= 0xF8;
			nim_reg_write(dev, R7C_DISEQC_CTRL + 0x01, &data, 1);
			//write the control bits
			nim_reg_read(dev, R7C_DISEQC_CTRL, &data, 1);
			temp = 0x04;
			data = ((data & 0xF8) | temp);
			nim_reg_write(dev, R7C_DISEQC_CTRL, &data, 1);
			//waiting for the send over
			timer = 0;
			timeout = 75 + 13 * cnt;
			while (timer < timeout)
			{
				nim_reg_read(dev, R7C_DISEQC_CTRL + 0x01, &data, 1);
				//change < to != by Joey
				if (0 != (data & 0x07))
				{
					break;
				}
				msleep(10);
				timer += 10;
			}
			if (1 == (data & 0x07))
			{
				//osal_task_sleep(2000);
				return SUCCESS;
			}
			else
			{
				return ERR_FAILED;
			}
			break;
		case NIM_DISEQC_MODE_BYTES_EXT_STEP1:
			//diseqc init for mode byte every time :test
			nim_reg_read(dev, R7C_DISEQC_CTRL, &data, 1);
			data = ((data & 0xF8) | 0x00);
			nim_reg_write(dev, R7C_DISEQC_CTRL, &data, 1);
			//write the writed data count
			nim_reg_read(dev, R7C_DISEQC_CTRL, &data, 1);
			temp = cnt - 1;
			data = ((data & 0xC7) | (temp << 3));
			nim_reg_write(dev, R7C_DISEQC_CTRL, &data, 1);
			//write the data
			for (i = 0; i < cnt; i++)
			{
				nim_reg_write(dev, (i + 0x7E), cmd + i, 1);
			}
			//clear the interupt
			nim_reg_read(dev, R7C_DISEQC_CTRL + 0x01, &data, 1);
			data &= 0xF8;
			nim_reg_write(dev, R7C_DISEQC_CTRL + 0x01, &data, 1);
			break;
		case NIM_DISEQC_MODE_BYTES_EXT_STEP2:
			//write the control bits
			nim_reg_read(dev, R7C_DISEQC_CTRL, &data, 1);
			temp = 0x04;
			data = ((data & 0xF8) | temp);
			nim_reg_write(dev, R7C_DISEQC_CTRL, &data, 1);
			//waiting for the send over
			timer = 0;
			timeout = 75 + 13 * cnt;
			while (timer < timeout)
			{
				nim_reg_read(dev, R7C_DISEQC_CTRL + 0x01, &data, 1);
				//change < to != by Joey
				if (0 != (data & 0x07))
				{
					break;
				}
				msleep(10);
				timer += 10;
			}
			if (1 == (data & 0x07))
			{
				//osal_task_sleep(2000);
				return SUCCESS;
			}
			else
			{
				return ERR_FAILED;
			}
			break;
		case NIM_DISEQC_MODE_ENVELOP_ON:
		{
			nim_reg_read(dev, R24_MATCH_FILTER, &data, 1);
			data |= 0x01;
			nim_reg_write(dev, R24_MATCH_FILTER, &data, 1);
		}
		break;
		case NIM_DISEQC_MODE_ENVELOP_OFF:
		{
			nim_reg_read(dev, R24_MATCH_FILTER, &data, 1);
			data &= 0xFE;
			nim_reg_write(dev, R24_MATCH_FILTER, &data, 1);
		}
		break;
		default :
			break;
	}
	return SUCCESS;
}
#endif

//lwj add begin
#if 0
//lwj add end
/*****************************************************************************
* INT32 nim_s3501_DiSEqC2X_operate(struct nim_device *dev, UINT32 mode, UINT8* cmd, UINT8 cnt, \
* UINT8 *rt_value, UINT8 *rt_cnt)
*
* defines DiSEqC 2.X operations
*
* Arguments:
* Parameter1: struct nim_device *dev
* Parameter2: UINT32 mode
* Parameter3: UINT8* cmd
* Parameter4: UINT8 cnt
* Parameter5: UINT8 *rt_value
* Parameter6: UINT8 *rt_cnt
*
* Return Value: Operation result.
*****************************************************************************/
#define DISEQC2X_ERR_NO_REPLY 0x01
#define DISEQC2X_ERR_REPLY_PARITY 0x02
#define DISEQC2X_ERR_REPLY_UNKNOWN 0x03
#define DISEQC2X_ERR_REPLY_BUF_FUL 0x04
static INT32 nim_s3501_DiSEqC2X_operate(struct nim_device *dev, UINT32 mode, UINT8 *cmd, UINT8 cnt, UINT8 *rt_value, UINT8 *rt_cnt)
{
	INT32 result;//,temp1,val_flag,reg_tmp;
	UINT8 data, temp, rec_22k = 0;//,ret;
	UINT16 timeout, timer;
	UINT8 i;
	//cmd[0] =cmd[1] =cmd[2] = 0xe2;
	switch (mode)
	{
		case NIM_DISEQC_MODE_BYTES:
		{
			// set unconti timer
			data = 0x40;
			nim_reg_write(dev, RBA_AGC1_REPORT, &data, 1);
			//set receive timer: 0x88 for default;
			data = 0x88;
			nim_reg_write(dev, R8E_DISEQC_TIME, &data, 1);
			data = 0xff;
			nim_reg_write(dev, R8E_DISEQC_TIME + 0x01, &data, 1);
			//set clock ratio: 90MHz is 0x5a;
			data = 0x64;
			nim_reg_write(dev, R90_DISEQC_CLK_RATIO, &data, 1);
			nim_reg_read(dev, R90_DISEQC_CLK_RATIO, &data, 1);
			//set min pulse width
			nim_reg_read(dev, R90_DISEQC_CLK_RATIO + 0x01, &data, 1);
			data = ((data & 0xf0) | (0x2));
			nim_reg_write(dev, R90_DISEQC_CLK_RATIO + 0x01, &data, 1);
		}
			//write the data to send buffer
		for (i = 0; i < cnt; i++)
		{
			data = cmd[i];
			nim_reg_write(dev, (i + R7C_DISEQC_CTRL + 0x02), &data, 1);
		}
			//set diseqc data counter
		temp = cnt - 1;
		nim_reg_read(dev, R7C_DISEQC_CTRL, &data, 1);
		data = ((data & 0x47) | (temp << 3));
		nim_reg_write(dev, R7C_DISEQC_CTRL, &data, 1);
			//enable diseqc interrupt mask event bit.
		nim_reg_read(dev, R03_IMASK, &data, 1);
		data |= 0x80;
		nim_reg_write(dev, R03_IMASK, &data, 1);
			//clear co-responding diseqc interrupt event bit.
		nim_reg_read(dev, R02_IERR, &data, 1);
		data &= 0x7f;
		nim_reg_write(dev, R02_IERR, &data, 1);
			//write the control bits, need reply
		nim_reg_read(dev, R7C_DISEQC_CTRL, &data, 1);
		temp = 0x84;
		data = ((data & 0x78) | temp);
		nim_reg_write(dev, R7C_DISEQC_CTRL, &data, 1);
			//waiting for the send over
		timer = 0;
		timeout = 75 + 13 * cnt + 200; //200 for reply time margin.
		data = 0;
			//check diseqc interrupt state.
		while (timer < timeout)
		{
			nim_reg_read(dev, R02_IERR, &data, 1);
			if (0x80 == (data & 0x80)) //event happen.
			{
				break;
			}
			msleep(10);
			timer += 1;
		}
			//init value for error happens.
		result = ERR_FAILED;
		rt_value[0] = DISEQC2X_ERR_NO_REPLY;
		*rt_cnt = 0;
		if (0x80 == (data & 0x80)) //event happen. //else, error occur.
		{
			nim_reg_read(dev, R7C_DISEQC_CTRL + 0x01, &data, 1);
			switch (data & 0x07)
			{
				case 1:
					*rt_cnt = (UINT8)((data >> 4) & 0x0f);
					if (*rt_cnt > 0)
					{
						for (i = 0; i < *rt_cnt; i++)
						{
							nim_reg_read(dev, (i + R86_DISEQC_RDATA), (rt_value + i), 1);
						}
						result = SUCCESS;
					}
					break;
				case 2:
					rt_value[0] = DISEQC2X_ERR_NO_REPLY;
					break;
				case 3:
					rt_value[0] = DISEQC2X_ERR_REPLY_PARITY;
					break;
				case 4:
					rt_value[0] = DISEQC2X_ERR_REPLY_UNKNOWN;
					break;
				case 5:
					rt_value[0] = DISEQC2X_ERR_REPLY_BUF_FUL;
					break;
				default:
					rt_value[0] = DISEQC2X_ERR_NO_REPLY;
					break;
			}
		}
		if ((rec_22k & 0x07) <= 0x01) //set 22k and polarity by origianl value; other-values are not care.
			nim_reg_write(dev, R7C_DISEQC_CTRL, &rec_22k, 1);
		return result;
		default :
			break;
	}
	nim_reg_read(dev, R7C_DISEQC_CTRL, &data, 1);
	nim_reg_read(dev, R7C_DISEQC_CTRL + 0x01, &data, 1);
	msleep(1000);
	return SUCCESS;
}
#endif
/*****************************************************************************
* INT32 nim_s3501_get_lock(struct nim_device *dev, UINT8 *lock)
*
* Read FEC lock status
*
*Arguments:
* Parameter1: struct nim_device *dev
* Parameter2: BOOL *fec_lock
*
*Return Value: INT32
*****************************************************************************/
static INT32 nim_s3501_get_lock(struct nim_device *dev, UINT8 *lock)
{
	UINT8 data;
	UINT8 h8psk_lock;
	////comm_delay(150);
	udelay(150);
	nim_reg_read(dev, R04_STATUS, &data, 1);
	if ((data & 0x80) == 0x80)
		h8psk_lock = 1;
	else
		h8psk_lock = 0;
	if (h8psk_lock & ((data & 0x2f) == 0x2f))
	{
		*lock = 1;
	}
	else if ((data & 0x3f) == 0x3f)
	{
		*lock = 1;
	}
	else
	{
		*lock = 0;
	}
	////comm_delay(150);
	udelay(150);
	return YW_NO_ERROR;
}

#if 0
/*****************************************************************************
* INT32 nim_s3501_get_freq(struct nim_device *dev, UINT32 *freq)
* Read S3501 frequence
*
* Arguments:
* Parameter1: struct nim_device *dev
* Parameter2: UINT16 *sym_rate : Symbol rate in kHz
*
* Return Value: void
*****************************************************************************/
static INT32 nim_s3501_get_freq(struct nim_device *dev, UINT32 *freq)
{
	nim_s3501_reg_get_freq(dev, freq);
	return SUCCESS;
}

/*****************************************************************************
* INT32 nim_s3501_get_symbol_rate(struct nim_device *dev, UINT32 *sym_rate)
* Read S3501 symbol rate
*
* Arguments:
* Parameter1: struct nim_device *dev
* Parameter2: UINT16 *sym_rate : Symbol rate in kHz
*
* Return Value: void
*****************************************************************************/
static INT32 nim_s3501_get_symbol_rate(struct nim_device *dev, UINT32 *sym_rate)
{
	nim_s3501_reg_get_symbol_rate(dev, sym_rate);
	return SUCCESS;
}
/*****************************************************************************
* INT32 nim_s3501_get_code_rate(struct nim_device *dev, UINT8* code_rate)
* Description: Read S3501 code rate
*
* Arguments:
* Parameter1: struct nim_device *dev
* Parameter2: UINT8* code_rate
*
* Return Value: INT32
*****************************************************************************/
static INT32 nim_s3501_get_code_rate(struct nim_device *dev, UINT8 *code_rate)
{
	nim_s3501_reg_get_code_rate(dev, code_rate);
	return SUCCESS;
}
#endif

/*****************************************************************************
* INT32 nim_s3501_get_AGC(struct nim_device *dev, UINT8 *agc)
*
* This function will access the NIM to determine the AGC feedback value
*
* Arguments:
* Parameter1: struct nim_device *dev
* Parameter2: UINT16* agc
*
* Return Value: INT32
*****************************************************************************/
// get signal intensity

static INT32 nim_s3501_get_AGC(struct nim_device *dev, UINT16 *agc)
{
	UINT8 data;//, temp;
	//INT16 idata;
	//UINT8 lock;
	struct nim_s3501_private *priv = (struct nim_s3501_private *) dev->priv;
	if (priv->Tuner_Config_Data.QPSK_Config & M3501_SIGNAL_DISPLAY_LIN)
	{
		nim_reg_read(dev, R04_STATUS, &data, 1);
		if (data & 0x01)
		{
			// AGC1 lock
			nim_reg_read(dev, R0A_AGC1_LCK_CMD + 0x01, &data, 1); // read AGC1 gain
			if (data > 0x7f)
				*agc = data - 0x80;
			else
				*agc = data + 0x80;
			return SUCCESS;
		}
		else
			*agc = 0;
	}
	else
	{
		//CR0B
		nim_reg_read(dev, R07_AGC1_CTRL + 0x04, &data, 1);
#if 0 // ????
		data = 255 - data;
		if (0x40 <= data)
			data -= 0x40;
		else if ((0x20 <= data) || (0x40 > data))
			data -= 0x20;
		else
			data -= 0;
		data /= 2;
		data += 16;
#endif // 0
		*agc = (UINT8) data;
	}
	return SUCCESS;
}

/*****************************************************************************
* INT32 nim_s3501_get_BER(struct nim_device *dev, UINT32 *RsUbc)
* Get bit error ratio
*
* Arguments:
* Parameter1: struct nim_device *dev
* Parameter2: UINT16* RsUbc
*
* Return Value: INT32
*****************************************************************************/
#if 0
#ifdef NIM_3501_FUNC_EXT

static INT32 nim_s3501_s2_get_BER(struct nim_device *dev, UINT32 *RsUbc)
{
	UINT8 data, rdata;
	UINT8 ber_data[3];
	UINT32 u32ber_data[3];
	UINT32 uber_data;
	//struct nim_s3501_private* priv = (struct nim_s3501_private *)dev->priv;
	// read single LDPC BER information
	nim_reg_read(dev, RD3_BER_REG, &rdata, 1);
	data = rdata & 0x7b;
	nim_reg_write(dev, RD3_BER_REG, &data, 1);
	nim_reg_read(dev, RD3_BER_REG + 0x01, &ber_data[0], 1);
	u32ber_data[0] = (UINT32) ber_data[0];
	nim_reg_read(dev, RD3_BER_REG + 0x01, &ber_data[1], 1);
	u32ber_data[1] = (UINT32) ber_data[1];
	u32ber_data[1] <<= 8;
	uber_data = u32ber_data[1] + u32ber_data[0];
	*RsUbc = uber_data;
	return SUCCESS;
}
#endif

#ifdef NIM_3501_FUNC_EXT
static INT32 nim_get_symbol(struct nim_device *dev)
{
	UINT8 data;
	UINT32 i;
	for (i = 0; i < 5000; i++)
	{
		data = 0xc1;
		nim_reg_write(dev, RC8_BIST_TOLERATOR, &data, 1);
		nim_reg_read(dev, RC9_CR_OUT_IO_RPT, &data, 1);
		printk("%02x", data);
		nim_reg_read(dev, RC9_CR_OUT_IO_RPT + 0x01, &data, 1);
		printk("%02x\n", data);
	}
	// set default value
	data = 0xc0;
	nim_reg_write(dev, RC8_BIST_TOLERATOR, &data, 1);
}
#endif
#endif
/*****************************************************************************
* INT32 nim_s3501_get_SNR(struct nim_device *dev, UINT8 *snr)
*
* This function returns an approximate estimation of the SNR from the NIM
* The Eb No is calculated using the SNR from the NIM, using the formula:
* Eb ~ 13312- M_SNR_H
* -- = ---------------- dB.
* NO 683
*
* Arguments:
* Parameter1: struct nim_device *dev
* Parameter2: UINT16* RsUbc
*
* Return Value: INT32
*****************************************************************************/
static INT32 nim_s3501_get_SNR(struct nim_device *dev, UINT16 *snr)
{
	//UINT8 work_mode;//, map_type;
	UINT8 lock; //coderate,
	//UINT32 Rs;
	UINT8 data;
	UINT32 tdata, iter_num; //ber, per,
	int i, total_iter, sum;
	struct nim_s3501_private *priv = (struct nim_s3501_private *) dev->priv;
	if (priv->Tuner_Config_Data.QPSK_Config & M3501_SIGNAL_DISPLAY_LIN)
	{
#define TDATA_SUM_LIN 6
		nim_reg_read(dev, R04_STATUS, &data, 1);
		if (data & 0x08)
		{
			// CR lock
			sum = 0;
			total_iter = 0;
			for (i = 0; i < TDATA_SUM_LIN; i++)
			{
				nim_reg_read(dev, R45_CR_LCK_DETECT + 0x01, &data, 1);
				tdata = data << 8;
				nim_reg_read(dev, R45_CR_LCK_DETECT, &data, 1);
				tdata |= data;
				if (tdata & 0x8000)
					tdata = 0x10000 - tdata;
				nim_s3501_get_LDPC(dev, &iter_num);
				total_iter += iter_num;
				tdata >>= 5;
				sum += tdata;
			}
			sum /= TDATA_SUM_LIN;
			total_iter /= TDATA_SUM_LIN;
			sum *= 3;
			sum /= 5;
			if (sum > 255)
				sum = 255;
			if (priv->t_Param.t_last_snr == -1)
			{
				*snr = sum;
				priv->t_Param.t_last_snr = *snr;
			}
			else
			{
				if (total_iter == priv->t_Param.t_last_iter)
				{
					*snr = priv->t_Param.t_last_snr;
					if (sum > priv->t_Param.t_last_snr)
						priv->t_Param.t_last_snr ++;
					else if (sum < priv->t_Param.t_last_snr)
						priv->t_Param.t_last_snr --;
				}
				else if ((total_iter > priv->t_Param.t_last_iter) && (sum < priv->t_Param.t_last_snr))
				{
					*snr = sum;
					priv->t_Param.t_last_snr = sum;
				}
				else if ((total_iter < priv->t_Param.t_last_iter) && (sum > priv->t_Param.t_last_snr))
				{
					*snr = sum;
					priv->t_Param.t_last_snr = sum;
				}
				else
				{
					*snr = priv->t_Param.t_last_snr;
				}
			}
			priv->t_Param.t_last_iter = total_iter;
		}
		else
		{
			// CR unlock
			*snr = 0;
			priv->t_Param.t_last_snr = -1;
			priv->t_Param.t_last_iter = -1;
		}
	}
	else
	{
#define TDATA_START 7
#define TDATA_SUM 4
		tdata = 0;
		for (i = 0; i < TDATA_SUM; i++)
		{
			data = snr_tab[nim_s3501_get_SNR_index(dev)];
			tdata += data;
		}
		tdata /= TDATA_SUM;
		//CR04
		nim_reg_read(dev, R04_STATUS, &data, 1);
		if ((0x3F == data) | (0x7f == data) | (0xaf == data))
		{
		}
		else
		{
#if (defined(SIGNAL_INDICATOR) && (SIGNAL_INDICATOR == 3))
			//To have linear signal indicator change between lock and unlock per Hicway's request
			//--Michael Xie 2005/8/17
			tdata = 0;
			if (0x01 == (data & 0x01)) //agc1
				tdata += 2;
			if (0x02 == (data & 0x02)) //agc2
				tdata += 10;
			if (0x04 == (data & 0x04)) //cr
				tdata += 16;
			if (0x08 == (data & 0x08)) //tr
				tdata += 16;
			if (0x10 == (data & 0x10)) //frame sync
				tdata += 20;
			if (0x20 == (data & 0x20))
				tdata += 26;
			if (0x40 == (data & 0x40))
				tdata += 26;
			if (0x80 == (data & 0x80))
				tdata += 26;
#else
			//tdata /= 2;
			tdata = TDATA_START;
			if (0x01 == (data & 0x01))
				tdata += 1;
			if (0x02 == (data & 0x02))
				tdata += 3;
			if (0x04 == (data & 0x04))
				tdata += 3;
			if (0x08 == (data & 0x08))
				tdata += 2;
			if (0x10 == (data & 0x10))
				tdata += 2;
			if (0x20 == (data & 0x20))
				tdata += 2;
			if (0x40 == (data & 0x40))
				tdata += 2;
			if (0x80 == (data & 0x80))
				tdata += 2;
#endif
		}
		*snr = tdata / 2;
	}
	if (priv->t_Param.t_aver_snr == -1)
	{
		priv->t_Param.t_aver_snr = (*snr);
	}
	else
	{
		priv->t_Param.t_aver_snr += (((*snr) - priv->t_Param.t_aver_snr) >> 2);
	}
	// no phase noise and actived after channel lock
	// if moniter_phase_noise default is 0, this function may before set_phase_noise!!!!!
	// the threshods are affected by coderate
	nim_s3501_get_lock(dev, &lock);
	if ((lock) && (priv->t_Param.t_phase_noise_detected == 0) && (priv->t_Param.phase_noise_detect_finish == 1))
	{
		if (priv->t_Param.t_snr_state == 0)
		{
			priv->t_Param.t_snr_state = 1;
			data = 0x52;
			nim_reg_write(dev, R33_CR_CTRL + 0x04, &data, 1);
			data = 0xba;
			nim_reg_write(dev, RB5_CR_PRS_TRA, &data, 1);
			YWOSTRACE((YWOS_TRACE_INFO, "@@@@initial snr state = 1, reg37 = 0x52, regb5 = 0xba;\n"));
		}
		if ((priv->t_Param.t_snr_state == 1) && (priv->t_Param.t_aver_snr > priv->t_Param.t_snr_thre1))
		{
			data = 0x4e;
			nim_reg_write(dev, R33_CR_CTRL + 0x04, &data, 1);
			data = 0xaa;
			nim_reg_write(dev, RB5_CR_PRS_TRA, &data, 1);
			YWOSTRACE((YWOS_TRACE_INFO, "snr state = 2, reg37 = 0x4e, regb5 = 0xaa;\n"));
			priv->t_Param.t_snr_state = 2;
		}
		else if (priv->t_Param.t_snr_state == 2)
		{
			if (priv->t_Param.t_aver_snr > priv->t_Param.t_snr_thre2)
			{
				data = 0x42;
				nim_reg_write(dev, R33_CR_CTRL + 0x04, &data, 1);
				data = 0x9a;
				nim_reg_write(dev, RB5_CR_PRS_TRA, &data, 1);
				YWOSTRACE((YWOS_TRACE_INFO, "snr state = 3, reg37 = 0x42, regb5 = 0x9a;\n"));
				priv->t_Param.t_snr_state = 3;
			}
			else if (priv->t_Param.t_aver_snr < (priv->t_Param.t_snr_thre1 - 5))
			{
				data = 0x52;
				nim_reg_write(dev, R33_CR_CTRL + 0x04, &data, 1);
				data = 0xba;
				nim_reg_write(dev, RB5_CR_PRS_TRA, &data, 1);
				YWOSTRACE((YWOS_TRACE_INFO, "snr state = 1, reg37 = 0x52, regb5 = 0xba;\n"));
				priv->t_Param.t_snr_state = 1;
			}
		}
		else if (priv->t_Param.t_snr_state == 3)
		{
			if (priv->t_Param.t_aver_snr > priv->t_Param.t_snr_thre3)
			{
				data = 0x3e;
				nim_reg_write(dev, R33_CR_CTRL + 0x04, &data, 1);
				data = 0x8a;
				nim_reg_write(dev, RB5_CR_PRS_TRA, &data, 1);
				YWOSTRACE((YWOS_TRACE_INFO, "snr state = 4, reg37 = 0x3e, regb5 = 0x8a;\n"));
				priv->t_Param.t_snr_state = 4;
			}
			else if (priv->t_Param.t_aver_snr < (priv->t_Param.t_snr_thre2 - 5))
			{
				data = 0x4e;
				nim_reg_write(dev, R33_CR_CTRL + 0x04, &data, 1);
				data = 0xaa;
				nim_reg_write(dev, RB5_CR_PRS_TRA, &data, 1);
				YWOSTRACE((YWOS_TRACE_INFO, "snr state = 2, reg37 = 0x4e, regb5 = 0xaa;\n"));
				priv->t_Param.t_snr_state = 2;
			}
		}
		else if (priv->t_Param.t_snr_state == 4)
		{
			if (priv->t_Param.t_aver_snr < (priv->t_Param.t_snr_thre3 - 5))
			{
				data = 0x42;
				nim_reg_write(dev, R33_CR_CTRL + 0x04, &data, 1);
				data = 0x9a;
				nim_reg_write(dev, RB5_CR_PRS_TRA, &data, 1);
				YWOSTRACE((YWOS_TRACE_INFO, "snr state = 3, reg37 = 0x42, regb5 = 0x9a;\n"));
				priv->t_Param.t_snr_state = 3;
			}
		}
	}
	if (priv->t_Param.t_dynamic_power_en)
		nim_s3501_dynamic_power(dev, (*snr));
	return SUCCESS;
}

/*****************************************************************************
* INT32 nim_s3501_get_BER(struct nim_device *dev, UINT32 *RsUbc)
* Get bit error ratio
*
* Arguments:
* Parameter1: struct nim_device *dev
* Parameter2: UINT16* RsUbc
*
* Return Value: INT32
*****************************************************************************/
static INT32 nim_s3501_get_BER(struct nim_device *dev, UINT32 *RsUbc)
{
	UINT8 data;
	UINT8 ber_data[3];
	UINT32 u32ber_data[3];
	UINT32 uber_data;
	struct nim_s3501_private *priv = (struct nim_s3501_private *) dev->priv;
	//CR78
	nim_reg_read(dev, R76_BIT_ERR + 0x02, &data, 1);
	if (0x00 == (0x80 & data))
	{
		// printk( "CR78= %x\n", data);
		//CR76
		nim_reg_read(dev, R76_BIT_ERR, &ber_data[0], 1);
		u32ber_data[0] = (UINT32) ber_data[0];
		//CR77
		nim_reg_read(dev, R76_BIT_ERR + 0x01, &ber_data[1], 1);
		u32ber_data[1] = (UINT32) ber_data[1];
		u32ber_data[1] <<= 8;
		//CR78
		nim_reg_read(dev, R76_BIT_ERR + 0x02, &ber_data[2], 1);
		u32ber_data[2] = (UINT32) ber_data[2];
		u32ber_data[2] <<= 16;
		uber_data = u32ber_data[2] + u32ber_data[1] + u32ber_data[0];
		uber_data *= 100;
		uber_data /= 1632;
		*RsUbc = uber_data;
		priv->ul_status.old_ber = uber_data;
		//CR78
		data = 0x80;
		nim_reg_write(dev, R76_BIT_ERR + 0x02, &data, 1);
	}
	else
	{
		*RsUbc = priv->ul_status.old_ber;
	}
	//------------------------------------------------------------------------
	//Carcy add for control power
	return SUCCESS;
}

/*****************************************************************************
* INT32 nim_s3501_dynamic_power(struct nim_device *dev, UINT32 *RsUbc)
* Get bit error ratio
*
* Arguments:
* Parameter1:
* Key word: power_ctrl
* Return Value: INT32
*****************************************************************************/
static INT32 nim_s3501_dynamic_power(struct nim_device *dev, UINT8 snr)
{
	UINT8 coderate;
	UINT32 ber;
	static UINT32 ber_sum = 0; // store the continuous ber
	static UINT32 last_ber_sum = 0;
	static UINT32 cur_ber_sum = 0;
	static UINT32 ber_thres = 0x180;
	static UINT8 cur_max_iter = 50; // static variable can not auto reset at channel change???
	static UINT8 snr_bak = 0;
	static UINT8 last_max_iter = 50;
	static int cnt3 = 0;
	// static int clock_flag = 1; // 0: slow 1:fast
	if (cnt3 >= 3)
	{
		// printk("ber_sum = %6x\n",ber_sum);
		last_ber_sum = cur_ber_sum;
		cur_ber_sum = ber_sum;
		cnt3 = 0;
		ber_sum = 0;
	}
	nim_s3501_get_BER(dev, &ber);
	nim_s3501_reg_get_code_rate(dev, &coderate);
	ber_sum += ber;
	cnt3 ++;
	if (coderate < 0x04) // 1/4 rate
		ber_thres = 0x120;
	else
		ber_thres = 0x70;
	// printk("ber/snr/cur_max_iter/snr_bak is %6x / %2x / %2d / %2x \n",ber,snr,cur_max_iter,snr_bak);
	if (cur_max_iter == 50)
	{
		if (ber_sum >= ber_thres)
		{
			if (snr > snr_bak)
				snr_bak = snr;
			cur_max_iter -= 15;
		}
	}
	else if (cur_max_iter < 50)
	{
		if (((cur_ber_sum + 0x80) < last_ber_sum) || (snr > (snr_bak + 2)))
		{
			cur_max_iter += 15;
			snr_bak = 0;
			cnt3 = 0;
			ber_sum = 0;
			last_ber_sum = 0;
			cur_ber_sum = 0;
		}
		else if (ber_sum > 3 * ber_thres)
		{
			cur_max_iter -= 15;
			if ((coderate < 0x04) && (cur_max_iter < 35))
				cur_max_iter = 35;
			else if (cur_max_iter < 20)
				cur_max_iter = 20;
		}
	}
	if (cur_max_iter != last_max_iter)
	{
		_DEBUG
		printk("----change cur_max_iter to %d----\n\n", cur_max_iter);
		nim_reg_write(dev, R57_LDPC_CTRL, &cur_max_iter, 1);
		last_max_iter = cur_max_iter;
	}
	return SUCCESS;
}

#if 0 //lwj remove this tempority
/*****************************************************************************
* INT32 nim_s3501_get_PER(struct nim_device *dev, UINT32 *RsUbc)
* Get packet error ratio
*
* Arguments:
* Parameter1: struct nim_device *dev
* Parameter2: UINT16* RsUbc
*
* Return Value: INT32
*****************************************************************************/
static INT32 nim_s3501_get_PER(struct nim_device *dev, UINT32 *RsUbc)
{
	UINT8 per[2];
	UINT16 percount;
	UINT8 data;
	struct nim_s3501_private *priv = (struct nim_s3501_private *) dev->priv;
	nim_reg_read(dev, R04_STATUS, &data, 1);
	if (0x00 != (0x20 & data))
	{
		nim_reg_read(dev, R79_PKT_ERR + 0x01, &data, 1);
		if (0x00 == (0x80 & data))
		{
			nim_reg_read(dev, R79_PKT_ERR + 0x01, &per[1], 1);
			printk("per[1]:%d\n", per[1]);
			if (0x00 == (0x80 & per[1]))
			{
				nim_reg_read(dev, R79_PKT_ERR, &per[0], 1);
				per[1] = per[1] & 0x7f;
				percount = (UINT16)(per[1] * 256 + per[0]);
				*RsUbc = (UINT32) percount;
				priv->ul_status.old_per = percount;
				printk("current PER is %d\n", percount);
				//CR7a
				data = 0x80;
				nim_reg_write(dev, R79_PKT_ERR + 0x01, &data, 1);
				return SUCCESS;
			}
			else
			{
				*RsUbc = priv->ul_status.old_per;
				return ERR_FAILED;
			}
		}
		else
		{
			*RsUbc = priv->ul_status.old_per;
		}
		return SUCCESS;
	}
	else
	{
		// printk( "current PER is %d\n",percount );
		return ERR_TIME_OUT;
	}
}
#endif
/*****************************************************************************
* INT32 nim_s3501_get_LDPC(struct nim_device *dev, UINT32 *RsUbc)
* Get LDPC average iteration number
*
* Arguments:
* Parameter1: struct nim_device *dev
* Parameter2: UINT16* RsUbc
*
* Return Value: INT32
*****************************************************************************/
static INT32 nim_s3501_get_LDPC(struct nim_device *dev, UINT32 *RsUbc)
{
	UINT8 data;
	//UINT8 ite_num;
	//struct nim_s3501_private* priv = (struct nim_s3501_private *)dev->priv;
	// read single LDPC iteration number
	nim_reg_read(dev, RAA_S2_FEC_ITER, &data, 1);
	*RsUbc = (UINT32) data;
	return SUCCESS;
}

INT32 nim_s3501_reg_get_freq(struct nim_device *dev, UINT32 *freq)
{
	INT32 freq_off;
	UINT8 data[3];
	UINT32 tdata;
	UINT32 temp;
	struct nim_s3501_private *priv = (struct nim_s3501_private *) dev->priv;
	temp = 0;
	// printk("Enter Fuction nim_s3501_reg_get_freq \n");
	nim_reg_read(dev, R69_RPT_CARRIER + 0x02, &data[2], 1);
	temp = data[2] & 0x01;
	temp = temp << 8;
	nim_reg_read(dev, R69_RPT_CARRIER + 0x01, &data[1], 1);
	temp = temp | (data[1] & 0xff);
	temp = temp << 8;
	nim_reg_read(dev, R69_RPT_CARRIER, &data[0], 1);
	temp = temp | (data[0] & 0xff);
	tdata = temp;
	if ((data[2] & 0x01) == 1)
		freq_off = (0xffff0000 | (tdata & 0xffff));
	else
		freq_off = tdata & 0xffff;
	if (priv->ul_status.m_s3501_type == NIM_CHIP_ID_M3501B)
		freq_off = (freq_off * (CRYSTAL_FREQ * 90 / 135)) / 90;
	else
		freq_off = (freq_off * (CRYSTAL_FREQ * 99 / 135)) / 90;
	freq_off /= 1024;
	*freq += freq_off;
	return SUCCESS;
}

INT32 nim_s3501_reg_get_code_rate(struct nim_device *dev, UINT8 *code_rate)
{
	UINT8 data;
	// printk("Enter Fuction nim_s3501_reg_get_code_rate \n");
	nim_reg_read(dev, R69_RPT_CARRIER + 0x02, &data, 1);
	*code_rate = ((data >> 1) & 0x0f);
	return SUCCESS;
	// Code rate list
	// for DVBS:
	// 0x0: 1/2,
	// 0x1: 2/3,
	// 0x2: 3/4,
	// 0x3: 5/6,
	// 0x4: 6/7,
	// 0x5: 7/8.
	// For DVBS2 :
	// 0x0: 1/4 ,
	// 0x1: 1/3 ,
	// 0x2: 2/5 ,
	// 0x3: 1/2 ,
	// 0x4: 3/5 ,
	// 0x5: 2/3 ,
	// 0x6: 3/4 ,
	// 0x7: 4/5 ,
	// 0x8: 5/6 ,
	// 0x9: 8/9 ,
	// 0xa: 9/10.
}

// Carcy add ldpc_code information .

static INT32 nim_s3501_reg_get_map_type(struct nim_device *dev, UINT8 *map_type)
{
	UINT8 data;
	// printk("Enter Fuction nim_s3501_reg_get_map_type \n");
	nim_reg_read(dev, R69_RPT_CARRIER + 0x02, &data, 1);
	*map_type = ((data >> 5) & 0x07);
	return SUCCESS;
	// Map type:
	// 0x0: HBCD.
	// 0x1: BPSK
	// 0x2: QPSK
	// 0x3: 8PSK
	// 0x4: 16APSK
	// 0x5: 32APSK
}

static INT32 nim_s3501_reg_get_work_mode(struct nim_device *dev, UINT8 *work_mode)
{
	UINT8 data;
	_DEBUG
	printk("Enter Fuction nim_s3501_reg_get_work_mode \n");
	nim_reg_read(dev, R68_WORK_MODE, &data, 1);
	_DEBUG
	printk("nim_s3501_reg_get_work_mode data = 0x%x\n", data);
	*work_mode = data & 0x03;
	_DEBUG
	printk("*$$$$$$$$$$$$$$$$$$work_mode = %d\n", *work_mode);
	return SUCCESS;
	// Work Mode
	// 0x0: DVB-S
	// 0x1: DVB-S2
	// 0x2: DVB-S2 HBC
}

static INT32 nim_s3501_reg_get_iqswap_flag(struct nim_device *dev, UINT8 *iqswap_flag)
{
	UINT8 data;
	// printk("Enter Fuction nim_s3501_reg_get_iqswap_flag \n");
	nim_reg_read(dev, R6C_RPT_SYM_RATE + 0x02, &data, 1);
	*iqswap_flag = ((data >> 4) & 0x01);
	return SUCCESS;
}

static INT32 nim_s3501_reg_get_roll_off(struct nim_device *dev, UINT8 *roll_off)
{
	UINT8 data;
	// printk("Enter Fuction nim_s3501_reg_get_roll_off \n");
	nim_reg_read(dev, R6C_RPT_SYM_RATE + 0x02, &data, 1);
	*roll_off = ((data >> 5) & 0x03);
	return SUCCESS;
	// DVBS2 Roll off report
	// 0x0: 0.35
	// 0x1: 0.25
	// 0x2: 0.20
	// 0x3: Reserved
}

INT32 nim_s3501_reg_get_symbol_rate(struct nim_device *dev, UINT32 *sym_rate)
{
	UINT8 data[3];//, i;
	UINT32 temp;
	UINT32 symrate;
	struct nim_s3501_private *priv = (struct nim_s3501_private *) dev->priv;
	temp = 0;
	nim_reg_read(dev, R6C_RPT_SYM_RATE + 0x02, &data[2], 1);
	temp = data[2] & 0x01;
	temp = temp << 8;
	nim_reg_read(dev, R6C_RPT_SYM_RATE + 0x01, &data[1], 1);
	temp = temp | (data[1] & 0xff);
	temp = temp << 8;
	nim_reg_read(dev, R6C_RPT_SYM_RATE, &data[0], 1);
	temp = temp | (data[0] & 0xff);
	symrate = temp;
	_DEBUG
	printk("symrate ==== 0x%x\n", symrate);
	symrate = (UINT32)((temp * 1000) / 2048);
	if (priv->ul_status.m_s3501_type == NIM_CHIP_ID_M3501B)
		symrate = (symrate * (CRYSTAL_FREQ * 90 / 135)) / 90;
	else
		symrate = (symrate * (CRYSTAL_FREQ * 99 / 135)) / 90;
	*sym_rate = symrate;
	_DEBUG
	printk(" *sym_rate is 0x%x\n", *sym_rate);
	return SUCCESS;
}

void nim_s3501_set_CodeRate(struct nim_device *dev, UINT8 coderate)
{
	return;
}

void nim_s3501_set_RS(struct nim_device *dev, UINT32 rs)
{
	UINT8 data, ver_data;
	struct nim_s3501_private *priv = (struct nim_s3501_private *) dev->priv;
	rs <<= 11;
	if (priv->ul_status.m_s3501_type == NIM_CHIP_ID_M3501B)
	{
		_DEBUG
		printk("nim_s3501_set_RS NIM_CHIP_ID_M3501B \n");
		rs = rs / (CRYSTAL_FREQ * 90 / 135) * 90;
	}
	else
		rs = rs / (CRYSTAL_FREQ * 99 / 135) * 90;
	rs /= 1000;
	data = 0;
	ver_data = 0;
	//CR3F
	data = (UINT8)(rs & 0xFF);
	nim_reg_read(dev, RA3_CHIP_ID + 0x01, &ver_data, 1);
	_DEBUG
	printk("1111111ooooooooooooo ver_data = 0x%x\n", ver_data);
	nim_reg_read(dev, R5F_ACQ_SYM_RATE, &ver_data, 1);
	_DEBUG
	printk("1111111ooooooooooooo ver_data = 0x%x\n", ver_data);
	nim_reg_write(dev, R5F_ACQ_SYM_RATE, &data, 1);
	nim_reg_read(dev, R5F_ACQ_SYM_RATE, &ver_data, 1);
	if (data != ver_data)
	{
		_DEBUG
		printk("wrong 0x5f reg write\n");
	}
	_DEBUG
	printk("nim_s3501_set_RS CR3F rs = 0x%x, ver_data = 0x%x\n", data, ver_data);
	//CR40
	data = (UINT8)((rs & 0xFF00) >> 8);
	nim_reg_read(dev, R5F_ACQ_SYM_RATE + 1, &ver_data, 1);
	_DEBUG
	printk("2222222ooooooooooooo ver_data = 0x%x\n", ver_data);
	nim_reg_write(dev, R5F_ACQ_SYM_RATE + 0x01, &data, 1);
	nim_reg_read(dev, R5F_ACQ_SYM_RATE + 0x01, &ver_data, 1);
	if (data != ver_data)
	{
		_DEBUG
		printk("wrong 0x60 reg write\n");
	}
	_DEBUG
	printk("nim_s3501_set_RS CR40 rs = 0x%x, ver_data = 0x%x\n", data, ver_data);
	//CR41
	data = (UINT8)((rs & 0x10000) >> 16);
	nim_reg_write(dev, R5F_ACQ_SYM_RATE + 0x02, &data, 1);
	nim_reg_read(dev, R5F_ACQ_SYM_RATE + 0x02, &ver_data, 1);
	if (data != ver_data)
	{
		_DEBUG
		printk("wrong 0x61 reg write\n");
	}
	_DEBUG
	printk("nim_s3501_set_RS CR41 rs = 0x%x, ver_data = 0x%x\n", data, ver_data);
}

void nim_s3501_set_freq_offset(struct nim_device *dev, INT32 delfreq)
{
	UINT8 data, ver_data;
	//UINT8 read_data;
	struct nim_s3501_private *priv = (struct nim_s3501_private *) dev->priv;
	delfreq = delfreq * 1024;
	if (priv->ul_status.m_s3501_type == NIM_CHIP_ID_M3501B)
		delfreq = (delfreq / (CRYSTAL_FREQ * 90 / 135)) * 90;
	else
		delfreq = (delfreq / (CRYSTAL_FREQ * 99 / 135)) * 90;
	delfreq /= 1000;
	//CR5C
	data = (UINT8)(delfreq & 0xFF);
	nim_reg_write(dev, R5C_ACQ_CARRIER, &data, 1);
	nim_reg_read(dev, R5C_ACQ_CARRIER, &ver_data, 1);
	if (data != ver_data)
	{
		_DEBUG
		printk(" wrong 0x5c reg write\n");
	}
	//CR5D
	data = (UINT8)((delfreq & 0xFF00) >> 8);
	nim_reg_write(dev, R5C_ACQ_CARRIER + 0x01, &data, 1);
	nim_reg_read(dev, R5C_ACQ_CARRIER + 0x01, &ver_data, 1);
	if (data != ver_data)
	{
		_DEBUG
		printk(" wrong 0x5d reg write\n");
	}
	//CR5E
	data = (UINT8)((delfreq & 0x10000) >> 16);
	nim_reg_write(dev, R5C_ACQ_CARRIER + 0x02, &data, 1);
	nim_reg_read(dev, R5C_ACQ_CARRIER + 0x02, &ver_data, 1);
	if (data != ver_data)
	{
		_DEBUG
		printk(" wrong 0x5e reg write\n");
	}
}

//for SNR use
static UINT8 nim_s3501_get_SNR_index(struct nim_device *dev)
{
	INT16 lpf_out16;
	INT16 agc2_ref5;
	INT32 snr_indx = 0;
	UINT8 data[2];
	UINT16 tdata[2];
	//CR45
	nim_reg_read(dev, R45_CR_LCK_DETECT, &data[0], 1);
	//printk("CR20 is 0x%x\n", data[0] );
	//CR46
	nim_reg_read(dev, R45_CR_LCK_DETECT + 0x01, &data[1], 1);
	//printk("CR21 is 0x%x\n", data[1] );
	tdata[0] = (UINT16) data[0];
	tdata[1] = (UINT16)(data[1] << 8);
	lpf_out16 = (INT16)(tdata[0] + tdata[1]);
	lpf_out16 /= (16 * 2);
	//CR07
	nim_reg_read(dev, R07_AGC1_CTRL, &data[0], 1);
	//printk("CR13 is 0x%x\n", data[0] );
	agc2_ref5 = (INT16)(data[0] & 0x1F);
	snr_indx = (lpf_out16 * agc2_ref5 / 21) - 27;//27~0
	if (snr_indx < 0)
		snr_indx = 0;
	else if (snr_indx > 176)
		snr_indx = 176;
	//printk("snr_indx is 0x%x\n", snr_indx );
	//printk("snr_tab[%d] is 0x%x\n", snr_indx, snr_tab[snr_indx] );
	return snr_indx;
}

INT32 nim_s3501_get_bitmode(struct nim_device *dev, UINT8 *bitMode)
{
	struct nim_s3501_private *priv = (struct nim_s3501_private *) dev->priv;
	if ((priv->Tuner_Config_Data.QPSK_Config & 0xc0) == M3501_1BIT_MODE)
		*bitMode = 0x60;
	else if ((priv->Tuner_Config_Data.QPSK_Config & 0xc0) == M3501_2BIT_MODE)
		*bitMode = 0x00;
	else if ((priv->Tuner_Config_Data.QPSK_Config & 0xc0) == M3501_4BIT_MODE)
		*bitMode = 0x20;
//	else if ((priv->Tuner_Config_Data.QPSK_Config & 0xc0) == M3501_8BIT_MODE)
//		*bitMode = 0x40;
	else
		*bitMode = 0x40;
	return SUCCESS;
}

// begin add operation for s3501 optimize
#if 0
static INT32 nim_s3501_set_err(struct nim_device *dev)
{
	struct nim_s3501_private *priv = (struct nim_s3501_private *) dev->priv;
	priv->ul_status.m_Err_Cnts++;
	return SUCCESS;
}
#endif
static INT32 nim_s3501_get_err(struct nim_device *dev)
{
	struct nim_s3501_private *priv = (struct nim_s3501_private *) dev->priv;
	if (priv->ul_status.m_Err_Cnts > 0x00)
		return ERR_FAILED;
	else
		return SUCCESS;
}
static INT32 nim_s3501_clear_err(struct nim_device *dev)
{
	struct nim_s3501_private *priv = (struct nim_s3501_private *) dev->priv;
	priv->ul_status.m_Err_Cnts = 0x00;
	return SUCCESS;
}

static INT32 nim_s3501_get_type(struct nim_device *dev)
{
	UINT8 temp[4] =
	{
		0x00, 0x00, 0x00, 0x00
	};
	UINT32 m_Value = 0x00;
	struct nim_s3501_private *priv = (struct nim_s3501_private *) dev->priv;
	//priv->ul_status.m_s3501_type = 0x00; //lwj remove
	nim_reg_read(dev, RA3_CHIP_ID, temp, 4);
	m_Value = temp[1];
	m_Value = (m_Value << 8) | temp[0];
	m_Value = (m_Value << 8) | temp[3];
	m_Value = (m_Value << 8) | temp[2];
	_DEBUG
	printk("############nim_s3501_get_type temp = 0x%x, m_Value = 0x%x\n",
		   (int)temp, (int)m_Value);
	//priv->ul_status.m_s3501_type = m_Value; //lwj remove
	nim_reg_read(dev, RC0_BIST_LDPC_REG, temp, 1);
	priv->ul_status.m_s3501_sub_type = temp[0];
	return SUCCESS;
}

static INT32 nim_s3501_i2c_open(struct nim_device *dev)
{
	struct nim_s3501_private *priv = (struct nim_s3501_private *) dev->priv;
	nim_s3501_clear_err(dev);
#if defined(NIM_S3501_DEBUG)
	printk("nim_s3501_i2c_open priv->Tuner_Config_Data.QPSK_Config = 0x%x\n", priv->Tuner_Config_Data.QPSK_Config);
#endif
	if (priv->Tuner_Config_Data.QPSK_Config & M3501_I2C_THROUGH)
	{
		UINT8 data, ver_data;
		data = 0xd4;
		nim_reg_write(dev, RCB_I2C_CFG, &data, 1);
		nim_reg_read(dev, RB3_PIN_SHARE_CTRL, &ver_data, 1);
		data = ver_data | 0x04;
		nim_reg_write(dev, RB3_PIN_SHARE_CTRL, &data, 1);
		//comm_delay(200);
		udelay(200);
		if (nim_s3501_get_err(dev))
			return ERR_FAILED;
		return SUCCESS;
	}
	return SUCCESS;
}

static INT32 nim_s3501_i2c_close(struct nim_device *dev)
{
	struct nim_s3501_private *priv = (struct nim_s3501_private *) dev->priv;
	nim_s3501_clear_err(dev);
	if (priv->Tuner_Config_Data.QPSK_Config & M3501_I2C_THROUGH)
	{
		UINT8 data, ver_data;
		udelay(200);
		nim_reg_read(dev, RB3_PIN_SHARE_CTRL, &ver_data, 1);
		data = ver_data & 0xfb;
		nim_reg_write(dev, RB3_PIN_SHARE_CTRL, &data, 1);
		data = 0xc4;
		nim_reg_write(dev, RCB_I2C_CFG, &data, 1);
		//printk(" @@@@Exit nim through\n");
		if (nim_s3501_get_err(dev))
			return ERR_FAILED;
		return SUCCESS;
	}
	return SUCCESS;
}

#if 0 //lwj remove
static INT32 nim_s3501_ext_lnb_config(struct nim_device *dev, struct QPSK_TUNER_CONFIG_API *ptrQPSK_Tuner)
{
	/****For external lnb controller config****/
	struct nim_s3501_private *priv_mem = (struct nim_s3501_private *) dev->priv;
	priv_mem->ext_lnb_control = NULL;
	if (ptrQPSK_Tuner->ext_lnb_config.ext_lnb_control)
	{
		UINT32 check_sum = 0;
		check_sum = (UINT32)(ptrQPSK_Tuner->ext_lnb_config.ext_lnb_control);
		check_sum += ptrQPSK_Tuner->ext_lnb_config.i2c_base_addr;
		check_sum += ptrQPSK_Tuner->ext_lnb_config.i2c_type_id;
		if (check_sum == ptrQPSK_Tuner->ext_lnb_config.param_check_sum)
		{
			priv_mem->ext_lnb_control = ptrQPSK_Tuner->ext_lnb_config.ext_lnb_control;
			priv_mem->ext_lnb_control(0, LNB_CMD_ALLOC_ID, (UINT32)(&priv_mem->ext_lnb_id));
			priv_mem->ext_lnb_control(priv_mem->ext_lnb_id, LNB_CMD_INIT_CHIP, (UINT32)(&ptrQPSK_Tuner->ext_lnb_config));
		}
	}
	return SUCCESS;
}
#endif
static INT32 nim_s3501_interrupt_mask_clean(struct nim_device *dev)
{
	UINT8 data = 0x00;
	nim_reg_write(dev, R02_IERR, &data, 1);
	data = 0xff;
	nim_reg_write(dev, R03_IMASK, &data, 1);
	return SUCCESS;
}

static INT32 nim_s3501_sym_config(struct nim_device *dev, UINT32 sym)
{
	struct nim_s3501_private *priv = (struct nim_s3501_private *) dev->priv;
	if (sym > 40000)
		priv->ul_status.c_RS = 8;
	else if (sym > 30000)
		priv->ul_status.c_RS = 4;
	else if (sym > 20000)
		priv->ul_status.c_RS = 2;
	else if (sym > 10000)
		priv->ul_status.c_RS = 1;
	else
		priv->ul_status.c_RS = 0;
	return SUCCESS;
}

static INT32 nim_s3501_adc_setting(struct nim_device *dev)
{
	struct nim_s3501_private *priv = (struct nim_s3501_private *) dev->priv;
	UINT8 data, ver_data;
	// ADC setting
	if (priv->Tuner_Config_Data.QPSK_Config & M3501_IQ_AD_SWAP)
		data = 0x4a;
	else
		data = 0xa;
	if (priv->Tuner_Config_Data.QPSK_Config & M3501_EXT_ADC)
		data |= 0x80;
	else
		data &= 0x7f;
	nim_reg_write(dev, R01_ADC, &data, 1);
	nim_reg_read(dev, R01_ADC, &ver_data, 1);
	if (data != ver_data)
	{
		_DEBUG
		printk(" 11111111111 wrong 0x8 reg write\n");
		return ERR_FAILED;
	}
	return SUCCESS;
}

static INT32 nim_s3501_set_hw_timeout(struct nim_device *dev, UINT8 time_thr)
{
	struct nim_s3501_private *priv = (struct nim_s3501_private *) dev->priv;
	// AGC1 setting
	if (time_thr != priv->ul_status.m_hw_timeout_thr)
	{
		nim_reg_write(dev, R05_TIMEOUT_TRH, &time_thr, 1);
		priv->ul_status.m_hw_timeout_thr = time_thr;
	}
	return SUCCESS;
}

static INT32 nim_s3501_agc1_ctrl(struct nim_device *dev, UINT8 low_sym, UINT8 s_Case)
{
	struct nim_s3501_private *priv = (struct nim_s3501_private *) dev->priv;
	UINT8 data;
	////printk(" nim_s3501_agc1_ctrl priv->Tuner_Config_Data.QPSK_Config = 0x%x\n",priv->Tuner_Config_Data.QPSK_Config);
	// AGC1 setting
	if (priv->Tuner_Config_Data.QPSK_Config & M3501_NEW_AGC1)
	{
		switch (s_Case)
		{
			case NIM_OPTR_CHL_CHANGE:
				if (1 == low_sym)
					data = 0xaf;
				else
					data = 0xb5;
				break;
			case NIM_OPTR_SOFT_SEARCH:
				if (1 == low_sym)
					data = 0xb1;
				else
					data = 0xb9;
				break;
			case NIM_OPTR_FFT_RESULT:
				data = 0xba;
				break;
		}
	}
	else
	{
		switch (s_Case)
		{
			case NIM_OPTR_CHL_CHANGE:
			case NIM_OPTR_SOFT_SEARCH:
				if (1 == low_sym)
					data = 0x3c;
				else
					data = 0x54;
				break;
			case NIM_OPTR_FFT_RESULT:
				data = 0x54;
				break;
		}
	}
	if ((priv->Tuner_Config_Data.QPSK_Config & M3501_AGC_INVERT) == 0x0) // STV6110's AGC be invert by QinHe
		data = data ^ 0x80;
	nim_reg_write(dev, R07_AGC1_CTRL, &data, 1);
	return SUCCESS;
}

static INT32 nim_s3501_freq_offset_set(struct nim_device *dev, UINT8 low_sym, UINT32 *s_Freq)
{
	struct nim_s3501_private *priv = (struct nim_s3501_private *) dev->priv;
	if (priv->Tuner_Config_Data.QPSK_Config & M3501_QPSK_FREQ_OFFSET)
	{
		if (1 == low_sym)
			*s_Freq += 3;
	}
	return SUCCESS;
}

static INT32 nim_s3501_freq_offset_reset(struct nim_device *dev, UINT8 low_sym)
{
	struct nim_s3501_private *priv = (struct nim_s3501_private *) dev->priv;
	if (priv->Tuner_Config_Data.QPSK_Config & M3501_QPSK_FREQ_OFFSET)
	{
		if (1 == low_sym)
			nim_s3501_set_freq_offset(dev, -3000);
		else
			nim_s3501_set_freq_offset(dev, 0);
	}
	else
		nim_s3501_set_freq_offset(dev, 0);
	return SUCCESS;
}

static INT32 nim_s3501_cr_setting(struct nim_device *dev, UINT8 s_Case)
{
	struct nim_s3501_private *priv = (struct nim_s3501_private *) dev->priv;
	UINT8 data;
	switch (s_Case)
	{
		case NIM_OPTR_SOFT_SEARCH:
			// set CR parameter
			data = 0xaa;
			nim_reg_write(dev, R33_CR_CTRL + 0x03, &data, 1);
			data = 0x45;
			nim_reg_write(dev, R33_CR_CTRL + 0x04, &data, 1);
			data = 0x87;
			nim_reg_write(dev, R33_CR_CTRL + 0x05, &data, 1);
			if (priv->ul_status.m_s3501_type == NIM_CHIP_ID_M3501B)
				data = 0x9a;
			else
				data = 0xaa; // S2 CR parameter
			nim_reg_write(dev, RB5_CR_PRS_TRA, &data, 1);
			break;
		case NIM_OPTR_CHL_CHANGE:
			// set CR parameter
			data = 0xaa;
			nim_reg_write(dev, R33_CR_CTRL + 0x03, &data, 1);
			data = 0x45;
			nim_reg_write(dev, R33_CR_CTRL + 0x04, &data, 1);
			data = 0x87;
			nim_reg_write(dev, R33_CR_CTRL + 0x05, &data, 1);
//		if (priv->ul_status.m_s3501_type == NIM_CHIP_ID_M3501B)
//			data = 0xaa;
//		else
			data = 0xaa; // S2 CR parameter
			nim_reg_write(dev, RB5_CR_PRS_TRA, &data, 1);
			break;
	}
	return SUCCESS;
}

static INT32 nim_s3501_ldpc_setting(struct nim_device *dev, UINT8 s_Case, UINT8 c_ldpc, UINT8 c_fec)
{
	struct nim_s3501_private *priv = (struct nim_s3501_private *) dev->priv;
	UINT8 data;
	// LDPC parameter
	switch (s_Case)
	{
		case NIM_OPTR_CHL_CHANGE:
			if (priv->ul_status.m_s3501_type == NIM_CHIP_ID_M3501B)
			{
				UINT8 temp[3] =
				{
					0x32, 0xc8, 0x08
				};
				nim_reg_write(dev, R57_LDPC_CTRL, temp, 3);
			}
			else
			{
				UINT8 temp[3] =
				{
					0x32, 0x44, 0x04
				};
				temp[0] = 0x1e - priv->ul_status.c_RS; // 30 time iteration
				nim_reg_write(dev, R57_LDPC_CTRL, temp, 3);
			}
			data = 0x01; // active ldpc avg iter
			nim_reg_write(dev, RC1_DVBS2_FEC_LDPC, &data, 1);
			break;
		case NIM_OPTR_SOFT_SEARCH:
			if (priv->ul_status.m_s3501_type == NIM_CHIP_ID_M3501B)
			{
				UINT8 temp[3] =
				{
					0x32, 0x48, 0x08
				};
				nim_reg_write(dev, R57_LDPC_CTRL, temp, 3);
			}
			else
			{
				UINT8 temp[3] =
				{
					0x1e, 0x46, 0x06
				};
				nim_reg_write(dev, R57_LDPC_CTRL, temp, 3);
			}
			data = 0x01; // active ldpc avg iter
			nim_reg_write(dev, RC1_DVBS2_FEC_LDPC, &data, 1);
			break;
		case NIM_OPTR_DYNAMIC_POW:
			data = c_ldpc - priv->ul_status.c_RS;
			nim_reg_write(dev, R57_LDPC_CTRL, &data, 1);
			break;
	}
	data = c_fec;
	nim_reg_write(dev, RC1_DVBS2_FEC_LDPC, &data, 1);
	return SUCCESS;
}

static INT32 nim_s3501_hw_init(struct nim_device *dev)
{
	struct nim_s3501_private *priv = (struct nim_s3501_private *) dev->priv;
	UINT8 data = 0xc0;
	nim_reg_write(dev, RA7_I2C_ENHANCE, &data, 1);
	nim_reg_read(dev, RCC_STRAP_PIN_CLOCK, &data, 1);
	data = data & 0xfb; //open IIC_TIME_THR_BPS, IIC will enter IDLE if long time wating.
	nim_reg_write(dev, RCC_STRAP_PIN_CLOCK, &data, 1);
	data = 0x10;
	nim_reg_write(dev, RB3_PIN_SHARE_CTRL, &data, 1);
	// set TR lock symbol number thr, k unit.
	data = 0x1f; // setting for soft search function
	nim_reg_write(dev, R1B_TR_TIMEOUT_BAND, &data, 1);
	// Carcy change PL time out, for low symbol rate. 2008-03-12
	data = 0x84;
	nim_reg_write(dev, R28_PL_TIMEOUT_BND + 0x01, &data, 1);
	// Set Hardware time out
	//data = 0xff;
	//nim_reg_write(dev,R05_TIMEOUT_TRH, &data, 1);
	nim_s3501_set_hw_timeout(dev, 0xff);
	//----eq demod setting
	// Open EQ controll for QPSK and 8PSK
	data = 0x04; // set EQ control
	nim_reg_write(dev, R21_BEQ_CTRL, &data, 1);
	data = 0x24; // set EQ mask mode, mask EQ for 1/4,1/3,2/5 code rate
	nim_reg_write(dev, R25_BEQ_MASK, &data, 1);
	//-----set analog pad driving and first TS gate open.
	if ((priv->Tuner_Config_Data.QPSK_Config & 0xc0) == M3501_1BIT_MODE)
		data = 0x08;
	else
		data = 0x00;
	nim_reg_write(dev, RAF_TSOUT_PAD, &data, 1);
	data = 0x00;
	nim_reg_write(dev, RB1_TSOUT_SMT, &data, 1);
	// Carcy add for 16APSK
	data = 0x6c;
	nim_reg_write(dev, R2A_PL_BND_CTRL + 0x02, &data, 1);
	//----eq demod setting end
	nim_s3501_adc_setting(dev);
	// config EQ
#ifdef DFE_EQ //question
	data = 0xf0;
	nim_reg_write(dev, RD7_EQ_REG, &data, 1);
	nim_reg_read(dev, RDC_EQ_DBG_TS_CFG, &data, 1);
	data = data | 0x01;
	nim_reg_write(dev, RDC_EQ_DBG_TS_CFG, &data, 1);
#else
	data = 0x00;
	nim_reg_write(dev, RD7_EQ_REG, &data, 1);
	nim_reg_read(dev, RDC_EQ_DBG_TS_CFG, &data, 1);
	data = data | 0x01;
	nim_reg_write(dev, RDC_EQ_DBG_TS_CFG, &data, 1);
#endif
	return SUCCESS;
}

static INT32 nim_s3501_demod_ctrl(struct nim_device *dev, UINT8 c_Value)
{
	UINT8 data = c_Value;
	nim_reg_write(dev, R00_CTRL, &data, 1);
	return SUCCESS;
}

static INT32 nim_s3501_hbcd_timeout(struct nim_device *dev, UINT8 s_Case)
{
	struct nim_s3501_private *priv = (struct nim_s3501_private *) dev->priv;
	UINT8 data;
	switch (s_Case)
	{
		case NIM_OPTR_CHL_CHANGE:
		case NIM_OPTR_IOCTL:
			if (priv->t_Param.t_reg_setting_switch & NIM_SWITCH_HBCD)
			{
				if (priv->ul_status.m_enable_dvbs2_hbcd_mode)
					data = priv->ul_status.m_dvbs2_hbcd_enable_value;
				else
					data = 0x00;
				nim_reg_write(dev, R47_HBCD_TIMEOUT, &data, 1);
				priv->t_Param.t_reg_setting_switch &= ~NIM_SWITCH_HBCD;
			}
			break;
		case NIM_OPTR_SOFT_SEARCH:
			if (!(priv->t_Param.t_reg_setting_switch & NIM_SWITCH_HBCD))
			{
				data = 0x00;
				nim_reg_write(dev, R47_HBCD_TIMEOUT, &data, 1);
				priv->t_Param.t_reg_setting_switch |= NIM_SWITCH_HBCD;
			}
			break;
		case NIM_OPTR_HW_OPEN:
			nim_reg_read(dev, R47_HBCD_TIMEOUT, &(priv->ul_status.m_dvbs2_hbcd_enable_value), 1);
			break;
	}
	return SUCCESS;
}

static INT32 nim_s3501_set_acq_workmode(struct nim_device *dev, UINT8 s_Case)
{
	struct nim_s3501_private *priv = (struct nim_s3501_private *) dev->priv;
	UINT8 data, work_mode = 0x00;
	switch (s_Case)
	{
		case NIM_OPTR_CHL_CHANGE0:
		case NIM_OPTR_DYNAMIC_POW0:
			data = 0x73;
			nim_reg_write(dev, R5B_ACQ_WORK_MODE, &data, 1);
			break;
		case NIM_OPTR_CHL_CHANGE:
			_DEBUG
			printk("####nim_s3501_set_acq_workmode NIM_OPTR_CHL_CHANGE \n");
			nim_s3501_reg_get_work_mode(dev, &work_mode);
			if (work_mode != M3501_DVBS2_MODE)// not in DVBS2 mode, key word: power_ctrl
			{
				_DEBUG
				printk("DVBS1===================\n");
				if (priv->ul_status.m_s3501_type == NIM_CHIP_ID_M3501B)
					priv->ul_status.phase_err_check_status = 1000;
				// slow down S2 clock
				data = 0x77;
				nim_reg_write(dev, R5B_ACQ_WORK_MODE, &data, 1);
			}
			break;
		case NIM_OPTR_SOFT_SEARCH:
		case NIM_OPTR_DYNAMIC_POW:
			data = 0x77;
			nim_reg_write(dev, R5B_ACQ_WORK_MODE, &data, 1);
			break;
		case NIM_OPTR_HW_OPEN:
			if (priv->ul_status.m_s3501_type == NIM_CHIP_ID_M3501B)
			{
				// enable ADC
				data = 0x00;
				nim_reg_write(dev, RA0_RXADC_REG + 0x02, &data, 1);
				// enable S2 clock
				data = 0x73;
				nim_reg_write(dev, R5B_ACQ_WORK_MODE, &data, 1);
			}
			break;
		case NIM_OPTR_HW_CLOSE:
			if (priv->ul_status.m_s3501_type == NIM_CHIP_ID_M3501B)
			{
				// close ADC
				data = 0x07;
				nim_reg_write(dev, RA0_RXADC_REG + 0x02, &data, 1);
				// close S2 clock
				data = 0x3f;
			}
			else
			{
				data = 0x7f;
			}
			nim_reg_write(dev, R5B_ACQ_WORK_MODE, &data, 1);
			break;
	}
	return SUCCESS;
}

static INT32 nim_s3501_set_FC_Search_Range(struct nim_device *dev, UINT8 s_Case, UINT32 rs)
{
	struct nim_s3501_private *priv = (struct nim_s3501_private *) dev->priv;
	UINT8 data, ver_data;
	UINT32 temp;
	switch (s_Case)
	{
		case NIM_OPTR_SOFT_SEARCH:
			if (!(priv->t_Param.t_reg_setting_switch & NIM_SWITCH_FC))
			{
				//CR62, fc search range.
				if (rs > 16000)
					temp = 5 * 16; //(4*90*16)/(CRYSTAL_FREQ*99/135);
				else if (rs > 5000)
					temp = 4 * 16; //(3*90*16)/(CRYSTAL_FREQ*99/135);
				else
					temp = 3 * 16; //(2*90*16)/(CRYSTAL_FREQ*99/135);
				data = temp & 0xff;
				nim_reg_write(dev, R62_FC_SEARCH, &data, 1);
				nim_reg_read(dev, R62_FC_SEARCH, &ver_data, 1);
				if (data != ver_data)
				{
					_DEBUG
					printk(" wrong 0x62 reg write\n");
				}
				//CR63
				data = (temp >> 8) & 0x3;
				if (rs > 10000)
					data |= 0xa0;
				else if (rs > 3000)
					data |= 0xc0; //amy change for 91.5E 3814/V/6666
				else
					data |= 0xb0; //amy change for 91.5E 3629/V/2200
				nim_reg_write(dev, R62_FC_SEARCH + 0x01, &data, 1);
				nim_reg_read(dev, R62_FC_SEARCH + 0x01, &ver_data, 1);
				if (data != ver_data)
				{
					_DEBUG
					printk(" wrong 0x63 reg write\n");
				}
				priv->t_Param.t_reg_setting_switch |= NIM_SWITCH_FC;
			}
			break;
		case NIM_OPTR_CHL_CHANGE:
			if (priv->t_Param.t_reg_setting_switch & NIM_SWITCH_FC)
			{
				// set sweep range
				if (priv->ul_status.m_s3501_type == NIM_CHIP_ID_M3501B)
					temp = (3 * 90 * 16) / (CRYSTAL_FREQ * 90 / 135);
				else
					temp = (3 * 90 * 16) / (CRYSTAL_FREQ * 99 / 135);
				data = temp & 0xff;
				nim_reg_write(dev, R62_FC_SEARCH, &data, 1);
				data = (temp >> 8) & 0x3;
				data |= 0xb0;
				nim_reg_write(dev, R62_FC_SEARCH + 0x01, &data, 1);
				priv->t_Param.t_reg_setting_switch &= ~NIM_SWITCH_FC;
			}
			break;
	}
	return SUCCESS;
}

static INT32 nim_s3501_RS_Search_Range(struct nim_device *dev, UINT8 s_Case, UINT32 rs)
{
	struct nim_s3501_private *priv = (struct nim_s3501_private *) dev->priv;
	UINT8 data, ver_data;
	UINT32 temp;
	switch (s_Case)
	{
		case NIM_OPTR_SOFT_SEARCH:
			if (!(priv->t_Param.t_reg_setting_switch & NIM_SWITCH_RS))
			{
				//CR64
				// temp = (3*90*16)/(CRYSTAL_FREQ*99/135);
				// data = temp&0xff;
				if (rs > 16000)
					temp = rs / 4000;
				else if (rs > 5000)
					temp = rs / 3000;
				else
					temp = rs / 2000;
				if (temp < 3)
					temp = 3;
				else if (temp > 11)
					temp = 11;
				temp = temp << 4;
				// temp = temp/(CRYSTAL_FREQ/135);
				data = temp & 0xff;
				nim_reg_write(dev, R64_RS_SEARCH, &data, 1);
				nim_reg_read(dev, R64_RS_SEARCH, &ver_data, 1);
				if (data != ver_data)
				{
					_DEBUG
					printk(" wrong 0x64 reg write\n");
				}
				//CR65
				data = (temp >> 8) & 0x3;
				if (rs > 6500)
					data |= 0xb0;
				else
					data |= 0xa0;
				// data |= 0xb0;
				nim_reg_write(dev, R64_RS_SEARCH + 0x01, &data, 1);
				nim_reg_read(dev, R64_RS_SEARCH + 0x01, &ver_data, 1);
				if (data != ver_data)
				{
					_DEBUG
					printk(" wrong 0x65 reg write\n");
				}
				priv->t_Param.t_reg_setting_switch |= NIM_SWITCH_RS;
			}
			break;
		case NIM_OPTR_CHL_CHANGE:
			if (priv->t_Param.t_reg_setting_switch & NIM_SWITCH_RS)
			{
				if (priv->ul_status.m_s3501_type == NIM_CHIP_ID_M3501B)
					temp = (3 * 90 * 16) / (CRYSTAL_FREQ * 90 / 135);
				else
					temp = (3 * 90 * 16) / (CRYSTAL_FREQ * 99 / 135);
				data = temp & 0xff;
				nim_reg_write(dev, R64_RS_SEARCH, &data, 1);
				data = (temp >> 8) & 0x3;
				data |= 0x30;
				nim_reg_write(dev, R64_RS_SEARCH + 0x01, &data, 1);
				priv->t_Param.t_reg_setting_switch &= ~NIM_SWITCH_RS;
			}
			break;
	}
	return SUCCESS;
}

static INT32 nim_s3501_TR_CR_Setting(struct nim_device *dev, UINT8 s_Case)
{
	UINT8 data = 0;
	struct nim_s3501_private *priv = (struct nim_s3501_private *) dev->priv;
	switch (s_Case)
	{
		case NIM_OPTR_SOFT_SEARCH:
			if (!(priv->t_Param.t_reg_setting_switch & NIM_SWITCH_TR_CR))
			{
				data = 0x4d;
				nim_reg_write(dev, R66_TR_SEARCH, &data, 1);
				data = 0x31;
				nim_reg_write(dev, R67_VB_CR_RETRY, &data, 1);
				priv->t_Param.t_reg_setting_switch |= NIM_SWITCH_TR_CR;
			}
			break;
		case NIM_OPTR_CHL_CHANGE:
			if (priv->t_Param.t_reg_setting_switch & NIM_SWITCH_TR_CR)
			{
				// set reg to default value
				data = 0x59;
				nim_reg_write(dev, R66_TR_SEARCH, &data, 1);
				data = 0x33;
				nim_reg_write(dev, R67_VB_CR_RETRY, &data, 1);
				priv->t_Param.t_reg_setting_switch &= ~NIM_SWITCH_TR_CR;
			}
			break;
	}
	return SUCCESS;
}
#if 0 //we don't need task init
static INT32 nim_s3501_task_init(struct nim_device *dev)
{
	UINT8 nim_device[3][3] =
	{
		'N', 'M', '0', 'N', 'M', '1', 'N', 'M', '2'
	};
	//ID nim_task_id ;//= OSAL_INVALID_ID;
	//T_CTSK t_ctsk;
	T_CTSK nim_task_praram;
	static UINT8 nim_task_num = 0x00;
	struct nim_s3501_private *priv = (struct nim_s3501_private *) dev->priv;
	if (nim_task_num > 1)
		return SUCCESS;
	nim_task_praram.task = nim_s3501_task;//dmx_m3327_record_task ;
	nim_task_praram.name[0] = nim_device[nim_task_num][0];
	nim_task_praram.name[1] = nim_device[nim_task_num][1];
	nim_task_praram.name[2] = nim_device[nim_task_num][2];
	nim_task_praram.stksz = 0xc00 ;
	nim_task_praram.itskpri = OSAL_PRI_NORMAL;
	nim_task_praram.quantum = 10 ;
	nim_task_praram.para1 = (UINT32) dev ;
	nim_task_praram.para2 = 0 ;//Reserved for future use.
	/* priv->tsk_status.m_task_id = osal_task_create(&nim_task_praram);
	 if (OSAL_INVALID_ID == priv->tsk_status.m_task_id)
	 {
	 //soc_printk("Task create error\n");
	 return OSAL_E_FAIL;
	 }*/ //lwj remove
	nim_task_num++;
	return SUCCESS;
}
static void nim_s3501_task(UINT32 param1, UINT32 param2)
{
	struct nim_device *dev = (struct nim_device *) param1;
	struct nim_s3501_private *priv = (struct nim_s3501_private *) dev->priv ;
	priv->tsk_status.m_sym_rate = 0x00;
	priv->tsk_status.m_code_rate = 0x00;
	priv->tsk_status.m_map_type = 0x00;
	priv->tsk_status.m_work_mode = 0x00;
	priv->tsk_status.m_info_data = 0x00;
#ifdef CHANNEL_CHANGE_ASYNC
	UINT32 flag_ptn;
#endif
	while (1)
	{
#ifdef CHANNEL_CHANGE_ASYNC
		flag_ptn = 0;
		if (NIM_FLAG_WAIT(&flag_ptn, priv->flag_id, NIM_FLAG_CHN_CHG_START, OSAL_TWF_ANDW | OSAL_TWF_CLR, 0) == OSAL_E_OK)
		{
			//osal_flag_clear(priv->flag_id, NIM_FLAG_CHN_CHG_START);
			NIM_FLAG_SET(priv->flag_id, NIM_FLAG_CHN_CHANGING);
			nim_s3501_waiting_channel_lock(dev, priv->cur_freq, priv->cur_sym);
			NIM_FLAG_CLEAR(priv->flag_id, NIM_FLAG_CHN_CHANGING);
		}
#endif
		if (priv->ul_status.m_s3501_type == NIM_CHIP_ID_M3501B)
		{
			if ((priv->tsk_status.m_lock_flag == NIM_LOCK_STUS_SETTING) && (priv->t_Param.t_i2c_err_flag == 0x00))
			{
				nim_s3501_get_lock(dev, &(priv->tsk_status.m_info_data));
				if (priv->tsk_status.m_info_data && (priv->t_Param.t_i2c_err_flag == 0x00))
				{
					nim_s3501_reg_get_symbol_rate(dev, &(priv->tsk_status.m_sym_rate));
					nim_s3501_reg_get_code_rate(dev, &(priv->tsk_status.m_code_rate));
					nim_s3501_reg_get_work_mode(dev, &(priv->tsk_status.m_work_mode));
					nim_s3501_reg_get_map_type(dev, &(priv->tsk_status.m_map_type));
					if ((priv->ul_status.m_enable_dvbs2_hbcd_mode == 0) &&
							((priv->tsk_status.m_map_type == 0) || (priv->tsk_status.m_map_type == 5)))
					{
						printk(" Demod Error: wrong map_type is %d\n", priv->tsk_status.m_map_type);
					}
					else
					{
						nim_s3501_set_ts_mode(dev, priv->tsk_status.m_work_mode, priv->tsk_status.m_map_type, priv->tsk_status.m_code_rate,
											  priv->tsk_status.m_sym_rate, 0x1);
						// open TS
						nim_reg_read(dev, R9C_DEMAP_BETA + 0x02, &(priv->tsk_status.m_info_data), 1);
						priv->tsk_status.m_info_data = priv->tsk_status.m_info_data | 0x80; // ts open //question
						nim_reg_write(dev, R9C_DEMAP_BETA + 0x02, &(priv->tsk_status.m_info_data), 1);
						priv->tsk_status.m_lock_flag = NIM_LOCK_STUS_CLEAR;
					}
				}
			}
		}
		else
			break;
		msleep(100);
	}
}
#endif

static INT32 nim_s3501_get_new_BER(struct nim_device *dev, UINT32 *ber)
{
	UINT8 data;
	UINT32 t_count, myber;
	myber = 0;
	for (t_count = 0; t_count < 200; t_count++)
	{
		nim_reg_read(dev, R76_BIT_ERR + 0x02, &data, 1);
		if ((data & 0x80) == 0)
		{
			myber = data & 0x7f;
			nim_reg_read(dev, R76_BIT_ERR + 0x01, &data, 1);
			myber <<= 8;
			myber += data;
			nim_reg_read(dev, R76_BIT_ERR, &data, 1);
			myber <<= 8;
			myber += data;
			break;
		}
	}
	*ber = myber;
	return SUCCESS;
}

static INT32 nim_s3501_get_new_PER(struct nim_device *dev, UINT32 *per)
{
	UINT8 data;
	UINT32 t_count, myper;
	myper = 0;
	for (t_count = 0; t_count < 200; t_count++)
	{
		nim_reg_read(dev, R79_PKT_ERR + 0x01, &data, 1);
		if ((data & 0x80) == 0)
		{
			myper = data & 0x7f;
			nim_reg_read(dev, R79_PKT_ERR, &data, 1);
			myper <<= 8;
			myper += data;
			break;
		}
	}
	*per = myper;
	// printk("!!!!!!!! myPER cost %d time, per = %d\n",t_count,myper);
	return SUCCESS;
}

static int d3501_init(struct dvb_frontend *fe)
{
	struct dvb_d3501_fe_state *state = fe->demodulator_priv;
	//dprintk(10, "%s <\n", __FUNCTION__);
	return nim_s3501_open(&state->spark_nimdev);
}

static void d3501_release(struct dvb_frontend *fe)
{
	struct dvb_d3501_fe_state *state = fe->demodulator_priv;
	nim_s3501_close(&state->spark_nimdev);
	d3501_term(&state->spark_nimdev);
#if 1
	stpio_free_pin(state->fe_lnb_13_18);
	stpio_free_pin(state->fe_lnb_on_off);
#endif /* 0 */
	kfree(state);
}

static int d3501_read_ber(struct dvb_frontend *fe, u32 *ber)
{
	struct dvb_d3501_fe_state *state = fe->demodulator_priv;
	return nim_s3501_get_BER(&state->spark_nimdev, ber);
}

static int d3501_read_snr(struct dvb_frontend *fe, u16 *snr)
{
	int iRet;
	struct dvb_d3501_fe_state *state = fe->demodulator_priv;
	iRet = nim_s3501_get_SNR(&state->spark_nimdev, (UINT16 *)snr); //quality
	if (*snr < 30)
		*snr = *snr * 7 / 3;
	else
		*snr = *snr / 3 + 60;
	if (*snr > 90)
		*snr = 90;
	*snr = *snr * 255 * 255 / 100;
	_DEBUG
	printk("*snr = %d\n", *snr);
	return iRet;
}

static int d3501_read_signal_strength(struct dvb_frontend *fe,
									  u16 *strength)
{
	int iRet;
	UINT16 *Intensity = (UINT16 *)strength;
	struct dvb_d3501_fe_state *state = fe->demodulator_priv;
	iRet = nim_s3501_get_AGC(&state->spark_nimdev, (UINT16 *)Intensity); //level
	//lwj add begin
	if (*Intensity > 100)
		*Intensity = 100;
#if 0
	if (*Intensity < 10)
		*Intensity = *Intensity * 11 / 2;
	else
		*Intensity = *Intensity / 2 + 50;
	if (*Intensity > 90)
		*Intensity = 90;
	printk("*Intensity = %d\n", *Intensity);
	*strength = *Intensity;
	printk("*strength = %d\n", *strength);
	//lwj add end
#endif // 0
	*Intensity = *Intensity * 255 * 255 / 100;
	*strength = *Intensity;
	_DEBUG
	printk("*strength = %d\n", *strength);
	return iRet;
}

static int d3501_read_status(struct dvb_frontend *fe, enum fe_status *status)

{
	int iRet;
//	int iTunerLock;
	UINT8 lock;
	struct dvb_d3501_fe_state *state = fe->demodulator_priv;
	iRet = nim_s3501_get_lock(&state->spark_nimdev, &lock);
#if defined(NIM_S3501_DEBUG)
	printk("lock = %d\n", lock);
#endif
	if (lock)
	{
		*status = FE_HAS_SIGNAL |
				  FE_HAS_CARRIER |
				  FE_HAS_VITERBI |
				  FE_HAS_SYNC |
				  FE_HAS_LOCK;
	}
	else
	{
		*status = 0;
	}
#if 0
	if (nim_s3501_i2c_open(&state->spark_nimdev))
		return S3501_ERR_I2C_NO_ACK;
	if (fe->ops.tuner_ops.get_status)
	{
		if (fe->ops.tuner_ops.get_status(fe, &iTunerLock) < 0)
		{
			_DEBUG
			printk("1. Tuner get_status err\n");
		}
	}
	if (iTunerLock)
	{
#if defined(NIM_S3501_DEBUG)
		printk("1. Tuner phase locked\n");
#endif
	}
	else
	{
		printk("1. Tuner unlocked\n");
	}
	if (nim_s3501_i2c_close(&state->spark_nimdev))
		return S3501_ERR_I2C_NO_ACK;
#endif /* 0 */
	return iRet;
}

static enum dvbfe_algo d3501_frontend_algo(struct dvb_frontend *fe)
{
	return DVBFE_ALGO_CUSTOM;
}

static int d3501_set_tuner_params(struct dvb_frontend *fe,
								  UINT32 freq,
								  UINT32 sym)
{
	int err = 0;
	struct dvb_frontend_parameters param;
	struct dvb_frontend_ops *frontend_ops = NULL;
	struct dvb_tuner_ops *tuner_ops = NULL;
	if (!&fe->ops)
	{
		return 0;
	}
	frontend_ops = &fe->ops;
	if (!&frontend_ops->tuner_ops)
	{
		return 0;
	}
	tuner_ops = &frontend_ops->tuner_ops;
	param.frequency = freq;
	param.u.qpsk.symbol_rate = sym;
	_DEBUG
	{
		printk("param.frequency = %d\n", param.frequency);
		printk("param.u.qpsk.symbol_rate = %d\n", param.u.qpsk.symbol_rate);
	}
	if (!tuner_ops->set_params)
	{
		return 0;
	}
	err = tuner_ops->set_params(fe, &param);
	if (err < 0)
	{
		_DEBUG
		printk("%s: Invalid parameter\n", __func__);
		return -1;
	}
	return 0;
}

#if 1
#if (DVB_API_VERSION < 5)
static enum dvbfe_search d3501_Search(struct dvb_frontend *fe,
									  struct dvbfe_params *p)
{
	//IOARCH_Handle_t IOHandle;
	struct nim_device *dev;
	struct nim_s3501_private *priv;
	//U32 TuneStartTime;
	UINT32 freq;
	UINT32 sym;
	//UINT8 result;
	UINT8 data = 0x10;
	UINT8 low_sym;
	int err;
	struct dvb_d3501_fe_state *state = fe->demodulator_priv;
	/******************************************/
	dev = (struct nim_device *)&state->spark_nimdev;
	priv = (struct nim_s3501_private *) dev->priv;
	_DEBUG
	printk("p->frequency is %d\n", p->frequency);
	priv->bLock = FALSE;
	//freq = 5150 - p->frequency / 1000;
	freq = p->frequency / 1000;
	sym = p->delsys.dvbs.symbol_rate / 1000;
	/******************************************/
////UINT8 channel_change_flag = 1; //bentao add for judge channel chang or soft_search in set_ts_mode
	//starttime = 0;
	////printk(" Enter Fuction nim_s3501_channel_change \n");
	_DEBUG
	{
		printk(" freq is %d\n", freq);
		printk(" sym is %d\n", sym);
	}
	//printk(" fec is %d\n", fec);
	priv->t_Param.t_phase_noise_detected = 0;
	priv->t_Param.t_dynamic_power_en = 0;
	priv->t_Param.t_last_snr = -1;
	priv->t_Param.t_last_iter = -1;
	priv->t_Param.t_aver_snr = -1;
	priv->t_Param.t_snr_state = 0;
	priv->t_Param.t_snr_thre1 = 256;
	priv->t_Param.t_snr_thre2 = 256;
	priv->t_Param.t_snr_thre3 = 256;
	priv->t_Param.phase_noise_detect_finish = 0x00;
	/*#ifdef CHANNEL_CHANGE_ASYNC
	 UINT32 flag_ptn = 0;
	 if (NIM_FLAG_WAIT(&flag_ptn, priv->flag_id, NIM_FLAG_CHN_CHG_START | NIM_FLAG_CHN_CHANGING, OSAL_TWF_ORW, 0) == OSAL_E_OK)
	 {
	 // channel chaning, stop the old changing first.
	 priv->ul_status.s3501_chanscan_stop_flag = 1;
	 //libc_printf("channel changing already, stop it first\n");
	 msleep(2); //sleep 2ms
	 }
	#endif*/ //lwj remove
	if (priv->ul_status.m_s3501_type == NIM_CHIP_ID_M3501B)
	{
		_DEBUG
		printk("NIM_CHIP_ID_M3501B ########### d3501_Search\n");
		priv->ul_status.phase_err_check_status = 0;
		priv->ul_status.s3501d_lock_status = NIM_LOCK_STUS_NORMAL;
	}
	priv->ul_status.m_setting_freq = freq;
	//reset first
	_DEBUG
	printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	nim_s3501_demod_ctrl(dev, NIM_DEMOD_CTRL_0X91);
	_DEBUG
	printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	msleep(5); //sleep 5ms lwj add
	if ((0 == freq) || (0 == sym))
		return DVBFE_ALGO_SEARCH_ERROR;
	_DEBUG
	printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	nim_s3501_sym_config(dev, sym);
	_DEBUG
	printk("[%s][%d]\n", __FUNCTION__, __LINE__);
#if 1
	if (priv->ul_status.s3501_chanscan_stop_flag)
	{
		priv->ul_status.s3501_chanscan_stop_flag = 0;
		return DVBFE_ALGO_SEARCH_FAILED;
	}
#else
	if (Inst->ForceSearchTerm)
	{
		return SUCCESS;
	}
#endif
	// time for channel change and sof search.
	// ttt
	_DEBUG
	printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	nim_s3501_TR_CR_Setting(dev, NIM_OPTR_CHL_CHANGE);
	_DEBUG
	printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	low_sym = sym < 6500 ? 1 : 0; /* Symbol rate is less than 10M, low symbol rate */
	_DEBUG
	printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	nim_s3501_freq_offset_set(dev, low_sym, &freq);
	_DEBUG
	{
		printk("[%s][%d]freq = %d\n", __FUNCTION__, __LINE__, freq);

		printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	}
	if (nim_s3501_i2c_open(dev))
		return DVBFE_ALGO_SEARCH_FAILED;
	_DEBUG
	printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	err = d3501_set_tuner_params(fe, freq, sym);
	if (err < 0)
	{
		return DVBFE_ALGO_SEARCH_FAILED;
	}
	_DEBUG
	printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	if (nim_s3501_i2c_close(dev))
		return DVBFE_ALGO_SEARCH_FAILED;
	msleep(1);
	_DEBUG
	printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	if (nim_s3501_i2c_open(dev))
		return DVBFE_ALGO_SEARCH_FAILED;
	if (fe->ops.tuner_ops.get_status)
	{
		int iTunerLock;
		_DEBUG
		printk("[%s][%d]\n", __FUNCTION__, __LINE__);
		if (fe->ops.tuner_ops.get_status(fe, &iTunerLock) < 0)
		{
			_DEBUG
			printk("1. Tuner get_status err\n");
		}
	}
	_DEBUG
	printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	if (nim_s3501_i2c_close(dev))
		return DVBFE_ALGO_SEARCH_FAILED;
	// nim_s3501_adc_setting(dev);
	data = 0x10;
	_DEBUG
	printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	nim_reg_write(dev, RB3_PIN_SHARE_CTRL, &data, 1);
	_DEBUG
	printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	nim_s3501_adc_setting(dev);
	_DEBUG
	printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	nim_s3501_interrupt_mask_clean(dev);
	// hardware timeout setting
	// ttt
	// data = 0xff;
	// nim_reg_write(dev,R05_TIMEOUT_TRH, &data, 1);
	_DEBUG
	printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	nim_s3501_set_hw_timeout(dev, 0xff);
	// data = 0x1f; // setting for soft search function
	// nim_reg_write(dev, R1B_TR_TIMEOUT_BAND, &data, 1);
	// AGC1 setting
	_DEBUG
	printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	nim_s3501_agc1_ctrl(dev, low_sym, NIM_OPTR_CHL_CHANGE);
	// Set symbol rate
	_DEBUG
	printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	nim_s3501_set_RS(dev, sym);
	if (priv->ul_status.m_s3501_type == NIM_CHIP_ID_M3501B)
	{
		// Only for M3501B
		_DEBUG
		printk("[%s][%d]\n", __FUNCTION__, __LINE__);
		nim_set_ts_rs(dev, sym);
	}
	// Set carry offset
	_DEBUG
	printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	nim_s3501_freq_offset_reset(dev, low_sym);
	// ttt
	_DEBUG
	printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	nim_s3501_cr_setting(dev, NIM_OPTR_CHL_CHANGE);
	// set workd mode
	// data = 0x34; // dvbs
	// data = 0x71; // dvbs2
	// data = 0x32; // dvbs2-hbcd-s
	// data = 0x52; // dvbs2-hbcd-s2
	// data = 0x73; // auto
	// ttt
	_DEBUG
	printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	nim_s3501_set_acq_workmode(dev, NIM_OPTR_CHL_CHANGE0);
	// set sweep range
	// ttt
	_DEBUG
	printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	nim_s3501_set_FC_Search_Range(dev, NIM_OPTR_CHL_CHANGE, 0x00);
	// ttt
	_DEBUG
	printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	nim_s3501_RS_Search_Range(dev, NIM_OPTR_CHL_CHANGE, 0x00);
	// ttt
	// LDPC parameter
	_DEBUG
	printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	nim_s3501_ldpc_setting(dev, NIM_OPTR_CHL_CHANGE, 0x00, 0x01);
	/*
	// use RS to caculate MOCLK
	data = 0x00;
	nim_reg_write(dev, RAD_TSOUT_SYMB, &data, 1);
	// Set IO pad driving
	// ttt
	data = 0x00;
	nim_reg_write(dev, RAF_TSOUT_PAD, &data, 1);
	*/
	// ttt
	/*
	data = 0x00;
	nim_reg_write(dev,RB1_TSOUT_SMT, &data, 1);
	*/
	// Carcy disable HBCD check, let time out. 2008-03-12
	// ttt
	_DEBUG
	printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	nim_s3501_hbcd_timeout(dev, NIM_OPTR_CHL_CHANGE);
	if (priv->ul_status.m_s3501_type == NIM_CHIP_ID_M3501B)
	{
		//when enter channel change, first close ts dummy.
		nim_close_ts_dummy(dev);
		//ECO_TS_EN = reg_cr9e[7], disable before dmy config successfully.
		nim_reg_read(dev, 0x9e, &data, 1);
		data = data & 0x7f;
		nim_reg_write(dev, 0x9e, &data, 1);
		/*
		 nim_reg_read(dev, 0xaf, &data, 1);
		 data = data | 0x10;
		 nim_reg_write(dev, 0xaf, &data, 1);
		*/
		/*
		 nim_reg_read(dev,RD8_TS_OUT_SETTING, &data, 1);
		 printk("when enter channel change reg_crd8 = %02x \n", data);
		*/
		//nim_s3501_set_dmy_format (dev);
		///-------------------------------------------------------------------------------
		///-------------------------------------------------------------------------------
	}
	else
	{
		//M3501A config.
		//use RS to caculate MOCLK
		//nim_s3501_get_bitmode(dev,&data);
		//nim_reg_write(dev,RAD_TSOUT_SYMB+0x01, &data, 1);
		nim_s3501_set_ts_mode(dev, 0x0, 0x0, 0x0, 0x0, 0X1);
	}
	//comm_delay(10);
	msleep(1);
	if (sym < 3000)
	{
		if (sym < 2000)
			data = 0x08;
		else
			data = 0x0a;
		nim_reg_write(dev, R1B_TR_TIMEOUT_BAND, &data, 1);
	}
	_DEBUG
	printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	nim_s3501_demod_ctrl(dev, NIM_DEMOD_CTRL_0X51);
	////printk("1111111[%dms]\n", YWOS_TimeNow() - TuneStartTime);
#ifdef CHANNEL_CHANGE_ASYNC
	priv->cur_freq = freq;
	priv->cur_sym = sym;
	NIM_FLAG_SET(priv->flag_id, NIM_FLAG_CHN_CHG_START);
#else
	nim_s3501_waiting_channel_lock(/*Inst,*/ dev, freq, sym);
#endif
	////printk(" Leave Fuction nim_s3501_channel_change \n");
	priv->ul_status.s3501_chanscan_stop_flag = 0;
	if (priv->bLock)
	{
		return DVBFE_ALGO_SEARCH_SUCCESS;
	}
	else
	{
		return DVBFE_ALGO_SEARCH_FAILED;
	}
}
#else
static enum dvbfe_search d3501_Search(struct dvb_frontend *fe,
									  struct dvb_frontend_parameters *p)
{
	//IOARCH_Handle_t IOHandle;
	struct nim_device *dev;
	struct nim_s3501_private *priv;
	//U32 TuneStartTime;
	UINT32 freq;
	UINT32 sym;
	//UINT8 result;
	UINT8 data = 0x10;
	UINT8 low_sym;
	int err;
	struct dvb_d3501_fe_state *state = fe->demodulator_priv;
	/******************************************/
	dev = (struct nim_device *)&state->spark_nimdev;
	priv = (struct nim_s3501_private *) dev->priv;
	_DEBUG
	printk("p->frequency is %d\n", p->frequency);
	priv->bLock = FALSE;
	//freq = 5150 - p->frequency / 1000;
	freq = p->frequency / 1000;
	sym = p->u.qpsk.symbol_rate / 1000;
	/******************************************/
////UINT8 channel_change_flag = 1; //bentao add for judge channel chang or soft_search in set_ts_mode
	//starttime = 0;
	////printk(" Enter Fuction nim_s3501_channel_change \n");
	_DEBUG
	{
		printk(" freq is %d\n", freq);
		printk(" sym is %d\n", sym);
	}
	//printk(" fec is %d\n", fec);
	priv->t_Param.t_phase_noise_detected = 0;
	priv->t_Param.t_dynamic_power_en = 0;
	priv->t_Param.t_last_snr = -1;
	priv->t_Param.t_last_iter = -1;
	priv->t_Param.t_aver_snr = -1;
	priv->t_Param.t_snr_state = 0;
	priv->t_Param.t_snr_thre1 = 256;
	priv->t_Param.t_snr_thre2 = 256;
	priv->t_Param.t_snr_thre3 = 256;
	priv->t_Param.phase_noise_detect_finish = 0x00;
	/*#ifdef CHANNEL_CHANGE_ASYNC
	 UINT32 flag_ptn = 0;
	 if (NIM_FLAG_WAIT(&flag_ptn, priv->flag_id, NIM_FLAG_CHN_CHG_START | NIM_FLAG_CHN_CHANGING, OSAL_TWF_ORW, 0) == OSAL_E_OK)
	 {
	 // channel chaning, stop the old changing first.
	 priv->ul_status.s3501_chanscan_stop_flag = 1;
	 //libc_printf("channel changing already, stop it first\n");
	 msleep(2); //sleep 2ms
	 }
#endif*/ //lwj remove
	if (priv->ul_status.m_s3501_type == NIM_CHIP_ID_M3501B)
	{
	_DEBUG
	printk("NIM_CHIP_ID_M3501B ########### d3501_Search\n");
	priv->ul_status.phase_err_check_status = 0;
	priv->ul_status.s3501d_lock_status = NIM_LOCK_STUS_NORMAL;
	}
	priv->ul_status.m_setting_freq = freq;
	//reset first
	_DEBUG
	printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	nim_s3501_demod_ctrl(dev, NIM_DEMOD_CTRL_0X91);
	_DEBUG
	printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	msleep(5); //sleep 5ms lwj add
	if ((0 == freq) || (0 == sym))
	return DVBFE_ALGO_SEARCH_ERROR;
	_DEBUG
	printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	nim_s3501_sym_config(dev, sym);
	_DEBUG
	printk("[%s][%d]\n", __FUNCTION__, __LINE__);
#if 1
	if (priv->ul_status.s3501_chanscan_stop_flag)
	{
	priv->ul_status.s3501_chanscan_stop_flag = 0;
	return DVBFE_ALGO_SEARCH_FAILED;
	}
#else
	if (Inst->ForceSearchTerm)
	{
	return SUCCESS;
	}
#endif
	// time for channel change and sof search.
	// ttt
	_DEBUG
	printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	nim_s3501_TR_CR_Setting(dev, NIM_OPTR_CHL_CHANGE);
	_DEBUG
	printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	low_sym = sym < 6500 ? 1 : 0; /* Symbol rate is less than 10M, low symbol rate */
	_DEBUG
	printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	nim_s3501_freq_offset_set(dev, low_sym, &freq);
	_DEBUG
	{
		printk("[%s][%d]freq = %d\n", __FUNCTION__, __LINE__, freq);

		printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	}
	if (nim_s3501_i2c_open(dev))
		return DVBFE_ALGO_SEARCH_FAILED;
	_DEBUG
	printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	err = d3501_set_tuner_params(fe, freq, sym);
	if (err < 0)
	{
		return DVBFE_ALGO_SEARCH_FAILED;
	}
	_DEBUG
	printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	if (nim_s3501_i2c_close(dev))
		return DVBFE_ALGO_SEARCH_FAILED;
	msleep(1);
	_DEBUG
	printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	if (nim_s3501_i2c_open(dev))
		return DVBFE_ALGO_SEARCH_FAILED;
	if (fe->ops.tuner_ops.get_status)
	{
		int iTunerLock;
		_DEBUG
		printk("[%s][%d]\n", __FUNCTION__, __LINE__);
		if (fe->ops.tuner_ops.get_status(fe, &iTunerLock) < 0)
		{
			_DEBUG
			printk("1. Tuner get_status err\n");
		}
	}
	_DEBUG
	printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	if (nim_s3501_i2c_close(dev))
		return DVBFE_ALGO_SEARCH_FAILED;
	// nim_s3501_adc_setting(dev);
	data = 0x10;
	_DEBUG
	printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	nim_reg_write(dev, RB3_PIN_SHARE_CTRL, &data, 1);
	_DEBUG
	printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	nim_s3501_adc_setting(dev);
	_DEBUG
	printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	nim_s3501_interrupt_mask_clean(dev);
	// hardware timeout setting
	// ttt
	// data = 0xff;
	// nim_reg_write(dev,R05_TIMEOUT_TRH, &data, 1);
	_DEBUG
	printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	nim_s3501_set_hw_timeout(dev, 0xff);
	// data = 0x1f; // setting for soft search function
	// nim_reg_write(dev, R1B_TR_TIMEOUT_BAND, &data, 1);
	// AGC1 setting
	_DEBUG
	printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	nim_s3501_agc1_ctrl(dev, low_sym, NIM_OPTR_CHL_CHANGE);
	// Set symbol rate
	_DEBUG
	printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	nim_s3501_set_RS(dev, sym);
	if (priv->ul_status.m_s3501_type == NIM_CHIP_ID_M3501B)
	{
		// Only for M3501B
		_DEBUG
		printk("[%s][%d]\n", __FUNCTION__, __LINE__);
		nim_set_ts_rs(dev, sym);
	}
	// Set carry offset
	_DEBUG
	printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	nim_s3501_freq_offset_reset(dev, low_sym);
	// ttt
	_DEBUG
	printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	nim_s3501_cr_setting(dev, NIM_OPTR_CHL_CHANGE);
	// set workd mode
	// data = 0x34; // dvbs
	// data = 0x71; // dvbs2
	// data = 0x32; // dvbs2-hbcd-s
	// data = 0x52; // dvbs2-hbcd-s2
	// data = 0x73; // auto
	// ttt
	_DEBUG
	printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	nim_s3501_set_acq_workmode(dev, NIM_OPTR_CHL_CHANGE0);
	// set sweep range
	// ttt
	_DEBUG
	printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	nim_s3501_set_FC_Search_Range(dev, NIM_OPTR_CHL_CHANGE, 0x00);
	// ttt
	_DEBUG
	printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	nim_s3501_RS_Search_Range(dev, NIM_OPTR_CHL_CHANGE, 0x00);
	// ttt
	// LDPC parameter
	_DEBUG
	printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	nim_s3501_ldpc_setting(dev, NIM_OPTR_CHL_CHANGE, 0x00, 0x01);
	/*
	// use RS to caculate MOCLK
	data = 0x00;
	nim_reg_write(dev, RAD_TSOUT_SYMB, &data, 1);
	// Set IO pad driving
	// ttt
	data = 0x00;
	nim_reg_write(dev, RAF_TSOUT_PAD, &data, 1);
	*/
	// ttt
	/*
	data = 0x00;
	nim_reg_write(dev,RB1_TSOUT_SMT, &data, 1);
	*/
	// Carcy disable HBCD check, let time out. 2008-03-12
	// ttt
	_DEBUG
	printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	nim_s3501_hbcd_timeout(dev, NIM_OPTR_CHL_CHANGE);
	if (priv->ul_status.m_s3501_type == NIM_CHIP_ID_M3501B)
	{
		//when enter channel change, first close ts dummy.
		nim_close_ts_dummy(dev);
		//ECO_TS_EN = reg_cr9e[7], disable before dmy config successfully.
		nim_reg_read(dev, 0x9e, &data, 1);
		data = data & 0x7f;
		nim_reg_write(dev, 0x9e, &data, 1);
		/*
		 nim_reg_read(dev, 0xaf, &data, 1);
		 data = data | 0x10;
		 nim_reg_write(dev, 0xaf, &data, 1);
		*/
		/*
		 nim_reg_read(dev,RD8_TS_OUT_SETTING, &data, 1);
		 printk("when enter channel change reg_crd8 = %02x \n", data);
		*/
		//nim_s3501_set_dmy_format (dev);
		///-------------------------------------------------------------------------------
		///-------------------------------------------------------------------------------
	}
	else
	{
		//M3501A config.
		//use RS to caculate MOCLK
		//nim_s3501_get_bitmode(dev,&data);
		//nim_reg_write(dev,RAD_TSOUT_SYMB+0x01, &data, 1);
		nim_s3501_set_ts_mode(dev, 0x0, 0x0, 0x0, 0x0, 0X1);
	}
	//comm_delay(10);
	msleep(1);
	if (sym < 3000)
	{
		if (sym < 2000)
			data = 0x08;
		else
			data = 0x0a;
		nim_reg_write(dev, R1B_TR_TIMEOUT_BAND, &data, 1);
	}
	_DEBUG
	printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	nim_s3501_demod_ctrl(dev, NIM_DEMOD_CTRL_0X51);
	////printk("1111111[%dms]\n", YWOS_TimeNow() - TuneStartTime);
#ifdef CHANNEL_CHANGE_ASYNC
	priv->cur_freq = freq;
	priv->cur_sym = sym;
	NIM_FLAG_SET(priv->flag_id, NIM_FLAG_CHN_CHG_START);
#else
	nim_s3501_waiting_channel_lock(/*Inst,*/ dev, freq, sym);
#endif
	////printk(" Leave Fuction nim_s3501_channel_change \n");
	priv->ul_status.s3501_chanscan_stop_flag = 0;
	if (priv->bLock)
	{
		return DVBFE_ALGO_SEARCH_SUCCESS;
	}
	else
	{
		return DVBFE_ALGO_SEARCH_FAILED;
	}
}
#endif
#endif

static int d3501_i2c_gate_ctrl(struct dvb_frontend *fe, int enable)
{
	struct dvb_d3501_fe_state *state = fe->demodulator_priv;
	if (enable)
	{
		if (nim_s3501_i2c_open(&state->spark_nimdev))
			return S3501_ERR_I2C_NO_ACK;
	}
	else
	{
		if (nim_s3501_i2c_close(&state->spark_nimdev))
			return S3501_ERR_I2C_NO_ACK;
	}
	return 0;
}

#if (DVB_API_VERSION < 5)
static int d3501_get_info(struct dvb_frontend *fe,
						  struct dvbfe_info *fe_info)
{
	//struct dvb_d0367_fe_ofdm_state* state = fe->demodulator_priv;
	/* get delivery system info */
	if (fe_info->delivery == DVBFE_DELSYS_DVBS2)
	{
		return 0;
	}
	else
	{
		return -EINVAL;
	}
	return 0;
}
#else
static int d3501_get_property(struct dvb_frontend *fe, struct dtv_property *tvp)
{
	//struct dvb_d0367_fe_ofdm_state* state = fe->demodulator_priv;
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
#endif

static int d3501_set_tone(struct dvb_frontend *fe, fe_sec_tone_mode_t tone)
{
	UINT8 data;
	struct dvb_d3501_fe_state *state = fe->demodulator_priv;
	struct nim_device *dev;
	struct nim_s3501_private *priv;
	dev = (struct nim_device *)&state->spark_nimdev;
	priv = (struct nim_s3501_private *) dev->priv;
	if (priv->ul_status.m_s3501_type == NIM_CHIP_ID_M3501B)
		data = (CRYSTAL_FREQ * 90 / 135);
	else
		data = (CRYSTAL_FREQ * 99 / 135);
	nim_reg_write(dev, R7C_DISEQC_CTRL + 0x14, &data, 1);
	_DEBUG
	printk("tone = %d\n", tone);
	if (tone == SEC_TONE_ON) /* Low band -> no 22KHz tone */
	{
		nim_reg_read(dev, R7C_DISEQC_CTRL, &data, 1);
		data = ((data & 0xF8) | 1);
		nim_reg_write(dev, R7C_DISEQC_CTRL, &data, 1);
	}
	else
	{
		nim_reg_read(dev, R7C_DISEQC_CTRL, &data, 1);
		data = ((data & 0xF8) | 0);
		nim_reg_write(dev, R7C_DISEQC_CTRL, &data, 1);
	}
	return 0;
}

static int d3501_set_voltage(struct dvb_frontend *fe, fe_sec_voltage_t voltage)
{
	struct dvb_d3501_fe_state *state = fe->demodulator_priv;
	switch (voltage)
	{
		case SEC_VOLTAGE_OFF:
			_DEBUG
			printk("set_voltage_off\n");
			stpio_set_pin(state->fe_lnb_on_off, 0);
			break;
		case SEC_VOLTAGE_13: /* vertical */
			_DEBUG
			printk("set_voltage_vertical \n");
			stpio_set_pin(state->fe_lnb_on_off, 1);
			msleep(1);
			stpio_set_pin(state->fe_lnb_13_18, 1);
			msleep(1);
			nim_s3501_set_polar(&state->spark_nimdev, NIM_PORLAR_VERTICAL);
			break;
		case SEC_VOLTAGE_18: /* horizontal */
			_DEBUG
			printk("set_voltage_horizontal\n");
			stpio_set_pin(state->fe_lnb_on_off, 1);
			msleep(1);
			stpio_set_pin(state->fe_lnb_13_18, 0);
			msleep(1);
			nim_s3501_set_polar(&state->spark_nimdev, NIM_PORLAR_HORIZONTAL);
			break;
		default:
			break;
	}
	return 0;
}

int d3501_send_diseqc_msg(struct dvb_frontend *fe,
						  struct dvb_diseqc_master_cmd *cmd)
{
	struct dvb_d3501_fe_state *state = fe->demodulator_priv;
	unsigned char *pCurrent_Data = cmd->msg;
	UINT8 data, temp;
	UINT16 timeout, timer;
	UINT8 i;
	struct nim_s3501_private *priv;
	struct nim_device *dev;
	dev = (struct nim_device *)&state->spark_nimdev;
	priv = (struct nim_s3501_private *) dev->priv;
	if (priv->ul_status.m_s3501_type == NIM_CHIP_ID_M3501B)
		data = (CRYSTAL_FREQ * 90 / 135);
	else
		data = (CRYSTAL_FREQ * 99 / 135);
	nim_reg_write(dev, R7C_DISEQC_CTRL + 0x14, &data, 1);
	{
		//diseqc init for mode byte every time :test
		nim_reg_read(dev, R7C_DISEQC_CTRL, &data, 1);
		data = ((data & 0xF8) | 0x00);
		nim_reg_write(dev, R7C_DISEQC_CTRL, &data, 1);
		//write the writed data count
		nim_reg_read(dev, R7C_DISEQC_CTRL, &data, 1);
		temp = cmd->msg_len - 1;
		data = ((data & 0xC7) | (temp << 3));
		nim_reg_write(dev, R7C_DISEQC_CTRL, &data, 1);
		//write the data
		for (i = 0; i < cmd->msg_len; i++)
		{
			nim_reg_write(dev, (i + 0x7E), pCurrent_Data + i, 1);
		}
		//clear the interupt
		nim_reg_read(dev, R7C_DISEQC_CTRL + 0x01, &data, 1);
		data &= 0xF8;
		nim_reg_write(dev, R7C_DISEQC_CTRL + 0x01, &data, 1);
		//write the control bits
		nim_reg_read(dev, R7C_DISEQC_CTRL, &data, 1);
		temp = 0x04;
		data = ((data & 0xF8) | temp);
		nim_reg_write(dev, R7C_DISEQC_CTRL, &data, 1);
		//waiting for the send over
		timer = 0;
		timeout = 75 + 13 * cmd->msg_len;
		while (timer < timeout)
		{
			nim_reg_read(dev, R7C_DISEQC_CTRL + 0x01, &data, 1);
			//change < to != by Joey
			if (0 != (data & 0x07))
			{
				break;
			}
			msleep(10);
			timer += 10;
		}
		if (1 == (data & 0x07))
		{
			msleep(100); // dhy 20100416 , very importantwe add this 100ms delay. some time if no delay the diseqc swithc not good.
			return 0;
		}
		else
		{
			return -1;
		}
	}
	return (0);
}

static struct dvb_frontend_ops spark_d3501_ops =
{

	.info = {
		.name = "spark_d3501",
		.type = FE_QPSK,
		.frequency_min = 950000,
		.frequency_max = 2150000,
		.frequency_stepsize = 0,
		.frequency_tolerance = 0,
		.symbol_rate_min = 1000000,
		.symbol_rate_max = 45000000,
		.caps = FE_CAN_INVERSION_AUTO |
		FE_CAN_FEC_AUTO |
		FE_CAN_QPSK |
		FE_CAN_2G_MODULATION
	},

	.init = d3501_init,
	.release = d3501_release,
	.read_ber = d3501_read_ber,
	.read_snr = d3501_read_snr,
	.read_signal_strength = d3501_read_signal_strength,
	.read_status = d3501_read_status,
	.get_frontend_algo = d3501_frontend_algo,
	.search = d3501_Search,
	.i2c_gate_ctrl = d3501_i2c_gate_ctrl,
#if (DVB_API_VERSION < 5)
	.get_info = d3501_get_info,
#else
	.get_property = d3501_get_property,
#endif

	.set_tone = d3501_set_tone,
	.set_voltage = d3501_set_voltage,
	.diseqc_send_master_cmd = d3501_send_diseqc_msg,
#if 0
	.sleep = stv090x_sleep,

	.diseqc_send_burst = stv090x_send_diseqc_burst,
	.diseqc_recv_slave_reply = stv090x_recv_slave_reply,

#endif
};

static int d3501_initition(struct nim_device *dev, struct i2c_adapter *i2c)
{
	struct nim_s3501_private *priv_mem;
	priv_mem = (struct nim_s3501_private *)Kzalloc(sizeof(struct nim_s3501_private));
	if (priv_mem == NULL)
	{
		kfree(dev);
		YWOSTRACE((YWOS_TRACE_ERROR, "[ERROR][demod_d3501_Open]Alloc nim device prive memory error! \n"));
		return YWHAL_ERROR_NO_MEMORY;
	}
	YWLIB_Memset(dev, 0, sizeof(struct nim_device));
	YWLIB_Memset(priv_mem, 0, sizeof(struct nim_s3501_private));
	dev->priv = (void *) priv_mem;
	//dev->base_addr = 0x66 ; /* 3501 i2c base address*/
	dev->base_addr = 0x66 >> 1 ; /* 3501 i2c base address*/
	priv_mem->i2c_type_id = 0;//question
	priv_mem->i2c_adap = i2c;
	if (!priv_mem->i2c_adap)
	{
		return -1;
	}
	_DEBUG
	printk("priv_mem->i2c_adap = %0x\n", (int)priv_mem->i2c_adap);
	//priv_mem->tuner_id = Handle; //lwj add important
	priv_mem->ext_dm_config.i2c_type_id = 0;
	priv_mem->ext_dm_config.i2c_base_addr = 0x66; //3501 i2c base addr //0xC0; //7306 i2c base address
	priv_mem->ul_status.m_enable_dvbs2_hbcd_mode = 0;
	priv_mem->ul_status.m_dvbs2_hbcd_enable_value = 0x7f;
	priv_mem->ul_status.nim_s3501_sema = OSAL_INVALID_ID;
	priv_mem->ul_status.s3501_autoscan_stop_flag = 0;
	priv_mem->ul_status.s3501_chanscan_stop_flag = 0;
	priv_mem->ul_status.old_ber = 0;
	priv_mem->ul_status.old_per = 0;
	priv_mem->ul_status.m_hw_timeout_thr = 0;
	priv_mem->ul_status.old_ldpc_ite_num = 0;
	priv_mem->ul_status.c_RS = 0;
	priv_mem->ul_status.phase_err_check_status = 0;
	priv_mem->ul_status.s3501d_lock_status = NIM_LOCK_STUS_NORMAL;
	priv_mem->ul_status.m_s3501_type = 0x00;
	priv_mem->ul_status.m_s3501_type = NIM_CHIP_ID_M3501B; //lwj add for 3501B
	priv_mem->ul_status.m_setting_freq = 123;
	priv_mem->ul_status.m_Err_Cnts = 0x00;
//	priv_mem->tsk_status.m_lock_flag = NIM_LOCK_STUS_NORMAL;//
//	priv_mem->tsk_status.m_task_id = 0x00;//
	priv_mem->t_Param.t_aver_snr = -1;
	priv_mem->t_Param.t_last_iter = -1;
	priv_mem->t_Param.t_last_snr = -1;
	priv_mem->t_Param.t_snr_state = 0;
	priv_mem->t_Param.t_snr_thre1 = 256;
	priv_mem->t_Param.t_snr_thre2 = 256;
	priv_mem->t_Param.t_snr_thre3 = 256;
	priv_mem->t_Param.t_dynamic_power_en = 0;
	priv_mem->t_Param.t_phase_noise_detected = 0;
	priv_mem->t_Param.t_reg_setting_switch = 0x0f;
	priv_mem->t_Param.t_i2c_err_flag = 0x00;
	priv_mem->flag_id = OSAL_INVALID_ID;
	priv_mem->bLock = FALSE;
	priv_mem->Tuner_Config_Data.QPSK_Config = 0xe9; //lwj
	//priv_mem->Tuner_Config_Data.QPSK_Config = 0x29; //lf
	nim_s3501_get_type(dev);
	if (priv_mem->ul_status.m_s3501_type == NIM_CHIP_ID_M3501A && // Chip 3501A
			(priv_mem->Tuner_Config_Data.QPSK_Config & 0xc0) == M3501_2BIT_MODE) //TS 2bit mode
	{
		//for M3606+M3501A full nim ssi-2bit patch, auto change to 1bit mode.
		priv_mem->Tuner_Config_Data.QPSK_Config &= 0x3f; // set to TS 1 bit mode
		//libc_printf("M3501A SSI 2bit mode, auto change to 1bit mode\n");
	}
	return 0;
}

static int d3501_term(struct nim_device *dev)
{
	struct nim_s3501_private *priv_mem;
	priv_mem = (struct nim_s3501_private *)dev->priv;
	i2c_put_adapter(priv_mem->i2c_adap);
	kfree(priv_mem);
	return 0;
}

struct dvb_frontend *dvb_d3501_fe_qpsk_attach(
	const struct d3501_config *config,
	struct i2c_adapter *i2c)
{
	struct dvb_d3501_fe_state *state = NULL;
	/* allocate memory for the internal state */
	state = kmalloc(sizeof(struct dvb_d3501_fe_state), GFP_KERNEL);
	if (state == NULL) goto error;
	memset(state, 0, sizeof(struct dvb_d3501_fe_state));
	/* create dvb_frontend */
	memcpy(&state->frontend.ops, &spark_d3501_ops, sizeof(struct dvb_frontend_ops));
	if (d3501_initition(&state->spark_nimdev, i2c) < 0)
	{
		kfree(state);
		return NULL;
	}
	memcpy(&state->frontend.ops.info.name, config->name, sizeof(config->name));
	state->frontend.demodulator_priv = state;
#if 1
	switch (config->i)
	{
		case 0:
		{
			state->fe_lnb_13_18 = stpio_request_pin(15, 6, "lnb_0 13/18", STPIO_OUT);
			state->fe_lnb_on_off = stpio_request_pin(15, 7, "lnb_0 onoff", STPIO_OUT);
			if (!state->fe_lnb_13_18 || !state->fe_lnb_on_off)
			{
				printk("lnb_0 request pin failed\n");
				if (state->fe_lnb_13_18)
				{
					stpio_free_pin(state->fe_lnb_13_18);
				}
				if (state->fe_lnb_on_off)
				{
					stpio_free_pin(state->fe_lnb_on_off);
				}
				kfree(state);
				return NULL;
			}
			break;
		}
		case 1:
		{
			state->fe_lnb_13_18 = stpio_request_pin(15, 4, "lnb_1 13/18", STPIO_OUT);
			state->fe_lnb_on_off = stpio_request_pin(15, 5, "lnb_1 onoff", STPIO_OUT);
			if (!state->fe_lnb_13_18 || !state->fe_lnb_on_off)
			{
				printk("lnb_1 request pin failed\n");
				if (state->fe_lnb_13_18)
				{
					stpio_free_pin(state->fe_lnb_13_18);
				}
				if (state->fe_lnb_on_off)
				{
					stpio_free_pin(state->fe_lnb_on_off);
				}
				kfree(state);
				return NULL;
			}
			break;
		}
		default:
			break;
	}
#endif /* 0 */
	{
		INT32 ret = ERR_FAILED;
		ret = nim_s3501_hw_check(&state->spark_nimdev);
		if (ret != SUCCESS)
		{
			d3501_term(&state->spark_nimdev);
			kfree(state);
			return NULL;
		}
	}
	return &state->frontend;
error:
	kfree(state);
	return NULL;
}

