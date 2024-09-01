/*****************************************************************************
* Copyright (C)2011 FULAN. All Rights Reserved.
*
* File: nim_panic6158.c
*
* Description: Source file in LLD.
*
* History:
*
* Date Athor Version Reason
* ============ ============= ========= =================
* 1.2011/12/12 DMQ Ver 0.1 Create file.
*
*****************************************************************************/
#include <linux/kernel.h> /* Kernel support */
#include <linux/delay.h>
#include <linux/i2c.h>

#include <linux/dvb/version.h>

#include "ywdefs.h"
#include "ywos.h"
#include "ywlib.h"
#include "ywhal_assert.h"
#include "ywtuner_ext.h"
#include "tuner_def.h"
#include "ioarch.h"

#include "ioreg.h"
#include "tuner_interface.h"
#include "ywtuner_def.h"

#include "mxl_common.h"
#include "mxL_user_define.h"
#include "mxl301rf.h"
#include "dvb_frontend.h"
#include "d6158.h"

#include "stv0367ofdm_init.h"

UINT8 nim_panic6158_cnt = 0;
#if 0
static const char *nim_panic6158_name[] =
{
	"NIM_PANIC6158_0",
	"NIM_PANIC6158_1",
};
#endif /* 0 */

INT32 tun_mxl301_set_addr(UINT32 tuner_idx, UINT8 addr, UINT32 i2c_mutex_id);
INT32 MxL_Check_RF_Input_Power(UINT32 tuner_idx, U32 *RF_Input_Level);

INT32 i2c_write(INT32 id, UINT8 slvadr, UINT8 *adr, UINT8 len)
{
	(void)id;
	(void)slvadr;
	(void)adr;
	(void)len;
	printf("%s\n", __FUNCTION__);
	printf("id = %d, slvadr = %d, len = %d\n", id, slvadr, len);
	return SUCCESS;
}

INT32 i2c_read(INT32 id, UINT8 slvadr, UINT8 *adr, UINT8 len)
{
	(void)id;
	(void)slvadr;
	(void)adr;
	(void)len;
	printf("%s\n", __FUNCTION__);
	printf("id = %d, slvadr = %d, len = %d\n", id, slvadr, len);
	return SUCCESS;
}

//=============================================================================
// MN_I2C.c
//=============================================================================

/*! Write 1byte */
INT32 DMD_I2C_Write(struct nim_panic6158_private *param, UINT8 slvadr, UINT8 adr, UINT8 data)
{
	/* '11/08/01 : OKAMOTO Implement I2C read / write handler. */
	UINT8 apData[2];
	UINT16 length;
	INT32 ret = SUCCESS;
	osal_mutex_lock(param->i2c_mutex_id, OSAL_WAIT_FOREVER_TIME);
	apData[0] = adr;
	apData[1] = data;
	length = (UINT16)sizeof(apData);
	if (SUCCESS != i2c_write(param->tc.ext_dm_config.i2c_type_id, slvadr, apData, length))
		ret = !SUCCESS;
	//SHARP6158_LOG(ret, 1, slvadr, apData, length);
	osal_mutex_unlock(param->i2c_mutex_id);
	return ret;
}

/*! Read 1byte */
INT32 DMD_I2C_Read(struct nim_panic6158_private *param, UINT8 slvadr, UINT8 adr, UINT8 *data)
{
	/* '11/08/01 : OKAMOTO Implement I2C read / write handler. */
	UINT8 apData[1];
	UINT16 length = 1;
	INT32 ret = SUCCESS;
	osal_mutex_lock(param->i2c_mutex_id, OSAL_WAIT_FOREVER_TIME);
	apData[0] = adr;
	if (SUCCESS != i2c_write(param->tc.ext_dm_config.i2c_type_id, slvadr, apData, length))
		ret = !SUCCESS;
	//SHARP6158_LOG(ret, 1, slvadr, apData, length);
	/* '11/08/05 : OKAMOTO Correct I2C read. */
	if (SUCCESS != i2c_read(param->tc.ext_dm_config.i2c_type_id, slvadr, data, 1))
		ret = !SUCCESS;
	//SHARP6158_LOG(ret, 0, slvadr, data, 1);
	osal_mutex_unlock(param->i2c_mutex_id);
	return ret;
}

INT32 DMD_I2C_MaskWrite(struct nim_panic6158_private *param, UINT8 slvadr, UINT8 adr, UINT8 mask, UINT8 data)
{
	UINT8 rd;
	INT32 ret = SUCCESS;
	ret = DMD_I2C_Read(param, slvadr, adr, &rd);
	rd |= mask & data;
	rd &= (mask ^ 0xff) | data;
	ret |= DMD_I2C_Write(param, slvadr, adr, rd);
	return ret;
}

/*! Write/Read any Length*/
/*====================================================*
 DMD_I2C_WriteRead
 --------------------------------------------------
 Description I2C Write/Read any Length.
 Argument
 <Common Prametor>
 slvadr (Slave Addr of demod without R/W bit.)
 <Write Prametors>
 adr (Address of demod.) ,
 wdata (Write data)
 wlen (Write length)
 <Read Prametors>
 rdata (Read result)
 rlen (Read length)
 Return Value DMD_ERROR_t (DMD_E_OK:success, DMD_E_ERROR:error)
 *====================================================*/
INT32 DMD_I2C_WriteRead(struct nim_panic6158_private *param, UINT8 slvadr, UINT8 adr, UINT8 *wdata, U32 wlen, UINT8 *rdata, U32 rlen)
{
	{
		//Write
		UINT16 length;
		if ((wlen + 1) > 0xFFFF)
			return !SUCCESS;
		else
			length = (UINT16)wlen + 1;
		osal_mutex_lock(param->i2c_mutex_id, OSAL_WAIT_FOREVER_TIME);
		{
			UINT8 *data = NULL;
			UINT8 *pNewBuf = NULL;
			UINT8 buf[128]; //In order to avoid to call malloc() frequently for small size command.
			if (length > 128)
			{
				pNewBuf = (UINT8 *)malloc(length);
				data = pNewBuf;
			}
			else
				data = buf;
			data[0] = adr;
			memcpy(&data[1], wdata, wlen);
			{
				BOOL bRetVal = i2c_write(param->tc.ext_dm_config.i2c_type_id, slvadr, data, length);
				//SHARP6158_LOG(bRetVal, 1, slvadr, data, length);
				if (pNewBuf)
				{
					free(pNewBuf);
					pNewBuf = NULL;
				}
				if (bRetVal != SUCCESS)
				{
					osal_mutex_unlock(param->i2c_mutex_id);
					return !SUCCESS;
				}
			}
		}
	}
	if (rlen != 0)
	{
		//Read
		BOOL bRetVal;
		UINT16 length;
		length = (UINT16)rlen;
		bRetVal = i2c_read(param->tc.ext_dm_config.i2c_type_id, slvadr, rdata, length);
		//SHARP6158_LOG(bRetVal, 0, slvadr, rdata, length);
		if (SUCCESS != bRetVal)
		{
			osal_mutex_unlock(param->i2c_mutex_id);
			return !SUCCESS;
		}
	}
	osal_mutex_unlock(param->i2c_mutex_id);
	return SUCCESS;
}

/* **************************************************** */
/*! Write&Read any length from/to Tuner via Demodulator */
/* **************************************************** */
INT32 DMD_TCB_WriteRead(void *nim_dev_priv, UINT8 tuner_address, UINT8 *wdata, int wlen, UINT8 *rdata, int rlen)
{
	//PANIC6158_T2_ADDR: T2 demodulator address.
	//data[]: data submitted by tuner driver, they will be sent to tuner.
	UINT8 d[DMD_TCB_DATA_MAX];
	int i;
	INT32 ret = SUCCESS;
	struct nim_panic6158_private *param = (struct nim_panic6158_private *)nim_dev_priv;
	if (wlen >= DMD_TCB_DATA_MAX || rlen >= DMD_TCB_DATA_MAX)
		return !SUCCESS;
	/* Set TCB Through Mode */
#if 0
	ret = DMD_I2C_Write(param, PANIC6158_T2_ADDR, DMD_TCBSET, 0x53);
#else
	/* '11/11/14 : OKAMOTO Update to "MN88472_Device_Driver_111028". */
	ret = DMD_I2C_MaskWrite(param, PANIC6158_T2_ADDR, DMD_TCBSET, 0x7f, 0x53);
#endif
	ret |= DMD_I2C_Write(param, PANIC6158_T2_ADDR, DMD_TCBADR, 0x00);
	if ((wlen == 0 && rlen == 0) || (wlen != 0))
	{
		//Write
		d[0] = tuner_address & 0xFE;
		for (i = 0; i < wlen; i++)
			d[i + 1] = wdata[i];
#if 0
//		ret |= DMD_I2C_WriteRead(param, PANIC6158_T2_ADDR , DMD_TCBCOM , d , wlen + 2 , 0 , 0 );
		ret |= DMD_I2C_WriteRead(param, PANIC6158_T2_ADDR, DMD_TCBCOM, d, wlen + 1, 0, 0);
#else
		/* Read/Write *//*看ret 的直，如果是0则通*/
		if (!rdata && rlen != 0)
			return !SUCCESS;
		ret |= DMD_I2C_WriteRead(param, PANIC6158_T2_ADDR, DMD_TCBCOM, d, wlen + 1, rdata, rlen);
#endif
	}
	else
	{
		//Read
		if (!rdata || rlen == 0)
			return !SUCCESS;
		d[0] = tuner_address | 1;
//		d[1] = PANIC6158_T2_ADDR | 1;
//		ret |= DMD_I2C_WriteRead(param, PANIC6158_T2_ADDR , DMD_TCBCOM , d , 2 , 0 , 0 );
// ret |= DMD_I2C_Read(param, PANIC6158_T2_ADDR , DMD_TCBCOM , data);
//		ret |= DMD_I2C_WriteRead(param, PANIC6158_T2_ADDR , DMD_TCBCOM , d , 2 , rdata , 1 );
		ret |= DMD_I2C_WriteRead(param, PANIC6158_T2_ADDR, DMD_TCBCOM, d, 1, rdata, 1);
	}
	return ret;
}

/* '11/08/05 : OKAMOTO Implement "DMD_wait". */
/*! Timer wait */
INT32 DMD_wait(U32 msecond)
{
	if (msecond > 0xFFFF)
		return !SUCCESS;
//	timer_delay_msec((UINT16) msecond);
	osal_task_sleep(msecond);
	return SUCCESS;
}

#if 0
static INT32 nim_reg_write(struct nim_device *dev, UINT8 mode, UINT8 reg, UINT8 *data, UINT8 len)
{
	INT32 ret = SUCCESS;
	UINT16 i, cycle;
	UINT8 value[DEMO_I2C_MAX_LEN + 1];
	struct nim_panic6158_private *priv = (struct nim_panic6158_private *)dev->priv;
	if (DEMO_BANK_C < mode)
		return !SUCCESS;
	cycle = len / DEMO_I2C_MAX_LEN;
	osal_mutex_lock(priv->i2c_mutex_id, OSAL_WAIT_FOREVER_TIME);
	for (i = 0; i < cycle; i++)
	{
		value[0] = reg + i * DEMO_I2C_MAX_LEN;
		MEMCPY(&value[1], &data[i * DEMO_I2C_MAX_LEN], DEMO_I2C_MAX_LEN);
		ret = i2c_write(dev->i2c_type_id, priv->i2c_addr[mode], value, DEMO_I2C_MAX_LEN + 1);
		if (SUCCESS != ret)
		{
			osal_mutex_unlock(priv->i2c_mutex_id);
			NIM_PANIC6158_PRINTF("nim write i2c failed!\n");
			return ret;
		}
	}
	if (len > i * DEMO_I2C_MAX_LEN)
	{
		value[0] = reg + i * DEMO_I2C_MAX_LEN;
		MEMCPY(&value[1], &data[i * DEMO_I2C_MAX_LEN], len - i * DEMO_I2C_MAX_LEN);
		ret = i2c_write(dev->i2c_type_id, priv->i2c_addr[mode], value, len - i * DEMO_I2C_MAX_LEN + 1);
		if (SUCCESS != ret)
		{
			osal_mutex_unlock(priv->i2c_mutex_id);
			NIM_PANIC6158_PRINTF("nim write i2c failed!\n");
			return ret;
		}
	}
	osal_mutex_unlock(priv->i2c_mutex_id);
	return ret;
}

