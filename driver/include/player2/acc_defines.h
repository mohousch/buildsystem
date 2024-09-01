/*$Id: acc_defines.h,v 1.6 2004/01/06 14:34:31 lassureg Exp $*/

#ifndef _ACC_DEFINES_H_
#define _ACC_DEFINES_H_

#include "packages.h"

enum eTrace
{
	TRACE_DISABLED,
	TRACE_KERNEL,
	TRACE_ERRORS,
	TRACE_WARNINGS,
	TRACE_DEBUG,
	TRACE_ALL
};

#ifdef _DEBUG
#ifndef TRACE_DEFAULT_LEVEL
#if !defined WIN32 && !defined _USE_POSIX_
#define TRACE_DEFAULT_LEVEL TRACE_ERRORS
#else
#define TRACE_DEFAULT_LEVEL TRACE_ALL
#endif
#endif
#else // release
#define trace_level TRACE_DISABLED
#endif

#ifdef WIN32
#define __restrict__
#endif

#ifdef _LXAUDIO_API_
#ifndef DP

#ifdef _DEBUG
#define DP printf
#else // #ifdef _DEBUG

#if !WIN32
void DebugPrintEmpty(const char *szFormat, ...);
#endif // #if !WIN32

#define DP while (0) DebugPrintEmpty
//#define DP

#endif // #ifdef _DEBUG

#endif // #ifndef DP
#endif // _LXAUDIO_API_ 
#define SYS_NB_MAX_CHANNELS (SYS_NPCMCHANS + SYS_NVCRCHANS)
#define SYS_NVCRCHANS 2
#define SYS_NPCMCHANS 8
#define SYS_NFRONTCHANS 3
#define SYS_NREARCHANS 4
#define SYS_NB_MAX_BITSTREAM_INFO 5
#define SYS_NB_MAX_PCMIN 4
#define SYS_NB_MAX_PCMOUT 4

#ifdef _OPTION_hdmi_
#define SWAP_CLFE 1
#else
#define SWAP_CLFE 0
#endif

enum eCodecCmd
{
	CMD_PLAY = 0,
	CMD_MUTE = 1,
	CMD_SKIP = 2,
	CMD_PAUSE = 4
};

enum eAccFlushingCmd
{
	ACC_FLUSHING_OFF = 0,
	ACC_FLUSHING_CMD = 1,
	ACC_FLUSHING_ACK = 2,
	ACC_FLUSHING_DONE = 4
};

enum eProcessType
{
	UNDEFINED,
	PCM_PROCESS,
	ENC_PROCESS,
	TC_PROCESS,
	DEC_PROCESS
};

enum eCodecCode
{
	_UNKNOWN_CODEC_,
	_AC3_,
	_DTS_,
	_MLP_,
	_PCM_,
	_MPGa_,
	_MP2a_,
	_MP2a_MC_,
	_MP2a_DAB_,
	_MP1_L3_,
	_MP2_L3_,
	_MP25_L3_,
	_MP2a_AAC_,
	_LPCM_VIDEO_,
	_LPCM_AUDIO_,
	_DV_,
	_WMA_,
	_HDCD_,
	_OGG_VORBIS_,
	_G729AD_,
	_MAD_,
	_WMAPROLSL_,
	_SBC_,
	_DTSHD_,
	_AMRWBP_,
	_DDPLUS_,
	_MP4_AAC_,
	_DTSLBR_,
	_TRUEHD_,
	_REAL_AUDIO_,
	_LAST_CODEC_
};

enum ePacketCode
{
	_ES_,
	_PES_MP2_,
	_PES_MP1_,
	_PES_DVD_VIDEO_,
	_PES_DVD_AUDIO_,
	_IEC_61937_,
	_PES_WMA_,
	_PES_WAV_,
	_PES_ST_,
	_PES_DVD_BD_,
	_PES_DVD_HD_,
	_UNKNOWN_PACKET_

};

