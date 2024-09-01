
#include <linux/module.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/slab.h>

#include <linux/dvb/version.h>

#include "D3501.h"

#include "ywtuner_ext.h"
#include "tuner_def.h"

#include "ioarch.h"
#include "ioreg.h"
#include "tuner_interface.h"

#include "stv0367ofdm_init.h"
#include "chip_0367ter.h"
#include "stv0367ofdm_drv.h"
#include "d0367_ter.h"

#include "dvb_frontend.h"
#include "D0367.h"

extern int debug_fe7162;
#define _DEBUG if (debug_fe7162)

struct dvb_d0367_fe_ofdm_state
{
	struct i2c_adapter *i2c;
	struct dvb_frontend frontend;
	IOARCH_Handle_t IOHandle;
	TUNER_IOREG_DeviceMap_t DeviceMap;
	struct dvb_frontend_parameters *p;
};

YW_ErrorType_T D0367ter_ScanFreq(TUNER_IOREG_DeviceMap_t *DeviceMap,
								 IOARCH_Handle_t IOHandle);

#if 0
static int dvb_d0367_fe_ofdm_i2c_write(struct dvb_frontend *fe,
									   U16 uAddr,
									   unsigned char *pcData,
									   int nData)
{
	int ret;
	struct dvb_d0367_fe_ofdm_state *state = fe->demodulator_priv;
	struct i2c_adapter *i2c = state->i2c;
	int i;
	u8 buf[2 + nData];
	struct i2c_msg msg[] =
	{
		{ .addr = 0x38 >> 1, .flags = 0, .buf = buf, .len = 2 + nData }
	};
	for (i = 0; i < nData; i++)
	{
		printk("%02x ", pcData[i]);
	}
	printk("\n");
	buf[0] = ((uAddr >> 8) & 0xFF);
	buf[1] = ((uAddr & 0xFF));
	memcpy(&buf[2], pcData, nData);
	ret = i2c_transfer(i2c, &msg[0], 1);
	if (ret != 1)
	{
		if (ret != -ERESTARTSYS)
			printk("write error, Reg=[0x%x], Status=%d\n", uAddr, ret);
		return ret < 0 ? ret : -EREMOTEIO;
	}
	return ret;
}

static int dvb_d0367_fe_ofdm_i2c_read(struct dvb_frontend *fe,
									  U16 uAddr,
									  unsigned char *pcData,
									  int nData)
{
	int ret;
	struct dvb_d0367_fe_ofdm_state *state = fe->demodulator_priv;
	struct i2c_adapter *i2c = state->i2c;
	u8 b0[2];
	int i;
	struct i2c_msg msg[] =
	{
		{ .addr = 0x38 >> 1, .flags = 0, .buf = b0, .len = 2 },
		{ .addr = 0x38 >> 1, .flags = I2C_M_RD, .buf = pcData, .len = nData }
	};
	b0[0] = ((uAddr >> 8) & 0xFF);
	b0[1] = ((uAddr & 0xFF));
	ret = i2c_transfer(i2c, msg, 2);
	//for (i = 0; i < nData; i++)
	//{
	// printk("%02x ", pcData[i]);
	//}
	//printk("\n");
	if (ret != 2)
	{
		if (ret != -ERESTARTSYS)
			printk("Read error, Reg=[0x%x], Status=%d\n", uAddr, ret);
		return ret < 0 ? ret : -EREMOTEIO;
	}
	return ret;
}
#endif /* 0 */

static int dvb_d0367_fe_ofdm_read_status(struct dvb_frontend *fe,
										 fe_status_t *status)
{
	struct dvb_d0367_fe_ofdm_state *state = fe->demodulator_priv;
	//int iTunerLock = 0;
	BOOL bIsLocked;
	//printk("%s>>\n", __FUNCTION__);
#if 0
	if (fe->ops.tuner_ops.get_status)
	{
		if (fe->ops.tuner_ops.get_status(fe, &iTunerLock) < 0)
		{
			printk("1. Tuner get_status err\n");
		}
	}
	if (iTunerLock)
		printk("1. Tuner phase locked\n");
	else
		printk("1. Tuner unlocked\n");
	if (fe->ops.i2c_gate_ctrl)
	{
		if (fe->ops.i2c_gate_ctrl(fe, 0) < 0)
			goto exit;
	}
#endif /* 0 */
	{
		bIsLocked = FE_367ofdm_lock(&state->DeviceMap, state->IOHandle);
		_DEBUG
		printk("bIsLocked = %d\n", bIsLocked);
	}
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
#if 0
	{
		U32 Quality = 0;
		U32 Intensity = 0;
		U32 Ber = 0;
		FE_STV0367TER_GetSignalInfo(&state->DeviceMap, state->IOHandle,
									&Quality, &Intensity, &Ber);
		printk("Quality = %d, Intensity = %d, Ber = %d\n",
			   Quality, Intensity, Ber);
	}
#endif /* 0 */
	//printk("%s<<\n", __FUNCTION__);
	return 0;
#if 0
exit:
	printk(KERN_ERR "%s: dvb_d0367_fe_ofdm_read_status Error\n", __func__);
	return -1;
#endif /* 0 */
}

