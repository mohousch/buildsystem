
/// @file : ACC/AAC_Transformer/SignalProcessingTypes.h
///
/// @brief : Signal Processing To be performed on the PCM samples
///
/// @par OWNER:
///
/// @author :
///
/// @par SCOPE:
///
/// @date : 2004-06-02
///
/// &copy; 2003 ST Microelectronics. All Rights Reserved.
///

#ifndef _SIGNALPROCESSING_TYPES_H_
#define _SIGNALPROCESSING_TYPES_H_

#include "acc_mmedefines.h"
//#include "mme_api.h"

#define NUM_FRAMES_PER_TRANSFORM 1

#if 0
enum eAccSignalProcessId
{
	ACC_WATERMARK_ID,
	ACC_SCORING_ID,

	ACC_LAST_SIGNALPROCESSING_ID
};
#endif

enum eAccSignalProcInput
{
	ACC_MUSIC_LINE,
	ACC_VOICE_LINE,
	ACC_MUSIC_AND_VOICE
};

enum eAccScoringResult
{
	ACC_SCORE_BAD,
	ACC_SCORE_GOOD,
	ACC_SCORE_NOT_READY
};

enum eAccWatermarkReady
{
	ACC_WATERMARK_RESULT_NOT_READY = 0,
	ACC_WATERMARK_RESULT_READY = 1
};

enum eAccSignalProcStatus
{
	ACC_STRUCT_SIZE,
	ACC_STATUS_PARAMS
};

//! Additional Signal Processing Capability structure
typedef struct
{
	U32 StructSize; //!< Size of this structure
	U32 DecoderCapabilityFlags;
	U32 DecoderCapabilityExtFlags;
	U32 SignalProcessingCapabilityFlags;
} MME_LxSignalProcessingTransformerInfo_t;

/* Enumeration type for Watermark processing Configuration */
enum eWatermarkIdx
{
	WM_ENABLE,
	WM_FIRST_TIME,
	WM_OUTPUT_READY,
	WM_OUTPUT_WATERMARK,
	/* Do not edit beyond this point */
	WM_NB_CONFIG_ELEMENTS
};

enum eWatermarkStatusIdx
{
	WM_STATUS_READY,
	WATERMARK_OUTPUT,
	WM_NB_STATUS_ELEMENTS
};

/* Global Parameters for watermark processing */
typedef struct
{
	enum eAccSignalProcId Id; /* Id of this processing */
	U32 StructSize;
	U32 Config[WM_NB_CONFIG_ELEMENTS];
	enum eAccProcessApply Apply; /* Enable/Disable watermark processing */

} MME_WmarkGlobalParams_t;

typedef struct
{
	U32 StructSize;
	U32 Status[WM_NB_STATUS_ELEMENTS];
	// eAccWatermarkReady WatermarkOutputReady; Field to indicate whether watermark output ready or not
	// U16 WatermarkOutput; Field to output the watermark
} MME_WmarkStatusParams_t;

/* Enumeration for configuring scoring block */
enum eScoringIdx
{
	SCORING_FIRST_TIME,
	SCORING_SCORE,
	SCORING_DELAY,
	SCORING_RHYTHM,
	SCORING_PITCH,
	/* Do not edit beyond this point */
	SCORING_NB_CONFIG_ELEMENTS
};

enum eScoringStatusIdx
{
	SCORING_STATUS_SCORE,
	SCORING_STATUS_DELAY,
	SCORING_STATUS_RHYTHM,
	SCORING_STATUS_PITCH,
	SCORING_NB_STATUS_ELEMENTS
};

/* Global Parameters for Scoring Block */
typedef struct
{
	enum eAccSignalProcId Id; //!< Id of this processing block
	U32 StructSize;
	U32 Config[SCORING_NB_CONFIG_ELEMENTS];
	enum eAccProcessApply Apply; //!< Enable/Disable scoring block

} MME_ScoringGlobalParams_t;

typedef struct
{
	U32 StructSize;
	U32 Status[SCORING_NB_STATUS_ELEMENTS];
#if 0
	U16 ScoringScore; //!< Field to display score of the scoring algorithm
	eAccScoringResult ScoringDelay; //!< Field to display delay parameter of scoring
	eAccScoringResult ScoringRhythm; //!< Field to display rhythm parameter of scoring
	eAccScoringResult ScoringPitch; //!< Field to display pitch parameter of scoring
#endif
} MME_ScoringStatusParams_t;

typedef struct
{
	U32 StructSize; //!< Size of this Structure

	MME_WmarkGlobalParams_t Watermark; //!< Watermark configuration
	MME_ScoringGlobalParams_t Scoring; //!< Scoring Configuration

} MME_LxSignalProcessingGlobalParams_t;

/* Global Parameters for the Signal Processing Transformer */
typedef struct
{
	U32 StructSize;
	enum eAccAcMode MusicAudioMode; //!< Audio Mode of Music Line input
	enum eAccFsCode MusicSamplingFreq; //!< Sampling Frequency of Music Line Input
	enum eAccAcMode SpeechAudioMode; //!< Audio Mode of Voice Line input
	enum eAccFsCode SpeechSamplingFreq; //!< Sampling Frequency of voice line input

	MME_LxSignalProcessingGlobalParams_t SignalParams;

} MME_LxSignalProcessingTransformerGlobalParams_t;

/* Initialization structure for Signal Processing Transformer */
typedef struct
{
	U32 StructSize;
	U8 NbChannelsInMusic; //!< Nb of Input Channels on Music Line
	U8 NbChannelsInVoice; //!< Nb of Input Channels on voice line
	MME_LxSignalProcessingTransformerGlobalParams_t GlobalParams;

} MME_LxSignalProcessingTransformerInitParams_t;

/* Frame Parameters */
typedef struct
{
	U32 NumInputBuffers; //!< Nb of Input MME Buffers.
	MME_DataBuffer_t buffers[2 * NUM_FRAMES_PER_TRANSFORM]; //!< The input MME Buffers

} MME_LxSignalProcessingTransformerFrameParams_t;

/* The frame Status structure */
typedef struct
{
	U32 StructSize;
	MME_WmarkStatusParams_t WmarkStatus;
	MME_ScoringStatusParams_t ScoringStatus;

} MME_LxSignalProcessingTransformerFrameStatus_t;

#endif /* _SIGNALPROCESSING_TYPES_H_ */
