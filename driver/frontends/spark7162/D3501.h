/*****************************************************************************
* Copyright (C)2003 Ali Corporation. All Rights Reserved.
*
* File: nim_s3501.h
*
* Description: Header file in LLD.
* History:
* Date Athor Version Reason
* ============ ============= ========= =================
* 1. 05/18/2006 Berg Ver 0.1 Create file for S3501 DVBS2 project
*
*****************************************************************************/

#ifndef __D3501_H__
#define __D3501_H__

//#include "Porting.h"
//#define NIM_3501_FUNC_EXT

typedef char INT8;
typedef unsigned char UINT8;
typedef char S8;
typedef unsigned char U8;

typedef short INT16;
typedef unsigned short UINT16;
typedef short S16;
typedef unsigned short U16;

//typedef long INT32;
//typedef unsigned long UINT32;
typedef int INT32;
typedef unsigned int UINT32;
typedef int S32;
typedef unsigned int U32;

//typedef unsigned int IOARCH_Handle_t;
typedef unsigned int YW_ErrorType_T;

typedef unsigned long long UINT64;
typedef unsigned long long U64;

typedef long long INT64;
typedef long long S64;

#ifndef DEFINED_BOOL
#define DEFINED_BOOL
typedef int BOOL;
/* BOOL type constant values */
#ifndef TRUE
#define TRUE (1 == 1)
#endif
#ifndef FALSE
#define FALSE (!TRUE)
#endif
#endif

#ifndef NULL
#define NULL 0
#endif

typedef const char *ST_Revision_t;
#define YWTUNERi_MAX_TUNER_NUM 4

#define YW_NO_ERROR 0
#define YWHAL_ERROR_NO_MEMORY -1
#define YWHAL_ERROR_INVALID_HANDLE -2
#define YWHAL_ERROR_BAD_PARAMETER -3
#define YWHAL_ERROR_FEATURE_NOT_SUPPORTED -4
#define YWHAL_ERROR_UNKNOWN_DEVICE -5

#if 0
#define SUCCESS 0
#define EPERM 1 /* Operation not permitted */
#define ENOENT 2 /* No such file or directory */
#define ESRCH 3 /* No such process */
#define EINTR 4 /* Interrupted system call */
#define EIO 5 /* Input/output error */
#define ENXIO 6 /* Device not configured */
#define E2BIG 7 /* Argument list too long */
#define ENOEXEC 8 /* Exec format error */
#define EBADF 9 /* Bad file descriptor */
#define ECHILD 10 /* No child processes */
#define EDEADLK 11 /* Resource deadlock avoided */
#define ENOMEM 12 /* Cannot allocate memory */
#define EACCES 13 /* Permission denied */
#define EFAULT 14 /* Bad address */
#define ENOTBLK 15 /* Block device required */
#define EBUSY 16 /* Device busy */
#define EEXIST 17 /* File exists */
#define EXDEV 18 /* Cross-device link */
#define ENODEV 19 /* Operation not supported by device */
#define ENOTDIR 20 /* Not a directory */
#define EISDIR 21 /* Is a directory */
#define EINVAL 22 /* Invalid argument */
#define ENFILE 23 /* Too many open files in system */
#define EMFILE 24 /* Too many open files */
#define ENOTTY 25 /* Inappropriate ioctl for device */
#define ETXTBSY 26 /* Text file busy */
#define EFBIG 27 /* File too large */
#define ENOSPC 28 /* No space left on device */
#define ESPIPE 29 /* Illegal seek */
#define EROFS 30 /* Read-only filesystem */
#define EMLINK 31 /* Too many links */
#define EPIPE 32 /* Broken pipe */
#endif

#define FFT_BITWIDTH 10
#define STATISTIC_LENGTH 2

//average length of data to determine threshold
//0:2;1:4;2:8

#define MAX_CH_NUMBER 32//maximum number of channels that can be stored
#define s3501_LOACL_FREQ 5150
#define s3501_DEBUG_FLAG 0
#define QPSK_TUNER_FREQ_OFFSET 4
#define M3501_IQ_AD_SWAP 0x04
#define M3501_EXT_ADC 0x02
#define M3501_QPSK_FREQ_OFFSET 0x01
#define M3501_I2C_THROUGH 0x08
#define M3501_NEW_AGC1 0x20
#define M3501_POLAR_REVERT 0x10
#define M3501_1BIT_MODE 0x00
#define M3501_2BIT_MODE 0x40
#define M3501_4BIT_MODE 0x80
#define M3501_8BIT_MODE 0xc0
#define M3501_AGC_INVERT 0x100
//#define M3501_SPI_SSI_MODE 0x200 8bit mode(SPI) 1bit mode(SSI)
#define M3501_USE_188_MODE 0x400
#define M3501_DVBS_MODE 0x00
#define M3501_DVBS2_MODE 0x01
#define M3501_SIGNAL_DISPLAY_LIN 0x800
#define NIM_S3501_BASE_IO_ADR 0xB8003000

