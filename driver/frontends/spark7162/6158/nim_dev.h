/*****************************************************************************
* Copyright (C)2003 Ali Corporation. All Rights Reserved.
*
* File: nim_dev.h
*
* Description: This file contains NIM device structure define.
* History:
* Date Athor Version Reason
* ============ ============= ========= =================
* 1. Feb.16.2003 Justin Wu Ver 0.1 Create file.
* 2. Jun.12.2003 George jiang Ver 0.2 Porting NIM.
* 3. Aug.22.2003 Justin Wu Ver 0.3 Update.
* 4. Sep.29.2009 DMQ Ver0.4 Update.
*****************************************************************************/

#ifndef __HLD_NIM_DEV_H__
#define __HLD_NIM_DEV_H__

#include "d6158_inner.h"

#define EXT_QPSK_MODE_SPI 0
#define EXT_QPSK_MODE_SSI 1

#if (defined(_M36F_SINGLE_CPU) || ((STB_BOARD_TYPE == FULAN_3606_220_1CA_1CI) ||(STB_BOARD_TYPE == FULAN_3606_220_SIMPLE)))
#define NIM_MAX_NUM 1//dmq add
#else
#define NIM_MAX_NUM 2 //dmq add
#endif
#define NIM_CHIP_ID_M3501A 0x350100C0
#define NIM_CHIP_ID_M3501B 0x350100D0
#define NIM_CHIP_SUB_ID_S3501D 0x00
#define NIM_CHIP_SUB_ID_M3501B 0xC0
#define NIM_CHIP_SUB_ID_M3501C 0xCC
#define NIM_FREQ_RETURN_REAL 0
#define NIM_FREQ_RETURN_SET 1
#define FS_MAXNUM 15000
#define TP_MAXNUM 2000
#define CRYSTAL_FREQ 13500 //13.5M
//#define CRYSTAL_FREQ 16000 //16M

/* NIM Device I/O control command */
enum nim_device_ioctrl_command
{
	NIM_DRIVER_READ_TUNER_STATUS, /* Read tuner lock status */
	NIM_DRIVER_READ_QPSK_STATUS, /* Read QPSK lock status */
	NIM_DRIVER_READ_FEC_STATUS, /* Read FEC lcok status */

	NIM_DRIVER_READ_QPSK_BER, /* Read QPSK Bit Error Rate */
	NIM_DRIVER_READ_VIT_BER, /* Read Viterbi Bit Error Rate */
	NIM_DRIVER_READ_RSUB, /* Read Reed Solomon Uncorrected block */
	NIM_DRIVER_STOP_ATUOSCAN, /* Stop autoscan */
	NIM_DRIVER_UPDATE_PARAM, /* Reset current parameters */
	NIM_DRIVER_TS_OUTPUT, /* Enable NIM output TS*/
	NIM_DRIVER_FFT,
	NIM_DRIVER_GET_CR_NUM,
	NIM_DRIVER_GET_CUR_FREQ,
	NIM_DRIVER_FFT_PARA,
	NIM_DRIVER_GET_SPECTRUM,
	NIM_FFT_JUMP_STEP, /* Get AutoScan FFT frequency jump step */ //huang hao add for m3327 qpsk
	NIM_DRIVER_READ_COFFSET, /* Read Carrier Offset state */ //joey add for stv0297.
	NIM_DRIVER_SEARCH_1ST_CHANNEL, /* Search channel spot*/ //joey add for stv0297.
	NIM_DRIVER_SET_TS_MODE, /*Set ts output mode:*/
	/*bit0: 1 serial out, 0 parallel out*/
	/*bit1: 1 clk rising, 0 clk falling*/
	/*bit2: 1 valid gap, 0 valid no gap*/
	NIM_DRIVER_SET_PARAMETERS, /* Set the parameters of nim,add by Roman at 060321 */
	NIM_DRIVER_SET_RESET_CALLBACK, /* When nim device need to be reset, call an callback to notice app */
	NIM_DRIVER_ENABLE_DVBS2_HBCD, /* For DVB-S2, enable/disable HBCD mode */
	NIM_DRIVER_STOP_CHANSCAN, /* Stop channel change because some low symbol rate TP too long to be locked */
	NIM_DRIVER_RESET_PRE_CHCHG, /* Reset nim device before channel change */