static int dvb_d0367_fe_ofdm_read_ber(struct dvb_frontend *fe, u32 *ber)
{
	struct dvb_d0367_fe_ofdm_state *state = fe->demodulator_priv;
	TUNER_IOREG_DeviceMap_t *DeviceMap;
	IOARCH_Handle_t IOHandle;
	DeviceMap = &state->DeviceMap;
	IOHandle = state->IOHandle;
	//printk("%s>>\n", __FUNCTION__);
	*ber = FE_367ofdm_GetErrors(DeviceMap, IOHandle);
	//printk("%s<<\n", __FUNCTION__);
	return 0;
}

static int dvb_d0367_fe_ofdm_read_signal_strength(struct dvb_frontend *fe,
												  u16 *strength)
{
	struct dvb_d0367_fe_ofdm_state *state = fe->demodulator_priv;
	TUNER_IOREG_DeviceMap_t *DeviceMap;
	IOARCH_Handle_t IOHandle;
	DeviceMap = &state->DeviceMap;
	IOHandle = state->IOHandle;
	//printk("%s>>\n", __FUNCTION__);
	*strength = FE_STV0367TER_GetPower(DeviceMap, IOHandle);
	//printk("%s<<\n", __FUNCTION__);
	return 0;
}

static int dvb_d0367_fe_ofdm_read_snr(struct dvb_frontend *fe, u16 *snr)
{
	struct dvb_d0367_fe_ofdm_state *state = fe->demodulator_priv;
	TUNER_IOREG_DeviceMap_t *DeviceMap;
	IOARCH_Handle_t IOHandle;
	DeviceMap = &state->DeviceMap;
	IOHandle = state->IOHandle;
	//printk("%s>>\n", __FUNCTION__);
	*snr = FE_STV0367TER_GetSnr(DeviceMap, IOHandle);
	//printk("%s<<\n", __FUNCTION__);
	return 0;
}

static int dvb_d0367_fe_ofdm_read_ucblocks(struct dvb_frontend *fe,
										   u32 *ucblocks)
{
	*ucblocks = 0;
	return 0;
}

static int dvb_d0367_fe_ofdm_get_frontend(struct dvb_frontend *fe,
										  struct dvb_frontend_parameters *p)
{
	return 0;
}

static int dvb_d0367_fe_ofdm_set_frontend(struct dvb_frontend *fe,
										  struct dvb_frontend_parameters *p)
{
	struct dvb_d0367_fe_ofdm_state *state = fe->demodulator_priv;
	TUNER_IOREG_DeviceMap_t *DeviceMap;
	IOARCH_Handle_t IOHandle;
	DeviceMap = &state->DeviceMap;
	IOHandle = state->IOHandle;
	state->p = p;
	D0367ter_ScanFreq(DeviceMap, IOHandle);
#if 0
	BOOL bIsLocked;
	bIsLocked = FE_367ofdm_lock(&state->DeviceMap, state->IOHandle);
	printk("bIsLocked = %d\n", bIsLocked);
#endif
	state->p = NULL;
	return 0;
}

static int dvb_d0367_fe_ofdm_sleep(struct dvb_frontend *fe)
{
	struct dvb_d0367_fe_ofdm_state *state = fe->demodulator_priv;
	TUNER_IOREG_DeviceMap_t *DeviceMap;
	IOARCH_Handle_t IOHandle;
	DeviceMap = &state->DeviceMap;
	IOHandle = state->IOHandle;
	ChipSetField_0367ter(DeviceMap, IOHandle, F367_STDBY, 1);
	ChipSetField_0367ter(DeviceMap, IOHandle, F367_STDBY_FEC, 1);
	ChipSetField_0367ter(DeviceMap, IOHandle, F367_STDBY_CORE, 1);
	return 0;
}

