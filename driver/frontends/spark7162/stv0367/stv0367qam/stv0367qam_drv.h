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
 @File stv0367ofdm_drv.h
 @brief

*/
#ifndef _STV0367QAM_DRV_H
#define _STV0367QAM_DRV_H

typedef enum
{
	FE_LLA_NO_ERROR,
	FE_LLA_INVALID_HANDLE,
	FE_LLA_ALLOCATION,
	FE_LLA_BAD_PARAMETER,
	FE_LLA_ALREADY_INITIALIZED,
	FE_LLA_I2C_ERROR,
	FE_LLA_SEARCH_FAILED,
	FE_LLA_TRACKING_FAILED,
	FE_LLA_TERM_FAILED

} FE_LLA_Error_t; /*Error Type*/ //lwj add

typedef enum
{
	FE_CAB_MOD_QAM4,
	FE_CAB_MOD_QAM16,
	FE_CAB_MOD_QAM32,
	FE_CAB_MOD_QAM64,
	FE_CAB_MOD_QAM128,
	FE_CAB_MOD_QAM256,
	FE_CAB_MOD_QAM512,
	FE_CAB_MOD_QAM1024
} FE_CAB_Modulation_t;

/* Monitoring structure */
typedef struct
{
	U32 FE_367qam_TotalBlocks,
		FE_367qam_TotalBlocksOld,
		FE_367qam_TotalBlocksOffset,
		FE_367qam_TotalCB,
		FE_367qam_TotalCBOld,
		FE_367qam_TotalCBOffset,
		FE_367qam_TotalUCB,
		FE_367qam_TotalUCBOld,
		FE_367qam_TotalUCBOffset,
		FE_367qam_BER_Reg,
		FE_367qam_BER_U32,
		FE_367qam_Saturation,
		FE_367qam_WaitingTime;
} FE_367qam_Monitor;

typedef enum
{
	FE_367qam_NOTUNER,
	FE_367qam_NOAGC,
	FE_367qam_NOSIGNAL,
	FE_367qam_NOTIMING,
	FE_367qam_TIMINGOK,
	FE_367qam_NOCARRIER,
	FE_367qam_CARRIEROK,
	FE_367qam_NOBLIND,
	FE_367qam_BLINDOK,
	FE_367qam_NODEMOD,
	FE_367qam_DEMODOK,
	FE_367qam_DATAOK
} FE_367qam_SIGNALTYPE_t;

typedef enum
{
	FE_CAB_SPECT_NORMAL,
	FE_CAB_SPECT_INVERTED
} FE_CAB_SpectInv_t;

typedef struct
{
	U32 TotalBlocks;
	U32 TotalBlocksOffset;
	U32 TotalCB;
	U32 TotalCBOffset;
	U32 TotalUCB;
	U32 TotalUCBOffset;
} FE_CAB_PacketCounter_t;

typedef struct
{
	BOOL Locked; /* channel found */
	U32 Frequency_kHz; /* found frequency (in kHz) */
	U32 SymbolRate_Bds; /* found symbol rate (in Bds) */
	FE_CAB_Modulation_t Modulation; /* modulation */
	FE_CAB_SpectInv_t SpectInv; /* Spectrum Inversion */
	U8 Interleaver; /* Interleaver in FEC B mode */
	FE_CAB_PacketCounter_t PacketCounter;
} FE_CAB_SearchResult_t;

/****************************************************************
 SEARCH STRUCTURES
 ****************************************************************/