	NIM_ENABLE_IC_SORTING, /*Enable IC sorting, set IC sorting param*/
	NIM_DRIVER_GET_OSD_FREQ_OFFSET, /*get OSD freq offset. */
	NIM_DRIVER_SET_RF_AD_GAIN, /* Set RF ad table for RSSI display.*/
	NIM_DRIVER_SET_IF_AD_GAIN, /* Set IF ad table for RSSI display. */
	NIM_DRIVER_GET_RF_IF_AD_VAL, /* get RF IF ad value. */
	NIM_DRIVER_GET_REC_PERFORMANCE_INFO,/* get receiver performance info. */
	NIM_DRIVER_ENABLE_DEBUG_LOG, /* enable nim driver debug infor out.*/
	NIM_DRIVER_DISABLE_DEBUG_LOG, /* disable nim driver debug infor out.*/
	NIM_DRIVER_GET_FRONTEND_STATE, /* Read front end state.*/
	NIM_DRIVER_SET_FRONTEND_LOCK_CHECK, /* Set front end lock check flag.*/
	NIM_DRIVER_LOG_IFFT_RESULT,
	NIM_DRIVER_CHANGE_TS_GAP, /* Change ts gap */
	NIM_DRIVER_SET_SSI_CLK, /* Set SSI Clock */

	NIM_DRIVER_5V_CONTROL,
	//use io control command to instead of function interface for combo soluation
	NIM_DRIVER_SET_POLAR, /*DVB-S NIM Device set LNB polarization */
	NIM_DRIVER_SET_12V, /*DVB-S NIM Device set LNB votage 12V enable or not */
	NIM_DRIVER_GET_SYM, /* Get Current NIM Device Channel Symbol Rate */
	NIM_DRIVER_GET_BER, /* Get Current NIM Device Channel Bit-Error Rate */
	NIM_DRIVER_GET_AGC, /* Get Current NIM Device Channel AGC Value */
	NIM_DRIVER_DISABLED, /* Disable Current NIM Device */
	NIM_DRIVER_GET_GUARD_INTERVAL,
	NIM_DRIVER_GET_FFT_MODE,
	NIM_DRIVER_GET_MODULATION,
	NIM_DRIVER_GET_SPECTRUM_INV,
	//AD2DMA tool
	NIM_DRIVER_ADC2MEM_START,
	NIM_DRIVER_ADC2MEM_STOP,
	NIM_DRIVER_ADC2MEM_SEEK_SET,
	NIM_DRIVER_ADC2MEM_READ_8K,
	// The following code is for tuner io command
	NIM_TUNER_COMMAND = 0x8000,
	NIM_TUNER_POWER_CONTROL,
	NIM_TUNER_GET_AGC,
	NIM_TUNER_GET_RSSI_CAL_VAL,
	NIM_TUNER_RSSI_CAL_ON, //RSSI calibration on
	NIM_TUNER_RSSI_CAL_OFF,
	NIM_TUNER_RSSI_LNA_CTL,//RSSI set LNA
	NIM_TUNER_SET_GPIO,
	NIM_TUNER_CHECK,
	NIM_TUNER_LOOPTHROUGH,//dmq add
	NIM_DRIVER_SET_BLSCAN_MODE, // Para = 0, NIM_SCAN_FAST; Para = 1, NIM_SCAN_ACCURATE(slower)
	NIM_TURNER_SET_STANDBY,/*set av2012 standby add by bill 2012.02.21*/
	NIM_DRIVER_STOP_TUNER_FRONTEND, //add by rxj 2011/06/10.
	NIM_TUNER_SET_THROUGH_MODE,
};
/* NIM Device I/O control command extension */
enum nim_device_ioctrl_command_ext
{
	NIM_DRIVER_AUTO_SCAN = 0x00FC0000, /* Do AutoScan Procedure */
	NIM_DRIVER_CHANNEL_CHANGE = 0x00FC0001, /* Do Channel Change */
	NIM_DRIVER_CHANNEL_SEARCH = 0x00FC0002, /* Do Channel Search */
	NIM_DRIVER_GET_RF_LEVEL = 0x00FC0003, /* Get RF level */
	NIM_DRIVER_GET_CN_VALUE = 0x00FC0004, /* Get CN value */
	NIM_DRIVER_GET_BER_VALUE = 0x00FC0005, /* Get BER value */
	NIM_DRIVER_QUICK_CHANNEL_CHANGE = 0x00FC0006,/* Do Quick Channel Change without waiting lock */
	NIM_DRIVER_GET_ID = 0x00FC0007, /*Get 3501 type: M3501A/M3501B */
	NIM_DRIVER_SET_PERF_LEVEL = 0x00FC0008, /* Set performance level */
	NIM_DRIVER_START_CAPTURE = 0x00FC0009, /* Start capture */
	NIM_DRIVER_GET_I2C_INFO = 0x00FC000A, /* Start capture */
	NIM_DRIVER_GET_FFT_RESULT = 0x00FC000B, /* Get Current NIM Device Channel FFT spectrum result */
	NIM_DRIVER_DISEQC_OPERATION = 0x00FC000C, /* NIM DiSEqC Device Opearation */
	NIM_DRIVER_DISEQC2X_OPERATION = 0x00FC000D, /* NIM DiSEqC2X Device Opearation */
	NIM_DRIVER_SET_NIM_MODE = 0x00FC000E, /* NIM J83AC/J83B mode setting */
	NIM_DRIVER_SET_QAM_INFO = 0x00FC000F,
};