static int dvb_d0367_fe_ofdm_init(struct dvb_frontend *fe)
{
	struct dvb_d0367_fe_ofdm_state *state = fe->demodulator_priv;
	TUNER_IOREG_DeviceMap_t *DeviceMap;
	IOARCH_Handle_t IOHandle;
	DeviceMap = &state->DeviceMap;
	IOHandle = state->IOHandle;
	D0367ter_Init(DeviceMap, IOHandle, TUNER_TUNER_SHARP6465);
#if 0
	D0367ter_ScanFreq(&state->DeviceMap, state->IOHandle);
	{
		U32 Quality = 0;
		U32 Intensity = 0;
		U32 Ber = 0;
		FE_STV0367TER_GetSignalInfo(&state->DeviceMap, state->IOHandle,
									&Quality, &Intensity, &Ber);
		printk("Quality = %d, Intensity = %d, Ber = %d\n",
			   Quality, Intensity, Ber);
	}
	{
		BOOL bIsLocked;
		bIsLocked = FE_367ofdm_lock(&state->DeviceMap, state->IOHandle);
		printk("bIsLocked = %d\n", bIsLocked);
	}
#endif
	return 0;
}

static void dvb_d0367_fe_ofdm_release(struct dvb_frontend *fe)
{
	struct dvb_d0367_fe_ofdm_state *state = fe->demodulator_priv;
	TUNER_IOREG_DeviceMap_t *DeviceMap;
	IOARCH_Handle_t IOHandle;
	DeviceMap = &state->DeviceMap;
	IOHandle = state->IOHandle;
	if (DeviceMap->RegMap)
	{
		kfree(DeviceMap->RegMap);
	}
	kfree(state);
}

#if 1
static int
dvb_d0367_fe_ofdm_i2c_gate_ctrl(struct dvb_frontend *fe, int enable)
{
	struct dvb_d0367_fe_ofdm_state *state = fe->demodulator_priv;
	TUNER_IOREG_DeviceMap_t *DeviceMap;
	IOARCH_Handle_t IOHandle;
	DeviceMap = &state->DeviceMap;
	IOHandle = state->IOHandle;
	if (enable)
	{
		return ChipSetField_0367ter(DeviceMap, IOHandle, F367_I2CT_ON, 1);
	}
	else
	{
		return ChipSetField_0367ter(DeviceMap, IOHandle, F367_I2CT_ON, 0);
	}
	return 0;
}
#else

static int dvb_d0367_fe_ofdm_i2c_gate_ctrl(struct dvb_frontend *fe, int enable)
{
	U16 reg_index = 0xF001;
	U8 data;
	if (enable)
	{
		dvb_d0367_fe_ofdm_i2c_read(fe, reg_index, &data, 1);
		data |= 0xa2;
		dvb_d0367_fe_ofdm_i2c_write(fe, reg_index, &data, 1);
	}
	else
	{
		dvb_d0367_fe_ofdm_i2c_read(fe, reg_index, &data, 1);
		data &= 0x00;
		dvb_d0367_fe_ofdm_i2c_write(fe, reg_index, &data, 1);
	}
	return 0;
}
#endif

#if (DVB_API_VERSION < 5)
static int dvb_d0367_fe_ofdm_get_info(struct dvb_frontend *fe,
									  struct dvbfe_info *fe_info)
{
	//struct dvb_d0367_fe_ofdm_state* state = fe->demodulator_priv;
	/* get delivery system info */
	if (fe_info->delivery == DVBFE_DELSYS_DVBT)
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
static int dvb_d0367_fe_ofdm_get_property(struct dvb_frontend *fe, struct dtv_property *tvp)
{
	//struct dvb_d0367_fe_ofdm_state* state = fe->demodulator_priv;
	/* get delivery system info */
	if (tvp->cmd == DTV_DELIVERY_SYSTEM)
	{
		switch (tvp->u.data)
		{
			case SYS_DVBT:
			case SYS_DVBC_ANNEX_AC:
				break;
			default:
				return -EINVAL;
		}
	}
	return 0;
}
#endif

static struct dvb_frontend_ops dvb_d0367_fe_ofdm_ops =
{

