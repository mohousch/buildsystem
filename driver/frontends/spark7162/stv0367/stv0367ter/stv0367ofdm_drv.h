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
#ifndef _STV0367OFDM_DRV_H
#define _STV0367OFDM_DRV_H
//typedef InternalParamsPtr FE_367ofdm_Handle_t;//lwj remove

//lwj add begin
/* enums common to all TER lla*/
typedef enum
{
	FE_TER_NOAGC = 0,
	FE_TER_AGCOK = 5,
	FE_TER_NOTPS = 6,
	FE_TER_TPSOK = 7,
	FE_TER_NOSYMBOL = 8,
	FE_TER_BAD_CPQ = 9,
	FE_TER_PRFOUNDOK = 10,
	FE_TER_NOPRFOUND = 11,
	FE_TER_LOCKOK = 12,
	FE_TER_NOLOCK = 13,
	FE_TER_SYMBOLOK = 15,
	FE_TER_CPAMPOK = 16,
	FE_TER_NOCPAMP = 17,
	FE_TER_SWNOK = 18

} FE_TER_SignalStatus_t;

/*To make similar structures between sttuner and LLA and removed dependency of sttuner SS*/
typedef enum
{
	FE_TER_MOD_NONE = 0x00, /* Modulation unknown */
	FE_TER_MOD_ALL = 0x1FF, /* Logical OR of all MODs */
	FE_TER_MOD_QPSK = 1,
	FE_TER_MOD_8PSK = (1 << 1),
	FE_TER_MOD_QAM = (1 << 2),
	FE_TER_MOD_4QAM = (1 << 3),
	FE_TER_MOD_16QAM = (1 << 4),
	FE_TER_MOD_32QAM = (1 << 5),
	FE_TER_MOD_64QAM = (1 << 6),
	FE_TER_MOD_128QAM = (1 << 7),
	FE_TER_MOD_256QAM = (1 << 8),
	FE_TER_MOD_BPSK = (1 << 9),
	FE_TER_MOD_16APSK,
	FE_TER_MOD_32APSK,
	FE_TER_MOD_8VSB = (1 << 10)
} FE_TER_Modulation_t;

typedef enum
{
	FE_TER_MODE_2K,
	FE_TER_MODE_8K,
	FE_TER_MODE_4K,
	FE_TER_MODE_1K, /*FFT Mode 2K*/
	FE_TER_MODE_16K, /*FFT Mode 8K*/
	FE_TER_MODE_32K /*FFT Mode 4K*/
}
FE_TER_Mode_t;

typedef enum
{
	FE_TER_GUARD_1_32, /* Guard interval = 1/32 */
	FE_TER_GUARD_1_16, /* Guard interval = 1/16 */
	FE_TER_GUARD_1_8, /* Guard interval = 1/8 */
	FE_TER_GUARD_1_4, /* Guard interval = 1/4 */
	FE_TER_GUARD_1_128, /* Guard interval = 1/16 */
	FE_TER_GUARD_19_128, /* Guard interval = 1/8 */
	FE_TER_GUARD_19_256
}
FE_TER_Guard_t;

typedef enum
{
	FE_TER_HIER_ALPHA_NONE, /* Regular modulation */
	FE_TER_HIER_ALPHA_1, /* Hierarchical modulation a = 1*/
	FE_TER_HIER_ALPHA_2, /* Hierarchical modulation a = 2*/
	FE_TER_HIER_ALPHA_4 /* Hierarchical modulation a = 4*/
}
FE_TER_Hierarchy_Alpha_t;

typedef enum
{
	FE_TER_HIER_NONE, /*Hierarchy None*/
	FE_TER_HIER_LOW_PRIO, /*Hierarchy : Low Priority*/
	FE_TER_HIER_HIGH_PRIO, /*Hierarchy : High Priority*/
	FE_TER_HIER_PRIO_ANY /*Hierarchy :Any*/
} FE_TER_Hierarchy_t;

typedef enum
{
	FE_TER_INVERSION_NONE = 0,
	FE_TER_INVERSION = 1,
	FE_TER_INVERSION_AUTO = 2,
	FE_TER_INVERSION_UNK = 4
}
FE_Spectrum_t;

typedef enum
{
	FE_TER_FORCENONE = 0,
	FE_TER_FORCE_M_G = 1
}
FE_TER_Force_t;

