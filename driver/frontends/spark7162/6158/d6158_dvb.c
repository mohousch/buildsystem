/*$Source$*/
/*****************************文件头部注释*************************************/
//
//			Copyright (C), 2013-2018, AV Frontier Tech. Co., Ltd.
//
//
// 文 件 名： $RCSfile$
//
// 创 建 者： D26LF
//
// 创建时间： 2013.12.16
//
// 最后更新： $Date$
//
//				$Author$
//
//				$Revision$
//
//				$State$
//
// 文件描述： d6158 dvb
//
/******************************************************************************/

/******************************** 文件包含************************************/

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
#include "tun_mxl301.h"

#include "dvb_frontend.h"
#include "d6158.h"
#include "nim_panic6158_ext.h"

#include "d6158_dvb.h"
#include "stv0367ofdm_init.h"

#include "../../base/mxl301.h"

/******************************** 常量定义************************************/

/******************************** 数据结构************************************/

struct dvb_d6158_fe_ofdm_state
{
	struct nim_device spark_nimdev;
	struct i2c_adapter *i2c;
	struct dvb_frontend frontend;
	//IOARCH_Handle_t IOHandle;
	TUNER_IOREG_DeviceMap_t DeviceMap;
	struct dvb_frontend_parameters *p;
};

/******************************** 宏 定 义************************************/

/******************************** 变量定义************************************/

/******************************** 变量引用************************************/

/******************************** 函数声明************************************/

/******************************** 函数定义************************************/

int d6158_read_snr(struct dvb_frontend *fe, u16 *snr)
{
	int iRet;
	struct dvb_d6158_fe_ofdm_state *state = fe->demodulator_priv;
	iRet = nim_panic6158_get_SNR(&(state->spark_nimdev), (UINT8 *)snr);
	*snr = *snr * 255 * 255 / 100;
	return iRet;
}

int d6158_read_ber(struct dvb_frontend *fe, UINT32 *ber)
{
	struct dvb_d6158_fe_ofdm_state *state = fe->demodulator_priv;
	return nim_panic6158_get_BER(&state->spark_nimdev, ber);
}

int d6158_read_signal_strength(struct dvb_frontend *fe, u16 *strength)
{
	int iRet;
	u32 Strength;
	u32 *Intensity = &Strength;
	struct dvb_d6158_fe_ofdm_state *state = fe->demodulator_priv;
	iRet = nim_panic6158_get_AGC(&state->spark_nimdev, (UINT8 *)Intensity);
	if (*Intensity > 90)
		*Intensity = 90;
	if (*Intensity < 10)
		*Intensity = *Intensity * 11 / 2;
	else
		*Intensity = *Intensity / 2 + 50;
	if (*Intensity > 90)
		*Intensity = 90;
	*Intensity = *Intensity * 255 * 255 / 100;
	//printk("*Intensity = %d\n", *Intensity);
	*strength = (*Intensity);
	YWOS_TaskSleep(100);
	return iRet;
}

int d6158_read_status(struct dvb_frontend *fe, enum fe_status *status)
{
	int j, iRet;
	struct dvb_d6158_fe_ofdm_state *state = fe->demodulator_priv;
	UINT8 bIsLocked;
	//printk("%s>>\n", __FUNCTION__);
	for (j = 0; j < (PANIC6158_T2_TUNE_TIMEOUT / 50); j++)
	{
		YWOS_TaskSleep(50);
		iRet = nim_panic6158_get_lock(&state->spark_nimdev, &bIsLocked);
		if (bIsLocked)
		{
			break;
		}
	}
	//printk("bIsLocked = %d\n", bIsLocked);
	if (bIsLocked)
	{
		*status = FE_HAS_SIGNAL
				  | FE_HAS_CARRIER
				  | FE_HAS_VITERBI
				  | FE_HAS_SYNC
				  | FE_HAS_LOCK;
	}
	else
	{
		*status = 0;
	}
	return 0;
}