	.info = {
		.name = "Tuner3-T(T/C)",
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
		FE_CAN_HIERARCHY_AUTO,
	},

	.release = dvb_d0367_fe_ofdm_release,

	.init = dvb_d0367_fe_ofdm_init,
	.sleep = dvb_d0367_fe_ofdm_sleep,

	.set_frontend = dvb_d0367_fe_ofdm_set_frontend,
	.get_frontend = dvb_d0367_fe_ofdm_get_frontend,

	.read_status = dvb_d0367_fe_ofdm_read_status,
	.read_ber = dvb_d0367_fe_ofdm_read_ber,
	.read_signal_strength = dvb_d0367_fe_ofdm_read_signal_strength,
	.read_snr = dvb_d0367_fe_ofdm_read_snr,
	.read_ucblocks = dvb_d0367_fe_ofdm_read_ucblocks,
	.i2c_gate_ctrl = dvb_d0367_fe_ofdm_i2c_gate_ctrl,
#if (DVB_API_VERSION < 5)
	.get_info = dvb_d0367_fe_ofdm_get_info,
#else
	.get_property = dvb_d0367_fe_ofdm_get_property,
#endif

};

void D0367ter_TunerSetFreq(TUNER_IOREG_DeviceMap_t *DeviceMap,
						   IOARCH_Handle_t IOHandle)

{
	struct dvb_d0367_fe_ofdm_state *state =
		(struct dvb_d0367_fe_ofdm_state *)DeviceMap->priv;
	struct dvb_frontend_parameters *p = state->p;
	struct dvb_frontend *fe = &state->frontend;
	if (fe->ops.tuner_ops.set_params)
	{
		fe->ops.tuner_ops.set_params(fe, p);
		if (fe->ops.i2c_gate_ctrl)
			fe->ops.i2c_gate_ctrl(fe, 0);
	}
}

FE_LLA_Error_t D0367ter_Algo(TUNER_IOREG_DeviceMap_t *DeviceMap,
							 IOARCH_Handle_t IOHandle,
							 FE_367ofdm_InternalParams_t *pParams,
							 FE_TER_SearchResult_t *pResult)
{
	U32 InternalFreq = 0, temp = 0 ;
	//t AgcIF=0;
	FE_LLA_Error_t error = FE_LLA_NO_ERROR;
	//OL TunerLocked;
	//TUNER_IOREG_DeviceMap_t *DeviceMap;
	//IOARCH_Handle_t IOHandle;
	//IOHandle = IOHandle;
	//DeviceMap = DeviceMap;
	if (pParams != NULL)
	{
		ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_CCS_ENABLE, 0);
		//printk("pIntParams->IF_IQ_Mode == %d\n",pParams->IF_IQ_Mode);
		switch (pParams->IF_IQ_Mode)
		{
			case FE_TER_NORMAL_IF_TUNER: /* Normal IF mode */
				ChipSetField_0367ter(DeviceMap, IOHandle, F367_TUNER_BB, 0);
				ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_LONGPATH_IF, 0);
				ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_DEMUX_SWAP, 0);
				break;
			case FE_TER_LONGPATH_IF_TUNER: /* Long IF mode */
				ChipSetField_0367ter(DeviceMap, IOHandle, F367_TUNER_BB, 0);
				ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_LONGPATH_IF, 1);
				ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_DEMUX_SWAP, 1);
				break;
			case FE_TER_IQ_TUNER: /* IQ mode */
				ChipSetField_0367ter(DeviceMap, IOHandle, F367_TUNER_BB, 1);
				ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_PPM_INVSEL, 0);
				/*spectrum inversion hw detection off *db*/
				break;
			default:
				return FE_LLA_SEARCH_FAILED;
		}
		//printk("pIntParams->Inv == %d\n",pParams->Inv);
		//printk("pIntParams->Sense == %d\n",pParams->Sense);
		if ((pParams->Inv == FE_TER_INVERSION_NONE))
		{
			if (pParams->IF_IQ_Mode == FE_TER_IQ_TUNER)
			{
				ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_IQ_INVERT, 0);
				pParams->SpectrumInversion = FALSE;
			}
			else
			{
				ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_INV_SPECTR, 0);
				pParams->SpectrumInversion = FALSE;
			}
		}
		else if (pParams->Inv == FE_TER_INVERSION)
		{
			if (pParams->IF_IQ_Mode == FE_TER_IQ_TUNER)
			{
				ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_IQ_INVERT, 1);
				pParams->SpectrumInversion = TRUE;
			}
			else
			{
				ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_INV_SPECTR, 1);
				pParams->SpectrumInversion = TRUE;
			}
		}
		else if ((pParams->Inv == FE_TER_INVERSION_AUTO) || ((pParams->Inv == FE_TER_INVERSION_UNK)/*&& (!pParams->first_lock)*/))
		{
			if (pParams->IF_IQ_Mode == FE_TER_IQ_TUNER)
			{
				if (pParams->Sense == 1)
				{
					ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_IQ_INVERT, 1);
					pParams->SpectrumInversion = TRUE;
				}
				else
				{
					ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_IQ_INVERT, 0);
					pParams->SpectrumInversion = FALSE;
				}
			}
			else
			{
				if (pParams->Sense == 1)
				{
					ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_INV_SPECTR, 1);
					pParams->SpectrumInversion = TRUE;
				}
				else
				{
					ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_INV_SPECTR, 0);
					pParams->SpectrumInversion = FALSE;
				}
			}
		}
		// printk("pIntParams->PreviousChannelBW == %d\n",pParams->PreviousChannelBW);
		// printk("pIntParams->Crystal_Hz == %d\n",pParams->Crystal_Hz);
		if ((pParams->IF_IQ_Mode != FE_TER_NORMAL_IF_TUNER) &&
				(pParams->PreviousChannelBW != pParams->ChannelBW))
		{
			FE_367TER_AGC_IIR_LOCK_DETECTOR_SET(DeviceMap, IOHandle);
			/*set fine agc target to 180 for LPIF or IQ mode*/
			/* set Q_AGCTarget */
			ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_SEL_IQNTAR, 1);
			ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_AUT_AGC_TARGET_MSB, 0xB);
			/*ChipSetField_0367ter(DeviceMap, IOHandle,AUT_AGC_TARGET_LSB,0x04); */
			/* set Q_AGCTarget */
			ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_SEL_IQNTAR, 0);
			ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_AUT_AGC_TARGET_MSB, 0xB);
			/*ChipSetField_0367ter(DeviceMap, IOHandle,AUT_AGC_TARGET_LSB,0x04); */
			if (!FE_367TER_IIR_FILTER_INIT(DeviceMap, IOHandle, pParams->ChannelBW, pParams->Crystal_Hz))
				return FE_LLA_BAD_PARAMETER;
			/*set IIR filter once for 6,7 or 8MHz BW*/
			pParams->PreviousChannelBW = pParams->ChannelBW;
			FE_367TER_AGC_IIR_RESET(DeviceMap, IOHandle);
		}
		//printk("pIntParams->Hierarchy == %d\n",pParams->Hierarchy);
		/*********Code Added For Hierarchical Modulation****************/
		if (pParams->Hierarchy == FE_TER_HIER_LOW_PRIO)
		{
			ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_BDI_LPSEL, 0x01);
		}
		else
		{
			ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_BDI_LPSEL, 0x00);
		}
		/*************************/
		InternalFreq = FE_367ofdm_GetMclkFreq(DeviceMap, IOHandle, pParams->Crystal_Hz) / 1000;
		//printk("InternalFreq = %d\n", InternalFreq);
		temp = (int)((((pParams->ChannelBW * 64 * PowOf2(15) * 100) / (InternalFreq)) * 10) / 7);
		//printk("pParams->ChannelBW = %d\n",pParams->ChannelBW);
		ChipSetFieldImage_0367ter(DeviceMap, IOHandle, F367ofdm_TRL_NOMRATE_LSB, temp % 2);
		temp = temp / 2;
		ChipSetFieldImage_0367ter(DeviceMap, IOHandle, F367ofdm_TRL_NOMRATE_HI, temp / 256);
		ChipSetFieldImage_0367ter(DeviceMap, IOHandle, F367ofdm_TRL_NOMRATE_LO, temp % 256);
		//ChipSetRegisters_0367ter(DeviceMap, IOHandle,R367ofdm_TRL_NOMRATE1,2);//lwj
		ChipSetRegisters_0367ter(DeviceMap, IOHandle, R367ofdm_TRL_NOMRATE1, 1);
		ChipSetRegisters_0367ter(DeviceMap, IOHandle, R367ofdm_TRL_NOMRATE2, 1);
		//ChipGetRegisters_0367ter(DeviceMap, IOHandle,R367ofdm_TRL_CTL,3);//lwj
		ChipGetRegisters_0367ter(DeviceMap, IOHandle, R367ofdm_TRL_CTL, 1);
		ChipGetRegisters_0367ter(DeviceMap, IOHandle, R367ofdm_TRL_NOMRATE1, 1);
		ChipGetRegisters_0367ter(DeviceMap, IOHandle, R367ofdm_TRL_NOMRATE2, 1);
		temp = ChipGetFieldImage_0367ter(DeviceMap, IOHandle, F367ofdm_TRL_NOMRATE_HI) * 512 +
			   ChipGetFieldImage_0367ter(DeviceMap, IOHandle, F367ofdm_TRL_NOMRATE_LO) * 2 +
			   ChipGetFieldImage_0367ter(DeviceMap, IOHandle, F367ofdm_TRL_NOMRATE_LSB);
		temp = (int)((PowOf2(17) * pParams->ChannelBW * 1000) / (7 * (InternalFreq)));
		ChipSetFieldImage_0367ter(DeviceMap, IOHandle, F367ofdm_GAIN_SRC_HI, temp / 256);
		ChipSetFieldImage_0367ter(DeviceMap, IOHandle, F367ofdm_GAIN_SRC_LO, temp % 256);
		//ChipSetRegisters_0367ter(DeviceMap, IOHandle,R367ofdm_GAIN_SRC1,2);
		ChipSetRegisters_0367ter(DeviceMap, IOHandle, R367ofdm_GAIN_SRC1, 1);
		ChipSetRegisters_0367ter(DeviceMap, IOHandle, R367ofdm_GAIN_SRC2, 1);
		//ChipGetRegisters_0367ter(DeviceMap, IOHandle,R367ofdm_GAIN_SRC1,2);
		ChipGetRegisters_0367ter(DeviceMap, IOHandle, R367ofdm_GAIN_SRC1, 1);
		ChipGetRegisters_0367ter(DeviceMap, IOHandle, R367ofdm_GAIN_SRC2, 1);
		temp = ChipGetFieldImage_0367ter(DeviceMap, IOHandle, F367ofdm_GAIN_SRC_HI) * 256 +
			   ChipGetFieldImage_0367ter(DeviceMap, IOHandle, F367ofdm_GAIN_SRC_LO);
		//if ( Inst->DriverParam.Ter.TunerType == TUNER_TUNER_SHARP6465)
		{
			temp = (int)((InternalFreq - 36166667 / 1000) * PowOf2(16) / (InternalFreq)); //IF freq
		}
		//else if ( Inst->DriverParam.Ter.TunerType == TUNER_TUNER_STV4100)
		//{
		// temp= (int) ((InternalFreq-0 )*PowOf2(16)/(InternalFreq) );
		//}
		ChipSetFieldImage_0367ter(DeviceMap, IOHandle, F367ofdm_INC_DEROT_HI, temp / 256);
		ChipSetFieldImage_0367ter(DeviceMap, IOHandle, F367ofdm_INC_DEROT_LO, temp % 256);
		//ChipSetRegisters_0367ter(DeviceMap, IOHandle,R367ofdm_INC_DEROT1,2);//lwj change
		ChipSetRegisters_0367ter(DeviceMap, IOHandle, R367ofdm_INC_DEROT1, 1);
		ChipSetRegisters_0367ter(DeviceMap, IOHandle, R367ofdm_INC_DEROT2, 1);
		//pParams->EchoPos = pSearch->EchoPos;
		ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_LONG_ECHO, pParams->EchoPos);
		D0367ter_TunerSetFreq(DeviceMap, IOHandle);
		/*********************************/
		if (FE_367ofdm_LockAlgo(DeviceMap, IOHandle, pParams) == FE_TER_LOCKOK)
		{
			pResult->Locked = TRUE;
			pResult->SignalStatus = FE_TER_LOCKOK;
		} /* if(FE_367_Algo(pParams) == FE_TER_LOCKOK) */
		else
		{
			pResult->Locked = FALSE;
			error = FE_LLA_SEARCH_FAILED;
		}
	} /*if((void *)Handle != NULL)*/
	else
	{
		error = FE_LLA_BAD_PARAMETER;
	}
	return error;
}