static INT32 nim_reg_read(struct nim_device *dev, UINT8 mode, UINT8 reg, UINT8 *data, UINT8 len)
{
	INT32 ret = SUCCESS;
	struct nim_panic6158_private *priv = (struct nim_panic6158_private *)dev->priv;
	if (DEMO_BANK_C < mode || DEMO_I2C_MAX_LEN < len)
		return !SUCCESS;
	osal_mutex_lock(priv->i2c_mutex_id, OSAL_WAIT_FOREVER_TIME);
	ret = i2c_write(dev->i2c_type_id, priv->i2c_addr[mode], &reg, 1);
	if (SUCCESS != ret)
	{
		osal_mutex_unlock(priv->i2c_mutex_id);
		NIM_PANIC6158_PRINTF("nim write i2c failed!\n");
		return ret;
	}
	ret = i2c_read(dev->i2c_type_id, priv->i2c_addr[mode], data, len);
	if (SUCCESS != ret)
	{
		NIM_PANIC6158_PRINTF("nim read i2c failed!\n");
	}
	osal_mutex_unlock(priv->i2c_mutex_id);
	return ret;
}
#else
static INT32 nim_reg_read(struct nim_device *dev, UINT8 mode, UINT8 bMemAdr, UINT8 *pData, UINT8 bLen)
{
	//INT32 err;
	//err = TUNER_IOARCH_ReadWrite(dev->DemodIOHandle[mode], TUNER_IO_SA_READ, bMemAdr, pData, bLen, 50);
	//return err;
	int ret;
	struct nim_panic6158_private *priv_mem = (struct nim_panic6158_private *)dev->priv;
	u8 b0[] = { bMemAdr };
	struct i2c_msg msg[] =
	{
		{ .addr = priv_mem->i2c_addr[mode] >> 1, .flags = 0, .buf = b0, .len = 1 },
		{ .addr = priv_mem->i2c_addr[mode] >> 1, .flags = I2C_M_RD, .buf = pData, .len = bLen }
	};
	ret = i2c_transfer(priv_mem->i2c_adap, msg, 2);
#if 0
	int i;
	printk("[ DEMOD ]len:%d MemAddr:%x addr%x\n", bLen, bMemAdr, dev->base_addr);
	for (i = 0; i < bLen; i++)
	{
		printk("[DATA]%x ", pData[i]);
	}
	printk("\n");
#endif
	if (ret != 2)
	{
		if (ret != -ERESTARTSYS)
			printk("READ ERROR, Reg=[0x%02x], Status=%d\n", bMemAdr, ret);
		return ret < 0 ? ret : -EREMOTEIO;
	}
	return SUCCESS;
}
static INT32 nim_reg_write(struct nim_device *dev, UINT8 mode, UINT8 bMemAdr, UINT8 *pData, UINT8 bLen)
{
	//INT32 err;
	//printf("mode = %d\n", mode);
	//printf("dev->DemodIOHandle[mode] = %d\n", dev->DemodIOHandle[mode]);
	//err = TUNER_IOARCH_ReadWrite(dev->DemodIOHandle[mode], TUNER_IO_SA_WRITE, bMemAdr, pData, bLen, 50) ;
	// return err;
	int ret;
	u8 buf[1 + bLen];
	struct nim_panic6158_private *priv_mem = (struct nim_panic6158_private *)dev->priv;
	struct i2c_msg i2c_msg = {.addr = priv_mem->i2c_addr[mode] >> 1, .flags = 0, .buf = buf, .len = 1 + bLen };
#if 0
	int i;
	for (i = 0; i < bLen; i++)
	{
		pData[i] = 0x92 + 3 * i;
		printk("%02x ", pData[i]);
	}
	printk(" write addr %x\n", dev->base_addr);
#endif
	priv_mem = (struct nim_panic6158_private *)dev->priv;
	buf[0] = bMemAdr;
	memcpy(&buf[1], pData, bLen);
	ret = i2c_transfer(priv_mem->i2c_adap, &i2c_msg, 1);
	if (ret != 1)
	{
		//if (ret != -ERESTARTSYS)
		printk("WRITE ERR:Reg=[0x%04x], Data=[0x%02x ...], Count=%u, Status=%d\n",
			   bMemAdr, pData[0], bLen, ret);
		return ret < 0 ? ret : -EREMOTEIO;
	}
	return SUCCESS;
}

#endif

static INT32 nim_reg_mask_write(struct nim_device *dev, UINT8 mode, UINT8 reg, UINT8 mask, UINT8 data)
{
	INT32 ret;
	UINT8 rd;
	//printf("[%s]%d\n",__FUNCTION__,__LINE__);
	ret = nim_reg_read(dev, mode, reg, &rd, 1);
	//printf("[%s]%d\n",__FUNCTION__,__LINE__);
	rd |= mask & data;
	rd &= (mask ^ 0xff) | data;
	//printf("[%s]%d\n",__FUNCTION__,__LINE__);
	ret |= nim_reg_write(dev, mode, reg, &rd, 1);
	//printf("[%s]%d\n",__FUNCTION__,__LINE__);
	return ret;
}

static INT32 log10_easy(UINT32 cnr)
{
	INT32 ret;
	UINT32 c;
	c = 0;
	while (cnr > 100)
	{
		cnr = cnr / 10;
		c++;
	}
	ret = logtbl[cnr] + c * 1000 + 1000;
	return ret;
}

static INT32 nim_calculate_ber(struct nim_device *dev, UINT32 *err, UINT32 *len, UINT32 *sum)
{
	UINT8 data[3];
	struct nim_panic6158_private *priv;
	priv = (struct nim_panic6158_private *)dev->priv;
	if (DEMO_BANK_T == priv->system || DEMO_BANK_C == priv->system)
	{
		//SET BERSET1[5] = 0
		nim_reg_read(dev, DEMO_BANK_T, DMD_BERSET1_T, data, 1);
		data[0] &= 0xDF; //1101_1111
		nim_reg_write(dev, DEMO_BANK_T, DMD_BERSET1_T, data, 1);
		//SET BERRDSET[3:0] = 0101
		nim_reg_read(dev, DEMO_BANK_T, DMD_BERRDSET_T, data, 1);
		data[0] = (data[0] & 0xF0) | 0x5;
		nim_reg_write(dev, DEMO_BANK_T, DMD_BERRDSET_T, data, 1);
		//Read ERROR
		nim_reg_read(dev, DEMO_BANK_T, DMD_BERRDU_T, data, 3);
		*err = data[0] * 0x10000 + data[1] * 0x100 + data[2];
		//Read BERLEN
		nim_reg_read(dev, DEMO_BANK_T, DMD_BERLENRDU_T, data, 2);
		*len = *sum = data[0] * 0x100 + data[1];
		*sum = *sum * 203 * 8;
	}
	else if (DEMO_BANK_T2 == priv->system)
	{
		UINT8 common = 0;
		nim_reg_read(dev, DEMO_BANK_T2, DMD_BERSET, data, 1);
		if (common == 0)
		{
			data[0] |= 0x20;//BERSET[5] = 1 (BER after LDPC)
			data[0] &= 0xef;//BERSET[4] = 0 (Data PLP)
		}
		else
		{
			data[0] |= 0x20;//BERSET[5] = 1 (BER after LDPC)
			data[0] |= 0x10;//BERSET[4] = 1 (Common PLP)
		}
		nim_reg_write(dev, DEMO_BANK_T2, DMD_BERSET, data, 1);
		//Read ERROR
		nim_reg_read(dev, DEMO_BANK_T2, DMD_BERRDU, data, 3);
		*err = data[0] * 0x10000 + data[1] * 0x100 + data[2];
		//Read BERLEN
		nim_reg_read(dev, DEMO_BANK_T2, DMD_BERLEN, data, 1);
		*len = *sum = (1 << (data[0] & 0xf));
		if (common == 0)
		{
			data[0] = 0x3;
			nim_reg_write(dev, DEMO_BANK_T2, DMD_TPDSET2, data, 1);
		}
		else
		{
			nim_reg_read(dev, DEMO_BANK_T2, DMD_TPD2, data, 1);
		}
		if ((data[0] & 0x1) == 1)
		{
			//FEC TYPE = 1
			switch ((data[0] >> 2) & 0x7)
			{
				case 0:
					*sum = (*sum) * 32400;
					break;
				case 1:
					*sum = (*sum) * 38880;
					break;
				case 2:
					*sum = (*sum) * 43200;
					break;
				case 3:
					*sum = (*sum) * 48600;
					break;
				case 4:
					*sum = (*sum) * 51840;
					break;
				case 5:
					*sum = (*sum) * 54000;
					break;
			}
		}
		else
		{
			//FEC TYPE = 0
			switch ((data[0] >> 2) & 0x7)
			{
				case 0:
					*sum = (*sum) * 7200 * 4;
					break;
				case 1:
					*sum = (*sum) * 9720 * 4;
					break;
				case 2:
					*sum = (*sum) * 10800 * 4;
					break;
				case 3:
					*sum = (*sum) * 11880 * 4;
					break;
				case 4:
					*sum = (*sum) * 12600 * 4;
					break;
				case 5:
					*sum = (*sum) * 13320 * 4;
					break;
			}
		}
	}
	return SUCCESS;
}

static INT32 nim_calculate_cnr(struct nim_device *dev, UINT32 *cnr_i, UINT32 *cnr_d)
{
	UINT8 data[4];
	INT32 cnr;
	INT32 sig, noise;
	struct nim_panic6158_private *priv;
	priv = (struct nim_panic6158_private *)dev->priv;
	switch (priv->system)
	{
		case DEMO_BANK_T:
			nim_reg_read(dev, DEMO_BANK_T, DMD_CNRDU_T, &data[0], 2);
			cnr = data[0] * 0x100 + data[1];
			if (cnr != 0)
			{
				cnr = 65536 / cnr;
				cnr = log10_easy(cnr) + 200;
				if (cnr < 0) cnr = 0;
			}
			else
				cnr = 0;
			*cnr_i = (UINT32) cnr / 100;
			*cnr_d = (UINT32) cnr % 100;
			break;
		case DEMO_BANK_T2:
			nim_reg_read(dev, DEMO_BANK_T2, DMD_CNFLG, &data[0], 3);
			cnr = data[1] * 0x100 + data[2];
			if (cnr != 0)
			{
				if (data[0] & 0x4)
				{
					//MISO
					cnr = 16384 / cnr;
					cnr = log10_easy(cnr) - 600;
					if (cnr < 0) cnr = 0;
					*cnr_i = (UINT32) cnr / 100;
					*cnr_d = (UINT32) cnr % 100;
				}
				else
				{
					//SISO
					cnr = 65536 / cnr;
					cnr = log10_easy(cnr) + 200;
					if (cnr < 0) cnr = 0;
					*cnr_i = (UINT32) cnr / 100;
					*cnr_d = (UINT32) cnr % 100;
				}
			}
			else
			{
				*cnr_i = 0;
				*cnr_d = 0;
			}
			break;
		case DEMO_BANK_C:
			nim_reg_read(dev, DEMO_BANK_C, DMD_CNMON1_C, data, 4);
			sig = data[0] * 0x100 + data[1];
			noise = data[2] * 0x100 + data[3];
			if (noise != 0)
				cnr = log10_easy(sig * 8 / noise);
			else
				cnr = 0;
			if (cnr < 0) cnr = 0;
			*cnr_i = (UINT32) cnr / 100;
			*cnr_d = (UINT32) cnr % 100;
			break;
		default:
			break;
	}
	return 0;
}