// For M3501 , 13.5--> 99MHz
// For S3501D, 13.5--> 90MHz
// Here the CRYSTAL_FREQ is very different with M3501
#define CRYSTAL_FREQ 135

#define NIM_GET_DWORD(i) (*(volatile U32 *)(i))
#define NIM_SET_DWORD(i,d) (*(volatile U32 *)(i)) = (d)

#define NIM_GET_WORD(i) (*(volatile U16 *)(i))
#define NIM_SET_WORD(i,d) (*(volatile U16 *)(i)) = (d)

#define NIM_GET_BYTE(i) (*(volatile U8 *)(i))
#define NIM_SET_BYTE(i,d) (*(volatile U8 *)(i)) = (d)

#define S3501_FREQ_OFFSET 1
#define LNB_LOACL_FREQ 5150
#define AS_FREQ_MIN 900
#define AS_FREQ_MAX 2200

#define NIM_OPTR_CHL_CHANGE0 0x70
#define NIM_OPTR_CHL_CHANGE 0x00
#define NIM_OPTR_SOFT_SEARCH 0x01
#define NIM_OPTR_FFT_RESULT 0x02
#define NIM_OPTR_DYNAMIC_POW 0x03
#define NIM_OPTR_DYNAMIC_POW0 0x73
#define NIM_OPTR_IOCTL 0x04
#define NIM_OPTR_HW_OPEN 0x05
#define NIM_OPTR_HW_CLOSE 0x06

#define NIM_DEMOD_CTRL_0X50 0x50
#define NIM_DEMOD_CTRL_0X51 0x51
#define NIM_DEMOD_CTRL_0X90 0x90
#define NIM_DEMOD_CTRL_0X91 0x91
#define NIM_DEMOD_CTRL_0X02 0x02
#define NIM_DEMOD_CTRL_0X52 0x52

#define NIM_SIGNAL_INPUT_OPEN 0x01
#define NIM_SIGNAL_INPUT_CLOSE 0x02

#define NIM_LOCK_STUS_NORMAL 0x00
#define NIM_LOCK_STUS_SETTING 0x01
#define NIM_LOCK_STUS_CLEAR 0x02

#define NIM_FLAG_CHN_CHG_START (1<<0)
#define NIM_FLAG_CHN_CHANGING (1<<1)
#define NIM_SWITCH_TR_CR 0x01
#define NIM_SWITCH_RS 0x02
#define NIM_SWITCH_FC 0x04
#define NIM_SWITCH_HBCD 0x08

#define TS_DYM_HEAD0 0x47
#define TS_DYM_HEAD1 0x1f
#define TS_DYM_HEAD2 0xff
#define TS_DYM_HEAD3 0x10
#define TS_DYM_HEAD4 0x00

#define YWTRANCE(x, y...) printk(y)

#define YWOSTRACE(x)

//#define YWOSTRACE(x) YWTRANCE x
#define YWLIB_Memset memset
#define Kzalloc(size) kzalloc(size, GFP_KERNEL)