enum eIec61937
{
	IEC61937_NULL_DATA_BURST,
	IEC61937_AC3,
	IEC61937_SMPTE_338M,
	IEC61937_PAUSE_BURST,
	IEC61937_MP1L1,
	IEC61937_MP1L2L3,
	IEC61937_MP2MC,
	IEC61937_MP2AAC,
	IEC61937_MP2L1LSF,
	IEC61937_MP2L2LSF,
	IEC61937_MP2L3LSF,
	IEC61937_DTS1,
	IEC61937_DTS2,
	IEC61937_DTS3,
	IEC61937_ATRAC,
	IEC61937_ATRAC2_3,
	IEC61937_RESERVED_16,
	IEC61937_DTS4 = 17,
	IEC61937_RESERVED_18,
	IEC61937_RESERVED_19,
	IEC61937_RESERVED_20,
	IEC61937_DDPLUS = 21,
	IEC61937_DTRUEHD = 22,
	IEC61937_RESERVED_23,
	IEC61937_RESERVED = IEC61937_RESERVED_23,
	IEC61937_RESERVED_24,
	IEC61937_RESERVED_25,
	IEC61937_RESERVED_26,
	IEC61937_RESERVED_27,
	IEC61937_RESERVED_28,
	IEC61937_RESERVED_29,
	IEC61937_RESERVED_30,
	IEC61937_RESERVED_31,

	// WARNING : the following types are not defined in IEC61937!!
	IEC60958_PCM = 32,
	IEC60958_DTS14 = 33, // 4096 sample frames (or not yet looked)
	IEC60958_DTS16 = 34,
	IEC60958_DTS1_14 = 35, // 512 sample frames
	IEC60958_DTS1_16 = 36,
	IEC60958_DTS2_14 = 37, // 1024 sample frames
	IEC60958_DTS2_16 = 38,
	IEC60958_DTS3_14 = 39, // 2048 sample frames
	IEC60958_DTS3_16 = 40,

	IEC60958_LAST
};

enum ePktStatus
{
	PACKET_SYNCHRONIZED,
	PACKET_NOT_SYNCHRONIZED,
	PACKET_SYNCHRO_NOT_FOUND
};

enum eMainChannelIdx
{
	MAIN_LEFT,
	MAIN_RGHT,
#if SWAP_CLFE==1 // C at 4th location after swap
	MAIN_LFE,
	MAIN_CNTR,
#else // C at 3rd location. 
	MAIN_CNTR,
	MAIN_LFE,
#endif
	MAIN_LSUR,
	MAIN_RSUR,
	MAIN_CSURL,
	MAIN_CSURR,
	AUX_LEFT,
	AUX_RGHT

};
#define MAIN_MSUR MAIN_LSUR
#define MAIN_M MAIN_CNTR
#define MAIN_V1 MAIN_LSUR
#define MAIN_V2 MAIN_RSUR

#define DEFAULT_INPUT_FILE "../test_vectors/test.pcm"
#define DEFAULT_OUTPUT_FILE "out.pcm"
#define DEFAULT_MAIN_OUTPUT_FILE "main_out.pcm"
#define DEFAULT_REPORT_FILE "../report/Lxaudio_Report.txt"
#define MAX_STRING_LENGTH 80

#define AC3_BLOCK_SIZE 256
enum eFsCode
{
	/* Range : 2^0 */
	FS48k,
	FS44k,
	FS32k,
	FS_reserved_3,
	/* Range : 2^1 */
	FS96k,
	FS88k,
	FS64k,
	FS_reserved_7,
	/* Range : 2^2 */
	FS192k,
	FS176k,
	FS128k,
	FS_reserved_11,
	/* Range : 2^3 */
	FS384k,
	FS352k,
	FS256k,
	FS_reserved_15,
	/* Range : 2^-2 */
	FS12k,
	FS11k,
	FS8k,
	FS_reserved_19,
	/* Range : 2^-1 */
	FS24k,
	FS22k,
	FS16k,
	FS_reserved_23,
	/* Range : 2^4 */
	FS768k,
	FS705k,
	FS512k,
	FS_reserved_27,

	FS_reserved,
	FS_ID = 0xFF

};