static INT32 nim_panic6158_set_reg(struct nim_device *dev, DMD_I2C_Register_t *reg)
{
	INT32 i;
	UINT8 mode;
	for (i = 0; i < DMD_REGISTER_MAX; i++)
	{
		if (DMD_E_END_OF_ARRAY == reg[i].flag)
			break;
		switch (reg[i].slvadr)
		{
			case 0x1c:
				mode = DEMO_BANK_T2;
				break;
			case 0x18:
				mode = DEMO_BANK_T;
				//continue;
				break;
			case 0x1a:
				mode = DEMO_BANK_C;
				//continue;
				break;
			default:
				return !SUCCESS;
				break;
		}
		if (SUCCESS != nim_reg_write(dev, mode, reg[i].adr, &reg[i].data, 1))
			return !SUCCESS;
	}
	return SUCCESS;
}

/* **************************************************** */
/*! Set Register setting for each broadcast system & bandwidth */
/* **************************************************** */
static INT32 nim_panic6158_set_system(struct nim_device *dev, UINT8 system, UINT8 bw, DMD_IF_FREQ_t if_freq)
{
	INT32 ret = SUCCESS;
	UINT8 nosupport = 0;
	switch (system)
	{
		case DEMO_BANK_T2:
			switch (bw)
			{
				case DMD_E_BW_8MHZ:
					ret = nim_panic6158_set_reg(dev, MN88472_REG_DVBT2_8MHZ);
					if (if_freq != DMD_E_IF_5000KHZ)
						nosupport = 1;
					break;
				case DMD_E_BW_6MHZ:
					ret = nim_panic6158_set_reg(dev, MN88472_REG_DVBT2_6MHZ);
					if (if_freq != DMD_E_IF_5000KHZ)
						nosupport = 1;
					break;
				case DMD_E_BW_5MHZ:
					ret = nim_panic6158_set_reg(dev, MN88472_REG_DVBT2_5MHZ);
					if (if_freq != DMD_E_IF_5000KHZ)
						nosupport = 1;
					break;
				case DMD_E_BW_1_7MHZ:
					ret = nim_panic6158_set_reg(dev, MN88472_REG_DVBT2_1_7MHZ);
					if (if_freq != DMD_E_IF_5000KHZ)
						nosupport = 1;
					break;
				case DMD_E_BW_7MHZ:
					ret = nim_panic6158_set_reg(dev, MN88472_REG_DVBT2_7MHZ);
					switch (if_freq)
					{
						case DMD_E_IF_5000KHZ:
							break;
						/* '11/08/12 : OKAMOTO Implement IF 4.5MHz for DVB-T/T2 7MHz. */
						case DMD_E_IF_4500KHZ:
							ret = nim_panic6158_set_reg(dev, MN88472_REG_DIFF_DVBT2_7MHZ_IF4500KHZ);
							break;
						default:
							nosupport = 1;
							break;
					}
					break;
				default:
					nosupport = 1;
					break;
			}
			break;
		case DEMO_BANK_T:
			switch (bw)
			{
				case DMD_E_BW_8MHZ:
					ret = nim_panic6158_set_reg(dev, MN88472_REG_DVBT_8MHZ);
					if (if_freq != DMD_E_IF_5000KHZ)
						nosupport = 1;
					break;
				case DMD_E_BW_7MHZ:
					ret = nim_panic6158_set_reg(dev, MN88472_REG_DVBT_7MHZ);
					switch (if_freq)
					{
						case DMD_E_IF_5000KHZ:
							break;
						/* '11/08/12 : OKAMOTO Implement IF 4.5MHz for DVB-T/T2 7MHz. */
						case DMD_E_IF_4500KHZ:
							ret = nim_panic6158_set_reg(dev, MN88472_REG_DIFF_DVBT_7MHZ_IF4500KHZ);
							break;
						default:
							nosupport = 1;
							break;
					}
					break;
				case DMD_E_BW_6MHZ:
					ret = nim_panic6158_set_reg(dev, MN88472_REG_DVBT_6MHZ);
					if (if_freq != DMD_E_IF_5000KHZ)
						nosupport = 1;
					break;
				default:
					nosupport = 1;
					break;
			}
			break;
		case DEMO_BANK_C:
			ret = nim_panic6158_set_reg(dev, MN88472_REG_DVBC);
			break;
		default:
			nosupport = 1;
			break;
	}
	if (1 == nosupport)
		NIM_PANIC6158_PRINTF("ERROR : Not Supported System");
	return ret;
}

static INT32 nim_panic6158_set_error_flag(struct nim_device *dev, UINT8 flag)
{
	UINT8 data;
	if (flag)
	{
		/* 1st,Adr:0x09(TSSET2) bit[2:0]=1 ("001") */
		if (SUCCESS != nim_reg_read(dev, DEMO_BANK_T2, DMD_TSSET2, &data, 1))
			return !SUCCESS;
		data &= 0xF8;
		data |= 0x1;
		if (SUCCESS != nim_reg_write(dev, DEMO_BANK_T2, DMD_TSSET2, &data, 1))
			return !SUCCESS;
		/* 1st,Adr:0xD9(FLGSET) bit[6:5]=0 ("00") */
		if (SUCCESS != nim_reg_read(dev, DEMO_BANK_T2, DMD_FLGSET, &data, 1))
			return !SUCCESS;
		data &= 0x9F;
		if (SUCCESS != nim_reg_write(dev, DEMO_BANK_T2, DMD_FLGSET, &data, 1))
			return !SUCCESS;
	}
	return SUCCESS;
}

/* '11/08/29 : OKAMOTO Select TS output. */
static INT32 nim_panic6158_set_ts_output(struct nim_device *dev, DMD_TSOUT ts_out)
{
	UINT8 data;
	switch (ts_out)
	{
		case DMD_E_TSOUT_PARALLEL_FIXED_CLOCK:
			//TS parallel (Fixed clock mode) SSET1:0x00 FIFOSET:0xE1
			data = 0x0;
			if (SUCCESS != nim_reg_write(dev, DEMO_BANK_T2, DMD_TSSET1, &data, 1))
				return !SUCCESS;
			data = 0xE1;
			if (SUCCESS != nim_reg_write(dev, DEMO_BANK_T, DMD_FIFOSET, &data, 1))
				return !SUCCESS;
			break;
		case DMD_E_TSOUT_PARALLEL_VARIABLE_CLOCK:
			//TS parallel (Variable clock mode) TSSET1:0x00 FIFOSET:0xE3
			data = 0x0;
			if (SUCCESS != nim_reg_write(dev, DEMO_BANK_T2, DMD_TSSET1, &data, 1))
				return !SUCCESS;
			data = 0xE3;
			if (SUCCESS != nim_reg_write(dev, DEMO_BANK_T, DMD_FIFOSET, &data, 1))
				return !SUCCESS;
			break;
		case DMD_E_TSOUT_SERIAL_VARIABLE_CLOCK:
			//TS serial(Variable clock mode) TSSET1:0x1D FIFOSET:0xE3
			data = 0x1D;
			if (SUCCESS != nim_reg_write(dev, DEMO_BANK_T2, DMD_TSSET1, &data, 1))
				return !SUCCESS;
			data = 0xE3;
			if (SUCCESS != nim_reg_write(dev, DEMO_BANK_T, DMD_FIFOSET, &data, 1))
				return !SUCCESS;
			break;
		default:
			break;
	}
	return SUCCESS;
}

INT32 nim_panic6158_set_info(struct nim_device *dev, UINT8 system, UINT32 id, UINT32 val)
{
	INT32 ret = !SUCCESS;
	UINT8 rd, data;
	switch (system)
	{
		case DEMO_BANK_T:
			switch (id)
			{
				case DMD_E_INFO_DVBT_HIERARCHY_SELECT:
					nim_reg_read(dev, system, DMD_RSDSET_T, &rd, 1);
					if (val == 1)
						rd |= 0x10;
					else
						rd &= 0xef;
					nim_reg_write(dev, system, DMD_RSDSET_T, &rd, 1);
					//param->info[DMD_E_INFO_DVBT_HIERARCHY_SELECT] = (rd >> 4) & 0x1;//dmq modify
					ret = SUCCESS;
					break;
				/* '11/11/14 : OKAMOTO Update to "MN88472_Device_Driver_111028". */
				case DMD_E_INFO_DVBT_MODE:
					ret = SUCCESS;
					if (val == DMD_E_DVBT_MODE_NOT_DEFINED)
					{
						data = 0xba;
						nim_reg_write(dev, system, DMD_MDSET_T, &data, 1);
						data = 0x13;
						nim_reg_write(dev, system, DMD_MDASET_T, &data, 1);
					}
					else
					{
						nim_reg_mask_write(dev, system, DMD_MDSET_T, 0xf0, 0xf0);
						data = 0;
						nim_reg_write(dev, system, DMD_MDASET_T, &data, 1);
						switch (val)
						{
							case DMD_E_DVBT_MODE_2K:
								nim_reg_mask_write(dev, system, DMD_MDSET_T, 0x0c, 0x00);
								break;
							case DMD_E_DVBT_MODE_8K:
								nim_reg_mask_write(dev, system, DMD_MDSET_T, 0x0c, 0x08);
								break;
							default:
								ret = !SUCCESS;
								break;
						}
					}
					break;
				/* '11/11/14 : OKAMOTO Update to "MN88472_Device_Driver_111028". */
				case DMD_E_INFO_DVBT_GI:
					ret = SUCCESS;
					if (val == DMD_E_DVBT_GI_NOT_DEFINED)
					{
						data = 0xba;
						nim_reg_write(dev, system, DMD_MDSET_T, &data, 1);
						data = 0x13;
						nim_reg_write(dev, system, DMD_MDASET_T, &data, 1);
					}
					else
					{
						nim_reg_mask_write(dev, system, DMD_MDSET_T, 0xf0, 0xf0);
						data = 0;
						nim_reg_write(dev, system, DMD_MDASET_T, &data, 1);
						switch (val)
						{
							case DMD_E_DVBT_GI_1_32:
								nim_reg_mask_write(dev, system, DMD_MDSET_T, 0x03, 0x00);
								break;
							case DMD_E_DVBT_GI_1_16:
								nim_reg_mask_write(dev, system, DMD_MDSET_T, 0x03, 0x01);
								break;
							case DMD_E_DVBT_GI_1_8:
								nim_reg_mask_write(dev, system, DMD_MDSET_T, 0x03, 0x02);
								break;
							case DMD_E_DVBT_GI_1_4:
								nim_reg_mask_write(dev, system, DMD_MDSET_T, 0x03, 0x03);
								break;
							default:
								ret = !SUCCESS;
								break;
						}
					}
					break;
			}
		case DEMO_BANK_T2:
			switch (id)
			{
				case DMD_E_INFO_DVBT2_SELECTED_PLP :
					rd = (UINT8) val;
					nim_reg_write(dev, system, DMD_PLPID, &rd, 1);
					ret = SUCCESS;
					break;
			}
		/* '11/10/19 : OKAMOTO Correct warning: enumeration value 乪DMD_E_ISDBT乫 not handled in switch */
		default:
			ret = !SUCCESS;
			break;
	}
	return ret;
}

INT32 nim_panic6158_get_lock(struct nim_device *dev, UINT8 *lock)
{
	UINT8 data = 0;
	struct nim_panic6158_private *priv = (struct nim_panic6158_private *)dev->priv;
	*lock = 0;
	if (DEMO_BANK_T2 == priv->system)
	{
		nim_reg_read(dev, priv->system, DMD_SSEQFLG, &data, 1);
		//printk("T2[%s]%d,data =0x%x\n",__FUNCTION__,__LINE__,data);
		if (13 <= (0x0F & data))
			*lock = 1;
	}
	else if (DEMO_BANK_T == priv->system)
	{
		nim_reg_read(dev, priv->system, DMD_SSEQRD_T, &data, 1);
		//printk("T[%s]%d,data =0x%x\n",__FUNCTION__,__LINE__,data);
		if (9 <= (0x0F & data))
			*lock = 1;
	}
	else if (DEMO_BANK_C == priv->system)
	{
		//printk("[%s]%d,DEMO_BANK_C\n",__FUNCTION__,__LINE__);
		nim_reg_read(dev, priv->system, DMD_SSEQMON2_C, &data, 1);
		//printk("[%s]%d,data =0x%x\n",__FUNCTION__,__LINE__,data);
		if (8 <= (0x0F & data))
			*lock = 1;
	}
	NIM_PANIC6158_PRINTF("data = %x\n", data & 0xf);
	return SUCCESS;
}

