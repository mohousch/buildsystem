/// $Id: PcmProcessing_DecoderTypes.h,v 1.9 2003/11/26 17:03:09 lassureg Exp $
/// @file : MMEPlus/Interface/Transformers/PcmProcessing_DecoderTypes.h
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

#ifndef _AUDIO_GENERATORTYPES_H_
#define _AUDIO_GENERATORTYPES_H_

#include "acc_mmedefines.h"
//#include "mme_api.h"

#define DRV_MME_AUDIO_GENERATOR_VERSION 0x041010

#define AUDIO_GENERATOR_MAX_CONFIG_SIZE 8

// Generator types
enum eAudioGeneratorId
{
	ACC_BEEPTONE_ID = (ACC_AUDIOGENERATOR_MT << 8),
	ACC_PINKNOISE_ID,

	// do not edit below this line
	ACC_LAST_GENERATOR_ID,
	ACC_UNKNOWN_GENERATOR_ID
};

// Specific generator config
typedef struct
{
	enum eAudioGeneratorId Id;
	int StructSize;

	int config[AUDIO_GENERATOR_MAX_CONFIG_SIZE];
} tGeneratorConfig;

#define BEEP_TONE_NB_MAX_CHANNELS 2
typedef struct
{
	enum eAudioGeneratorId Id;
	int StructSize;

	enum eAccAcMode AudioMode; //!< MODE20 / MODE10
	int Frequency[BEEP_TONE_NB_MAX_CHANNELS];
	int Volume [BEEP_TONE_NB_MAX_CHANNELS];
} tGenBeepToneConfig;

typedef struct
{
	enum eAudioGeneratorId Id;
	int StructSize;

	enum eAccMainChannelIdx OutChannel;
	int OutVolume;
} tGenPinkNoiseConfig;

//! audio generator capability structure
typedef struct
{
	U32 StructSize; //!< Size of this structure
	U32 GeneratorCapabilityFlags;
	U32 GeneratorCapabilityExtFlags;
	U32 PcmProcessorCapabilityFlags;
} MME_AudioGeneratorInfo_t;

typedef struct
{
	U32 StructSize; //!< Size of this structure

	enum eAccFsCode SamplingFreq; //!< Sampling Frequency of Input PcmBuffer
	int NbSamples; //!< Number of output samples

	tGeneratorConfig Config; //!< Specific Audio Generator config : tPinkNoiseConfig or tBeepToneConfig

} MME_AudioGeneratorGlobalParams_t;

typedef struct
{
	U32 NumInputBuffers; //!< input MME_DataBuffer_t structures, even if it's zero
	U32 NumOutputBuffers; //!< output MME_DataBuffer_t structures, even if it's zero
	MME_AudioGeneratorGlobalParams_t GlobalParams; //!< Main Params

} MME_AudioGeneratorUpdateGlobalParams_t;

typedef struct
{
	U32 StructSize; //!< Size of this structure
	enum eAccProcessApply CacheFlush; //!< If ACC_DISABLED then the cached data aren't sent back at th

	U8 NbChannelsOut; //!< Number of Interlaced Channels in Main Output PcmBuffers

	MME_AudioGeneratorGlobalParams_t GlobalParams; //!< Main Params

} MME_AudioGeneratorInitParams_t;

//! This structure must be passed when sending the TRANSFORM command to the decoder

typedef struct
{
	U32 NumInputBuffers; //!< input MME_DataBuffer_t structures, even if it's zero
	U32 NumOutputBuffers; //!< output MME_DataBuffer_t structures, even if it's zero
	MME_DataBuffer_t buffers[NUM_FRAMES_PER_TRANSFORM * 2]; //!< The input/output buffers
	// after NumInputBuffers input buffers the output buffers are also here
	// U32 NumberOfFrames; //!< AC3 frames contained in these buffers
} MME_AudioGeneratorFrameParams_t;

typedef struct
{
	U32 StructSize; //!< Size of this structure
} MME_AudioGeneratorFrameStatus_t;

#endif // _PCMPROCESSING_DECODERTYPES_H_