#define FSCODE_FUNDAMENTAL (0x03)
#define FSCODE_HARMONIC (~FSCODE_FUNDAMENTAL)

#define FSCODE_GET_FUNDAMENTAL(fscode) ((enum eFsCode) (fscode & FSCODE_FUNDAMENTAL))
#define FSCODE_GET_HARMONIC(fscode) ((enum eFsCode) (fscode & FSCODE_HARMONIC))
#define FSCODE_SET_FUNDAMENTAL(fscode, fundament) ((enum eFsCode) (FSCODE_GET_HARMONIC(fscode) | FSCODE_GET_FUNDAMENTAL(fundament)))
#define FSCODE_SET_HARMONIC(fscode, harmonic) ((enum eFsCode) (FSCODE_GET_FUNDAMENTAL(fscode) | FSCODE_GET_HARMONIC(harmonic))

#define MODE11 0
enum eAcMode
{
	MODE20t, /* 0 */
	MODE10, /* 1 */
	MODE20, /* 2 */
	MODE30, /* 3 */
	MODE21, /* 4 */
	MODE31, /* 5 */
	MODE22, /* 6 */
	MODE32, /* 7 */
	MODE23, /* 8 */
	MODE33, /* 9 */
	MODE24, /* A */
	MODE34, /* B :: CLR LssRss Lfe LsrRsr */
	MODE42, /* C */
	MODE44, /* D */
	MODE52, /* E :: CLR LsRs LcRc */
	MODE53, /* F */

	MODEk10_V1V2OFF, /* 10 */
	MODEk10_V1ON, /* 11 */
	MODEk10_V2ON, /* 12 */
	MODEk10_V1V2ON, /* 13 */

	MODE30_T100, /* 14 :: CLR Ch */
	MODE30_T200, /* 15 :: CLR LhRh*/
	MODE22_T010, /* 16 :: LR LsRs Ts */

	MODE32_T020, /* 17 :: CLR LsRs LhsRhs */
	MODE23_T100, /* 18 :: LR LsRs CsCh */
	MODE23_T010, /* 19 :: LR LsRs CsTs */

	MODEk_AWARE, /* 1A */ /* Default 2/0 */
	MODEk_AWARE10, /* 1B */
	MODEk_AWARE20, /* 1C */
	MODEk_AWARE30, /* 1D */

	MODE52_WIDE = 0x1E, /* 1E :: CLR LsRs LwRw */
	MODE34SS = 0x1F, /* 1F :: CLR LsRs LsrRsr */

	MODEk20_V1V2OFF, /* 20 */
	MODEk20_V1ON, /* 21 */
	MODEk20_V2ON, /* 22 */
	MODEk20_V1V2ON, /* 23 */
	MODEk20_V1Left, /* 24 */
	MODEk20_V2Right, /* 25 */

	MODE_undefined_26,
	MODE_undefined_27,
	MODE_undefined_28,
	MODE_undefined_29,
	MODE_undefined_2A,
	MODE24_DIRECT, /* 2B :: LR LsRs LdRd */
	MODE34_DIRECT, /* 2C :: CLR LsRs LdRd */
	MODE_undefined_2D,
	MODE_undefined_2E,
	MODE_undefined_2F,

	MODEk30_V1V2OFF, /* 30 */
	MODEk30_V1ON, /* 31 */
	MODEk30_V2ON, /* 32 */
	MODEk30_V1V2ON, /* 33 */
	MODEk30_V1Left, /* 34 */
	MODEk30_V2Right, /* 35 */

	MODE_undefined_36,
	MODE_undefined_37,
	MODE_undefined_38,
	MODE_undefined_39,
	MODE_undefined_3A,
	MODE_undefined_3B,
	MODE_undefined_3C,
	MODE_undefined_3D,
	MODE_undefined_3E,
	MODE_undefined_3F,

	MODE20LPCMA = 0x40,/* 40 */
	HDMI_MODE20 = 0x40,
	HDMI_MODE30,
	HDMI_MODE21,
	HDMI_MODE31,

	HDMI_MODE22,
	HDMI_MODE32,
	HDMI_MODE23,
	HDMI_MODE33,