int d6158_read_ucblocks(struct dvb_frontend *fe,
						u32 *ucblocks)
{
	*ucblocks = 0;
	return 0;
}

int dvb_d6158_get_property(struct dvb_frontend *fe, struct dtv_property *tvp)
{
	//struct dvb_d0367_fe_ofdm_state* state = fe->demodulator_priv;
	/* get delivery system info */
	if (tvp->cmd == DTV_DELIVERY_SYSTEM)
	{
		switch (tvp->u.data)
		{
			case SYS_DVBT:
			case SYS_DVBT2:
			case SYS_DVBC_ANNEX_AC:
				break;
			default:
				return -EINVAL;
		}
	}
	return 0;
}

int dvb_d6158_fe_qam_get_property(struct dvb_frontend *fe, struct dtv_property *tvp)
{
	//struct dvb_d0367_fe_ofdm_state* state = fe->demodulator_priv;
	/* get delivery system info */
	if (tvp->cmd == DTV_DELIVERY_SYSTEM)
	{
		switch (tvp->u.data)
		{
			case SYS_DVBC_ANNEX_AC:
				break;
			default:
				return -EINVAL;
		}
	}
	return 0;
}

int d6158_set_frontend(struct dvb_frontend *fe,
					   struct dvb_frontend_parameters *p)
{
	struct dvb_d6158_fe_ofdm_state *state = fe->demodulator_priv;
	struct dtv_frontend_properties *props = &fe->dtv_property_cache;

	printk("-----------------------d6158_set_frontend\n");
	printk("freq %d, bw %d, system %d, stream_id %d\n", props->frequency, props->bandwidth_hz,
	       props->delivery_system, props->stream_id);

	state->p = p;
	demod_d6158_ScanFreq(p, &state->spark_nimdev, props->delivery_system,
				props->stream_id != NO_STREAM_ID_FILTER ? props->stream_id : 0);
	state->p = NULL;
	return 0;
}

int d6158earda_set_frontend(struct dvb_frontend *fe,
							struct dvb_frontend_parameters *p)
{
	struct dvb_d6158_fe_ofdm_state *state = fe->demodulator_priv;
	struct dtv_frontend_properties *props = &fe->dtv_property_cache;

	printk("-----------------------d6158earda_set_frontend\n");
	printk("freq %d, bw %d, system %d, stream_id %d\n", props->frequency, props->bandwidth_hz,
	       props->delivery_system, props->stream_id);

	state->p = p;
	demod_d6158earda_ScanFreq(p, &state->spark_nimdev, props->delivery_system,
				  props->stream_id != NO_STREAM_ID_FILTER ? props->stream_id : 0);
	state->p = NULL;
	return 0;
}

static struct dvb_frontend_ops dvb_d6158_fe_ofdm_ops =
{

	.info = {
		.name = "Tuner3-T/T2(T/T2/C)",
		.type = FE_OFDM,
		.frequency_min = 47000000,
		.frequency_max = 863250000,
		.frequency_stepsize = 62500,
		.caps = FE_CAN_FEC_1_2 | FE_CAN_FEC_2_3 | FE_CAN_FEC_3_4 |
		FE_CAN_FEC_4_5 | FE_CAN_FEC_5_6 | FE_CAN_FEC_6_7 |
		FE_CAN_FEC_7_8 | FE_CAN_FEC_8_9 | FE_CAN_FEC_AUTO |
		FE_CAN_QAM_16 | FE_CAN_QAM_64 | FE_CAN_QAM_AUTO |
		FE_CAN_TRANSMISSION_MODE_AUTO |
		FE_CAN_GUARD_INTERVAL_AUTO |
		FE_CAN_HIERARCHY_AUTO |
		FE_CAN_2G_MODULATION |
		FE_CAN_MULTISTREAM
	},

	.init = NULL,
	.release = NULL,
	.sleep = NULL,
	.set_frontend = d6158_set_frontend,
	.get_frontend = NULL,