typedef enum
{
	FE_TER_CHAN_BW_6M = 6,
	FE_TER_CHAN_BW_7M = 7,
	FE_TER_CHAN_BW_8M = 8,
	FE_TER_CHAN_BW_5M = 5, /*BW : 5MHz*/
	FE_TER_CHAN_BW_17M = 17, /*BW : 1.7MHz*/
	FE_TER_CHAN_BW_10M = 10 /*BW : 10MHz*/
}
FE_TER_ChannelBW_t;

typedef enum
{
	FE_TER_FEC_NONE = 0x00, /* no FEC rate specified */
	FE_TER_FEC_ALL = 0xFF, /* Logical OR of all FECs */
	FE_TER_FEC_1_2 = 1,
	FE_TER_FEC_2_3 = (1 << 1),
	FE_TER_FEC_3_4 = (1 << 2),
	FE_TER_FEC_4_5 = (1 << 3),
	FE_TER_FEC_5_6 = (1 << 4),
	FE_TER_FEC_6_7 = (1 << 5),
	FE_TER_FEC_7_8 = (1 << 6),
	FE_TER_FEC_8_9 = (1 << 7),
	FE_TER_FEC_1_4 = (1 << 8),
	FE_TER_FEC_1_3 = (1 << 9),
	FE_TER_FEC_2_5 = (1 << 10),
	FE_TER_FEC_3_5 = (1 << 11),
	FE_TER_FEC_9_10 = (1 << 12)
}
FE_TER_FECRate_t;

typedef enum
{
	FE_TER_TPS_1_2 = 0,
	FE_TER_TPS_2_3 = 1,
	FE_TER_TPS_3_4 = 2,
	FE_TER_TPS_5_6 = 3,
	FE_TER_TPS_7_8 = 4
} FE_TER_Rate_TPS_t;

typedef enum
{
	FE_TER_FE_1_2 = 0,
	FE_TER_FE_2_3 = 1,
	FE_TER_FE_3_4 = 2,
	FE_TER_FE_5_6 = 3,
	FE_TER_FE_6_7 = 4,
	FE_TER_FE_7_8 = 5
} FE_TER_Rate_t;

typedef enum
{
	FE_TER_NO_FORCE = 0,
	FE_TER_FORCE_PR_1_2 = 1,
	FE_TER_FORCE_PR_2_3 = 1 << 1,
	FE_TER_FORCE_PR_3_4 = 1 << 2,
	FE_TER_FORCE_PR_5_6 = 1 << 3,
	FE_TER_FORCE_PR_7_8 = 1 << 4
} FE_TER_Force_PR_t ;

typedef enum
{
	FE_TER_NOT_FORCED = 0,
	FE_TER_WAIT_TRL = 1,
	FE_TER_WAIT_AGC = 2,
	FE_TER_WAIT_SYR = 3,
	FE_TER_WAIT_PPM = 4,
	FE_TER_WAIT_TPS = 5,
	FE_TER_MONITOR_TPS = 6,
	FE_TER_RESERVED = 7
} FE_TER_State_Machine_t ;

typedef enum
{
	FE_TER_NORMAL_IF_TUNER = 0,
	FE_TER_LONGPATH_IF_TUNER = 1,
	FE_TER_IQ_TUNER = 2

} FE_TER_IF_IQ_Mode;

typedef enum
{
	FE_TER_PARITY_ON,
	FE_TER_PARITY_OFF
} FE_TER_DataParity_t;

typedef enum
{
	FE_TS_MODE_DEFAULT, /* TS output not changeable */
	FE_TS_MODE_SERIAL, /* TS output serial */
	FE_TS_MODE_PARALLEL, /* TS output parallel */
	FE_TS_MODE_DVBCI /* TS output DVB-CI */
} FE_TER_TSOutputMode_t;

typedef enum
{
	FE_DVBT,
	FE_DVBT2
} FE_TER_Type_t;

/* typedef enum
 {
 FE_TER_PARALLEL_CLOCK,
 FE_TER_SERIAL_CLOCK
 }
 FE_TER_Clock_t; */

typedef enum
{
	FE_TS_OUTPUTMODE_DEFAULT, /*Use default register setting*/
	FE_TS_SERIAL_PUNCT_CLOCK, /*Serial punctured clock : serial STBackend */
	FE_TS_SERIAL_CONT_CLOCK, /*Serial continues clock */
	FE_TS_PARALLEL_PUNCT_CLOCK, /*Parallel punctured clock : Parallel STBackend */
	FE_TS_DVBCI_CLOCK /*Parallel continues clock : DVBCI */

} FE_TS_OutputMode_t;