	HDMI_MODE24,
	HDMI_MODE34,
	HDMI_MODE40,
	HDMI_MODE50,

	HDMI_MODE41,
	HDMI_MODE51,
	HDMI_MODE42,
	HDMI_MODE52,

	HDMI_MODE32_T100,
	HDMI_MODE32_T010,
	HDMI_MODE22_T200,
	HDMI_MODE42_WIDE,

	HDMI_MODE33_T010,
	HDMI_MODE33_T100,
	HDMI_MODE32_T110,
	HDMI_MODE32_T200,

	HDMI_MODE52_WIDE,
	HDMI_MODE_RESERV0x32,
	HDMI_MODE_RESERV0x34,
	HDMI_MODE_RESERV0x36,

	HDMI_MODE_RESERV0x38,
	HDMI_MODE_RESERV0x3A,
	HDMI_MODE_RESERV0x3C,
	HDMI_MODE_RESERV0x3E,

	HDMI_MODE_RESERVED = HDMI_MODE_RESERV0x32,

	MODE_1p1 = 0x60,          /* 60 */
	MODE11p20,                /* 60 */
	MODE10p20,                /* 61 */
	MODE20p20,                /* 62 */
	MODE30p20,                /* 63 */

	MODE_NOLFE_RAW = 0x7F,    /* 7F :: MODE_ID without LFE */

	MODE20t_LFE = 0x80,       /* 80 */
	MODE10_LFE,               /* 81 */
	MODE20_LFE,               /* 82 */
	MODE30_LFE,               /* 83 */
	MODE21_LFE,               /* 84 */
	MODE31_LFE,               /* 85 */
	MODE22_LFE,               /* 86 */
	MODE32_LFE,               /* 87 */
	MODE23_LFE,               /* 88 */
	MODE33_LFE,               /* 89 */
	MODE24_LFE,               /* 8A */
	MODE34_LFE,               /* 8B */
	MODE42_LFE,               /* 8C */
	MODE44_LFE,               /* 8D */
	MODE52_LFE,               /* 8E */
	MODE53_LFE,               /* 8F : WARNING :: 9 ch is not possible in 7.1 configuration */

	MODE30_LFE_T100 = 0x94,   /* 94 :: CLR Lfe Ch */
	MODE30_LFE_T200,          /* 95 :: CLR Lfe LhRh */
	MODE22_LFE_T010,          /* 96 :: LR Lfe LsRs Ts */
	MODE32_LFE_T020,          /* 97 :: CLR Lfe LsRs LhsRhs */
	MODE23_LFE_T100,          /* 98 :: LR Lfe LsRs CsCh */
	MODE23_LFE_T010,          /* 99 :: LR Lfe LsRs CsTs */

	MODE34SS_LFE = 0x9F,      /* 9F :: CLR LsRs Lfe LsrRsr */

	MODE24_LFE_DIRECT = 0xAB, /* AB :: LR LsRs LdRd Lfe */
	MODE34_LFE_DIRECT = 0xAC, /* AC :: CLR LsRs LdRd Lfe */

	MODE11p20_LFE = 0xE1,     /* E1 */
	MODE10p20_LFE,            /* E2 */
	MODE20p20_LFE,            /* E3 */
	MODE30p20_LFE,            /* E4 */

	MODE_ALL0 = 0xA0,
	MODE_ALL1,
	MODE_ALL2,
	MODE_ALL3,
	MODE_ALL4,
	MODE_ALL5,
	MODE_ALL6,
	MODE_ALL7,
	MODE_ALL8,
	MODE_ALL9,
	MODE_ALL10,

	MODE_ID = 0xFF            /* FF */
};

#define SYS_NB_DEFAULT_ACMOD 16
#define HDMI_NB_DEFAULT_ACMOD (HDMI_MODE_RESERVED - HDMI_MODE20)
#define MODE_HDMI HDMI_MODE20 // Range of first HDMI mode
#define MODE_HDMI_RANGE (HDMI_MODE_RESERVED & 0xF0) // Range of last defined HDMI mode

