
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

#include "stv0367qam_init.h"
#include "chip_0367qam.h"
#include "stv0367qam_drv.h"
#include "d0367_qam.h"

#include "dvb_frontend.h"
#include "D0367.h"

extern int debug_fe7162;
#define _DEBUG if (debug_fe7162)

struct dvb_d0367_fe_qam_state
{
	struct i2c_adapter *i2c;
	struct dvb_frontend frontend;
	IOARCH_Handle_t IOHandle;
	TUNER_IOREG_DeviceMap_t DeviceMap;
	struct dvb_frontend_parameters *p;
};

YW_ErrorType_T D0367qam_ScanFreq(TUNER_IOREG_DeviceMap_t *DemodDeviceMap,
								 IOARCH_Handle_t DemodIOHandle);

static int dvb_d0367_fe_qam_read_ber(struct dvb_frontend *fe, u32 *ber)
{
	struct dvb_d0367_fe_qam_state *state = fe->demodulator_priv;
	TUNER_IOREG_DeviceMap_t *DeviceMap;
	IOARCH_Handle_t IOHandle;
	DeviceMap = &state->DeviceMap;
	IOHandle = state->IOHandle;
	*ber = FE_STV0367qam_GetErrors(DeviceMap, IOHandle);
	return 0;
}

static int dvb_d0367_fe_qam_read_signal_strength(struct dvb_frontend *fe,
												 u16 *strength)
{
	struct dvb_d0367_fe_qam_state *state = fe->demodulator_priv;
	TUNER_IOREG_DeviceMap_t *DeviceMap;
	IOARCH_Handle_t IOHandle;
	DeviceMap = &state->DeviceMap;
	IOHandle = state->IOHandle;
	*strength = FE_STV0367qam_GetPower(DeviceMap, IOHandle);
	return 0;
}

static int dvb_d0367_fe_qam_read_snr(struct dvb_frontend *fe, u16 *snr)
{
	struct dvb_d0367_fe_qam_state *state = fe->demodulator_priv;
	TUNER_IOREG_DeviceMap_t *DeviceMap;
	IOARCH_Handle_t IOHandle;
	DeviceMap = &state->DeviceMap;
	IOHandle = state->IOHandle;
	*snr = FE_STV0367qam_GetSnr(DeviceMap, IOHandle);
	return 0;
}

static int dvb_d0367_fe_qam_read_ucblocks(struct dvb_frontend *fe,
										  u32 *ucblocks)
{
	*ucblocks = 0;
	return 0;
}

static int dvb_d0367_fe_qam_sleep(struct dvb_frontend *fe)
{
	struct dvb_d0367_fe_qam_state *state = fe->demodulator_priv;
	TUNER_IOREG_DeviceMap_t *DeviceMap;
	IOARCH_Handle_t IOHandle;
	DeviceMap = &state->DeviceMap;
	IOHandle = state->IOHandle;
	D0367qam_Sleep(DeviceMap, IOHandle);
	return 0;
}

static int dvb_d0367_fe_qam_get_frontend(struct dvb_frontend *fe,
										 struct dvb_frontend_parameters *p)
{
	return 0;
}

static int dvb_d0367_fe_qam_set_frontend(struct dvb_frontend *fe,
										 struct dvb_frontend_parameters *p)
{
	struct dvb_d0367_fe_qam_state *state = fe->demodulator_priv;
	TUNER_IOREG_DeviceMap_t *DeviceMap;
	IOARCH_Handle_t IOHandle;
	DeviceMap = &state->DeviceMap;
	IOHandle = state->IOHandle;
	state->p = p;
	D0367qam_ScanFreq(DeviceMap, IOHandle);
#if 0
	BOOL bIsLocked;
	bIsLocked = FE_367qam_Status(&state->DeviceMap, state->IOHandle);
	printk("%d:bIsLocked = %d\n", __LINE__, bIsLocked);
#endif
	state->p = NULL;
	return 0;
}

