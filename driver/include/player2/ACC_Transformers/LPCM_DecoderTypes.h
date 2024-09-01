/// $Id: LPCM_DecoderTypes.h,v 1.1 2003/11/26 17:03:08 lassureg Exp $
/// @file : MMEPlus/Interface/Transformers/Lpcm_DecoderTypes.h
///
/// @brief : LPCM Audio Decoder specific types for MME
///
/// @par OWNER: Gael Lassure
///
/// @author : Gael Lassure
///
/// @par SCOPE:
///
/// @date : 2003-07-04
///
/// &copy; 2003 ST Microelectronics. All Rights Reserved.
///

#ifndef _LPCM_DECODERTYPES_H_
#define _LPCM_DECODERTYPES_H_

#define DRV_MME_LPCM_DECODER_VERSION 0x041220

#include "PcmProcessing_DecoderTypes.h"
#include "acc_mmedefines.h"
#include "Audio_DecoderTypes.h"

// Number of frames to decode per MME transform operation
#define NUM_FRAMES_PER_TRANSFORM 1
#define LPCM_DEFAULT_CHANNEL_ASSIGNMENT 0xFF

enum eLPCMCapabilityFlags
{
	ACC_LPCM_DVD_VIDEO,
	ACC_LPCM_DVD_AUDIO,
	ACC_LPCM_CDDA,
	ACC_LPCM_8BIT
};

enum eAccLpcmWs
{
	ACC_LPCM_WS16,
	ACC_LPCM_WS20,
	ACC_LPCM_WS24,
	ACC_LPCM_WS_UNDEFINED,
	ACC_LPCM_WS32,
	ACC_LPCM_WS_NO_GR2 = 0xF,

	ACC_LPCM_WS8s = 0x10, // 8bit signed
	ACC_LPCM_WS8u = 0x11, // 8bit unsigned
	ACC_LPCM_WS16le = 0x10000, // 16bit little endian
	ACC_LPCM_WS16u = 0x20000, // 16bit unsigned big endian
	ACC_LPCM_WS16ule = 0x30000, // 16bit unsigned little endian
	ACC_LPCM_WS32be = 0x20000 | ACC_LPCM_WS32 // 32bit big endian
};

enum eAccLpcmMode
{
	ACC_LPCM_VIDEO,
	ACC_LPCM_AUDIO,
	ACC_RAW_PCM,
	ACC_LPCM_HD,
	ACC_LPCM_BD,
	ACC_LPCM_MS_ADPCM,
	ACC_LPCM_IMA_ADPCM,

	// do not edit below this line
	ACC_LAST_LPCM_MODE
};

enum eAccLpcmRspl
{
	ACC_LPCM_RSPL_48,
	ACC_LPCM_RSPL_96,
	ACC_LPCM_AUTO_RSPL,
	ACC_LPCM_NO_RSPL
};

enum eAccLpcmFs
{
	ACC_LPCM_FS48,
	ACC_LPCM_FS96,
	ACC_LPCM_FS192,
	ACC_LPCM_FS44 = 8,
	ACC_LPCM_FS88,
	ACC_LPCM_FS176,
	ACC_LPCM_FS_NO_GR2 = 0xF,
	ACC_LPCM_FS_UNDEFINED
};

enum eLpcmConfigIdx
{
	LPCM_MODE,
	LPCM_DRC_CODE,
	LPCM_DRC_ENABLE,
	LPCM_MUTE_FLAG,
	LPCM_EMPHASIS_FLAG,
	LPCM_NB_CHANNELS,
	LPCM_WS_CH_GR1,
	LPCM_WS_CH_GR2,
	LPCM_FS_CH_GR1,
	LPCM_FS_CH_GR2,
	LPCM_BIT_SHIFT_CH_GR2,
	LPCM_CHANNEL_ASSIGNMENT,
	LPCM_MIXING_PHASE,
	LPCM_NB_ACCESS_UNITS,
	LPCM_OUT_RESAMPLING,
	LPCM_NB_SAMPLES,

	/* Do not edit below this comment */
	LPCM_NB_CONFIG_ELEMENTS
};

typedef struct
{
	enum eAccDecoderId DecoderId;
	U32 StructSize;

	//U8 Config[LPCM_NB_CONFIG_ELEMENTS];
	U32 Config[LPCM_NB_CONFIG_ELEMENTS];
	//!< [ 0] : ACC_LPCM_VIDEO / ACC_LPCM_AUDIO
	//!< [ 1] : 8bit coefficient given in PES Private Audio Information
	//!< [ 2] : DRC Enable : ACC_MME_FALSE / ACC_MME_TRUE
	//!< [ 3] : Mute Flag : ACC_MME_FALSE / ACC_MME_TRUE
	//!< [ 4] : Emphasis Flag : ACC_MME_FALSE / ACC_MME_TRUE
	//!< [ 5] : nb channels
	//!< [ 6] : ( enum eAccLpcmWs ) WordSize chan group 1
	//!< [ 7] : ( enum eAccLpcmWs ) WordSize chan group 2
	//!< [ 8] : ( enum eAccLpcmFs ) Sampling Freq chan group 1
	//!< [ 9] : ( enum eAccLpcmFs ) Sampling Freq chan group 2
	//!< [10] : ( int ) Bit Shift chan group 2
	//!< [11] : ( int ) mixing phase
	//!< [12] : ( int ) NbAccessUnits
	//!< [13] : ( enum eAccLpcmRspl ) output resampling
	//!< [14] : ( int ) NbSamples to process

} MME_LxLpcmConfig_t;

/*
 These are the indicies of the parameters that can be passed to update for each command
 the number of samples to process and the Drc (that can be also be set through a set global command
*/
#define ACC_LPCM_FRAME_PARAMS_NBSAMPLES_INDEX 0
#define ACC_LPCM_FRAME_PARAMS_DRC_INDEX 1

// Define the FrameStatus
enum eLpcmFrameStatus
{
	LPCM_STATUS_0, // Reserved
	LPCM_STATUS_NBSAMPLES,
	LPCM_STATUS_2, // Reserved
	LPCM_STATUS_3 // Reserved
};

#endif /* _LPCM_DECODERTYPES_H_ */