U32 D0367ter_GeFrequency(TUNER_IOREG_DeviceMap_t *DeviceMap,
						 IOARCH_Handle_t IOHandle)
{
	U32 Frequency = 722000;
	struct dvb_d0367_fe_ofdm_state *state = NULL;
	struct dvb_frontend_parameters *p = NULL;
	IOHandle = IOHandle;
	state = (struct dvb_d0367_fe_ofdm_state *)DeviceMap->priv;
	if (!state)
	{
		return Frequency;
	}
	p = state->p;
	if (!state)
	{
		return Frequency;
	}
	Frequency = p->frequency / 1000;
	return Frequency;
}

U32 D0367ter_GeChannelBW(TUNER_IOREG_DeviceMap_t *DeviceMap,
						 IOARCH_Handle_t IOHandle)
{
	U32 ChannelBW = 8;
	struct dvb_d0367_fe_ofdm_state *state = NULL;
	struct dvb_frontend_parameters *p = NULL;
	IOHandle = IOHandle;
	state = (struct dvb_d0367_fe_ofdm_state *)DeviceMap->priv;
	if (!state)
	{
		return ChannelBW;
	}
	p = state->p;
	if (!state)
	{
		return ChannelBW;
	}
	switch (p->u.ofdm.bandwidth)
	{
		case BANDWIDTH_8_MHZ:
			ChannelBW = 8;
			break;
		case BANDWIDTH_7_MHZ:
			ChannelBW = 7;
			break;
		case BANDWIDTH_6_MHZ:
			ChannelBW = 6;
			break;
		default:
			ChannelBW = 8;
			break;
	}
	return ChannelBW;
}