/* 3501 register define */
enum NIM3501_REGISTER_ADDRESS
{
	R00_CTRL = 0x00, // NIM3501 control register
	R01_ADC = 0x01, // ADC Configuration Register
	R02_IERR = 0x02, // Interrupt Events Register
	R03_IMASK = 0x03, // Interrupt Mask Register
	R04_STATUS = 0x04, // Status Register
	R05_TIMEOUT_TRH = 0x05, // HW Timeout Threshold Register(LSB)
	R07_AGC1_CTRL = 0x07, // AGC1 reference value register
	R0A_AGC1_LCK_CMD = 0x0a, // AGC1 lock command register
	R10_DCC_CFG = 0x10, // DCC Configure Register
	R11_DCC_OF_I = 0x11, // DCC Offset I monitor Register
	R12_DCC_OF_Q = 0x12, // DCC Offset Q monitor Register
	R13_IQ_BAL_CTRL = 0x13, // IQ Balance Configure Register
	R15_FLT_ROMINDX = 0x15, // Filter Bank Rom Index Register
	R16_AGC2_REFVAL = 0x16, // AGC2 Reference Value Register
	R17_AGC2_CFG = 0x17, // AGC2 configure register
	R18_TR_CTRL = 0x18, // TR acquisition gain register
	R1B_TR_TIMEOUT_BAND = 0x1b, // TR Time out band register
	R21_BEQ_CTRL = 0x21, // BEQ Control REgister
	R22_BEQ_CMA_POW = 0x22, // BEQ CMA power register
	R24_MATCH_FILTER = 0x24, // Match Filter Register
	R25_BEQ_MASK = 0x25, // BEQ Mask Register
	R26_TR_LD_LPF_OPT = 0x26, // TR LD LPF Output register
	R28_PL_TIMEOUT_BND = 0x28, // PL Time out Band REgister
	R2A_PL_BND_CTRL = 0x2a, // PL Time Band Control
	R2E_PL_ANGLE_UPDATE = 0x2e, // PL Angle Update High/Low limit register
	R30_AGC3_CTRL = 0x30, // AGC3 Control Register
	R33_CR_CTRL = 0x33, // CR DVB-S/DVBS-S2 CONTROL register
	R45_CR_LCK_DETECT = 0x45, // CR lock detecter lpf monitor register
	R47_HBCD_TIMEOUT = 0x47, // HBCD Time out band register
	R48_VITERBI_CTRL = 0x48, // Viterbi module control register
	R57_LDPC_CTRL = 0x57, // LDPC control register
	R5B_ACQ_WORK_MODE = 0x5b, // Acquiescent work mode register
	R5C_ACQ_CARRIER = 0x5c, // Acquiescent carrier control register
	R5F_ACQ_SYM_RATE = 0x5f, // Acquiescent symbol rate register
	R62_FC_SEARCH = 0x62, // FC Search Range Register
	R64_RS_SEARCH = 0x64, // RS Search Range Register
	R66_TR_SEARCH = 0x66, // TR Search Step register
	R67_VB_CR_RETRY = 0x67, // VB&CR Maximum Retry Number Register
	R68_WORK_MODE = 0x68, // Work Mode Report Register
	R69_RPT_CARRIER = 0x69, // Report carrier register
	R6C_RPT_SYM_RATE = 0x6c, // report symbol rate register
	R6F_FSM_STATE = 0x6f, // FSM State Moniter Register
	R70_CAP_REG = 0x70, // Capture Param register
	R74_PKT_STA_NUM = 0x74, // Packet Statistic Number Register
	R76_BIT_ERR = 0x76, // Bit Error Register
	R79_PKT_ERR = 0x79, // Packet Error Register
	R7B_TEST_MUX = 0x7b, // Test Mux Select REgister
	R7C_DISEQC_CTRL = 0x7c, // DISEQC Control Register
	R86_DISEQC_RDATA = 0x86, // Diseqc data for read
	R8E_DISEQC_TIME = 0x8e, // Diseqc time register
	R90_DISEQC_CLK_RATIO = 0x90,// Diseqc clock ratio register
	R97_S2_FEC_THR = 0x97, // S2 FEC Threshold register
	R99_H8PSK_THR = 0x99, // H8PSK CR Lock Detect threshold register
	R9C_DEMAP_BETA = 0x9c, // Demap Beta register
	RA0_RXADC_REG = 0xa0, // RXADC ANATST/POWER register
	RA3_CHIP_ID = 0xa3, // Chip ID REgister
	RA5_VER_ID = 0xa5, // version ID register
	RA7_I2C_ENHANCE = 0xa7, // I2C Enhance Register
	RA8_M90_CLK_DCHAN = 0xa8, // M90 clock delay chain register
	RA9_M180_CLK_DCHAN = 0xa9, // M180 Clock delay chain register
	RAA_S2_FEC_ITER = 0xaa, // S2 FEC iteration counter register
	RAD_TSOUT_SYMB = 0xad, // ts out setting SYMB_PRD_FORM_REG
	RAF_TSOUT_PAD = 0xaf, // TS out setting and pad driving register
	RB0_PLL_CONFIG = 0xb0, // PLL configure REgister
	RB1_TSOUT_SMT = 0xb1, // TS output Setting and Pad driving
	RB3_PIN_SHARE_CTRL = 0xb3, // Pin Share Control register
	RB5_CR_PRS_TRA = 0xb5, // CR DVB-S/S2 PRS in Tracking State
	RB6_H8PSK_CETA = 0xb6, // H8PSK COS/SIN Ceta Value Register
	RB8_LOW_RS_CLIP = 0xb8, // Low RS Clip Value REgister
	RBA_AGC1_REPORT = 0xba, // AGC1 report register
	RBD_CAP_PRM = 0xbd, // Capture Config/Block register
	RBF_S2_FEC_DBG = 0xbf, // DVB-S2 FEC Debug REgister
	RC0_BIST_LDPC_REG = 0xc0, // LDPC Average Iteration counter register
	RC1_DVBS2_FEC_LDPC = 0xc1, // DVBS2 FEC LDPC Register
	RC8_BIST_TOLERATOR = 0xc8, // 0xc0 Tolerator MBIST register
	RC9_CR_OUT_IO_RPT = 0xc9, // Report CR OUT I Q
	// for s3501B
	RCB_I2C_CFG = 0xcb, // I2C Slave Configure Register
	RCC_STRAP_PIN_CLOCK = 0xcc, // strap pin and clock enable register
	RCD_I2C_CLK = 0xcd, // I2C AND CLOCK ENABLE REGISTER
	RCE_TS_FMT_CLK = 0xce, // TS Format and clock enable register
	RD0_DEMAP_NOISE_RPT = 0xd0, // demap noise rtp register
	RD3_BER_REG = 0xd3, // BER register
	RD6_LDPC_REG = 0xd6, // LDPC register
	RD7_EQ_REG = 0xd7, // EQ register
	RD8_TS_OUT_SETTING = 0xd8, // TS output setting register
	RD9_TS_OUT_CFG = 0xd9, // BYPASS register
	RDA_EQ_DBG = 0xda, // EQ Debug Register
	RDC_EQ_DBG_TS_CFG = 0xdc, // EQ debug and ts config register
	RDD_TS_OUT_DVBS = 0xdd, // TS output dvbs mode setting
	RDF_TS_OUT_DVBS2 = 0xdf, // TS output dvbs2 mode setting
};