typedef enum
{
	FE_TS_CLOCKPOLARITY_DEFAULT,
	FE_TS_RISINGEDGE_CLOCK, /* TS clock rising edge */
	FE_TS_FALLINGEDGE_CLOCK /* TS clock falling edge */
} FE_TS_ClockPolarity_t;

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

/****************************************************************
 INIT STRUCTURES
 ****************************************************************/

#if 0 //lwj remove
typedef struct
{
	char DemodName[20]; /* STB0297E name */
	U8 DemodI2cAddr; /* STB0297E I2c address */
	TUNER_Model_t TunerModel; /* Tuner model */
	char TunerName[20]; /* Tuner name */
	U8 TunerI2cAddr, TunerI2cAddr2; /*Tuner I2c address*/
	FE_TS_ClockPolarity_t ClockPolarity; /* Parity bytes */

	FE_TS_OutputMode_t PathTSClock;
	S32 Demod_Crystal_Hz; /* XTAL value */
	S32 Tuner_Crystal_Hz; /* XTAL value */
	U32 TunerIF_kHz; /* Tuner IF value */
} FE_TER_InitParams_t;

typedef struct
{
	FE_TER_SignalStatus_t SignalType; /* Type of founded signal */
	FE_TER_FECRate_t PunctureRate; /* Puncture rate found */
	double Frequency; /* Transponder frequency (KHz)*/
	FE_TER_Mode_t Mode; /* Mode 2K or 8K */
	FE_TER_Guard_t Guard; /* Guard interval */
	FE_TER_Modulation_t Modulation; /* modulation */
	FE_TER_Hierarchy_t hier;
	FE_TER_Rate_TPS_t HPRate;
	FE_TER_Rate_TPS_t LPRate;
	FE_TER_Rate_TPS_t pr;
	U32 SymbolRate; /* Symbol rate (Bds) */
} FE_TER_InternalResults_t;
#endif
/****************************************************************
 SEARCH STRUCTURES
 ****************************************************************/

typedef struct
{
	U32 Frequency;
	FE_TER_IF_IQ_Mode IF_IQ_Mode;
	FE_TER_Mode_t Mode;
	FE_TER_Guard_t Guard;
	FE_TER_Force_t Force;
	FE_TER_Hierarchy_t Hierarchy;
	FE_Spectrum_t Inv;
	FE_TER_ChannelBW_t ChannelBW;
	S8 EchoPos;
	U32 SearchRange;
	U32 DataPLPId;
	FE_TER_Type_t DVB_Type;
	//sony_dvb_demod_t2_preset_t pPresetInfo;//question
} FE_TER_SearchParams_t;

typedef struct
{
	U32 Frequency;
	U32 Agc_val;
	FE_TER_Mode_t Mode;
	FE_TER_Guard_t Guard;
	FE_TER_Modulation_t Modulation; /*modulation*/
	FE_TER_Hierarchy_t hier;
	FE_Spectrum_t Sense; /*0 spectrum not inverted */
	FE_TER_Hierarchy_Alpha_t Hierarchy_Alpha; /** Hierarchy Alpha level used for reporting**/
	FE_TER_Rate_TPS_t HPRate;
	FE_TER_Rate_TPS_t LPRate;
	FE_TER_Rate_TPS_t pr;
	FE_TER_FECRate_t FECRates;
	FE_TER_State_Machine_t State;
	S8 Echo_pos;
	FE_TER_SignalStatus_t SignalStatus;
	BOOL Locked;
	int ResidualOffset;
	//sony_dvb_demod_tpsinfo_t TPSInfo;//question lwj
} FE_TER_SearchResult_t;

/************************
 INFO STRUCTURE
************************/

typedef struct
{
	BOOL Locked; /* Channel locked */
	U32 Frequency; /* Channel frequency (in KHz) */
	U32 SymbolRate; /* Channel symbol rate (in Mbds) */
	FE_TER_Modulation_t Modulation; /* modulation */
	FE_Spectrum_t SpectInversion; /* Spectrum Inversion */
	S32 Power_dBmx10; /* Power of the RF signal (dBm x 10) */
	U32 CN_dBx10; /* Carrier to noise ratio (dB x 10) */
	U32 BER; /* Bit error rate (x 10000) */
	FE_TER_Hierarchy_t Hierarchy;
	FE_TER_Hierarchy_Alpha_t Hierarchy_Alpha; /** Hierarchy Alpha level used for reporting**/
	FE_TER_Rate_TPS_t HPRate;
	FE_TER_Rate_TPS_t LPRate;
	/*For DVB-T2*/
	//sony_dvb_dvbt2_plp_btype_t type;//
	//sony_dvb_dvbt2_plp_t pPLPInfo;//
	//sony_dvb_demod_t2_preset_t pPresetInfo;//
	S32 pCNR;
	//sony_dvb_demod_tpsinfo_t TPSInfo;//
	U32 FER;
	FE_TER_Rate_TPS_t pr;
	FE_TER_Mode_t Mode;
	FE_TER_Guard_t Guard;
	int ResidualOffset;

} FE_TER_SignalInfo_t;