INT32 nim_panic6158_get_AGC_301(struct nim_device *dev, UINT8 *agc)
{
	//INT32 result;
	//UINT32 flgptn;
	U32 m_agc = 0;
	struct nim_panic6158_private *priv;
	priv = (struct nim_panic6158_private *)dev->priv;
	MxL_Check_RF_Input_Power(priv->tuner_id, &m_agc);
	*agc = (UINT8)m_agc;
	//libc_printf_direct("agc= %f %d\n", m_agc, *agc);
	NIM_PANIC6158_PRINTF("agc = %d\n", *agc);
//UINT32 ber, per;
//nim_panic6158_get_PER(dev, &per);
//nim_panic6158_get_BER(dev, &ber);
	return SUCCESS;
}

UINT32 DMD_AGC(struct nim_device *dev)
{
	UINT8 rd;
	UINT32 ret;
	nim_reg_read(dev, DEMO_BANK_T2, DMD_AGCRDU, &rd, 1);
	ret = rd * 4;
	nim_reg_read(dev, DEMO_BANK_T2, DMD_AGCRDL, &rd, 1);
	ret += rd >> 6;
	return ret;
}

INT32 nim_panic6158_get_AGC_603(struct nim_device *dev, UINT8 *agc)
{
	UINT32 agc1, max_agc;
	//result = osal_flag_wait(&flgptn, priv->flag_id, NIM_SCAN_END, OSAL_TWF_ANDW, 0);
	agc1 = DMD_AGC(dev);
	max_agc = 0xff * 4 + 3; //10 bit.
	*agc = 100 - (agc1 * 100 / max_agc);
	//NIM_PANIC6158_PRINTF("agc = %d\n", *agc);
	return SUCCESS;
}

INT32 nim_panic6158_get_AGC(struct nim_device *dev, UINT8 *agc)
{
	if (dev->get_AGC)
	{
		dev->get_AGC(dev, agc);
	}
	NIM_PANIC6158_PRINTF("agc = %d\n", *agc);
	return SUCCESS;
}

INT32 nim_panic6158_get_SNR(struct nim_device *dev, UINT8 *snr)
{
	//INT32 result;
	UINT32 cnr;
	INT32 cnr_rel;
	UINT8 data[3];
	UINT32 beri;
	UINT32 mod, cr;
	UINT32 sqi, ber_sqi;
	UINT32 ber_err, ber_sum, ber_len;
	UINT32 cnr_i = 0, cnr_d = 0;
	struct nim_panic6158_private *priv;
	//UINT32 tick;
	//tick = osal_get_tick();
	priv = (struct nim_panic6158_private *)dev->priv;
	nim_calculate_ber(dev, &ber_err, &ber_len, &ber_sum);
	nim_calculate_cnr(dev, &cnr_i, &cnr_d);
	cnr = cnr_i * 100 + cnr_d;
	if (0 != ber_err)
		beri = ber_sum / ber_err;
	else
		beri = 100000000;
	//printf("[%s]cnr=%d,beri=%d\n",__FUNCTION__,cnr,beri);
	if (DEMO_BANK_T == priv->system)
	{
		nim_reg_read(dev, DEMO_BANK_T, DMD_TMCCD2_T, data, 2);
		mod = (data[0] >> 3) & 0x3;
		cr = data[1] & 0x07;
		if (mod >= 3 || cr >= 5)
			return 0;
		cnr_rel = cnr - DMD_DVBT_CNR_P1[mod][cr];
	}
	else if (DEMO_BANK_T2 == priv->system)
	{
		data[0] = 0;
		data[1] = 0x3;
		nim_reg_write(dev, DEMO_BANK_T2, DMD_TPDSET1, data, 2);
		nim_reg_read(dev, DEMO_BANK_T2, DMD_TPD2, data, 1);
		mod = data[0] >> 5;
		cr = ((data[0] >> 2) & 0x7);
		if (mod >= 4 || cr >= 6)
			return 0;
		cnr_rel = cnr - DMD_DVBT2_CNR_P1[mod][cr];
	}
	else
	{
		//jhy add start
		if (beri)
			*snr = 100;
		//jhy add end
		return 0;
	}
	if (cnr_rel < -700)
	{
		sqi = 0;
	}
	else
	{
		if (beri < 1000) //BER>10-3
		{
			ber_sqi = 0;
		}
		else if (beri < 10000000) //BER>10-7
		{
			ber_sqi = 20 * log10_easy(beri) - 40000;
			ber_sqi /= 1000;
		}
		else
		{
			ber_sqi = 100;
		}
		if (cnr_rel <= 300)
			sqi = (((cnr_rel - 300) / 10) + 100) * ber_sqi;
		else
			sqi = ber_sqi * 100;
	}
	*snr = (UINT8)(sqi / 100);
	NIM_PANIC6158_PRINTF("snr =%d\n", *snr);
	//soc_printf("%d\n", osal_get_tick() - tick);
	return SUCCESS;
}

static INT32 nim_panic6158_get_PER(struct nim_device *dev, UINT32 *per)
{
	UINT8 data[4];
	UINT32 err, sum;
	struct nim_panic6158_private *priv;
	priv = (struct nim_panic6158_private *)dev->priv;
	nim_reg_read(dev, DEMO_BANK_T, DMD_PERRDU, data, 2);
	err = data[0] * 0x100 + data[1];
	nim_reg_read(dev, DEMO_BANK_T, DMD_PERLENRDU, data, 2);
	sum = data[0] * 0x100 + data[1];
	if (0 != sum)
		*per = err * 100 / sum;
	else
		*per = 0;
	NIM_PANIC6158_PRINTF("per = %d\n", *per);
	return SUCCESS;
}

INT32 nim_panic6158_get_BER(struct nim_device *dev, UINT32 *ber)
{
	//INT32 result;
	UINT32 error, len, sum;
	nim_calculate_ber(dev, &error, &len, &sum);
	if (0 != sum)
	{
		*ber = (error * 100) / len;
	}
	else
	{
		*ber = 0;
	}
	//NIM_PANIC6158_PRINTF("ber = %d\n", *ber);
	return SUCCESS;
}

static INT32 nim_panic6158_get_freq(struct nim_device *dev, UINT32 *freq)
{
	struct nim_panic6158_private *priv;
	priv = (struct nim_panic6158_private *)dev->priv;
	*freq = priv->frq;
	NIM_PANIC6158_PRINTF("freq = %d KHz \n", *freq);
	return SUCCESS;
}

static INT32 nim_panic6158_get_PLP_num(struct nim_device *dev, UINT8 *plp_num)
{
	struct nim_panic6158_private *priv;
	priv = (struct nim_panic6158_private *)dev->priv;
	*plp_num = priv->PLP_num;
	NIM_PANIC6158_PRINTF("number= %d \n", *plp_num);
	return SUCCESS;
}

static INT32 nim_panic6158_get_PLP_id(struct nim_device *dev, UINT8 *id)
{
	struct nim_panic6158_private *priv;
	priv = (struct nim_panic6158_private *)dev->priv;
	*id = priv->PLP_id;
	NIM_PANIC6158_PRINTF("PLP_id = %d \n", *id);
	return SUCCESS;
}

static INT32 nim_panic6158_get_system(struct nim_device *dev, UINT8 *system)
{
	struct nim_panic6158_private *priv;
	priv = (struct nim_panic6158_private *)dev->priv;
	if (DEMO_BANK_T == priv->system)
		*system = DEMO_DVBT;
	else if (DEMO_BANK_T2 == priv->system)
		*system = DEMO_DVBT2;
	else if (DEMO_BANK_C == priv->system)
		*system = DEMO_DVBC;
	else
		*system = 0;
	NIM_PANIC6158_PRINTF("Frontend type %d\n", *system);
	return SUCCESS;
}

static INT32 nim_panic6158_get_modulation(struct nim_device *dev, UINT8 *modulation)
{
	UINT8 data[2];
	UINT8 mode = 0;
	struct nim_panic6158_private *priv;
	priv = (struct nim_panic6158_private *)dev->priv;
	if (DEMO_BANK_T == priv->system)
	{
		nim_reg_read(dev, DEMO_BANK_T, DMD_TMCCD2_T, data, 1);
		mode = (data[0] >> 3) & 0x3;
		if (mode >= 3)
			return 0;
	}
	else if (DEMO_BANK_T2 == priv->system)
	{
		data[0] = 0;
		data[1] = 0x3;
		nim_reg_write(dev, DEMO_BANK_T2, DMD_TPDSET1, data, 2);
		nim_reg_read(dev, DEMO_BANK_T2, DMD_TPD2, data, 1);
		mode = data[0] >> 5;
	}
	*modulation = mode;
	NIM_PANIC6158_PRINTF("modulation = %d KHz \n", *modulation);
	return SUCCESS;
}

static INT32 nim_panic6158_get_code_rate(struct nim_device *dev, UINT8 *fec)
{
	UINT8 data[2];
	UINT8 code_rate = 0;
	struct nim_panic6158_private *priv;
	priv = (struct nim_panic6158_private *)dev->priv;
	if (DEMO_BANK_T == priv->system)
	{
		nim_reg_read(dev, DEMO_BANK_T, DMD_TMCCD3_T, data, 1);
		code_rate = data[0] & 0x07;
		if (code_rate >= 5)
			return 0;
	}
	else if (DEMO_BANK_T2 == priv->system)
	{
		data[0] = 0;
		data[1] = 0x3;
		nim_reg_write(dev, DEMO_BANK_T2, DMD_TPDSET1, data, 2);
		nim_reg_read(dev, DEMO_BANK_T2, DMD_TPD2, data, 1);
		code_rate = ((data[0] >> 2) & 0x7);
	}
	*fec = code_rate;
	return SUCCESS;
}

static INT32 nim_panic6158_get_sym(struct nim_device *dev, UINT32 *sym)
{
	struct nim_panic6158_private *priv;
	priv = (struct nim_panic6158_private *)dev->priv;
	if (DEMO_BANK_C == priv->system)
	{
		*sym = priv->sym;
	}
	return SUCCESS;
}

static INT32 nim_panic6158_get_bandwidth(struct nim_device *dev, INT32 *bandwidth)
{
	struct nim_panic6158_private *priv;
	priv = (struct nim_panic6158_private *)dev->priv;
	*bandwidth = priv->bw;
	NIM_PANIC6158_PRINTF("bandwidth=%d KHz \n", *bandwidth);
	return SUCCESS;
}