YW_ErrorType_T D0367ter_ScanFreq(TUNER_IOREG_DeviceMap_t *DeviceMap,
								 IOARCH_Handle_t IOHandle)
{
	YW_ErrorType_T Error = YW_NO_ERROR;
	/*U8 trials[2]; */
	S8 num_trials = 1;
	U8 index;
	FE_LLA_Error_t error = FE_LLA_NO_ERROR;
	U8 flag_spec_inv;
	U8 flag;
	U8 SenseTrials[2];
	U8 SenseTrialsAuto[2];
	FE_367ofdm_InternalParams_t pParams;
	FE_TER_SearchResult_t pResult;
	//question pParams->Inv
	YWLIB_Memset(&pParams, 0, sizeof(FE_367ofdm_InternalParams_t));
	YWLIB_Memset(&pResult, 0, sizeof(FE_TER_SearchResult_t));
#if 0 //question
	SenseTrials[0] = INV;
	SenseTrials[1] = NINV;
	SenseTrialsAuto[0] = INV;
	SenseTrialsAuto[1] = NINV;
#else
	SenseTrials[1] = INV;
	SenseTrials[0] = NINV;
	SenseTrialsAuto[1] = INV;
	SenseTrialsAuto[0] = NINV;
#endif
	// printk("demod_d0367ter_ScanFreq #########Index = %d\n", Index);
	pParams.Frequency = D0367ter_GeFrequency(DeviceMap, IOHandle);
	pParams.ChannelBW = D0367ter_GeChannelBW(DeviceMap, IOHandle);
	_DEBUG
	{
		printk("pParams.Frequency = %d\n", pParams.Frequency);
		printk("pParams.ChannelBW = %d\n", pParams.ChannelBW);
	}
	pParams.Crystal_Hz = DeviceMap->RegExtClk;
	pParams.IF_IQ_Mode = FE_TER_NORMAL_IF_TUNER;//FE_TER_IQ_TUNER; //most tuner is IF mode, stv4100 is I/Q mode
	pParams.Inv = FE_TER_INVERSION_AUTO; //FE_TER_INVERSION_AUTO
	pParams.Hierarchy = FE_TER_HIER_NONE;
	pParams.EchoPos = 0;
	pParams.first_lock = 0;
	//pLook.Frequency = pSearch->Frequency;
	//pLook.Mode = pSearch->Mode;
	//pLook.Guard = pSearch->Guard;
	//pLook.Force = pSearch->Force;
	//pLook.ChannelBW = pSearch->ChannelBW;
	//pLook.EchoPos = pSearch->EchoPos;
	//pLook.IF_IQ_Mode= pSearch->IF_IQ_Mode;
	// pParams->Inv = pSearch->Inv;
	//pLook.Hierarchy=pParams->Hierarchy = pSearch->Hierarchy; /*added for hierarchical*/
	/*printk(" in FE_367TER_Search () value of pParams.Hierarchy %d\n",pParams.Hierarchy );*/
	flag_spec_inv = 0;
	flag = ((pParams.Inv >> 1) & 1);
	switch (flag) /* sw spectrum inversion for LP_IF &IQ &normal IF *db* */
	{
		case 0: /*INV+ INV_NONE*/
			if ((pParams.Inv == FE_TER_INVERSION_NONE) || (pParams.Inv == FE_TER_INVERSION))
			{
				num_trials = 1;
			}
			else /*UNK */
			{
				num_trials = 2;
			}
			break;
		case 1:/*AUTO*/
			num_trials = 2;
			if ((pParams.first_lock) && (pParams.Inv == FE_TER_INVERSION_AUTO))
			{
				num_trials = 1;
			}
			break;
		default:
			return FE_TER_NOLOCK;
			break;
	}
	pResult.SignalStatus = FE_TER_NOLOCK;
	index = 0;
	while (((index) < num_trials) && (pResult.SignalStatus != FE_TER_LOCKOK))
	{
		if (!pParams.first_lock)
		{
			if (pParams.Inv == FE_TER_INVERSION_UNK)
			{
				pParams.Sense = SenseTrials[index];
			}
			if (pParams.Inv == FE_TER_INVERSION_AUTO)
			{
				pParams.Sense = SenseTrialsAuto[index];
			}
		}
		error = D0367ter_Algo(DeviceMap, IOHandle, &pParams, &pResult);
		if ((pResult.SignalStatus == FE_TER_LOCKOK) &&
				(pParams.Inv == FE_TER_INVERSION_AUTO) && (index == 1))
		{
			SenseTrialsAuto[index] = SenseTrialsAuto[0]; /* invert spectrum sense */
			SenseTrialsAuto[(index + 1) % 2] = (SenseTrialsAuto[1] + 1) % 2;
		}
		index++;
	}
	if (!Error)
	{
		//Inst->DriverParam.Ter.Result = Inst->DriverParam.Ter.Param;
		if (pResult.Locked)
		{
			//printk("TUNER_STATUS_LOCKED #######################\n");
			//Inst->Status = TUNER_STATUS_LOCKED;
		}
		else
		{
			//printk("TUNER_STATUS_UNLOCKED #######################\n");
			//Inst->Status = TUNER_STATUS_UNLOCKED;
		}
	}
	else
	{
		//printk("TUNER_STATUS_UNLOCKED Error#######################\n");
		//Inst->Status = TUNER_STATUS_UNLOCKED;
	}
	return (Error);
}