#define MODE40 HDMI_MODE40
#define MODE41 HDMI_MODE41
#define MODE50 HDMI_MODE50
#define MODE51 HDMI_MODE51
#define MODE42_WIDE HDMI_MODE42_WIDE
#define MODE52_WIDE HDMI_MODE52_WIDE
#define MODE22_T200 HDMI_MODE22_T200
#define MODE32_T100 HDMI_MODE32_T100
#define MODE32_T110 HDMI_MODE32_T110
#define MODE32_T200 HDMI_MODE32_T200
#define MODE32_T010 HDMI_MODE32_T010
#define MODE33_T010 HDMI_MODE33_T010
#define MODE33_T100 HDMI_MODE33_T100

#define MODE40_LFE (HDMI_MODE40 + MODE_LFE)
#define MODE41_LFE (HDMI_MODE41 + MODE_LFE)
#define MODE50_LFE (HDMI_MODE50 + MODE_LFE)
#define MODE51_LFE (HDMI_MODE51 + MODE_LFE)
#define MODE42_LFE_WIDE (HDMI_MODE42_WIDE + MODE_LFE)
#define MODE52_LFE_WIDE (HDMI_MODE52_WIDE + MODE_LFE)
#define MODE22_LFE_T200 (HDMI_MODE22_T200 + MODE_LFE)
#define MODE32_LFE_T100 (HDMI_MODE32_T100 + MODE_LFE)
#define MODE32_LFE_T110 (HDMI_MODE32_T110 + MODE_LFE)
#define MODE32_LFE_T200 (HDMI_MODE32_T200 + MODE_LFE)
#define MODE32_LFE_T010 (HDMI_MODE32_T010 + MODE_LFE)
#define MODE33_LFE_T010 (HDMI_MODE33_T010 + MODE_LFE)
#define MODE33_LFE_T100 (HDMI_MODE33_T100 + MODE_LFE)

#define MODE_ALL MODE_ALL0
#define MODE_SECOND_STEREO MODE11p20
#define ACMOD_BIT_LFE 7
#define ACMOD_BIT_AUX 15
#define MODE_LFE (1 << ACMOD_BIT_LFE)
#define MODE_AUX (1 << ACMOD_BIT_AUX)
#define MODE_BASE (0x7F)
#define MODE_AUX_BASE (MODE_AUX >> 8)
#define MODE_5_1 MODE32
#define MODE_6_1 MODE33
#define MODE_7_1 MODE52
#define ACMOD_GET_MAIN(acmod) (acmod & 0xFF)
#define ACMOD_GET_AUX(acmod) (acmod >> 8)
#define ACMOD_RESET_AUX(outmode) outmode = ACMOD_GET_MAIN(outmode)
#define ACMOD_SET_AUX(outmode, mode_aux) outmode = (mode_aux != 0) ? outmode | ((mode_aux | MODE_AUX_BASE ) << 8) : outmode
#define ACMOD_SET_LFE(outmode, inuse_lfe) outmode = (inuse_lfe == ACC_FALSE) ? outmode : (outmode | MODE_LFE)

#define ACMOD_CHECK_LFE(acmod) (acmod & MODE_LFE)
#define ACMOD_CHECK_AUX(acmod) (acmod & MODE_AUX)
#define ACMOD_CHECK_BASE(acmod) (acmod & MODE_BASE)
#define ACMOD_CHECK_5_1(acmod) (ACMOD_CHECK_BASE(acmod) <= MODE_5_1)
#define ACMOD_CHECK_6_1(acmod) (ACMOD_CHECK_BASE(acmod) <= MODE_6_1)
#define ACMOD_CHECK_7_1(acmod) (ACMOD_CHECK_BASE(acmod) <= MODE_7_1)
#define ACMOD_CHECK_HDMI(acmod) (((acmod & MODE_BASE) >= MODE_HDMI) && ((acmod & MODE_BASE) < HDMI_MODE_RESERVED))