	.read_ber = d6158_read_ber,
	.read_snr = d6158_read_snr,
	.read_signal_strength = d6158_read_signal_strength,
	.read_status = d6158_read_status,

	.read_ucblocks = d6158_read_ucblocks,
	.i2c_gate_ctrl = NULL,
#if (DVB_API_VERSION < 5)
	.get_info = NULL,
#else
	.get_property = dvb_d6158_get_property,
#endif

};

static struct dvb_frontend_ops dvb_d6158_fe_qam_ops =
{

	.info = {
		.name = "Tuner3-C(T/T2/C)",
		.type = FE_QAM,
		.frequency_stepsize = 62500,
		.frequency_min = 51000000,
		.frequency_max = 858000000,
		.symbol_rate_min = (57840000 / 2) / 64, /* SACLK/64 == (XIN/2)/64 */
		.symbol_rate_max = (57840000 / 2) / 4, /* SACLK/4 */
		.caps = FE_CAN_QAM_16 | FE_CAN_QAM_32 | FE_CAN_QAM_64 |
		FE_CAN_QAM_128 | FE_CAN_QAM_256 |
		FE_CAN_FEC_AUTO | FE_CAN_INVERSION_AUTO
	},

	.init = NULL,
	.release = NULL,
	.sleep = NULL,
	.set_frontend = d6158_set_frontend,
	.get_frontend = NULL,

	.read_ber = d6158_read_ber,
	.read_snr = d6158_read_snr,
	.read_signal_strength = d6158_read_signal_strength,
	.read_status = d6158_read_status,

	.read_ucblocks = d6158_read_ucblocks,
	.i2c_gate_ctrl = NULL,
#if (DVB_API_VERSION < 5)
	.get_info = NULL,
#else
	.get_property = dvb_d6158_fe_qam_get_property,
#endif

};

