/// $Id: Karaoke_ProcessorTypes.h,v 1.7 2003/11/26 17:03:08 lassureg Exp $
/// @file : Karaoke_ProcessorTypes.h
///
/// @brief : Decoder PostProcessing specific types for MME
///
/// @par OWNER: Gael Lassure
///
/// @author : Gael Lassure
///
/// @par SCOPE:
///
/// @date : 2003-08-18
///
/// &copy; 2003 ST Microelectronics. All Rights Reserved.
///

#ifndef _KARAOKE_PROCESSORTYPES_H_
#define _KARAOKE_PROCESSORTYPES_H_

#define DRV_MME_KARAOKE_VOICE_LINE_VERSION 0x031030

#include "acc_mmedefines.h"
//#include "mme_api.h"

// Number of frames to decode per MME transform operation
#define NUM_FRAMES_PER_TRANSFORM 1

//! Additional decoder capability structure
typedef struct
{
	U32 StructSize; //!< Size of this structure
	U32 DecoderCapabilityFlags;
	U32 DecoderCapabilityExtFlags;
	U32 PcmProcessorCapabilityFlags;
}
MME_LxKokVoiceLineTransformerInfo_t;

enum eKOKVLConfigIdx
{
	KOKVL_DELAY,
	KOKVL_BETA,
	KOKVL_ATTENUATION,
	KOKVL_UPSAMPLE,
	KOKVL_VOICELINE_EFFECT,
	KOKVL_LEFT_GAIN,
	KOKVL_RGHT_GAIN,
	KOKVL_CHORUS_VOICE,
	KOKVL_FIRST_TIME,
	KOKVL_VOICELINE_ENERGY,

	/* Do not edit beyond this comment */
	KOKVL_NB_CONFIG_ELEMENTS
};

typedef struct
{
	enum eAccDecPcmProcId Id; //!< ID of this processing
	U32 StructSize; //!< Size of this structure
	enum eAccProcessApply Apply;

	U32 Config[KOKVL_NB_CONFIG_ELEMENTS];
	//!< [0] : Delay : if 0 then disable echo!
	//!< [1] : Beta coefficient
	//!< [2] : Attenuation factor
	//!< [3] : SRC Information : SamplingFreqIn[0..7] | SamplingFreqOut[8..15]
	//!< [4] : Left Gain [Q15]
	//!< [5] : Right Gain [Q15]
}
MME_KokVoiceLineGlobalParams_t;

typedef struct
{
	U32 StructSize; //!< Size of this structure

	enum eAccAcMode AudioModeIn; //!< AudioConfiguration Mode of Input PcmBuffer
	enum eAccFsCode SamplingFreqIn; //!< Sampling Frequency of Input PcmBuffer

	MME_KokVoiceLineGlobalParams_t KokvlParams;

}
MME_LxKaraokeVoiceLineTransformerGlobalParams_t;

typedef struct
{
	U32 StructSize; //!< Size of this structure

	//! System Init
	enum eAccProcessApply CacheFlush; //!< If ACC_DISABLED then the cached data aren't sent back at the end of the command

	U8 NbChannelsIn; //!< Number of Interlaced Channels in Input PcmBuffers
	U8 NbChannelsOut; //!< Number of Interlaced Channels in Output PcmBuffers

	MME_LxKaraokeVoiceLineTransformerGlobalParams_t GlobalParams; //!< Main Params

}
MME_LxKaraokeVoiceLineTransformerInitParams_t;

//! This structure must be passed when sending the TRANSFORM command to the decoder

typedef struct
{
	U32 NumInputBuffers; //!< input MME_DataBuffer_t structures, even if it's zero
	U32 NumOutputBuffers; //!< output MME_DataBuffer_t structures, even if it's zero
	MME_DataBuffer_t buffers[NUM_FRAMES_PER_TRANSFORM * 2]; //!< The input/output buffers

}
MME_LxKaraokeVoiceLineTransformerFrameParams_t;

typedef struct
{
	U32 StructSize; //!< Size of this structure

	int NbOutSamples;
	enum eAccAcMode AudioMode;
	enum eAccFsCode SamplingFreq;
}
MME_KaraokeVoiceLineTransformerFrameDecodeStatusParams_t;

#endif // _KARAOKE_PROCESSORTYPES_H_