/* dual modes definitions */
typedef enum eDualMode
{
	DUAL_LR, // Lmono in L, Rmono in R
	DUAL_LEFT_MONO, // Only Left Mono
	DUAL_RIGHT_MONO, // Only Right Mono
	DUAL_MIX_LR_MONO, // Mix Left and Right Mono

	/* Following modes to be used only for dual stereo case */

	DUAL_STEREO_LR, // Use LR Pair as stereo
	DUAL_STEREO_LsRs // Use LsRs Pair as stereo
} eDualMode;

enum eMixOutput
{
	MIX_MAIN,
	MIX_AUX,
	MIX_SPDIF_OUT,
	MIX_HDMI_OUT,

	// Do not edit beyond this line.
	MIX_NBMAX_OUT
};

#define MIX_MAIN_AND_AUX MIX_AUX+1

#define BYTES_PER_32BIT_WORD (4)
#define BYTES_PER_16BIT_WORD (2)
enum eWordSizeCode
{
	WS32,
	WS16,
	WS8,
	WS24
};

#define BYTES_PER_WORD(x) ( 4 >> (x)) // WARNING :: not valid for WS24!!

enum eBoolean
{
	ACC_FALSE,
	ACC_TRUE
};

enum
{
	NO,
	YES
};

enum eProcessApply
{
	ACC_DISABLED,
	ACC_ENABLED,
	ACC_AUTO
};

enum eEndianess
{
	ACC_BIG_ENDIAN16,
	ACC_LITTLE_ENDIAN16
};

enum eErrorCode
{
	ACC_EC_OK = 0,
	ACC_EC_ERROR = 1,
	ACC_EC_ERROR_SYNC = 2,
	ACC_EC_ERROR_CMP = 4,
	ACC_EC_END_OF_FILE = 8,
	ACC_EC_ERROR_MEM_ALLOC = 16,
	ACC_EC_ERROR_PARSE_COMMAND = 32,
	ACC_EC_EXIT = 64,
	ACC_EC_ERROR_CRC = 128,
	ACC_EC_ERROR_FATAL = 1024
};

#define NB_ERROR_MSG 8

enum ePtsDts
{
	NO_PTS_DTS,
	PTS_INTERPOLATED, // PTS_DTS_FLAG_FORBIDDEN,
	PTS_PRESENT,
	PTS_DTS_PRESENT
};

enum ePtsValidity
{
	PTS_INVALID,
	PTS_VALID,
	PTS_COMPUTED
};

enum eEmphasis
{
	EMPHASIS_DISABLED,
	EMPHASIS_50_15_US,
	EMPHASIS_RESERVED,
	EMPHASIS_CCITT
};

//! This Enum list provides the natural order in which a pcmprocessing sequence will be processed
enum ePcmId
{
	PCM_CMC_ID,
	PCM_RESAMPLE2X_ID,
	PCM_DeEMPH_ID,
	PCM_AC3Ex_ID,
	PCM_CUSTOM_MAIN_ID,
	PCM_PLII_ID,
	PCM_CSII_ID,
	PCM_CHSYN_ID,
	PCM_NEO_ID,
	PCM_TSXT_ID,
	PCM_OMNI_ID,
	PCM_STWD_ID,
	PCM_CUSTOM_N22_ID,
	PCM_DB_HEADPHONE_ID,
	PCM_ST_HEADPHONE_ID,
	PCM_CUSTOM_HEADPHONE_ID,
	PCM_DMIX_ID,
	PCM_DOWNSAMPLE_ID,
	PCM_KOKPS_ID, /* Pitch Shift */
	PCM_KOKVC_ID, /* Voice Cancel */
	PCM_TEMPO_ID,
	PCM_KOKMUSIC_ID,
	PCM_BASSMGT_ID,
	PCM_EQUALIZER_ID,
	PCM_VIQ_ID,
	PCM_CUSTOM_ID,
	PCM_LIMITER_ID,
	PCM_DCREMOVE_ID,
	PCM_DELAY_ID,
	PCM_SIGANALYSIS_ID,
	PCM_SFC_ID,
	PCM_UPSAMPLE_ID,
	PCM_ES2PES_ENCODER_ID,