typedef struct
{
	//STCHIP_Handle_t hDemod; /* Handle to the demod */
	//STCHIP_Handle_t hTuner; /* Handle to the tuner */
	//STCHIP_Handle_t hTuner2; /* Handle to the tuner */

	FE_367qam_SIGNALTYPE_t State;

	S32 Crystal_Hz, /* Crystal frequency (Hz) */
		IF_Freq_kHz,
		Frequency_kHz, /* Current tuner frequency (KHz) */
		SymbolRate_Bds, /* Symbol rate (Bds) */
		MasterClock_Hz, /* Master clock frequency (Hz) */
		AdcClock_Hz, /* ADC clock frequency (Hz) */
		SearchRange_Hz, /* Search range (Hz) */
		DerotOffset_Hz; /* Derotator offset during software zigzag (Hz)*/
	U32 FirstTimeBER;
	FE_CAB_Modulation_t Modulation; /* QAM Size */
	FE_367qam_Monitor Monitor_results; /* Monitorting counters */
	FE_CAB_SearchResult_t DemodResult; /* Search results */
} FE_367qam_InternalParams_t;

int FE_STV0367QAM_GetSignalInfo(U8 Handle, U32 *CN_dBx10, U32 *Power_dBmx10, U32 *Ber, U32 *FirstTimeBER);
BOOL FE_367qam_Status(TUNER_IOREG_DeviceMap_t *DemodDeviceMap, IOARCH_Handle_t DemodIOHandle);
U32 FE_367qam_SetDerotFreq(TUNER_IOREG_DeviceMap_t *DemodDeviceMap, IOARCH_Handle_t DemodIOHandle, U32 AdcClk_Hz, S32 DerotFreq_Hz);
U32 FE_367qam_GetADCFreq(TUNER_IOREG_DeviceMap_t *DemodDeviceMap, IOARCH_Handle_t DemodIOHandle, U32 ExtClk_Hz);
U32 FE_367qam_GetMclkFreq(TUNER_IOREG_DeviceMap_t *DemodDeviceMap, IOARCH_Handle_t DemodIOHandle, U32 ExtClk_Hz);
FE_CAB_Modulation_t FE_367qam_SetQamSize(TUNER_ScanTaskParam_T *Inst, TUNER_IOREG_DeviceMap_t *DemodDeviceMap, IOARCH_Handle_t DemodIOHandle, U32 SearchFreq_kHz, U32 SymbolRate, FE_CAB_Modulation_t QAMSize);
U32 FE_367qam_SetSymbolRate(TUNER_IOREG_DeviceMap_t *DemodDeviceMap, IOARCH_Handle_t DemodIOHandle, U32 AdcClk_Hz, U32 MasterClk_Hz, U32 SymbolRate, FE_CAB_Modulation_t QAMSize);
U32 FE_367qam_GetDerotFreq(TUNER_IOREG_DeviceMap_t *DemodDeviceMap, IOARCH_Handle_t DemodIOHandle, U32 AdcClk_Hz);
U32 FE_367qam_GetSymbolRate(TUNER_IOREG_DeviceMap_t *DemodDeviceMap, IOARCH_Handle_t DemodIOHandle, U32 MasterClk_Hz);

S32 FE_STV0367qam_GetSnr(TUNER_IOREG_DeviceMap_t *DemodDeviceMap,
						 IOARCH_Handle_t DemodIOHandle);
S32 FE_STV0367qam_GetPower(TUNER_IOREG_DeviceMap_t *DemodDeviceMap,
						   IOARCH_Handle_t DemodIOHandle);
S32 FE_STV0367qam_GetErrors(TUNER_IOREG_DeviceMap_t *DemodDeviceMap,
							IOARCH_Handle_t DemodIOHandle);
int FE_STV0367qam_GetSignalInfo(TUNER_IOREG_DeviceMap_t *DemodDeviceMap,
								IOARCH_Handle_t DemodIOHandle,
								U32 *CN_dBx10,
								U32 *Power_dBmx10,
								U32 *Ber,
								U32 *FirstTimeBER);
FE_CAB_Modulation_t D367qam_SetQamSize(TUNER_TunerType_T TunerType,
									   TUNER_IOREG_DeviceMap_t *DemodDeviceMap,
									   IOARCH_Handle_t DemodIOHandle,
									   U32 SearchFreq_kHz,
									   U32 SymbolRate,
									   FE_CAB_Modulation_t QAMSize);

/////////////////////////////////////////////////////////////

#endif