static INT32 nim_panic6158_tune_action(struct nim_device *dev, UINT8 system, UINT32 frq, UINT8 bw, UINT8 qam, UINT32 wait_time, UINT8 PLP_id)
{
	INT32 ret = !SUCCESS;
	INT32 retTuner = !SUCCESS;
	UINT16 i, j;
	UINT8 data, lock = 0;
	struct nim_panic6158_private *priv;
	UINT8 qam_array[] = {QAM64, QAM256, QAM128, QAM32, QAM16};
	priv = (struct nim_panic6158_private *) dev->priv;
	if (DEMO_BANK_T == system || DEMO_BANK_C == system)
	{
		NIM_PANIC6158_PRINTF("tune T or C\n");
		NIM_PANIC6158_PRINTF("[%s]%d,T frq =%d, bw= %d,system =%d\n", __FUNCTION__, __LINE__, frq, bw, system);
		if (DEMO_BANK_C == system)
		{
			for (i = 0; i < ARRAY_SIZE(qam_array); i++)
			{
				if (qam_array[i] == qam)
					break;
			}
			if (i >= ARRAY_SIZE(qam_array))
				i = 0;
			nim_reg_mask_write(dev, system, DMD_CHSRCHSET_C, 0x0F, 0x08 | i);
		}
		else if (DEMO_BANK_T == system)
		{
			NIM_PANIC6158_PRINTF("tune T\n");
			nim_panic6158_set_info(dev, system, DMD_E_INFO_DVBT_MODE, DMD_E_DVBT_MODE_NOT_DEFINED);
			nim_panic6158_set_info(dev, system, DMD_E_INFO_DVBT_GI, DMD_E_DVBT_GI_NOT_DEFINED);
		}
		if (1 == priv->scan_stop_flag)
			return SUCCESS;
		//tune tuner
		tun_mxl301_set_addr(priv->tuner_id, priv->i2c_addr[DEMO_BANK_T2], priv->i2c_mutex_id);
		if (NULL != priv->tc.nim_Tuner_Control)
			retTuner = priv->tc.nim_Tuner_Control(priv->tuner_id, frq, bw, 0, NULL, 0);
		osal_task_sleep(10);
		if (NULL != priv->tc.nim_Tuner_Status)
			priv->tc.nim_Tuner_Status(priv->tuner_id, &lock);
		if (1 == priv->scan_stop_flag)
			return SUCCESS;
		NIM_PANIC6158_PRINTF("[%s]%d,T/C tuner lock =%d\n", __FUNCTION__, __LINE__, lock);
		//get tuner lock state
		if (1 != lock)
		{
			NIM_PANIC6158_PRINTF("tuner lock failed \n");
			return !SUCCESS;
		}
		osal_task_sleep(100);
		//reset demo
		data = 0x9F;
		nim_reg_write(dev, DEMO_BANK_T2, DMD_RSTSET1, &data, 1);
		NIM_PANIC6158_PRINTF("dev->get_lock = 0x%x\n", (int)dev->get_lock);
		for (i = 0; i < (wait_time / 50); i++)
		{
			if (NULL != dev->get_lock)
			{
				if (1 == priv->scan_stop_flag)
					break;
				osal_task_sleep(50);
				dev->get_lock(dev, &lock);
				NIM_PANIC6158_PRINTF("[%s]%d,T/C demod lock =%d\n", __FUNCTION__, __LINE__, lock);
				if (1 == lock)
				{
					ret = SUCCESS;
					break;
				}
			}
		}
	}
	else if (DEMO_BANK_T2 == system)
	{
		data = 0x43;
		nim_reg_write(dev, DEMO_BANK_T2, DMD_SSEQSET, &data, 1);
		NIM_PANIC6158_PRINTF("tune T2\n");
		NIM_PANIC6158_PRINTF("[%s]%d,T2 frq =%d, bw= %d,system =%d\n", __FUNCTION__, __LINE__, frq, bw, system);
		for (i = 0; i < PANIC6158_PLP_TUNE_NUM; i++)
		{
			data = 0x80;
			nim_reg_write(dev, DEMO_BANK_T2, DMD_FECSET1, &data, 1);
			data = PLP_id;
			nim_reg_write(dev, DEMO_BANK_T2, DMD_PLPID, &data, 1);
			if (1 == priv->scan_stop_flag)
				break;
			//tune tuner
			tun_mxl301_set_addr(priv->tuner_id, priv->i2c_addr[DEMO_BANK_T2], priv->i2c_mutex_id);
			if (NULL != priv->tc.nim_Tuner_Control)
				priv->tc.nim_Tuner_Control(priv->tuner_id, frq, bw, 0, NULL, 0);
			osal_task_sleep(10);
			if (NULL != priv->tc.nim_Tuner_Status)
				priv->tc.nim_Tuner_Status(priv->tuner_id, &lock);
			//printk("[%s]%d,T/C tuner lock =%d\n",__FUNCTION__,__LINE__,lock);
			//get tuner lock state
			if (1 != lock)
			{
				NIM_PANIC6158_PRINTF("tuner lock failed \n");
				return !SUCCESS;
			}
			if (1 == priv->scan_stop_flag)
				break;
			osal_task_sleep(100);
			//reset demo
			data = 0x9F;
			nim_reg_write(dev, DEMO_BANK_T2, DMD_RSTSET1, &data, 1);
			//receive judgement
			for (j = 0; j < (PANIC6158_T2_TUNE_TIMEOUT / 50); j++)
			{
				if (1 == priv->scan_stop_flag)
					break;
				osal_task_sleep(50);
				nim_reg_read(dev, priv->system, DMD_SSEQFLG, &data, 1);
				//if bit6 or bit5 is 1, then not need to wait
				if ((0x60 & data) || (1 == priv->scan_stop_flag))
				{
					break;
				}
				if (13 <= data)
				{
					data = 0x0;
					nim_reg_write(dev, DEMO_BANK_T2, DMD_TPDSET1, &data, 1);
					data = 0x7;
					nim_reg_write(dev, DEMO_BANK_T2, DMD_TPDSET2, &data, 1);
					nim_reg_read(dev, DEMO_BANK_T2, 0xCA, &data, 1);
					NIM_PANIC6158_PRINTF("%s T2 lock, PLP: %d! \n", __FUNCTION__, data);
					return SUCCESS;
				}
			}
			if (1 == priv->scan_stop_flag)
				break;
		}
	}
	NIM_PANIC6158_PRINTF("ret = %d\n", ret);
	return ret;
}

INT32 nim_panic6158_channel_change(struct nim_device *dev, struct NIM_Channel_Change *param)
{
	INT32 i;//, j;
	INT32 tune_num = 1;
	UINT32 wait_time;
	UINT8 mode[2];
	struct nim_panic6158_private *priv;
	printk(" nim_panic6158_channel_change\n");
	priv = (struct nim_panic6158_private *) dev->priv;
	priv->frq = param->freq;
	priv->bw = param->bandwidth;
	priv->qam = param->modulation;
	priv->PLP_id = param->PLP_id;
	priv->PLP_num = 0;//Init PLP num

	switch (param->priv_param)	  
	{
	case DEMO_DVBC:
		mode[0] = DEMO_BANK_C;
		break;
	case DEMO_DVBT:
		mode[0] = DEMO_BANK_T;
		break;
	case DEMO_DVBT2:
		mode[0] = DEMO_BANK_T2;
		break;
	default:
		tune_num = PANIC6158_TUNE_MAX_NUM;
		mode[0] = (1 == priv->first_tune_t2) ? DEMO_BANK_T2 : DEMO_BANK_T;
		mode[1] = (DEMO_BANK_T2 == mode[0]) ? DEMO_BANK_T : DEMO_BANK_T2;
		break;
	}

	NIM_PANIC6158_PRINTF("frq: %dKHz, bw: %dMHz, plp_id: %d system mode: %d, qam: %d\n", priv->frq/1000, priv->bw, priv->PLP_id, mode[0], priv->qam);
	NIM_PANIC6158_PRINTF("tune_num:%d\n", tune_num);

	priv->scan_stop_flag = 0;
	for (i = 0; i < tune_num; i++)
	{
		priv->system = mode[i];
		//set IF freq
		if (DMD_E_BW_7MHZ == priv->bw && (DEMO_BANK_T == mode[i] || DEMO_BANK_T2 == mode[i]))
			priv->if_freq = DMD_E_IF_4500KHZ;
		else
			priv->if_freq = DMD_E_IF_5000KHZ;
		printk("[%s]%d\n", __FUNCTION__, __LINE__);
		//nim_panic6158_open(dev);
		nim_panic6158_set_reg(dev, MN88472_REG_COMMON);
		//config demo
		nim_panic6158_set_system(dev, priv->system, priv->bw, priv->if_freq);
		//nim_panic6158_set_error_flag(dev, 1);
		//nim_panic6158_set_ts_output(dev, DMD_E_TSOUT_SERIAL_VARIABLE_CLOCK);
		if (1 == priv->scan_stop_flag)
			break;
		//tune demo
		wait_time = (PANIC6158_TUNE_MAX_NUM == tune_num && 0 == i) ? 1000 : 300;//ms
		if (SUCCESS == nim_panic6158_tune_action(dev, mode[i], priv->frq, priv->bw, priv->qam, wait_time, priv->PLP_id))
			break;
	}
	return SUCCESS;
}

static INT32 nim_panic6158_ioctl(struct nim_device *dev, INT32 cmd, UINT32 param)
{
	INT32 ret = SUCCESS;
	struct nim_panic6158_private *priv;
	(void)param;
	priv = (struct nim_panic6158_private *)dev->priv;
	switch (cmd)
	{
		case NIM_DRIVER_STOP_CHANSCAN:
		case NIM_DRIVER_STOP_ATUOSCAN:
			priv->scan_stop_flag = 1;
			break;
		default:
			break;
	}
	return ret;
}

static INT32 nim_panic6158_ioctl_ext(struct nim_device *dev, INT32 cmd, void *param_list)
{
	INT32 ret = SUCCESS;
	struct nim_t10023_private *priv;
	(void)param_list;
	priv = (struct nim_t10023_private *)dev->priv;
	switch (cmd)
	{
		case NIM_DRIVER_AUTO_SCAN:
			ret = SUCCESS;
			break;
		case NIM_DRIVER_CHANNEL_CHANGE:
			//ret = nim_panic6158_channel_change(dev, (struct NIM_Channel_Change *) (param_list));
			break;
		case NIM_DRIVER_CHANNEL_SEARCH:
			ret = SUCCESS;
			break;
		case NIM_DRIVER_GET_RF_LEVEL:
			break;
		case NIM_DRIVER_GET_CN_VALUE:
			break;
		case NIM_DRIVER_GET_BER_VALUE:
			break;
		default:
			ret = SUCCESS;
			break;
	}
	return ret;
}

INT32 nim_panic6158_open(struct nim_device *dev)
{
	INT32 i;
	INT32 ret;
	UINT8 data;
	/* Demodulator LSI Initialize */
	ret = nim_panic6158_set_reg(dev, MN88472_REG_COMMON);
	if (SUCCESS != ret)
	{
		printk("nim_panic6158_open Error\n");
		NIM_PANIC6158_PRINTF("ERROR: Demodulator LSI Initialize");
		return ret;
	}
	/* Auto Control Sequence Transfer */
	data = 0x20;
	ret = nim_reg_write(dev, DEMO_BANK_T, DMD_PSEQCTRL, &data, 1);
	data = 0x03;
#if 1
	ret |= nim_reg_write(dev, DEMO_BANK_T, DMD_PSEQSET, &data, 1);
	for (i = 0; i < MN88472_REG_AUTOCTRL_SIZE; i++)
		ret |= nim_reg_write(dev, DEMO_BANK_T, DMD_PSEQPRG, &MN88472_REG_AUTOCTRL[i], 1);
	data = 0x0;
	ret |= nim_reg_write(dev, DEMO_BANK_T, DMD_PSEQSET, &data, 1);
	/* Check Parity bit */
	ret |= nim_reg_read(dev, DEMO_BANK_T, DMD_PSEQFLG, &data, 1);
	if (data & 0x10)
	{
		NIM_PANIC6158_PRINTF("ERROR: PSEQ Parity");
		return !SUCCESS;
	}
	ret = nim_panic6158_set_error_flag(dev, 0);
	//printf("[%s]ret=0x%x................\n",__FUNCTION__,ret);
	nim_panic6158_set_ts_output(dev, DMD_E_TSOUT_PARALLEL_VARIABLE_CLOCK /**//*DMD_E_TSOUT_PARALLEL_FIXED_CLOCK*//*DMD_E_TSOUT_SERIAL_VARIABLE_CLOCK*/);
#endif
	return ret;
}