struct nim_s3501_TskStatus
{
	U32 m_lock_flag;
//	ID m_task_id;//lwj remove
	U32 m_sym_rate;
	U8 m_work_mode;
	U8 m_map_type;
	U8 m_code_rate;
	U8 m_info_data;
};
struct nim_s3501_TParam
{
	int t_last_snr;
	int t_last_iter;
	int t_aver_snr;
	int t_snr_state;
	int t_snr_thre1;
	int t_snr_thre2;
	int t_snr_thre3;
	int t_phase_noise_detected;
	int t_dynamic_power_en;
	U32 phase_noise_detect_finish;
	U32 t_reg_setting_switch;
	U8 t_i2c_err_flag;
};
struct nim_s3501_LStatus
{
	U16 nim_s3501_sema;
	int ret;
	U8 s3501_autoscan_stop_flag;
	U8 s3501_chanscan_stop_flag;
	U32 old_ber ;
	U32 old_per ;
	U32 old_ldpc_ite_num;
	U8 *ADCdata;// = (unsigned char *)__MM_DMX_FFT_START_BUFFER;//[2048];
	U8 *ADCdata_raw_addr;
	int m_Freq[256];
	U32 m_Rs[256];
	int FFT_I_1024[1024];
	int FFT_Q_1024[1024];
	U8 m_CRNum;
	U32 m_CurFreq;
	U8 c_RS ;
	U32 m_StepFreq;
//	pfn_nim_reset_callback m_pfn_reset_s3501;//lwj remove
	U8 m_enable_dvbs2_hbcd_mode;
	U8 m_dvbs2_hbcd_enable_value;
	U8 s3501d_lock_status;
	U32 phase_err_check_status;
	U32 m_s3501_type;
	U32 m_s3501_sub_type;
	U32 m_setting_freq;
	U32 m_Err_Cnts;
	U8 m_hw_timeout_thr;
};