struct dvb_frontend *dvb_d6158_attach(struct i2c_adapter *i2c, UINT8 system)
{
	struct dvb_d6158_fe_ofdm_state *state = NULL;
	struct nim_panic6158_private *priv;
	int ret;
	struct COFDM_TUNER_CONFIG_API Tuner_API;
	/* allocate memory for the internal state */
	state = kzalloc(sizeof(struct dvb_d6158_fe_ofdm_state), GFP_KERNEL);
	if (state == NULL) goto error;
	priv = (PNIM_PANIC6158_PRIVATE)YWOS_Malloc(sizeof(struct nim_panic6158_private));
	if (NULL == priv)
	{
		goto error;
	}
	/* create dvb_frontend */
	if (system == DEMO_BANK_T2) //dvb-t
	{
		printk("DEMO_BANK_T2\n");
		memcpy(&state->frontend.ops, &dvb_d6158_fe_ofdm_ops, sizeof(struct dvb_frontend_ops));
	}
	else if (system == DEMO_BANK_C) //dvb-c
	{
		printk("DEMO_BANK_C\n");
		memcpy(&state->frontend.ops, &dvb_d6158_fe_qam_ops, sizeof(struct dvb_frontend_ops));
	}
	state->frontend.demodulator_priv = state;
	state->i2c = i2c;
	state->DeviceMap.Timeout = IOREG_DEFAULT_TIMEOUT;
	state->DeviceMap.Registers = STV0367ofdm_NBREGS;
	state->DeviceMap.Fields = STV0367ofdm_NBFIELDS;
	state->DeviceMap.Mode = IOREG_MODE_SUBADR_16;
	state->DeviceMap.RegExtClk = 27000000; //Demod External Crystal_HZ
	state->DeviceMap.RegMap = (TUNER_IOREG_Register_t *)
							  kzalloc(state->DeviceMap.Registers * sizeof(TUNER_IOREG_Register_t),
									  GFP_KERNEL);
	state->DeviceMap.priv = (void *)state;
	state->spark_nimdev.priv = priv;
	state->spark_nimdev.base_addr = PANIC6158_T2_ADDR;
	//state->spark_nimdev.i2c_type_id= pConfig->ext_dm_config.i2c_type_id;
	//state->spark_nimdev.nim_idx = pConfig->ext_dm_config.nim_idx;//distinguish left or right
	priv->i2c_adap = i2c;
	priv->i2c_addr[0] = PANIC6158_T_ADDR;
	priv->i2c_addr[1] = PANIC6158_T2_ADDR;
	priv->i2c_addr[2] = PANIC6158_C_ADDR;
	priv->if_freq = DMD_E_IF_5000KHZ;
	priv->flag_id = OSAL_INVALID_ID;
	priv->i2c_mutex_id = OSAL_INVALID_ID;
	priv->system = system; //T2 C
	priv->first_tune_t2 = 1;
	priv->tuner_id = 2;
	if (tuner_mxl301_Identify((IOARCH_Handle_t *)i2c) != YW_NO_ERROR)
	{
		printk("tuner_mxl301_Identify error!\n");
		YWOS_Free(priv);
		kfree(state);
		return NULL;
	}
	YWOS_TaskSleep(50);
	YWLIB_Memset(&Tuner_API, 0, sizeof(struct COFDM_TUNER_CONFIG_API));
	Tuner_API.nim_Tuner_Init = tun_mxl301_init;
	Tuner_API.nim_Tuner_Status = tun_mxl301_status;
	Tuner_API.nim_Tuner_Control = tun_mxl301_control;
	Tuner_API.tune_t2_first = 1;//when demod_d6158_ScanFreq,param.priv_param >DEMO_DVBT2,tuner tune T2 then t
	Tuner_API.tuner_config.demo_type = PANASONIC_DEMODULATOR;
	Tuner_API.tuner_config.cTuner_Base_Addr = 0xC2;
	Tuner_API.tuner_config.i2c_adap = (IOARCH_Handle_t *)i2c;
	//printf("[%s]%d,i=%d,ret=%d \n",__FUNCTION__,__LINE__,i,ret);
	if (NULL != Tuner_API.nim_Tuner_Init)
	{
		if (SUCCESS == Tuner_API.nim_Tuner_Init(&priv->tuner_id, &Tuner_API.tuner_config))
		{
			priv->tc.nim_Tuner_Init = Tuner_API.nim_Tuner_Init;
			priv->tc.nim_Tuner_Control = Tuner_API.nim_Tuner_Control;
			priv->tc.nim_Tuner_Status = Tuner_API.nim_Tuner_Status;
			YWLIB_Memcpy(&priv->tc.tuner_config, &Tuner_API.tuner_config, sizeof(struct COFDM_TUNER_CONFIG_EXT));
		}
	}
	state->spark_nimdev.get_lock = nim_panic6158_get_lock;
	state->spark_nimdev.get_AGC = nim_panic6158_get_AGC_301;
	ret = nim_panic6158_open(&state->spark_nimdev);
	printk("[%s]%d,open result=%d \n", __FUNCTION__, __LINE__, ret);
	/* Setup init work mode */
	return &state->frontend;
error:
	kfree(state);
	return NULL;
}