/* NIM performance level setting */
enum nim_perf_level
{
	NIM_PERF_DEFAULT = 0,
	NIM_PERF_SAFER = 1,
	NIM_PERF_RISK = 2,
};

/* FrontEnd Type */
//enum
//{
//	FE_QPSK = 0x01, /* DVB-S */
//	FE_OFDM = 0x02, /* DVB-T */
//	FE_QAM = 0x04, /* DVB-C */
//	FE_T2_C = 0x06, /*T2, T, C*/
//};
//dmq add end
enum nim_cofdm_work_mode
{
	NIM_COFDM_SOC_MODE = 0,
	NIM_COFDM_ONLY_MODE = 1
};

enum nim_cofdm_ts_mode
{
	NIM_COFDM_TS_SPI_8B = 0,
	NIM_COFDM_TS_SPI_4B = 1,
	NIM_COFDM_TS_SPI_2B = 2,
	NIM_COFDM_TS_SSI = 3
};
//dmq add
typedef enum
{
	SONY_DVB_SYSTEM_UNKNOWN, /**< Unknown. */
	SONY_DVB_SYSTEM_DVBC, /**< DVB-C. */
	SONY_DVB_SYSTEM_DVBT, /**< DVB-T. */
	SONY_DVB_SYSTEM_DVBT2 /**< DVB-T2. */
} sony_dvb_system_t;
enum
{
	DEMO_UNKNOWN = SONY_DVB_SYSTEM_UNKNOWN,
	DEMO_DVBC = SONY_DVB_SYSTEM_DVBC,
	DEMO_DVBT = SONY_DVB_SYSTEM_DVBT,
	DEMO_DVBT2 = SONY_DVB_SYSTEM_DVBT2,
};

/* Structure for NIM Device Status */
struct nim_device_stats
{
	UINT16 temp_num; /* blank */
};

/* Structure for NIM DiSEqC Device Information parameters */
struct t_diseqc_info
{
	UINT8 sat_or_tp; /* 0:sat, 1:tp*/
	UINT8 diseqc_type;
	UINT8 diseqc_port;
	UINT8 diseqc11_type;
	UINT8 diseqc11_port;
	UINT8 diseqc_k22;
	UINT8 diseqc_polar; /* 0: auto,1: H,2: V */
	UINT8 diseqc_toneburst; /* 0: off, 1: A, 2: B */

	UINT8 positioner_type; /*0-no positioner 1-1.2 positioner support, 2-1.3 USALS*/
	UINT8 position; /*use for DiSEqC1.2 only*/
	UINT16 wxyz; /*use for USALS only*/
};

/* Structure for Channel Change parameters */
struct NIM_Channel_Change
{
	UINT32 freq; /* Channel Center Frequency: in MHz unit */
	UINT32 sym; /* Channel Symbol Rate: in KHz unit */
	UINT8 fec; /* Channel FEC rate */
	UINT32 bandwidth; /* Channel Symbol Rate: same as Channel Symbol Rate ? -- for DVB-T */
	UINT8 guard_interval; /* Guard Interval -- for DVB-T */
	UINT8 fft_mode; /* -- for DVB-T */
	UINT8 modulation; /* -- for DVB-T */
	UINT8 usage_type; /* -- for DVB-T */
	UINT8 inverse; /* -- for DVB-T */
	UINT8 priority; /* -- for DVB-T *///dmq add
	UINT8 op_mode; //0:channel change 1:search mode //dmq add
	UINT8 priv_param; //dmq add
	UINT8 dvb_t2_c; //add by rxj 0:t2,t 1:c
	UINT8 PLP_id; //add for t2
	UINT8 scan_mode; //add for t2
};