INT32 nim_panic6158_close(struct nim_device *dev)
{
	//UINT8 i;
	INT32 ret = SUCCESS;
	/*if (OSAL_INVALID_ID != dev->mutex_id)
	{
	 if (RET_SUCCESS != osal_mutex_delete(dev->mutex_id))
	 {
	 NIM_PANIC6158_PRINTF("nim_panic6158_close: Delete mutex failed!\n");
	 return !SUCCESS;
	 }
	 dev->mutex_id = OSAL_INVALID_ID;
	}

	if (OSAL_INVALID_ID != dev->flags)
	{
	 if (RET_SUCCESS != osal_flag_delete(dev->flags))
	 {
	 NIM_PANIC6158_PRINTF("nim_panic6158_close: Delete flag failed!\n");
	 return !SUCCESS;
	 }
	 dev->flags = OSAL_INVALID_ID;
	}*/
	//FREE(dev->priv);
	//dev_free(dev);
	YWOS_Free(dev->priv);
	YWOS_Free(dev);
	dev = NULL;
	return ret;
}

INT32 nim_panic6158_attach(UINT8 Handle, PCOFDM_TUNER_CONFIG_API pConfig, TUNER_OpenParams_T *OpenParams)
{
	INT32 ret = 0;
	UINT8 i = 0, data = 0;
	struct nim_device *dev;
	struct nim_panic6158_private *priv;
	TUNER_ScanTaskParam_T *Inst = NULL;
	TUNER_IOARCH_OpenParams_t Tuner_IOParams;
	if (NULL == pConfig || 2 < nim_panic6158_cnt)
		return ERR_NO_DEV;
	Inst = TUNER_GetScanInfo(Handle);
	//dev = (struct nim_device *)dev_alloc(nim_panic6158_name[nim_panic6158_cnt], HLD_DEV_TYPE_NIM, sizeof(struct nim_device));
	dev = (struct nim_device *)YWOS_Malloc(sizeof(struct nim_device));
	if (NULL == dev)
		return ERR_NO_MEM;
	priv = (PNIM_PANIC6158_PRIVATE)YWOS_Malloc(sizeof(struct nim_panic6158_private));
	if (NULL == priv)
	{
		//dev_free(dev);
		YWOS_Free(dev);
		//NIM_PANIC6158_PRINTF("Alloc nim device prive memory error!/n");
		return ERR_NO_MEM;
	}
	YWLIB_Memset(dev, 0, sizeof(struct nim_device));
	YWLIB_Memset(priv, 0, sizeof(struct nim_panic6158_private));
	YWLIB_Memcpy((void *) & (priv->tc), (void *)pConfig, sizeof(struct COFDM_TUNER_CONFIG_API));
	//priv->tuner_if_freq = pConfig->tuner_config.wTuner_IF_Freq;
	dev->priv = (void *)priv;
	//jhy add start
	if (YWTUNER_DELIVER_TER == Inst->Device)
	{
		dev->DemodIOHandle[0] = Inst->DriverParam.Ter.DemodIOHandle;
		dev->DemodIOHandle[1] = Inst->DriverParam.Ter.TunerIOHandle;
	}
	else if (YWTUNER_DELIVER_CAB == Inst->Device)
	{
		dev->DemodIOHandle[0] = Inst->DriverParam.Cab.DemodIOHandle;
		dev->DemodIOHandle[1] = Inst->DriverParam.Cab.TunerIOHandle;
	}
	//printf("dev->DemodIOHandle[0] = %d\n", dev->DemodIOHandle[0]);
	//printf("dev->DemodIOHandle[1] = %d\n", dev->DemodIOHandle[1]);
	Tuner_IOParams.IORoute = TUNER_IO_DIRECT;
	Tuner_IOParams.IODriver = OpenParams->TunerIO.Driver;
	YWLIB_Strcpy((S8 *)Tuner_IOParams.DriverName, (S8 *)OpenParams->TunerIO.DriverName);
	Tuner_IOParams.Address = PANIC6158_C_ADDR;//OpenParams->TunerIO.Address;
	Tuner_IOParams.hInstance = NULL;//&Inst->DriverParam.Ter.DemodIOHandle;
	Tuner_IOParams.TargetFunction = NULL;//Inst->DriverParam.Ter.DemodDriver.Demod_repeat;
	Tuner_IOParams.I2CIndex = OpenParams->I2CBusNo;
	TUNER_IOARCH_Open(&dev->DemodIOHandle[2], &Tuner_IOParams);
	//printf("dev->DemodIOHandle[2] = %d\n", dev->DemodIOHandle[2]);
	//printf("[%s]%d,DemodIOHandle[1]=0x%x,DemodIOHandle[2]=0x%x\n",__FUNCTION__,__LINE__,dev->DemodIOHandle[1],dev->DemodIOHandle[2]);
	//jhy add end
	dev->i2c_type_id = pConfig->ext_dm_config.i2c_type_id;
	dev->nim_idx = pConfig->ext_dm_config.nim_idx;//distinguish left or right
	dev->init = NULL;
	dev->open = nim_panic6158_open;
	dev->stop = nim_panic6158_close;
	dev->disable = NULL; //nim_m3101_disable;
	dev->do_ioctl = nim_panic6158_ioctl;//nim_m3101_ioctl;
	//dev->channel_change = nim_panic6158_channel_change;
	dev->do_ioctl_ext = nim_panic6158_ioctl_ext;//nim_m3101_ioctl;
	dev->channel_search = NULL;//nim_m3101_channel_search;
	dev->get_lock = nim_panic6158_get_lock;
	dev->get_freq = nim_panic6158_get_freq;//nim_m3101_get_freq;
	dev->get_sym = nim_panic6158_get_sym;
	dev->get_FEC = nim_panic6158_get_code_rate; ////nim_m3101_get_code_rate;
	dev->get_AGC = nim_panic6158_get_AGC_301;
	dev->get_SNR = nim_panic6158_get_SNR;
	dev->get_BER = nim_panic6158_get_BER;
	dev->get_PER = nim_panic6158_get_PER;
	dev->get_bandwidth = nim_panic6158_get_bandwidth;//nim_m3101_get_bandwith;
	dev->get_frontend_type = nim_panic6158_get_system;;
	//added for DVB-T additional elements
	dev->get_guard_interval = NULL;//nim_m3101_get_GI;
	dev->get_fftmode = NULL;//nim_m3101_get_fftmode;
	dev->get_modulation = nim_panic6158_get_modulation;//nim_m3101_get_modulation;
	dev->get_spectrum_inv = NULL;//nim_m3101_get_specinv;
	dev->get_freq_offset = NULL;//nim_m3101_get_freq_offset;
	priv->i2c_addr[0] = PANIC6158_T_ADDR;
	priv->i2c_addr[1] = PANIC6158_T2_ADDR;
	priv->i2c_addr[2] = PANIC6158_C_ADDR;
	priv->if_freq = DMD_E_IF_5000KHZ;
	priv->flag_id = OSAL_INVALID_ID;
	priv->i2c_mutex_id = OSAL_INVALID_ID;
	priv->first_tune_t2 = pConfig->tune_t2_first;
	priv->tuner_id = Handle; //jhy added
	//priv->system = DEMO_BANK_T2;//jhy added ,DEMO_BANK_T or DEMO_BANK_T2
	ret = SUCCESS;
	for (i = 0; i < 3; i++)
	{
		ret = nim_reg_read(dev, DEMO_BANK_T, DMD_CHIPRD, &data, 1);
		if (2 == data)
			break;
	}
	//printf("[%s]%d,i=%d,ret=%d \n",__FUNCTION__,__LINE__,i,ret);
	if (3 <= i || 0 < ret)
	{
		TUNER_IOARCH_CloseParams_t CloseParams;
		TUNER_IOARCH_Close(dev->DemodIOHandle[2], &CloseParams);
		YWOS_Free(priv);
		YWOS_Free(dev);
		return !SUCCESS;
	}
	if (NULL != pConfig->nim_Tuner_Init)
	{
		if (SUCCESS == pConfig->nim_Tuner_Init(&priv->tuner_id, &pConfig->tuner_config))
		{
			//priv_mem->tuner_type = tuner_type;
			priv->tc.nim_Tuner_Init = pConfig->nim_Tuner_Init;
			priv->tc.nim_Tuner_Control = pConfig->nim_Tuner_Control;
			priv->tc.nim_Tuner_Status = pConfig->nim_Tuner_Status;
			YWLIB_Memcpy(&priv->tc.tuner_config, &pConfig->tuner_config, sizeof(struct COFDM_TUNER_CONFIG_EXT));
		}
		else
		{
			TUNER_IOARCH_CloseParams_t CloseParams;
			NIM_PANIC6158_PRINTF("Error: Init Tuner Failure!\n");
			TUNER_IOARCH_Close(dev->DemodIOHandle[2], &CloseParams);
			//FREE(priv);
			//dev_free(dev);
			YWOS_Free(priv);
			YWOS_Free(dev);
			return ERR_NO_DEV;
		}
	}
	Inst->userdata = (void *)dev;
	nim_panic6158_cnt++;
	/* Open this device */
	ret = nim_panic6158_open(dev);
	//printf("[%s]%d,open result=%d \n",__FUNCTION__,__LINE__,ret);
	/* Setup init work mode */
	if (ret != SUCCESS)
	{
		TUNER_IOARCH_CloseParams_t CloseParams;
		TUNER_IOARCH_Close(dev->DemodIOHandle[2], &CloseParams);
		YWOS_Free(priv);
		YWOS_Free(dev);
		return ERR_NO_DEV;
	}
	return SUCCESS;
}