static int dvb_d0367_fe_qam_init(struct dvb_frontend *fe)
{
	struct dvb_d0367_fe_qam_state *state = fe->demodulator_priv;
	TUNER_IOREG_DeviceMap_t *DeviceMap;
	IOARCH_Handle_t IOHandle;
	DeviceMap = &state->DeviceMap;
	IOHandle = state->IOHandle;
	D0367qam_Init(DeviceMap, IOHandle, TUNER_TUNER_SHARP5469C);
	return 0;
}

static int dvb_d0367_fe_qam_read_status(struct dvb_frontend *fe,
										fe_status_t *status)
{
	struct dvb_d0367_fe_qam_state *state = fe->demodulator_priv;
	//int iTunerLock = 0;
	BOOL bIsLocked;
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
		bIsLocked = FE_367qam_Status(&state->DeviceMap, state->IOHandle);
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
#if 1
	{
		U32 Quality;
		U32 Intensity;
		U32 Ber;
		FE_STV0367qam_GetSignalInfo(&state->DeviceMap, state->IOHandle,
									&Quality, &Intensity, &Ber, FirstTimeBER);
		_DEBUG
		printk("Quality = %d, Intensity = %d, Ber = %d\n",
			   Quality, Intensity, Ber);
	}
#endif /* 0 */
	return 0;
#if 0
exit:
	printk(KERN_ERR "%s: dvb_d0367_fe_qam_read_status Error\n", __func__);
	return -1;
#endif /* 0 */
}

static void dvb_d0367_fe_qam_release(struct dvb_frontend *fe)
{
	struct dvb_d0367_fe_qam_state *state = fe->demodulator_priv;
	if (state->DeviceMap.RegMap)
	{
		kfree(state->DeviceMap.RegMap);
	}
	kfree(state);
}

static int dvb_d0367_fe_qam_i2c_gate_ctrl(struct dvb_frontend *fe, int enable)
{
	struct dvb_d0367_fe_qam_state *state = fe->demodulator_priv;
	TUNER_IOREG_DeviceMap_t *DeviceMap;
	IOARCH_Handle_t IOHandle;
	DeviceMap = &state->DeviceMap;
	IOHandle = state->IOHandle;
	if (enable)
	{
		return D0367qam_I2ctOn(DeviceMap, IOHandle);
	}
	else
	{
		return D0367qam_I2ctOff(DeviceMap, IOHandle);
	}
	return 0;
}