/* Structure for Channel Search parameters */
struct NIM_Channel_Search
{
	UINT32 freq; /* Channel Center Frequency: in MHz unit */
	UINT32 bandwidth; /* -- for DVB-T */
	UINT8 guard_interval; /* -- for DVB-T */
	UINT8 fft_mode; /* -- for DVB-T */
	UINT8 modulation; /* -- for DVB-T */
	UINT8 fec; /* -- for DVB-T */
	UINT8 usage_type; /* -- for DVB-T */
	UINT8 inverse; /* -- for DVB-T */
	UINT8 op_mode; //0:channel change 1:search mode //dmq add
	UINT16 freq_offset; /* -- for DVB-T */
	UINT16 symbol; //dmq add

};

/* Structure for Auto Scan parameters */
struct NIM_Auto_Scan
{
	//>>> Unicable begin
	UINT8 unicable;
//	UINT8 invert; /* Fub INVERT? */
	UINT16 Fub; /* Unicable: UB slots centre freq (MHz) */
	//<<< Unicable end

	UINT32 sfreq; /* Start Frequency of the Scan procedure: in MHz unit */
	UINT32 efreq; /* End Frequency of the Scan procedure: in MHz unit */
	INT32(*callback)(UINT8 status, UINT8 polar, UINT32 freq, UINT32 sym, UINT8 fec); /* Callback Function pointer */
};

/*M3328F external QPSK configuration*/
struct EXT_QPSK_Config
{
	UINT8 gpio_28f_reset; /* External M3328F reset GPIO*/
	UINT8 ext_qpsk_mod; /* EXT_QPSK_MODE_SSI / EXT_QPSK_MODE_SPI */
	UINT8 host_uart_id; /* 1: UART1; 2: UART2; 3: UART3; other value: UART2(default)*/
	UINT8 tsi_mode; /* Supposed TSI mode. Output parameter from NIM driver.*/
};

/* for NIM_DRIVER_SET_RESET_CALLBACK */
typedef INT32(*pfn_nim_reset_callback)(UINT32 param);

typedef struct
{
	UINT8 lock;
	BOOL valid;
	UINT32 ber;
	UINT32 per;
} nim_rec_performance_t;

/* Structure for IC sorting parameters */
struct NIM_IC_Soring
{
	BOOL bEnable; /* Enable/disable ic sorting*/
	UINT8 *pError_Indicator; /* pointer to global error indicator */
};

/* structure for RSSI display.*/
#if 1//def NEW_AD_GAIN_T
typedef UINT16 nim_ad_gain_t;
#else
typedef struct
{
	UINT16 ad;
	INT8 gain;
} nim_ad_gain_t;
#endif

typedef struct
{
	nim_ad_gain_t *p_table;
	int count;
} nim_ad_gain_table_t;

typedef struct
{
	UINT16 rf_ad_val;
	UINT16 if_ad_val;
} nim_rf_if_ad_val_t;

struct NIM_I2C_Info
{
	UINT32 i2c_type;
	UINT32 i2c_addr;
};

typedef struct
{
	UINT32 freq;
	UINT32 *start_addr;
} nim_get_fft_result_t; //for NIM_DRIVER_GET_FFT_RESULT

typedef struct
{
	UINT32 mode;
	UINT8 *cmd;
	UINT8 *rt_value;
	UINT8 *rt_cnt;
	UINT8 cnt;
} nim_diseqc_operate_para_t; //for NIM_DRIVER_DISEQC_OPERATION
#define HLD_MAX_NAME_SIZE 16 /* Max device name length */

/* Structure nim_device, the basic structure between HLD and LLD of demomulator device */
struct nim_device
{
	struct nim_device *next; /* Next nim device structure */
	UINT32 type; /* Interface hardware type */
	INT8 name[HLD_MAX_NAME_SIZE]; /* Device name */
	UINT32 base_addr; /* Demodulator address */
	UINT32 Tuner_select; /* I2C TYPE for TUNER select */
	UINT16 flags; /* Interface flags, status and ability */

	UINT32 DemodIOHandle[3]; //jhy add, ·Ö±ðÎªt ,t2 ,c