static INT32 nim_panic6158_set_system_earda(struct nim_device *dev, UINT8 system, UINT8 bw, DMD_IF_FREQ_t if_freq)
{
	INT32 ret = SUCCESS;
	UINT8 nosupport = 0;
	switch (system)
	{
		case DEMO_BANK_T2:
			switch (bw)
			{
				case DMD_E_BW_8MHZ:
					ret = nim_panic6158_set_reg(dev, MN88472_REG_24MHZ_DVBT2_8MHZ);
					if (if_freq != DMD_E_IF_5000KHZ)
						nosupport = 1;
					break;
				case DMD_E_BW_6MHZ:
					ret = nim_panic6158_set_reg(dev, MN88472_REG_24MHZ_DVBT2_6MHZ);
					if (if_freq != DMD_E_IF_5000KHZ)
						nosupport = 1;
					break;
				case DMD_E_BW_5MHZ:
					ret = nim_panic6158_set_reg(dev, MN88472_REG_24MHZ_DVBT2_5MHZ);
					if (if_freq != DMD_E_IF_5000KHZ)
						nosupport = 1;
					break;
				case DMD_E_BW_1_7MHZ:
					ret = nim_panic6158_set_reg(dev, MN88472_REG_24MHZ_DVBT2_1_7MHZ);
					if (if_freq != DMD_E_IF_5000KHZ)
						nosupport = 1;
					break;
				case DMD_E_BW_7MHZ:
					ret = nim_panic6158_set_reg(dev, MN88472_REG_24MHZ_DVBT2_7MHZ);
#if 0
					switch (if_freq)
					{
						case DMD_E_IF_5000KHZ:
							break;
						/* '11/08/12 : OKAMOTO Implement IF 4.5MHz for DVB-T/T2 7MHz. */
						case DMD_E_IF_4500KHZ:
							ret = nim_panic6158_set_reg(dev, MN88472_REG_DIFF_DVBT2_7MHZ_IF4500KHZ);
							break;
						default:
							nosupport = 1;
							break;
					}
#endif
					break;
				default:
					nosupport = 1;
					break;
			}
			break;
		case DEMO_BANK_T:
			switch (bw)
			{
				case DMD_E_BW_8MHZ:
					ret = nim_panic6158_set_reg(dev, MN88472_REG_24MHZ_DVBT_8MHZ);
					if (if_freq != DMD_E_IF_5000KHZ)
						nosupport = 1;
					break;
				case DMD_E_BW_7MHZ:
					ret = nim_panic6158_set_reg(dev, MN88472_REG_24MHZ_DVBT_7MHZ);
#if 0
					switch (if_freq)
					{
						case DMD_E_IF_5000KHZ:
							break;
						/* '11/08/12 : OKAMOTO Implement IF 4.5MHz for DVB-T/T2 7MHz. */
						case DMD_E_IF_4500KHZ:
							ret = nim_panic6158_set_reg(dev, MN88472_REG_DIFF_DVBT_7MHZ_IF4500KHZ);
							break;
						default:
							nosupport = 1;
							break;
					}
#endif
					break;
				case DMD_E_BW_6MHZ:
					ret = nim_panic6158_set_reg(dev, MN88472_REG_24MHZ_DVBT_6MHZ);
					if (if_freq != DMD_E_IF_5000KHZ)
						nosupport = 1;
					break;
				default:
					nosupport = 1;
					break;
			}
			break;
		case DEMO_BANK_C:
			ret = nim_panic6158_set_reg(dev, MN88472_REG_24MHZ_DVBC);
			break;
		default:
			nosupport = 1;
			break;
	}
	if (1 == nosupport)
		NIM_PANIC6158_PRINTF("ERROR : Not Supported System");
	return ret;
}
static INT32 nim_panic6158_tune_action_earda(struct nim_device *dev, UINT8 system, UINT32 frq, UINT8 bw, UINT8 qam, UINT32 wait_time, UINT8 scan_mode, UINT8 PLP_id)
{
	INT32 ret = !SUCCESS;
	UINT16 i, j, cycle = 1;
	UINT8 data, lock = 0;
	struct nim_panic6158_private *priv;
	UINT8 qam_array[] = {QAM64, QAM256, QAM128, QAM32, QAM16};
	priv = (struct nim_panic6158_private *) dev->priv;
	if (DEMO_BANK_T == system || DEMO_BANK_C == system)
	{
		NIM_PANIC6158_PRINTF("tune T\n");
		if (DEMO_BANK_C == system)
		{
			for (i = 0; i < ARRAY_SIZE(qam_array); i++)
			{
				if (qam_array[i] == qam)
					break;
			}
			if (i >= ARRAY_SIZE(qam_array))
				i = 0;
			nim_reg_mask_write(dev, system, DMD_CHSRCHSET_C, 0x0F, 0x08 | i);
		}
		else if (DEMO_BANK_T == system)
		{
			nim_panic6158_set_info(dev, system, DMD_E_INFO_DVBT_MODE, DMD_E_DVBT_MODE_NOT_DEFINED);
			nim_panic6158_set_info(dev, system, DMD_E_INFO_DVBT_GI, DMD_E_DVBT_GI_NOT_DEFINED);
		}
		if (1 == priv->scan_stop_flag)
			return SUCCESS;
		//tune tuner
		//tun_mxl301_set_addr(priv->tuner_id, priv->i2c_addr[DEMO_BANK_T2], priv->i2c_mutex_id);
		tun_mxl603_set_addr(priv->tuner_id, priv->i2c_addr[DEMO_BANK_T2], priv->i2c_mutex_id);
		if (NULL != priv->tc.nim_Tuner_Control)
			priv->tc.nim_Tuner_Control(priv->tuner_id, frq, bw, 0, NULL, 0);
		osal_task_sleep(10);
		if (NULL != priv->tc.nim_Tuner_Status)
			priv->tc.nim_Tuner_Status(priv->tuner_id, &lock);
		if (1 == priv->scan_stop_flag)
			return SUCCESS;
		NIM_PANIC6158_PRINTF("[%s]%d,T/C tuner lock =%d\n", __FUNCTION__, __LINE__, lock);
		//get tuner lock state
		if (1 != lock)
		{
			NIM_PANIC6158_PRINTF("tuner lock failed \n");
			return !SUCCESS;
		}
		osal_task_sleep(100);
		//reset demo
		data = 0x9F;
		nim_reg_write(dev, DEMO_BANK_T2, DMD_RSTSET1, &data, 1);
		for (i = 0; i < (wait_time / 50); i++)
		{
			if (NULL != dev->get_lock)
			{
				if (1 == priv->scan_stop_flag)
					break;
				osal_task_sleep(50);
				dev->get_lock(dev, &lock);
				if (1 == lock)
				{
					ret = SUCCESS;
					break;
				}
			}
		}
	}
	else if (DEMO_BANK_T2 == system)
	{
		NIM_PANIC6158_PRINTF("tune T2\n");
		if (PANIC6158_SEARCH_MODE == scan_mode)
		{
			cycle = PANIC6158_T2_SEARCH_NUM;
			data = 0x43;
			nim_reg_write(dev, DEMO_BANK_T2, DMD_SSEQSET, &data, 1);
		}
		for (i = 0; i < cycle; i++)
		{
			data = 0x80;
			nim_reg_write(dev, DEMO_BANK_T2, DMD_FECSET1, &data, 1);
			data = PLP_id;
			nim_reg_write(dev, DEMO_BANK_T2, DMD_PLPID, &data, 1);
			if (1 == priv->scan_stop_flag)
				break;
			//tune tuner
			//tun_mxl301_set_addr(priv->tuner_id, priv->i2c_addr[DEMO_BANK_T2], priv->i2c_mutex_id);
			tun_mxl603_set_addr(priv->tuner_id, priv->i2c_addr[DEMO_BANK_T2], priv->i2c_mutex_id);
			if (NULL != priv->tc.nim_Tuner_Control)
			{
				priv->tc.nim_Tuner_Control(priv->tuner_id, frq, bw, 0, NULL, 0);
			}
			osal_task_sleep(10);
			if (NULL != priv->tc.nim_Tuner_Status)
			{
				priv->tc.nim_Tuner_Status(priv->tuner_id, &lock);
			}
			NIM_PANIC6158_PRINTF("[%s]%d,T2 tuner lock =%d\n", __FUNCTION__, __LINE__, lock);
			//get tuner lock state
			if (1 != lock)
			{
				NIM_PANIC6158_PRINTF("tuner lock failed \n");
				return !SUCCESS;
			}
			if (1 == priv->scan_stop_flag)
				break;
			osal_task_sleep(100);
			//reset demo
			data = 0x9F;
			nim_reg_write(dev, DEMO_BANK_T2, DMD_RSTSET1, &data, 1);
			for (j = 0; j < (PANIC6158_T2_TUNE_TIMEOUT / 50); j++)
			{
				if (1 == priv->scan_stop_flag)
					break;
				osal_task_sleep(50);
				nim_reg_read(dev, priv->system, DMD_SSEQFLG, &data, 1);
				//if bit6 or bit5 is 1, then not need to wait
				if ((0x60 & data) || (1 == priv->scan_stop_flag))
					break;
				if (13 <= data)
				{
					if (PANIC6158_SEARCH_MODE != scan_mode)
						return SUCCESS;
#if 0
					//get PLP id
					data = 0x0;
					nim_reg_write(dev, DEMO_BANK_T2, DMD_TPDSET1, &data, 1);
					data = 0x7;
					nim_reg_write(dev, DEMO_BANK_T2, DMD_TPDSET2, &data, 1);
					nim_reg_read(dev, DEMO_BANK_T2, 0xC6/*0xCA*/, &data, 1);
					libc_printf_direct("%s T2 lock, PLP: %d! \n", __FUNCTION__, data);
#endif
					//get PLP num
					data = 0x0;
					nim_reg_write(dev, DEMO_BANK_T2, DMD_TPDSET1, &data, 1);
					data = 0x2;
					nim_reg_write(dev, DEMO_BANK_T2, DMD_TPDSET2, &data, 1);
					nim_reg_read(dev, DEMO_BANK_T2, 0xC6, &data, 1);
					//libc_printf_direct("%s T2 lock, PLP num: %d! \n", __FUNCTION__, data);
					priv->PLP_num = data;
					return SUCCESS;
				}
			}
			if (1 == priv->scan_stop_flag)
				break;
		}
	}
	return ret;
}

INT32 nim_panic6158_channel_change_earda(struct nim_device *dev, struct NIM_Channel_Change *param)
{
	INT32 i, j;
	INT32 tune_num = 1;
	UINT32 wait_time;
	UINT8 mode[2];
	UINT8 scan_mode;
	struct nim_panic6158_private *priv;
	UINT8 qam_array[] = {QAM64, QAM256, QAM128, QAM32, QAM16};
	priv = (struct nim_panic6158_private *) dev->priv;
	priv->frq = param->freq;
	priv->bw = param->bandwidth;
	priv->qam = param->modulation;
	priv->PLP_id = param->PLP_id;
	priv->PLP_num = 0;//Init PLP num

	switch (param->priv_param)	  
	{
	case DEMO_DVBC:
		mode[0] = DEMO_BANK_C;
		break;
	case DEMO_DVBT:
		mode[0] = DEMO_BANK_T;
		break;
	case DEMO_DVBT2:
		mode[0] = DEMO_BANK_T2;
		break;
	default:
		tune_num = PANIC6158_TUNE_MAX_NUM;
		mode[0] = (1 == priv->first_tune_t2) ? DEMO_BANK_T2 : DEMO_BANK_T;
		mode[1] = (DEMO_BANK_T2 == mode[0]) ? DEMO_BANK_T : DEMO_BANK_T2;
		break;
	}

	NIM_PANIC6158_PRINTF("frq: %dKHz, bw: %dMHz, plp_id: %d system mode: %d, qam: %d\n", priv->frq/1000, priv->bw, priv->PLP_id, mode[0], priv->qam);
	NIM_PANIC6158_PRINTF("tune_num:%d\n", tune_num);

	priv->scan_stop_flag = 0;
	//libc_printf_direct("--PANIC6158 T2 PLP_id=%d -------\n", priv->PLP_id);
	scan_mode = (0 == priv->PLP_id) ? PANIC6158_SEARCH_MODE : PANIC6158_TUNE_MODE;
	for (i = 0; i < tune_num; i++)
	{
		priv->system = mode[i];
		//set IF freq
		if (DMD_E_BW_7MHZ == priv->bw && (DEMO_BANK_T == mode[i] || DEMO_BANK_T2 == mode[i]))
			priv->if_freq = DMD_E_IF_4500KHZ;
		else
			priv->if_freq = DMD_E_IF_5000KHZ;
		//config demo
		nim_panic6158_set_reg(dev, MN88472_REG_24MHZ_COMMON);
		NIM_PANIC6158_PRINTF("mode[%d] = %d\n", i, mode[i]);
		NIM_PANIC6158_PRINTF("priv->if_freq = %d\n", priv->if_freq);
		nim_panic6158_set_system_earda(dev, priv->system, priv->bw, priv->if_freq);
		//nim_panic6158_set_error_flag(dev, 1);
		//nim_panic6158_set_ts_output(dev, DMD_E_TSOUT_SERIAL_VARIABLE_CLOCK);
		if (1 == priv->scan_stop_flag)
			break;
		//tune demo
		wait_time = (PANIC6158_TUNE_MAX_NUM == tune_num && 0 == i) ? 1000 : 300;//ms
//#ifdef DVBC_BLIND_SCAN
#if 1
		if ((mode[0] == DEMO_BANK_C) && (priv->qam == 0)) //for DVB-C blind scan
		{
			for (j = 0; j < (INT32)ARRAY_SIZE(qam_array); j++)
			{
				priv->qam = qam_array[j];
				NIM_PANIC6158_PRINTF("auto scan frq:%dKHz, QAM:%d\n", priv->frq/1000, qam_array[j]);
				if (SUCCESS == nim_panic6158_tune_action_earda(dev, mode[i], priv->frq, priv->bw, qam_array[j], wait_time, scan_mode, priv->PLP_id))
					break;
			}
		}
		else
#endif
		{
			if (SUCCESS == nim_panic6158_tune_action_earda(dev, mode[i], priv->frq, priv->bw, priv->qam, wait_time, scan_mode, priv->PLP_id))
				break;
		}
	}
	return SUCCESS;
}