typedef struct
{
	U32 Frequency ;
	FE_TER_SearchResult_t Result;
} FE_TER_Scan_Result_t;

//lwj add end

/****************************************************************
 COMMON STRUCTURES AND TYPEDEFS
 ****************************************************************/
#if 0
#ifndef STFRONTEND_FORCE_STI2C_DEPRECATED
STCHIP_Error_t STV367ofdm_RepeaterFn(STCHIP_Handle_t hChip, BOOL State, U8 *Buffer);
#else
STCHIP_Error_t STV367ofdm_RepeaterFn(STCHIP_Handle_t hChip, BOOL State);
#endif
#endif
typedef struct
{
	/* hChip must be the first item in the structure !!! */
	//STCHIP_Handle_t hDemod; /* Handle to the chip */
	//STCHIP_Handle_t hTuner; /* Handle to the tuner */
	//STCHIP_Handle_t hTuner2; /* Handle to the tuner */
	FE_TER_SignalStatus_t State; /* Current state of the search algorithm */
	FE_TER_IF_IQ_Mode IF_IQ_Mode;
	FE_TER_Mode_t Mode; /* Mode 2K or 8K */
	FE_TER_Guard_t Guard; /* Guard interval */
	FE_TER_Hierarchy_t Hierarchy;
	U32 Frequency; /* Current tuner frequency (KHz) */
	U8 I2Cspeed; /* */
	FE_Spectrum_t Inv ; /* 0 no spectrum inverted search to be perfomed*/
	/*U8 Delta; / * offset of frequency*/
	U8 Sense; /* current search,spectrum not inveerted*/
	U8 Force; /* force mode/guard */
	U8 ChannelBW; /* channel width */
	U8 PreviousChannelBW; /* channel width used during previous lock */
	U32 PreviousBER;
	U32 PreviousPER;
	S8 EchoPos; /* echo position */
	U8 first_lock; /* */
	S32 Crystal_Hz; /* XTAL value */
	/*U8 prev_lock_status; / * verbose status of the previous lock (for scan ) */
	////FE_TER_InternalResults_t Results;/* Results of the search */ //question
	U8 Unlockcounter;
	BOOL SpectrumInversion;
	////FE_DEMOD_Model_t DemodModel; /* Demod that is used with the tuner */ //question
} FE_367ofdm_InternalParams_t;

/****************************************************************
 API FUNCTIONS
****************************************************************/
#if 0
FE_LLA_Error_t FE_367ofdm_Init(FE_TER_InitParams_t *pInit, FE_367ofdm_Handle_t *Handle);

FE_LLA_Error_t FE_367ofdm_Search(FE_367ofdm_Handle_t Handle,
								 FE_TER_SearchParams_t *pSearch,
								 FE_TER_SearchResult_t *pResult);

FE_LLA_Error_t FE_367ofdm_LookFor(FE_367ofdm_Handle_t Handle,
								  FE_TER_SearchParams_t *pParams,
								  FE_TER_SearchResult_t *pResult);
FE_LLA_Error_t GetTunerInfo(FE_367ofdm_Handle_t Handle,
							FE_TER_SearchParams_t *pParams,
							FE_TER_SearchResult_t *pResult);
BOOL FE_367ofdm_Status(FE_367ofdm_Handle_t Handle);
FE_LLA_Error_t FE_367ofdm_GetSignalInfo(FE_367ofdm_Handle_t Handle, FE_TER_SignalInfo_t *pInfo);