	/* Hardware privative structure */
	void *priv; /* pointer to private data */
	/* Functions of this dem device */
	INT32(*init)(void); /* NIM Device Initialization */
	INT32(*open)(struct nim_device *dev); /* NIM Device Open */
	INT32(*stop)(struct nim_device *dev); /* NIM Device Stop */
	INT32(*do_ioctl)(struct nim_device *dev, INT32 cmd, UINT32 param); /* NIM Device I/O Control */
	INT32(*do_ioctl_ext)(struct nim_device *dev, INT32 cmd, void *param_list); /* NIM Device I/O Control Extension */
	INT32(*get_lock)(struct nim_device *dev, UINT8 *lock); /* Get Current NIM Device Channel Lock Status */
	INT32(*get_freq)(struct nim_device *dev, UINT32 *freq); /* Get Current NIM Device Channel Frequency */
	INT32(*get_FEC)(struct nim_device *dev, UINT8 *fec); /* Get Current NIM Device Channel FEC Rate */
	INT32(*get_SNR)(struct nim_device *dev, UINT8 *snr); /* Get Current NIM Device Channel SNR Value */

	UINT8 diseqc_typex; /* NIM DiSEqC Device Type */
	UINT8 diseqc_portx; /* NIM DiSEqC Device Port */
	struct t_diseqc_info diseqc_info; /* NIM DiSEqC Device Information Structure */
	INT32(*set_polar)(struct nim_device *dev, UINT8 polar); /*DVB-S NIM Device set LNB polarization */
	INT32(*set_12v)(struct nim_device *dev, UINT8 flag); /*DVB-S NIM Device set LNB votage 12V enable or not */
	INT32(*DiSEqC_operate)(struct nim_device *dev, UINT32 mode, UINT8 *cmd, UINT8 cnt); /* NIM DiSEqC Device Opearation */
	INT32(*DiSEqC2X_operate)(struct nim_device *dev, UINT32 mode, UINT8 *cmd, UINT8 cnt, UINT8 *rt_value, UINT8 *rt_cnt); /* NIM DiSEqC2X Device Opearation */
	INT32(*get_sym)(struct nim_device *dev, UINT32 *sym); /* Get Current NIM Device Channel Symbol Rate */
	INT32(*get_BER)(struct nim_device *dev, UINT32 *ebr); /* Get Current NIM Device Channel Bit-Error Rate */
	INT32(*get_PER)(struct nim_device *dev, UINT32 *per); /* Get Current NIM Device Channel Bit-Error Rate */
	INT32(*get_AGC)(struct nim_device *dev, UINT8 *agc); /* Get Current NIM Device Channel AGC Value */
	INT32(*get_fft_result)(struct nim_device *dev, UINT32 freq, UINT32 *start_addr); /* Get Current NIM Device Channel FFT spectrum result */
	INT32(*channel_search)(struct nim_device *dev, struct NIM_Channel_Search *param); //should be opened in DVBS

	const char *(*get_ver_infor)(struct nim_device *dev); /* Get Current NIM Device Version Number */

	INT32(*channel_change)(struct nim_device *dev, struct NIM_Channel_Change *param);
	INT32(*disable)(struct nim_device *dev); /* Disable Current NIM Device */
	INT32(*get_guard_interval)(struct nim_device *dev, UINT8 *gi);
	INT32(*get_fftmode)(struct nim_device *dev, UINT8 *fftmode);
	INT32(*get_modulation)(struct nim_device *dev, UINT8 *modulation);
	INT32(*get_spectrum_inv)(struct nim_device *dev, UINT8 *inv);
	INT32(*get_freq_offset)(struct nim_device *dev, INT32 *freq_offset);
	INT32(*get_bandwidth)(struct nim_device *dev, INT32 *bandwith);
	INT32(*get_frontend_type)(struct nim_device *dev, UINT8 *system); //dmq 2011/11/28 add
	INT32(*get_workmode)(struct nim_device *dev, INT32 *qpsk_mode);
	INT32(*get_PLP_num)(struct nim_device *dev, UINT8 *plp_num);
	INT32(*get_PLP_id)(struct nim_device *dev, UINT8 *plp_id);

	UINT32 i2c_type_id; //dmq modify
	UINT32 mutex_id; //dmq add
	UINT16 nim_idx; //dmq add tuner1/tuner2
};

#if 0
/*for dvbs front end, define diseqc support*/
#if ( SYS_PROJECT_FE == PROJECT_FE_DVBS||SYS_PROJECT_FE == PROJECT_FE_DVBS2)
#define DISEQC_SUPPORT
#endif
#endif /* 0 */

#endif /* __HLD_NIM_DEV_H__ */