	PCM_ENCODER_ID,
	PCM_SPDIFOUT_ID,
	PCM_BTSC_ID,
	PCM_31_ID,

	/* Do not edit beyond this comment */
	PCM_LAST_ID
};

enum eErrorStrmBuf
{
	STRMBUF_OK,
	STRMBUF_EOF
};

#define DEFAULT_FS FS48k
#define DEFAULT_BLOCK_SIZE AC3_BLOCK_SIZE
#define DEFAULT_NCH 2
#define DEFAULT_AUDIO_MODE 2

#define Q31_to_Q23(x) (x>>8)
#define Q23_to_Q31(x) (x<<8)

/* scaling factors ordering in tab downmix_value[3] */
/**** downmix scaling factors ****/

enum eAttenuation
{
	UNITY = 0x7FFF, /* DSPf2w16(1.0) */
	M3DB = 0x5A83, /* DSPf2w16(0.707106781) */
	M6DB = 0x4000, /* DSPf2w16(0.5) */
	M9DB = 0x2D41, /* DSPf2w16(0.35) */
	M4_5DB = 0x4C1C, /* DSPf2w16(0.594603557) */
	M1_5DB = 0x68F6, /* DSPf2w16(0.82) */
	M4_75DB = 0x4A3D /* DSPf2w16(0.58) */

};
enum eAttenuationsIdx
{
	unity,
	m3db,
	m6db,
	m9db,
	m4_5db,
	m1_5db,
	m4_75db
};

enum eMonitorMode
{
	MONITOR_DEFAULT,
	MONITOR_USER_DEFINED
};

enum eEventMonitoring
{
	TIM_DHIT,
	TIM_DMISS,
	TIM_DMISS_CYCLES,
	TIM_PFT_ISSUED,
	TIM_PFT_HITS,
	TIM_WB_HITS,
	TIM_IHIT,
	TIM_IMISS,
	TIM_IMISS_CYCLES,
	TIM_IBUF_INVALID,
	TIM_BUNDLES,
	TIM_LDST,
	TIM_TAKEN_BR,
	TIM_NOT_TAKEN_BR,
	TIM_EXCEPTIONS,
	TIM_INTERRUPTS,
	TIM_RESERVED_EVENT
};

enum eIecValidity
{
	IEC_BURST_VALID,
	IEC_BURST_NOT_VALID
};

#ifdef _little_endian
#define FOURCC(a,b,c,d) ( ( (U32) a << 0 ) +\
			  ( (U32) b << 8 ) +\
			  ( (U32) c << 16 ) +\
			  ( (U32) d << 24 ) \
			)
#else
#define FOURCC(a,b,c,d) ( ( (U32) a << 24 ) +\
			  ( (U32) b << 16 ) +\
			  ( (U32) c << 8 ) +\
			  ( (U32) d << 0 ) \
			)
#endif /* _little_endian */

enum eAccPurgeCache
{
	ACC_PURGE_OFF,
	ACC_PURGE_IN,
	ACC_PURGE_OUT,
	ACC_PURGE_IO
};

#ifndef _EXTERN_C_
#ifdef __cplusplus
#define _EXTERN_C_ extern "C"
#else
#define _EXTERN_C_ extern
#endif
#endif

#ifdef _ACC_AMP_SYSTEM_

#ifdef _LX_INLINE_

#include "ACF_Unpack/io_inline.c"

#else

_EXTERN_C_ short int Q31_to_Q15(int var32);

#ifdef _little_endian
_EXTERN_C_ int big_endian_int(int var32);
_EXTERN_C_ int big_endian_short_int(short int var16);

#define little_endian_int(x) x
#define little_endian_short_int(x) x

#else

_EXTERN_C_ int little_endian_int(int var32);
_EXTERN_C_ int little_endian_short_int(short int var16);

#define big_endian_int(x) x
#define big_endian_short_int(x) x

#endif /* _little_endian */
#endif /* _LX_INLINE_ */
#endif /* _ACC_AMP_SYSTEM_ */

#endif /* _ACC_DEFINES_H_ */