struct dvb_frontend *dvb_d0367_fe_ofdm_attach(struct i2c_adapter *i2c)
{
	struct dvb_d0367_fe_ofdm_state *state = NULL;
	/* allocate memory for the internal state */
	state = kzalloc(sizeof(struct dvb_d0367_fe_ofdm_state), GFP_KERNEL);
	if (state == NULL) goto error;
	/* create dvb_frontend */
	memcpy(&state->frontend.ops, &dvb_d0367_fe_ofdm_ops, sizeof(struct dvb_frontend_ops));
	state->frontend.demodulator_priv = state;
	state->i2c = i2c;
	state->IOHandle = (IOARCH_Handle_t)i2c;
	state->DeviceMap.Timeout = IOREG_DEFAULT_TIMEOUT;
	state->DeviceMap.Registers = STV0367ofdm_NBREGS;
	state->DeviceMap.Fields = STV0367ofdm_NBFIELDS;
	state->DeviceMap.Mode = IOREG_MODE_SUBADR_16;
	state->DeviceMap.RegExtClk = 27000000; //Demod External Crystal_HZ
	state->DeviceMap.RegMap = (TUNER_IOREG_Register_t *)
							  kzalloc(state->DeviceMap.Registers * sizeof(TUNER_IOREG_Register_t),
									  GFP_KERNEL);
	state->DeviceMap.priv = (void *)state;
	return &state->frontend;
error:
	kfree(state);
	return NULL;
}

MODULE_DESCRIPTION("DVB d0367 Frontend");
MODULE_AUTHOR("oSaiYa");
MODULE_LICENSE("GPL");

EXPORT_SYMBOL(dvb_d0367_fe_ofdm_attach);