FE_LLA_Error_t FE_367ofdm_Term(FE_367ofdm_Handle_t Handle);
void FE_367ofdm_Tracking(FE_367ofdm_InternalParams_t *pParams);
int SpeedInit(STCHIP_Handle_t hChip);
void SET_TRLNOMRATE_REGS(STCHIP_Handle_t hChip, short unsigned int value);
short unsigned int GET_TRLNOMRATE_VALUE(STCHIP_Handle_t hChip);
signed int GET_TRL_OFFSET(STCHIP_Handle_t hChip);
int duration(S32 mode, int tempo1, int tempo2, int tempo3);
U32 FE_367ofdm_GetMclkFreq(STCHIP_Handle_t hChip, U32 ExtClk_Hz);
U32 FE_367ofdm_GetBerErrors(FE_367ofdm_InternalParams_t *pParams);
U32 FE_367ofdm_GetPerErrors(FE_367ofdm_InternalParams_t *pParams);
U32 FE_367ofdm_GetErrors(FE_367ofdm_InternalParams_t *pParams);
FE_LLA_Error_t FE_367ofdm_SetAbortFlag(FE_367ofdm_Handle_t Handle, BOOL Abort);
FE_LLA_Error_t FE_STV0367ofdm_SetStandby(FE_367ofdm_Handle_t Handle, U8 StandbyOn);
FE_LLA_Error_t FE_367ofdm_SwitchDemodToDVBT(FE_367ofdm_Handle_t Handle);
BOOL FE_367ofdm_lock(FE_367ofdm_Handle_t Handle);

void demod_get_pd(void *dummy_handle, unsigned short *level, STCHIP_Handle_t hTuner);
void demod_get_agc(void *dummy_handle, U16 *rf_agc, U16 *bb_agc, STCHIP_Handle_t hTuner);
void demod_set_agclim(void *dummy_handle, U16 dir_up, STCHIP_Handle_t hTuner);

#endif
void FE_STV0367TER_SetCLKgen(TUNER_IOREG_DeviceMap_t *DemodDeviceMap,
							 IOARCH_Handle_t DemodIOHandle,
							 U32 DemodXtalFreq);
void FE_STV0367ofdm_SetTS_Parallel_Serial(TUNER_IOREG_DeviceMap_t *DeviceMap,
										  IOARCH_Handle_t IOHandle,
										  FE_TS_OutputMode_t PathTS);
void FE_STV0367ofdm_SetCLK_Polarity(TUNER_IOREG_DeviceMap_t *DemodDeviceMap,
									IOARCH_Handle_t DemodIOHandle,
									FE_TS_ClockPolarity_t clock);

int FE_367TER_IIR_FILTER_INIT(TUNER_IOREG_DeviceMap_t *DeviceMap,
							  IOARCH_Handle_t IOHandle,
							  U8 Bandwidth,
							  U32 DemodXtalValue);
void FE_367TER_AGC_IIR_RESET(TUNER_IOREG_DeviceMap_t *DeviceMap,
							 IOARCH_Handle_t IOHandle);
U32 FE_367ofdm_GetMclkFreq(TUNER_IOREG_DeviceMap_t *DemodDeviceMap,
						   IOARCH_Handle_t DemodIOHandle,
						   U32 ExtClk_Hz);
void FE_367TER_AGC_IIR_LOCK_DETECTOR_SET(TUNER_IOREG_DeviceMap_t *DemodDeviceMap,
										 IOARCH_Handle_t DemodIOHandle);
BOOL FE_367ofdm_lock(TUNER_IOREG_DeviceMap_t *DemodDeviceMap,
					 IOARCH_Handle_t DemodIOHandle);
FE_TER_SignalStatus_t
FE_367ofdm_LockAlgo(TUNER_IOREG_DeviceMap_t *pDemod_DeviceMap,
					IOARCH_Handle_t DemodIOHandle,
					FE_367ofdm_InternalParams_t *pParams);
U32 FE_367ofdm_GetErrors(TUNER_IOREG_DeviceMap_t *DemodDeviceMap,
						 IOARCH_Handle_t DemodIOHandle); /*Fec2*/
int FE_STV0367TER_GetPower(TUNER_IOREG_DeviceMap_t *DeviceMap,
						   IOARCH_Handle_t IOHandle);
int FE_STV0367TER_GetSnr(TUNER_IOREG_DeviceMap_t *DeviceMap,
						 IOARCH_Handle_t IOHandle);
FE_LLA_Error_t
FE_367ofdm_GetSignalInfo(TUNER_IOREG_DeviceMap_t *pDemod_DeviceMap,
						 IOARCH_Handle_t DemodIOHandle,
						 FE_367ofdm_InternalParams_t *pParams,
						 FE_TER_SignalInfo_t *pInfo);
int FE_STV0367TER_GetSignalInfo(TUNER_IOREG_DeviceMap_t *pDemod_DeviceMap,
								IOARCH_Handle_t DemodIOHandle,
								U32 *CN_dBx10, U32 *Power_dBmx10, U32 *Ber);
#endif