#define HLD_MAX_NAME_SIZE 16 /* Max device name length */
struct nim_device
{
//	UINT32 type; /* Interface hardware type */
//	INT8 name[HLD_MAX_NAME_SIZE]; /* Device name */
	UINT32 base_addr; /* Demodulator address */
//	UINT32 Tuner_select; /* I2C TYPE for TUNER select */
//	UINT16 flags; /* Interface flags, status and ability */
	//IOARCH_Handle_t DemodIOHandle;
	/* Hardware privative structure */
	void *priv; /* pointer to private data */
	/* Functions of this dem device */
	UINT32 i2c_type_id; //dmq modify
	UINT16 nim_idx; //dmq add tuner1/tuner2
};
struct QPSK_TUNER_CONFIG_EXT
{
	UINT16 wTuner_Crystal; /* Tuner Used Crystal: in KHz unit */
	UINT8 cTuner_Base_Addr; /* Tuner BaseAddress for Write Operation: (BaseAddress + 1) for Read */
	UINT8 cTuner_Out_S_D_Sel; /* Tuner Output mode Select: 1 --> Single end, 0 --> Differential */
	UINT32 i2c_type_id; /*i2c type and dev id select. bit16~bit31: type, I2C_TYPE_SCB/I2C_TYPE_GPIO. bit0~bit15:dev id, 0/1.*/
};
struct QPSK_TUNER_CONFIG_DATA
{
	UINT16 Recv_Freq_Low;
	UINT16 Recv_Freq_High;
	UINT16 Ana_Filter_BW;
	UINT8 Connection_config;
	UINT8 Reserved_byte;
	UINT8 AGC_Threshold_1;
	UINT8 AGC_Threshold_2;
	UINT16 QPSK_Config;/*bit0:QPSK_FREQ_OFFSET,bit1:EXT_ADC,bit2:IQ_AD_SWAP,bit3:I2C_THROUGH,bit4:polar revert bit5:NEW_AGC1,bit6bit7:QPSK bitmode:
 00:1bit,01:2bit,10:4bit,11:8bit*/
};
/*external demodulator config parameter*/
struct EXT_DM_CONFIG
{
	UINT32 i2c_base_addr;
	UINT32 i2c_type_id;
	UINT32 dm_crystal;
	UINT32 dm_clock;
	UINT32 polar_gpio_num;
	UINT32 lock_polar_reverse;
};
struct nim_s3501_private
{
	int (*nim_Tuner_Init)(U8 *, struct QPSK_TUNER_CONFIG_EXT *); // Tuner Initialization Function
	int (*nim_Tuner_Control)(U8, U32, U32); // Tuner Parameter Configuration Function
	int (*nim_Tuner_Status)(U8, BOOL *);
	struct QPSK_TUNER_CONFIG_DATA Tuner_Config_Data;
	U32 tuner_id;
	struct i2c_adapter *i2c_adap; /* i2c bus of the tuner */
	U32 i2c_type_id;
	U32 polar_gpio_num;
	U32 sys_crystal;
	U32 sys_clock;
	U16 pre_freq ;
	U16 pre_sym ;
	S8 autoscan_stop_flag;
//	struct nim_device_stats stats;
	U8 chip_id;
	struct EXT_DM_CONFIG ext_dm_config;
	struct nim_s3501_LStatus ul_status;
	int ext_lnb_id;
	int (*ext_lnb_control)(U8, U32, U32);
	struct nim_s3501_TskStatus tsk_status;
	struct nim_s3501_TParam t_Param;
	U32 cur_freq;
	U32 cur_sym;
	U32 flag_id;
	BOOL bLock;
};

struct QPSK_TUNER_CONFIG_API
{
	/* Extension struct for Tuner Configuration */
	struct QPSK_TUNER_CONFIG_EXT tuner_config;
};

//lwj add begin
#define EXT_QPSK_MODE_SPI 0
#define EXT_QPSK_MODE_SSI 1
#define NIM_CHIP_ID_M3501A 0x350100C0
#define NIM_CHIP_ID_M3501B 0x350100D0
#define NIM_CHIP_SUB_ID_S3501D 0x00
#define NIM_CHIP_SUB_ID_M3501B 0xC0
#define NIM_FREQ_RETURN_REAL 0
#define NIM_FREQ_RETURN_SET 1
//lwj add end

int nim_s3501_FFT(struct nim_device *dev, U32 startFreq);
U32 nim_s3501_get_CURFreq(struct nim_device *dev);
U8 nim_s3501_get_CRNum(struct nim_device *dev);
void nim_s3501_set_FFT_para(struct nim_device *dev);

INT32 nim_s3501_reg_get_freq(struct nim_device *dev, UINT32 *freq);//
INT32 nim_s3501_reg_get_symbol_rate(struct nim_device *dev, UINT32 *sym_rate);
INT32 nim_s3501_reg_get_code_rate(struct nim_device *dev, UINT8 *code_rate);

void nim_s3501_clear_int(struct nim_device *dev);
void nim_s3501_set_CodeRate(struct nim_device *dev, U8 coderate);
void nim_s3501_set_RS(struct nim_device *dev, UINT32 rs);;
void nim_s3501_set_freq_offset(struct nim_device *dev, INT32 delfreq);

INT32 nim_s3501_get_bitmode(struct nim_device *dev, UINT8 *bitMode);

INT32 nim_reg_read(struct nim_device *dev, UINT8 bMemAdr, UINT8 *pData, UINT8 bLen);
INT32 nim_reg_write(struct nim_device *dev, UINT8 bMemAdr, UINT8 *pData, UINT8 bLen);

S32 PowOf2(S32 number);

//lwj add end
#endif // __LLD_NIM_S3501_H__ */