struct dvb_frontend *dvb_d6158earda_attach(struct i2c_adapter *i2c, UINT8 system)
{
	struct dvb_d6158_fe_ofdm_state *state = NULL;
	struct nim_panic6158_private *priv;
	int ret;
	struct COFDM_TUNER_CONFIG_API Tuner_API;
	/* allocate memory for the internal state */
	state = kzalloc(sizeof(struct dvb_d6158_fe_ofdm_state), GFP_KERNEL);
	if (state == NULL) goto error;
	priv = (PNIM_PANIC6158_PRIVATE)YWOS_Malloc(sizeof(struct nim_panic6158_private));
	if (NULL == priv)
	{
		goto error;
	}
	MEMSET(priv, 0, sizeof(struct nim_panic6158_private));
	/* create dvb_frontend */
	if (system == DEMO_BANK_T2) //dvb-t
	{
		printk("DEMO_BANK_T2\n");
		memcpy(&state->frontend.ops, &dvb_d6158_fe_ofdm_ops, sizeof(struct dvb_frontend_ops));
	}
	else if (system == DEMO_BANK_C) //dvb-c
	{
		printk("DEMO_BANK_C\n");
		memcpy(&state->frontend.ops, &dvb_d6158_fe_qam_ops, sizeof(struct dvb_frontend_ops));
	}
	printf("[%s]%d\n", __FUNCTION__, __LINE__);
	state->frontend.ops.set_frontend = d6158earda_set_frontend;
	state->frontend.demodulator_priv = state;
	state->i2c = i2c;
	state->DeviceMap.Timeout = IOREG_DEFAULT_TIMEOUT;
	state->DeviceMap.Registers = STV0367ofdm_NBREGS;
	state->DeviceMap.Fields = STV0367ofdm_NBFIELDS;
	state->DeviceMap.Mode = IOREG_MODE_SUBADR_16;
	state->DeviceMap.RegExtClk = 27000000; //Demod External Crystal_HZ
	state->DeviceMap.RegMap = (TUNER_IOREG_Register_t *)
							  kzalloc(state->DeviceMap.Registers * sizeof(TUNER_IOREG_Register_t),
									  GFP_KERNEL);
	state->DeviceMap.priv = (void *)state;
	state->spark_nimdev.priv = priv;
	state->spark_nimdev.base_addr = PANIC6158_T2_ADDR;
	//state->spark_nimdev.i2c_type_id= pConfig->ext_dm_config.i2c_type_id;
	//state->spark_nimdev.nim_idx = pConfig->ext_dm_config.nim_idx;//distinguish left or right
	priv->i2c_adap = i2c;
	priv->i2c_addr[0] = PANIC6158_T_ADDR;
	priv->i2c_addr[1] = PANIC6158_T2_ADDR;
	priv->i2c_addr[2] = PANIC6158_C_ADDR;
	priv->if_freq = DMD_E_IF_5000KHZ;
	priv->flag_id = OSAL_INVALID_ID;
	priv->i2c_mutex_id = OSAL_INVALID_ID;
	priv->system = system; //T2 C
	priv->first_tune_t2 = 1;
	priv->tuner_id = 2;
	YWOS_TaskSleep(50);
	nim_config_EARDATEK11658(&Tuner_API, 0, 0);
	Tuner_API.tuner_config.i2c_adap = (IOARCH_Handle_t *)i2c;
	MEMCPY((void *) & (priv->tc), (void *)&Tuner_API, sizeof(struct COFDM_TUNER_CONFIG_API));
	printf("[%s]%d\n", __FUNCTION__, __LINE__);
	if (NULL != Tuner_API.nim_Tuner_Init)
	{
		if (SUCCESS == Tuner_API.nim_Tuner_Init(&priv->tuner_id, &Tuner_API.tuner_config))
		{
			DEM_WRITE_READ_TUNER ThroughMode;
			printf("[%s]%d\n", __FUNCTION__, __LINE__);
			priv->tc.nim_Tuner_Init = Tuner_API.nim_Tuner_Init;
			priv->tc.nim_Tuner_Control = Tuner_API.nim_Tuner_Control;
			priv->tc.nim_Tuner_Status = Tuner_API.nim_Tuner_Status;
			YWLIB_Memcpy(&priv->tc.tuner_config, &Tuner_API.tuner_config, sizeof(struct COFDM_TUNER_CONFIG_EXT));
			ThroughMode.nim_dev_priv = state->spark_nimdev.priv;
			printf("[%s]%d\n", __FUNCTION__, __LINE__);
			ThroughMode.Dem_Write_Read_Tuner = DMD_TCB_WriteRead;///////////////////////////////////
			nim_panic6158_ioctl_earda(&state->spark_nimdev, NIM_TUNER_SET_THROUGH_MODE, (UINT32)&ThroughMode);
			printf("[%s]%d\n", __FUNCTION__, __LINE__);
		}
	}
	printf("[%s]%d\n", __FUNCTION__, __LINE__);
	state->spark_nimdev.get_lock = nim_panic6158_get_lock;
	state->spark_nimdev.get_AGC = nim_panic6158_get_AGC_603;
	ret = nim_panic6158_open(&state->spark_nimdev);
	printk("[%s]%d,open result=%d \n", __FUNCTION__, __LINE__, ret);
	/* Setup init work mode */
	return &state->frontend;
error:
	kfree(state);
	return NULL;
}
struct MXL301_state
{
	struct dvb_frontend *fe;
	struct i2c_adapter *i2c;
	const struct MXL301_config *config;