#if (DVB_API_VERSION < 5)
static int dvb_d0367_fe_qam_get_info(struct dvb_frontend *fe,
									 struct dvbfe_info *fe_info)
{
	//struct dvb_d0367_fe_qam_state* state = fe->demodulator_priv;
	/* get delivery system info */
	if (fe_info->delivery == DVBFE_DELSYS_DVBC)
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
static int dvb_d0367_fe_qam_get_property(struct dvb_frontend *fe, struct dtv_property *tvp)
{
	//struct dvb_d0367_fe_qam_state* state = fe->demodulator_priv;
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
#endif

static void dvb_d0367_fe_qam_tuner_is_lock(struct dvb_frontend *fe,
										   u32 *status)
{
	if (fe->ops.tuner_ops.get_status)
	{
		if (fe->ops.tuner_ops.get_status(fe, status) < 0)
		{
			printk("1. Tuner get_status err\n");
		}
	}
}

static void D0367qam_TunerIsLock(TUNER_IOREG_DeviceMap_t *DeviceMap,
								 u32 *status)
{
	struct dvb_d0367_fe_qam_state *state =
		(struct dvb_d0367_fe_qam_state *)DeviceMap->priv;
	struct dvb_frontend *fe = &state->frontend;
	dvb_d0367_fe_qam_tuner_is_lock(fe, status);
}

U32 D0367qam_GeFrequencyKhz(TUNER_IOREG_DeviceMap_t *DeviceMap,
							IOARCH_Handle_t IOHandle)
{
	U32 Frequency = 474000;
	struct dvb_d0367_fe_qam_state *state = NULL;
	struct dvb_frontend_parameters *p = NULL;
	IOHandle = IOHandle;
	state = (struct dvb_d0367_fe_qam_state *)DeviceMap->priv;
	if (!state)
	{
		return Frequency;
	}
	p = state->p;
	Frequency = p->frequency / 1000;
	return Frequency;
}

U32 D0367qam_GeSymbolRate(TUNER_IOREG_DeviceMap_t *DeviceMap,
						  IOARCH_Handle_t IOHandle)
{
	U32 SymbolRate = 6875000;
	struct dvb_d0367_fe_qam_state *state = NULL;
	struct dvb_frontend_parameters *p = NULL;
	IOHandle = IOHandle;
	state = (struct dvb_d0367_fe_qam_state *)DeviceMap->priv;
	if (!state)
	{
		return SymbolRate;
	}
	p = state->p;
	_DEBUG
	printk("p->u.qam.symbol_rate = %d\n", p->u.qam.symbol_rate);
	SymbolRate = p->u.qam.symbol_rate;
	return SymbolRate;
}

U32 D0367qam_GeModulation(TUNER_IOREG_DeviceMap_t *DeviceMap,
						  IOARCH_Handle_t IOHandle)
{
	U32 Modulation = FE_CAB_MOD_QAM64;
	struct dvb_d0367_fe_qam_state *state = NULL;
	struct dvb_frontend_parameters *p = NULL;
	IOHandle = IOHandle;
	state = (struct dvb_d0367_fe_qam_state *)DeviceMap->priv;
	if (!state)
	{
		return Modulation;
	}
	p = state->p;
	switch (p->u.qam.modulation)
	{
		case QAM_16:
			Modulation = FE_CAB_MOD_QAM16;
			break;
		case QAM_32:
			Modulation = FE_CAB_MOD_QAM32;
			break;
		case QAM_64:
			Modulation = FE_CAB_MOD_QAM64;
			break;
		case QAM_128:
			Modulation = FE_CAB_MOD_QAM128;
			break;
		case QAM_256:
			Modulation = FE_CAB_MOD_QAM256;
			break;
		default:
			Modulation = FE_CAB_MOD_QAM64;
			break;
	}
	return Modulation;
}

void D0367qam_TunerSetFreq(TUNER_IOREG_DeviceMap_t *DeviceMap,
						   IOARCH_Handle_t IOHandle)
{
	struct dvb_d0367_fe_qam_state *state =
		(struct dvb_d0367_fe_qam_state *)DeviceMap->priv;
	struct dvb_frontend_parameters *p = state->p;
	struct dvb_frontend *fe = &state->frontend;
	IOHandle = IOHandle;
	if (fe->ops.tuner_ops.set_params)
	{
		fe->ops.tuner_ops.set_params(fe, p);
		if (fe->ops.i2c_gate_ctrl)
			fe->ops.i2c_gate_ctrl(fe, 0);
	}
}

static struct dvb_frontend_ops dvb_d0367_fe_qam_ops =
{

	.info = {
		.name = "Tuner3-C(T/C)",
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

	.release = dvb_d0367_fe_qam_release,
	.init = dvb_d0367_fe_qam_init,
	.get_frontend = dvb_d0367_fe_qam_get_frontend,
	.set_frontend = dvb_d0367_fe_qam_set_frontend,
	.sleep = dvb_d0367_fe_qam_sleep,
	.read_ber = dvb_d0367_fe_qam_read_ber,
	.read_signal_strength = dvb_d0367_fe_qam_read_signal_strength,
	.read_snr = dvb_d0367_fe_qam_read_snr,
	.read_ucblocks = dvb_d0367_fe_qam_read_ucblocks,
	.i2c_gate_ctrl = dvb_d0367_fe_qam_i2c_gate_ctrl,
	.read_status = dvb_d0367_fe_qam_read_status,
#if (DVB_API_VERSION < 5)
	.get_info = dvb_d0367_fe_qam_get_info,
#else
	.get_property = dvb_d0367_fe_qam_get_property,
#endif

};

/*****************************************************
--FUNCTION :: FE_367qam_Algo
--ACTION :: Driver functio of the STV0367QAM chip
--PARAMS IN :: *pIntParams ==> Demod handle
-- Tuner handle
 Search frequency
 Symbolrate
 QAM Size
 DerotOffset
 SweepRate
 Search Range
 Master clock
 ADC clock
 Structure for result storage
--PARAMS OUT:: NONE
--RETURN :: Handle to STV0367QAM
--***************************************************/
static FE_367qam_SIGNALTYPE_t
D0367qam_Algo(TUNER_IOREG_DeviceMap_t *DemodDeviceMap,
			  IOARCH_Handle_t DemodIOHandle,
			  FE_367qam_InternalParams_t *pIntParams)
{
	FE_367qam_SIGNALTYPE_t signalType = FE_367qam_NOAGC; /* Signal search statusinitialization */
	U32 QAMFEC_Lock, QAM_Lock, u32_tmp ;
	BOOL TunerLock = FALSE;
	U32 LockTime, TRLTimeOut, AGCTimeOut, CRLSymbols, CRLTimeOut, EQLTimeOut, DemodTimeOut, FECTimeOut;
	U8 TrackAGCAccum;
	TUNER_IOREG_DeviceMap_t *DeviceMap = DemodDeviceMap;
	IOARCH_Handle_t IOHandle = DemodIOHandle;
	U32 FreqResult = 0;
	/* Timeouts calculation */
	/* A max lock time of 25 ms is allowed for delayed AGC */
	AGCTimeOut = 25;
	/* 100000 symbols needed by the TRL as a maximum value */
	TRLTimeOut = 100000000 / pIntParams->SymbolRate_Bds;
	/* CRLSymbols is the needed number of symbols to achieve a lock within [-4%,+4%] of the symbol rate.
	 CRL timeout is calculated for a lock within [-SearchRange_Hz, +SearchRange_Hz].
	 EQL timeout can be changed depending on the micro-reflections we want tohandle.
	 A characterization must be performed with these echoes to get new timeoutvalues.
	*/
	switch (pIntParams->Modulation)
	{
		case FE_CAB_MOD_QAM16:
			CRLSymbols = 150000;
			EQLTimeOut = 100;
			break;
		case FE_CAB_MOD_QAM32:
			CRLSymbols = 250000;
			EQLTimeOut = 100;
			break;
		case FE_CAB_MOD_QAM64:
			CRLSymbols = 200000;
			EQLTimeOut = 100;
			break;
		case FE_CAB_MOD_QAM128:
			CRLSymbols = 250000;
			EQLTimeOut = 100;
			break;
		case FE_CAB_MOD_QAM256:
			CRLSymbols = 250000;
			EQLTimeOut = 100;
			break;
		default:
			CRLSymbols = 200000;
			EQLTimeOut = 100;
			break;
	}
	if (pIntParams->SearchRange_Hz < 0)
	{
		CRLTimeOut = (25 * CRLSymbols * (-pIntParams->SearchRange_Hz / 1000)) / (pIntParams->SymbolRate_Bds / 1000);
	}
	else
	{
		CRLTimeOut = (25 * CRLSymbols * (pIntParams->SearchRange_Hz / 1000)) / (pIntParams->SymbolRate_Bds / 1000);
	}
	CRLTimeOut = (1000 * CRLTimeOut) / pIntParams->SymbolRate_Bds;
	/* Timeouts below 50ms are coerced */
	if (CRLTimeOut < 50) CRLTimeOut = 50;
	/* A maximum of 100 TS packets is needed to get FEC lock even in case thespectrum inversion needs to be changed.
	 This is equal to 20 ms in case of the lowest symbol rate of 0.87Msps
	*/
	FECTimeOut = 20;
	DemodTimeOut = AGCTimeOut + TRLTimeOut + CRLTimeOut + EQLTimeOut;
	/* Reset the TRL to ensure nothing starts until the
	 AGC is stable which ensures a better lock time
	*/
	ChipSetOneRegister_0367qam(DeviceMap, IOHandle, R367qam_CTRL_1, 0x04);
	/* Set AGC accumulation time to minimum and lock threshold to maximum inorder to speed up the AGC lock */
	TrackAGCAccum = ChipGetField_0367qam(DeviceMap, IOHandle, F367qam_AGC_ACCUMRSTSEL);
	ChipSetField_0367qam(DeviceMap, IOHandle, F367qam_AGC_ACCUMRSTSEL, 0x0);
	/* Modulus Mapper is disabled */
	ChipSetField_0367qam(DeviceMap, IOHandle, F367qam_MODULUSMAP_EN, 0);
	/* Disable the sweep function */
	ChipSetField_0367qam(DeviceMap, IOHandle, F367qam_SWEEP_EN, 0);
	/* The sweep function is never used, Sweep rate must be set to 0 */
	/* Set the derotator frequency in Hz */
	//FE_367qam_SetDerotFreq(DeviceMap,IOHandle,pIntParams->AdcClock_Hz,(1000*(S32)FE_TunerGetIF_Freq(pIntParams->hTuner)+pIntParams->DerotOffset_Hz));
	FE_367qam_SetDerotFreq(DeviceMap, IOHandle, pIntParams->AdcClock_Hz, (1000 * (36125000 / 1000) + pIntParams->DerotOffset_Hz)); //question if freq
	/* Disable the Allpass Filter when the symbol rate is out of range */
	if ((pIntParams->SymbolRate_Bds > 10800000) || (pIntParams->SymbolRate_Bds < 1800000))
	{
		ChipSetField_0367qam(DeviceMap, IOHandle, F367qam_ADJ_EN, 0);
		ChipSetField_0367qam(DeviceMap, IOHandle, F367qam_ALLPASSFILT_EN, 0);
	}
#if 0
	/* Check if the tuner is locked */
	if (Inst->DriverParam.Cab.TunerDriver.tuner_IsLocked != NULL)
	{
		Inst->DriverParam.Cab.TunerDriver.tuner_IsLocked(Handle, &TunerLock);
	}
#else
	D0367qam_TunerIsLock(DeviceMap, &TunerLock);
#endif /* 0 */
	if (TunerLock == 0)
	{
		return FE_367qam_NOTUNER;
	}
	/* Relase the TRL to start demodulator acquisition */
	/* Wait for QAM lock */
	LockTime = 0;
	ChipSetOneRegister_0367qam(DeviceMap, IOHandle, R367qam_CTRL_1, 0x00);
	do
	{
		QAM_Lock = ChipGetField_0367qam(DeviceMap, IOHandle, F367qam_FSM_STATUS);
		if ((LockTime >= (DemodTimeOut - EQLTimeOut)) && (QAM_Lock == 0x04))
			/* We don't wait longer, the frequency/phase offset must be too big */
			LockTime = DemodTimeOut;
		else if ((LockTime >= (AGCTimeOut + TRLTimeOut)) && (QAM_Lock == 0x02))
			/* We don't wait longer, either there is no signal or it is not the rightsymbol rate or it is an analog carrier */
		{
			LockTime = DemodTimeOut;
			ChipGetRegisters_0367qam(DeviceMap, IOHandle, R367qam_AGC_PWR_RD_L, 3);
			u32_tmp = ChipGetFieldImage_0367qam(DeviceMap, IOHandle, F367qam_AGC_PWR_WORD_LO)
					  + (ChipGetFieldImage_0367qam(DeviceMap, IOHandle, F367qam_AGC_PWR_WORD_ME) << 8)
					  + (ChipGetFieldImage_0367qam(DeviceMap, IOHandle, F367qam_AGC_PWR_WORD_HI) << 16);
			if (u32_tmp >= 131072)
				u32_tmp = 262144 - u32_tmp;
			u32_tmp = u32_tmp / (PowOf2(11 - ChipGetField_0367qam(DeviceMap, IOHandle, F367qam_AGC_IF_BWSEL)));
			if (u32_tmp < ChipGetField_0367qam(DeviceMap, IOHandle, F367qam_AGC_PWRREF_LO) + 256 * ChipGetField_0367qam(DeviceMap, IOHandle, F367qam_AGC_PWRREF_HI) - 10)
				QAM_Lock = 0x0f;
		}
		else
		{
			ChipWaitOrAbort_0367qam(FALSE, 10); /* wait 10 ms */
			LockTime += 10;
		}
	}
	while (((QAM_Lock != 0x0c) && (QAM_Lock != 0x0b)) && (LockTime < DemodTimeOut));
	if ((QAM_Lock == 0x0c) || (QAM_Lock == 0x0b))
	{
		/* Wait for FEC lock */
		LockTime = 0;
		do
		{
			ChipWaitOrAbort_0367qam(FALSE, 5);/* wait 5 ms */
			LockTime += 5;
			QAMFEC_Lock = ChipGetField_0367qam(DeviceMap, IOHandle, F367qam_QAMFEC_LOCK);
		}
		while (!QAMFEC_Lock && (LockTime < FECTimeOut));
	}
	else
		QAMFEC_Lock = 0;
	if (QAMFEC_Lock)
	{
		signalType = FE_367qam_DATAOK;
		pIntParams->DemodResult.Modulation = pIntParams->Modulation;
		pIntParams->DemodResult.SpectInv = ChipGetField_0367qam(DeviceMap, IOHandle, F367qam_QUAD_INV);
#if 0
		//lwj add begin
		if (Inst->DriverParam.Cab.TunerDriver.tuner_GetFreq != NULL) //lwj add
		{
			Inst->DriverParam.Cab.TunerDriver.tuner_GetFreq(Handle, &FreqResult);
		}
#else
		FreqResult = D0367qam_GeFrequencyKhz(DeviceMap, IOHandle);
#endif /* 0 */
		//lwj add end
		//if (FE_TunerGetIF_Freq(pIntParams->hTuner) != 0)
#if 0 //lwj remove
		if (0) //IF
		{
			if (FE_TunerGetIF_Freq(pIntParams->hTuner) > pIntParams->AdcClock_Hz / 1000)
			{
				pIntParams->DemodResult.Frequency_kHz = FreqResult - FE_367qam_GetDerotFreq(DeviceMap, IOHandle, pIntParams->AdcClock_Hz) - pIntParams->AdcClock_Hz / 1000 + FE_TunerGetIF_Freq(pIntParams->hTuner);
			}
			else
			{
				pIntParams->DemodResult.Frequency_kHz = FreqResult - FE_367qam_GetDerotFreq(DeviceMap, IOHandle, pIntParams->AdcClock_Hz) + FE_TunerGetIF_Freq(pIntParams->hTuner);
			}
		}
		else
#endif
		{
			pIntParams->DemodResult.Frequency_kHz = FreqResult + FE_367qam_GetDerotFreq(DeviceMap, IOHandle, pIntParams->AdcClock_Hz) - pIntParams->AdcClock_Hz / 4000;
		}
		pIntParams->DemodResult.SymbolRate_Bds = FE_367qam_GetSymbolRate(DeviceMap, IOHandle, pIntParams->MasterClock_Hz);
		pIntParams->DemodResult.Locked = 1;
		/* ChipSetField(pIntParams->hDemod,F367qam_AGC_ACCUMRSTSEL,7);*/
	}
	else
	{
		switch (QAM_Lock)
		{
			case 1:
				signalType = FE_367qam_NOAGC;
				break;
			case 2:
				signalType = FE_367qam_NOTIMING;
				break;
			case 3:
				signalType = FE_367qam_TIMINGOK;
				break;
			case 4:
				signalType = FE_367qam_NOCARRIER;
				break;
			case 5:
				signalType = FE_367qam_CARRIEROK;
				break;
			case 7:
				signalType = FE_367qam_NOBLIND;
				break;
			case 8:
				signalType = FE_367qam_BLINDOK;
				break;
			case 10:
				signalType = FE_367qam_NODEMOD;
				break;
			case 11:
				signalType = FE_367qam_DEMODOK;
				break;
			case 12:
				signalType = FE_367qam_DEMODOK;
				break;
			case 13:
				signalType = FE_367qam_NODEMOD;
				break;
			case 14:
				signalType = FE_367qam_NOBLIND;
				break;
			case 15:
				signalType = FE_367qam_NOSIGNAL;
				break;
			default:
				break;
		}
	}
	/* Set the AGC control values to tracking values */
	ChipSetField_0367qam(DeviceMap, IOHandle, F367qam_AGC_ACCUMRSTSEL, TrackAGCAccum);
	return signalType;
}

YW_ErrorType_T D0367qam_ScanFreq(TUNER_IOREG_DeviceMap_t *DemodDeviceMap,
								 IOARCH_Handle_t DemodIOHandle)
{
	YW_ErrorType_T Error = YW_NO_ERROR;
	IOARCH_Handle_t IOHandle = DemodIOHandle;
	TUNER_IOREG_DeviceMap_t *DeviceMap = DemodDeviceMap;
	FE_367qam_InternalParams_t pParams;
	U32 ZigzagScan = 0;
	S32 SearchRange_Hz_Tmp;
	FirstTimeBER[0] = 1;
	YWLIB_Memset(&pParams, 0, sizeof(FE_367qam_InternalParams_t));
	pParams.State = FE_367qam_NOTUNER;
	pParams.Modulation = D0367qam_GeModulation(DeviceMap, IOHandle);
	pParams.Crystal_Hz = DeviceMap->RegExtClk; //30M 还是27M，由硬件决定
	pParams.SearchRange_Hz = 280000;/*280 kHz*/ //question
	pParams.SymbolRate_Bds = D0367qam_GeSymbolRate(DeviceMap, IOHandle);////
	pParams.Frequency_kHz = D0367qam_GeFrequencyKhz(DeviceMap, IOHandle);////
	pParams.AdcClock_Hz = FE_367qam_GetADCFreq(DeviceMap, IOHandle, pParams.Crystal_Hz);
	pParams.MasterClock_Hz = FE_367qam_GetMclkFreq(DeviceMap, IOHandle, pParams.Crystal_Hz);
	_DEBUG
	{
		printk("demod_d0367qam_ScanFreq Frequency === %d\n", pParams.Frequency_kHz);
		printk("demod_d0367qam_ScanFreq Modulation === %d\n", pParams.Modulation);
		printk("SymbolRate_Bds =========== %d\n", pParams.SymbolRate_Bds);
// printk("pParams.AdcClock_Hz ====== %d\n",pParams.AdcClock_Hz);
// printk("pParams.MasterClock_Hz ===== %d\n",pParams.MasterClock_Hz);
	}
#if 0
	if (Inst->DriverParam.Cab.TunerDriver.tuner_SetFreq != NULL)
	{
		Error = (Inst->DriverParam.Cab.TunerDriver.tuner_SetFreq)(Index, pParams.Frequency_kHz, NULL);
	}
#else
	D0367qam_TunerSetFreq(DeviceMap, IOHandle);
#endif /* 0 */
	/* Sets the QAM size and all the related parameters */
	D367qam_SetQamSize(TUNER_TUNER_SHARP5469C, DeviceMap, IOHandle, pParams.Frequency_kHz, pParams.SymbolRate_Bds, pParams.Modulation);
	/* Sets the symbol and all the related parameters */
	FE_367qam_SetSymbolRate(DeviceMap, IOHandle, pParams.AdcClock_Hz, pParams.MasterClock_Hz, pParams.SymbolRate_Bds, pParams.Modulation);
	/* Zigzag Algorithm test */
	if (25 * pParams.SearchRange_Hz > pParams.SymbolRate_Bds)
	{
		pParams.SearchRange_Hz = -(S32)(pParams.SymbolRate_Bds) / 25;
		ZigzagScan = 1;
	}
	/* Search algorithm launch, [-1.1*RangeOffset, +1.1*RangeOffset] scan */
	pParams.State = D0367qam_Algo(DeviceMap, IOHandle, &pParams);
	SearchRange_Hz_Tmp = pParams.SearchRange_Hz; //lwj add
	if (ZigzagScan && (pParams.State != FE_367qam_DATAOK))
	{
		do
		{
#if 1 //lwj modify
			pParams.SearchRange_Hz = -pParams.SearchRange_Hz;
			if (pParams.SearchRange_Hz > 0)
				pParams.DerotOffset_Hz = -pParams.DerotOffset_Hz + pParams.SearchRange_Hz;
			else
				pParams.DerotOffset_Hz = -pParams.DerotOffset_Hz;
#endif
			/* Search algorithm launch, [-1.1*RangeOffset, +1.1*RangeOffset] scan */
			pParams.State = D0367qam_Algo(DeviceMap, IOHandle, &pParams);
		}
		while (((pParams.DerotOffset_Hz + pParams.SearchRange_Hz) >= -(S32)SearchRange_Hz_Tmp) && (pParams.State != FE_367qam_DATAOK));
	}
	/* check results */
	if ((pParams.State == FE_367qam_DATAOK) && (!Error))
	{
		/* update results */
		_DEBUG
		printk("TUNER_STATUS_LOCKED #######################\n");
		//Inst->Status = TUNER_STATUS_LOCKED;
		//pResult->Frequency_kHz = pIntParams->DemodResult.Frequency_kHz;
		//pResult->SymbolRate_Bds = pIntParams->DemodResult.SymbolRate_Bds;
		//pResult->SpectInv = pIntParams->DemodResult.SpectInv;
		//pResult->Modulation = pIntParams->DemodResult.Modulation;
	}
	else
	{
		_DEBUG
		printk("TUNER_STATUS_UNLOCKED #######################\n");
		//Inst->Status = TUNER_STATUS_UNLOCKED;
	}
	return (Error);
}

struct dvb_frontend *dvb_d0367_fe_qam_attach(struct i2c_adapter *i2c)
{
	struct dvb_d0367_fe_qam_state *state = NULL;
	TUNER_IOREG_DeviceMap_t *DeviceMap;
	IOARCH_Handle_t IOHandle;
	/* allocate memory for the internal state */
	state = kzalloc(sizeof(struct dvb_d0367_fe_qam_state), GFP_KERNEL);
	if (state == NULL) goto error;
	/* create dvb_frontend */
	memcpy(&state->frontend.ops, &dvb_d0367_fe_qam_ops, sizeof(struct dvb_frontend_ops));
	state->frontend.demodulator_priv = state;
	state->i2c = i2c;
	state->IOHandle = (IOARCH_Handle_t)i2c;
	state->DeviceMap.Timeout = IOREG_DEFAULT_TIMEOUT;
	state->DeviceMap.Registers = STV0367qam_NBREGS;
	state->DeviceMap.Fields = STV0367qam_NBFIELDS;
	state->DeviceMap.Mode = IOREG_MODE_SUBADR_16;
	state->DeviceMap.RegExtClk = 27000000; //Demod External Crystal_HZ
	state->DeviceMap.RegMap = (TUNER_IOREG_Register_t *)
							  kzalloc(state->DeviceMap.Registers * sizeof(TUNER_IOREG_Register_t),
									  GFP_KERNEL);
	state->DeviceMap.priv = (void *)state;
	DeviceMap = &state->DeviceMap;
	IOHandle = state->IOHandle;
	{
		U8 data = 0xFF;
		data = ChipGetField_0367qam(DeviceMap, IOHandle, R367qam_ID);
		_DEBUG
		printk("set data = 0x%02x\n", data);
	}
#if 0
	{
		U32 Quality;
		U32 Intensity;
		U32 Ber;
		FE_STV0367qam_GetSignalInfo(DeviceMap, IOHandle,
									&Quality, &Intensity, &Ber, FirstTimeBER);
		printk("Quality = %d, Intensity = %d, Ber = %d\n",
			   Quality, Intensity, Ber);
	}
#if 0
	{
		BOOL bIsLocked;
		bIsLocked = FE_367qam_Status(DeviceMap, IOHandle);
		printk("bIsLocked = %d\n", bIsLocked);
	}
#endif /* 0 */
#endif /* 0 */
	return &state->frontend;
error:
	kfree(state);
	return NULL;
}

MODULE_DESCRIPTION("DVB d0367 Frontend");
MODULE_AUTHOR("oSaiYa");
MODULE_LICENSE("GPL");

EXPORT_SYMBOL(dvb_d0367_fe_qam_attach);

