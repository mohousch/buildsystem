
///
/// @file MMEPlus/Interface/Transformer/DVAudioTransformerTypes.h
///
/// @brief DV Audio specific definitions to be used with MME
///
/// @par OWNER:
/// Martin Stephan
///
/// @author Martin Stephan (martin.stephan@st.com)
///
/// @par SCOPE:
/// INTERNAL Header File
///
/// @date 2004-02-11
///
/// &copy; 2004 STMicroelectronics Design and Application. All Rights Reserved.
///

#ifndef DVAUDIOTRANSFORMERTYPES_H
#define DVAUDIOTRANSFORMERTYPES_H

#include "DVDecoderTransformerTypes.h"
#include "acc_mmedefines.h"

// number of buffers used (no. of overall, no. of input, no. of output buffers)
#define NUM_DV_AUDIO_TRANSFORM_BUFFERS 2
#define NUM_DV_AUDIO_INPUT_BUFFERS 1
#define NUM_DV_AUDIO_OUTPUT_BUFFERS 1

// DV Audio global parameters.
// Passed to the transformer when sending the MME_SET_GLOBAL_TRANSFORM_PARAMS command.
typedef struct
{
	U32 NumberInputBuffers;
	U32 NumberOutputBuffers;
	MME_DVVideoFormat VideoFormat;
	U32 DnmixCoeff[2][4];
} MME_DVAudioGlobalParams_t;

// DV Audio transformer initialization parameters.
// Passed to the transformer when calling MMEInitTransformer.
typedef struct
{
	U32 StructSize;
	eAccAcMode OutAcMode;
	BOOL PerformSRC;
	MME_DVAudioGlobalParams_t GlobalParams;
} MME_DVAudioInitParams_t;

// DV Audio transform parameters.
// Passed to the transformer with each MME_TRANSFORM command.
typedef struct
{
	U32 NumberInputBuffers;
	U32 NumberOutputBuffers;
	MME_DataBuffer_t DataBuffer[NUM_DV_AUDIO_TRANSFORM_BUFFERS];
} MME_DVAudioFrameParams_t;

// enum of audio I/O buffers
enum DVAudioIOBuffer
{
	MME_DV_AUDIO_IN_BUFFER = 0, // input buffer
	MME_AUDIO_OUT_BUFFER = 1 // output buffer
};

// DV Audio stream information
// Filled out by the DV Audio transformer and returned
// within the MME_DVAudioReturnParams_t structure after
// each TRANSFORM command.
typedef struct
{
	BOOL MainLockedAudioFlag;
	BOOL AuxLockedAudioFlag;
	eAccFsCode SamplingFrequency;
	U32 Nchan; // number of channels in stream
	BOOL ChannelExist[4]; // channels in stream exist
	U32 AfSize; // number of samples per channel
} MME_DVAudioStreamInfo;

// DV Audio return parameters.
// Filled out by the DV audio transformer.
// Contained in the AdditionalInfo_p field of a MME_Command_t structure.
typedef struct
{
	U32 NumberInputBytesLeft; // number of bytes left
	U32 NumberOutputBytes; // number of bytes written
	eAccAcMode OutAcMode; // audio coding mode
	MME_DVAudioStreamInfo tDVAudioStreamInfo; // info about the audio data
} MME_DVAudioReturnParams_t;

#endif // DVAUDIOTRANSFORMERTYPES_H