	u32 frequency;
	u32 bandwidth;
};

static struct dvb_tuner_ops mxl301_ops =
{
	.set_params = NULL,
	.release = NULL,
};

struct dvb_frontend *mxl301_attach(struct dvb_frontend *fe,
								   const struct MXL301_config *config,
								   struct i2c_adapter *i2c)
{
	struct MXL301_state *state = NULL;
	struct dvb_tuner_info *info;
	state = kzalloc(sizeof(struct MXL301_state), GFP_KERNEL);
	if (state == NULL)
		goto exit;
	state->config = config;
	state->i2c = i2c;
	state->fe = fe;
	fe->tuner_priv = state;
	fe->ops.tuner_ops = mxl301_ops;
	info = &fe->ops.tuner_ops.info;
	memcpy(info->name, config->name, sizeof(config->name));
	printk("%s: Attaching sharp6465 (%s) tuner\n", __func__, info->name);
	return fe;
exit:
	kfree(state);
	return NULL;
}

//add by yabin

/***********************************************************************
 函数名称: tuner_mxl301_Identify

 函数说明: mxl301 的校验函数

************************************************************************/
YW_ErrorType_T tuner_mxl301_Identify(IOARCH_Handle_t *i2c_adap)
{
	YW_ErrorType_T ret = YW_NO_ERROR;
	unsigned char ucData = 0;
	U8 value[2] = {0};
	U8 reg = 0xEC;
	U8 mask = 0x7F;
	U8 data = 0x53;
	U8 cmd[3];
	U8 cTuner_Base_Addr = 0xC2;
	//tuner soft reset
	cmd[0] = cTuner_Base_Addr;
	cmd[1] = 0xFF;
	ret |= I2C_ReadWrite(i2c_adap, TUNER_IO_SA_WRITE, 0XF7, cmd, 2, 100);
	YWOS_TaskSleep(20);
	//reset end
	ret |= I2C_ReadWrite(i2c_adap, TUNER_IO_SA_READ, reg, value, 1, 100);
	value[0] |= mask & data;
	value[0] &= (mask ^ 0xff) | data;
	ret |= I2C_ReadWrite(i2c_adap, TUNER_IO_SA_WRITE, reg, value, 1, 100);
	cmd[0] = 0;
	ret |= I2C_ReadWrite(i2c_adap, TUNER_IO_SA_WRITE, 0XEE, cmd, 1, 100);
	cmd[0] = cTuner_Base_Addr;
	cmd[1] = 0xFB;
	cmd[2] = 0x17;
	ret |= I2C_ReadWrite(i2c_adap, TUNER_IO_SA_WRITE, 0XF7, cmd, 3, 100);
	cmd[0] = cTuner_Base_Addr | 0x1;
	ret |= I2C_ReadWrite(i2c_adap, TUNER_IO_SA_WRITE, 0XF7, cmd, 1, 100);
	ret |= I2C_ReadWrite(i2c_adap, TUNER_IO_READ, 0XF7, cmd, 1, 100);
	ucData = cmd[0];
	if (YW_NO_ERROR != ret || 0x09 != (U8)(ucData & 0x0F))
	{
		return YWHAL_ERROR_UNKNOWN_DEVICE;
	}
	return ret;
}

/* EOF------------------------------------------------------------------------*/

/* BOL-------------------------------------------------------------------*/
//$Log$
/* EOL-------------------------------------------------------------------*/