INT32 nim_panic6158_ioctl_earda(struct nim_device *dev, INT32 cmd, UINT32 param)
{
	INT32 ret = SUCCESS;
	struct nim_panic6158_private *priv;
	priv = (struct nim_panic6158_private *)dev->priv;
	//dym add for mx603
	if (cmd & NIM_TUNER_COMMAND)
	{
		if (NIM_TUNER_CHECK == cmd)
			return ret;
		if (priv->tc.nim_Tuner_Command != NULL)
		{
			ret = priv->tc.nim_Tuner_Command(priv->tuner_id, cmd, param);
		}
		else
		{
			ret = ERR_FAILUE;
		}
	}
	return ret;
}

static INT32 nim_panic6158_ioctl_ext_earda(struct nim_device *dev, INT32 cmd, void *param_list)
{
	INT32 ret = SUCCESS;
	struct nim_t10023_private *priv;
	priv = (struct nim_t10023_private *)dev->priv;
	switch (cmd)
	{
		case NIM_DRIVER_AUTO_SCAN:
			ret = SUCCESS;
			break;
		case NIM_DRIVER_CHANNEL_CHANGE:
			ret = nim_panic6158_channel_change_earda(dev, (struct NIM_Channel_Change *)(param_list));
			break;
		case NIM_DRIVER_CHANNEL_SEARCH:
			ret = SUCCESS;
			break;
		case NIM_DRIVER_GET_RF_LEVEL:
			break;
		case NIM_DRIVER_GET_CN_VALUE:
			break;
		case NIM_DRIVER_GET_BER_VALUE:
			break;
		default:
			ret = SUCCESS;
			break;
	}
	return ret;
}

INT32 nim_panic6158_attach_earda(UINT8 Handle, PCOFDM_TUNER_CONFIG_API pConfig, TUNER_OpenParams_T *OpenParams)
{
	INT32 ret;
	UINT8 data, i;
	struct nim_device *dev;
	struct nim_panic6158_private *priv;
	DEM_WRITE_READ_TUNER ThroughMode;
	TUNER_ScanTaskParam_T *Inst = NULL;
	TUNER_IOARCH_OpenParams_t Tuner_IOParams;
	if (NULL == pConfig || 2 < nim_panic6158_cnt)
	{
		NIM_PANIC6158_PRINTF("pConfig nim_panic6158_cnt = %d!\n", nim_panic6158_cnt);
		return ERR_NO_DEV;
	}
	Inst = TUNER_GetScanInfo(Handle);
	dev = (struct nim_device *)dev_alloc(nim_panic6158_name[nim_panic6158_cnt], HLD_DEV_TYPE_NIM, sizeof(struct nim_device));
	if (NULL == dev)
	{
		NIM_PANIC6158_PRINTF("Alloc nim_device error!\n");
		return ERR_NO_MEM;
	}
	priv = (PNIM_PANIC6158_PRIVATE)MALLOC(sizeof(struct nim_panic6158_private));
	if (NULL == priv)
	{
		dev_free(dev);
		NIM_PANIC6158_PRINTF("Alloc nim device prive memory error!\n");
		return ERR_NO_MEM;
	}
	MEMSET(priv, 0, sizeof(struct nim_panic6158_private));
	MEMCPY((void *) & (priv->tc), (void *)pConfig, sizeof(struct COFDM_TUNER_CONFIG_API));
	//priv->tuner_if_freq = pConfig->tuner_config.wTuner_IF_Freq;
	dev->priv = (void *)priv;
	if (YWTUNER_DELIVER_TER == Inst->Device)
	{
		dev->DemodIOHandle[0] = Inst->DriverParam.Ter.DemodIOHandle;
		dev->DemodIOHandle[1] = Inst->DriverParam.Ter.TunerIOHandle;
	}
	else if (YWTUNER_DELIVER_CAB == Inst->Device)
	{
		dev->DemodIOHandle[0] = Inst->DriverParam.Cab.DemodIOHandle;
		dev->DemodIOHandle[1] = Inst->DriverParam.Cab.TunerIOHandle;
	}
	NIM_PANIC6158_PRINTF("dev->DemodIOHandle[0] = %d\n", dev->DemodIOHandle[0]);
	NIM_PANIC6158_PRINTF("dev->DemodIOHandle[1] = %d\n", dev->DemodIOHandle[1]);
	Tuner_IOParams.IORoute = TUNER_IO_DIRECT;
	Tuner_IOParams.IODriver = OpenParams->TunerIO.Driver;
	YWLIB_Strcpy((S8 *)Tuner_IOParams.DriverName, (S8 *)OpenParams->TunerIO.DriverName);
	Tuner_IOParams.Address = PANIC6158_C_ADDR;//OpenParams->TunerIO.Address;
	Tuner_IOParams.hInstance = NULL;//&Inst->DriverParam.Ter.DemodIOHandle;
	Tuner_IOParams.TargetFunction = NULL;//Inst->DriverParam.Ter.DemodDriver.Demod_repeat;
	Tuner_IOParams.I2CIndex = OpenParams->I2CBusNo;
	TUNER_IOARCH_Open(&dev->DemodIOHandle[2], &Tuner_IOParams);
	NIM_PANIC6158_PRINTF("dev->DemodIOHandle[2] = %d\n", dev->DemodIOHandle[2]);
	//printf("[%s]%d,DemodIOHandle[1]=0x%x,DemodIOHandle[2]=0x%x\n",__FUNCTION__,__LINE__,dev->DemodIOHandle[1],dev->DemodIOHandle[2]);
	//jhy add end
	dev->i2c_type_id = pConfig->ext_dm_config.i2c_type_id;
	dev->nim_idx = pConfig->ext_dm_config.nim_idx;//distinguish left or right
	dev->init = NULL;
	dev->open = nim_panic6158_open;
	dev->stop = nim_panic6158_close;
	dev->disable = NULL; //nim_m3101_disable;
	dev->do_ioctl = nim_panic6158_ioctl_earda;//nim_m3101_ioctl;
	dev->do_ioctl_ext = nim_panic6158_ioctl_ext_earda;//nim_m3101_ioctl;
	dev->channel_change = nim_panic6158_channel_change_earda;
	dev->channel_search = NULL;//nim_m3101_channel_search;
	dev->get_lock = nim_panic6158_get_lock;
	dev->get_freq = nim_panic6158_get_freq;//nim_m3101_get_freq;
	dev->get_sym = nim_panic6158_get_sym;
	dev->get_FEC = nim_panic6158_get_code_rate; ////nim_m3101_get_code_rate;
	dev->get_AGC = nim_panic6158_get_AGC_603;
	dev->get_SNR = nim_panic6158_get_SNR;
	dev->get_BER = nim_panic6158_get_BER;
	dev->get_PER = nim_panic6158_get_PER;
	dev->get_bandwidth = nim_panic6158_get_bandwidth;//nim_m3101_get_bandwith;
	dev->get_frontend_type = nim_panic6158_get_system;
	dev->get_PLP_id = nim_panic6158_get_PLP_id;
	dev->get_PLP_num = nim_panic6158_get_PLP_num;
	//added for DVB-T additional elements
	dev->get_guard_interval = NULL;//nim_m3101_get_GI;
	dev->get_fftmode = NULL;//nim_m3101_get_fftmode;
	dev->get_modulation = nim_panic6158_get_modulation;//nim_m3101_get_modulation;
	dev->get_spectrum_inv = NULL;//nim_m3101_get_specinv;
	dev->get_freq_offset = NULL;//nim_m3101_get_freq_offset;
	dev->mutex_id = OSAL_INVALID_ID;
	priv->i2c_addr[0] = PANIC6158_T_ADDR;
	priv->i2c_addr[1] = PANIC6158_T2_ADDR;
	priv->i2c_addr[2] = PANIC6158_C_ADDR;
	priv->if_freq = DMD_E_IF_5000KHZ;
	priv->flag_id = OSAL_INVALID_ID;
	priv->i2c_mutex_id = OSAL_INVALID_ID;
	priv->first_tune_t2 = pConfig->tune_t2_first;
	priv->tuner_id = Handle; //jhy added
	NIM_PANIC6158_PRINTF("priv->tuner_id = %d\n", priv->tuner_id);
	priv->i2c_mutex_id = osal_mutex_create();
	if (OSAL_INVALID_ID == priv->i2c_mutex_id)
	{
		TUNER_IOARCH_CloseParams_t CloseParams;
		osal_flag_delete(priv->flag_id);
		TUNER_IOARCH_Close(dev->DemodIOHandle[2], &CloseParams);
		FREE(priv);
		dev_free(dev);
		NIM_PANIC6158_PRINTF("%s: no more mutex\n", __FUNCTION__);
		return ERR_ID_FULL;
	}
	osal_mutex_unlock(priv->i2c_mutex_id);
	/* Add this device to queue */
	if (SUCCESS != dev_register(dev))
	{
		TUNER_IOARCH_CloseParams_t CloseParams;
		NIM_PANIC6158_PRINTF("Error: Register nim device error!\n");
		osal_flag_delete(priv->flag_id);
		osal_mutex_delete(priv->i2c_mutex_id);
		TUNER_IOARCH_Close(dev->DemodIOHandle[2], &CloseParams);
		FREE(priv);
		dev_free(dev);
		return ERR_NO_DEV;
	}
	ret = SUCCESS;
	for (i = 0; i < 3; i++)
	{
		ret = nim_reg_read(dev, DEMO_BANK_T, DMD_CHIPRD, &data, 1);
		if (2 == data)
			break;
	}
	if (3 <= i || 0 < ret)
	{
		TUNER_IOARCH_CloseParams_t CloseParams;
		osal_flag_delete(priv->flag_id);
		osal_mutex_delete(priv->i2c_mutex_id);
		TUNER_IOARCH_Close(dev->DemodIOHandle[2], &CloseParams);
		FREE(priv);
		dev_free(dev);
		NIM_PANIC6158_PRINTF("Error: read DMD_CHIPRD data = %d!\n", data);
		return !SUCCESS;
	}
	if (NULL != pConfig->nim_Tuner_Init)
	{
		if (SUCCESS == pConfig->nim_Tuner_Init(&priv->tuner_id, &pConfig->tuner_config))
		{
			//priv_mem->tuner_type = tuner_type;
			priv->tc.nim_Tuner_Init = pConfig->nim_Tuner_Init;
			priv->tc.nim_Tuner_Control = pConfig->nim_Tuner_Control;
			priv->tc.nim_Tuner_Status = pConfig->nim_Tuner_Status;
			MEMCPY(&priv->tc.tuner_config, &pConfig->tuner_config, sizeof(struct COFDM_TUNER_CONFIG_EXT));
			ThroughMode.nim_dev_priv = dev->priv;
			ThroughMode.Dem_Write_Read_Tuner = DMD_TCB_WriteRead;///////////////////////////////////
			nim_panic6158_ioctl_earda(dev, NIM_TUNER_SET_THROUGH_MODE, (UINT32)&ThroughMode);
		}
		else
		{
			NIM_PANIC6158_PRINTF("Error: Init earda Tuner Failure!\n");
			osal_flag_delete(priv->flag_id);
			osal_mutex_delete(priv->i2c_mutex_id);
			TUNER_IOARCH_CloseParams_t CloseParams;
			TUNER_IOARCH_Close(dev->DemodIOHandle[2], &CloseParams);
			FREE(priv);
			dev_free(dev);
			return ERR_NO_DEV;
		}
	}
	Inst->userdata = (void *)dev;
	nim_panic6158_cnt++;
	ret = nim_panic6158_open(dev);
	if (ret != SUCCESS)
	{
		TUNER_IOARCH_CloseParams_t CloseParams;
		TUNER_IOARCH_Close(dev->DemodIOHandle[2], &CloseParams);
		YWOS_Free(priv);
		YWOS_Free(dev);
		return ERR_NO_DEV;
	}
	return SUCCESS;
}
