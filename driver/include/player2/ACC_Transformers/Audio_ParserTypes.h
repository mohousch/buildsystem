/// $Id: Audio_ParserTypes.h,v Exp $
/// @file : ACC_Multicom/ACC_Transformers/Audio_ParserTypes.h
///
/// @brief : Generic Audio Parser types for Multicom
///
/// @par OWNER: Gael Lassure
///
/// @author : Gael Lassure
///
/// @par SCOPE:
///
/// @date : 2006 - 07 - 20
///
/// &copy; 2006 ST Microelectronics. All Rights Reserved.
///

#ifndef _AUDIO_PARSERTYPES_H_
#define _AUDIO_PARSERTYPES_H_

#define DRV_MULTICOM_AUDIO_PARSER_VERSION 0x060720

#include "mme.h"
#include "acc_mmedefines.h"
#include "Audio_DecoderTypes.h"

//! Additional parser capability structure
typedef struct
{
	U32 StructSize; //!< Size of this structure
	U32 DecoderCapabilityFlags; //!< 1 bit per decoder
	U32 StreamDecoderCapabilityFlags; //!< 1 bit per decoder
	U32 DecoderCapabilityExtFlags[2]; //!< 4 bit per decoder
	U32 PcmProcessorCapabilityFlags[1]; //!< 1 bit per pcm processing
} MME_AudioParserInfo_t;

typedef struct
{
	enum eAccDecoderId DecoderId;
	U32 StructSize;

	U32 Config;

} MME_DecConfig_t;

typedef struct
{
	U32 StructSize; //!< Size of this structure
	U32 DecConfig[ACC_MAX_DEC_CONFIG_SIZE]; //!< Private Config of Audio Decoder
} MME_AudioParserGlobalParams_t;

typedef struct
{
	U32 StructSize; //!< Size of this structure

	//! System Init
	enum eAccProcessApply CacheFlush; //!< If ACC_DISABLED then the cached data aren't sent back to DDR

	uADFeatures BlockWise; //!< use AudioDecoder macros to read this field

	//! Decoder specific parameters
	MME_AudioParserGlobalParams_t GlobalParams;

} MME_AudioParserInitParams_t;

//! This structure must be passed when sending the TRANSFORM command to the decoder
enum eParseCmdCode
{
	PARSE_CMD_PLAY,
	PARSE_CMD_MUTE,
	PARSE_CMD_SKIP,
	PARSE_CMD_PAUSE,

	// Do not edit beyond this entry
	PARSE_LAST_CMD
};

typedef struct
{
	enum eAccBoolean Restart; //!< Restart decoder as if 1st frame
	U16 Cmd; //!< (enum eParseCmdCode) Play/Skip/Mute/Pause
	U16 CmdDuration; //!< Duration of Pause/Skip Cmd in samples
	U32 FrameParams[ACC_MAX_DEC_FRAME_PARAMS]; //!< Decoder Specific Parameters
} MME_AudioParserFrameParams_t;

/*enum eParserBufferType
{
 AUDIOPARSER_INFO,
 AUDIOPARSER_STREAM,
 AUDIOPARSER_EOF
};*/
enum eParserBufferType
{
	AUDIOPARSER_RUNNING,
	AUDIOPARSER_EOF,
	AUDIOPARSER_INFO,
	AUDIOPARSER_STREAM
};

enum eParserBufferParams
{
	AUDIOPARSER_BUFFER_TYPE,
	AUDIOPARSER_BUFFER_CONFIG, // Reserved for future use
	// Do not edit below
	AUDIOPARSER_NB_BUFFERPARAMS
};

/*typedef struct
{
 U32 StructSize;
 U32 BufferParams[AUDIOPARSER_NB_BUFFERPARAMS]; //!< Decoder Specific Parameters
} MME_AudioParserBufferParams_t;*/

enum eAudioParserFrameStatus
{
	AP_STATUS_FRAMESIZE, // current framesize , shall be used to dimension the
	// buffers to send considering constant framesize
	AP_STATUS_EOF, // Signal when last buffer has been consumed.
	AP_STATUS_FLAG0, // Codec specific
	AP_STATUS_FLAG1, // Codec specific
};

typedef struct
{
	U32 StructSize;

	int NbOutSamples; //<! Report the actual number of samples that have been output
	enum eAccAcMode AudioMode; //<! Channel Output configuration
	enum eAccFsCode SamplingFreq; //<! Sampling Frequency of Output PcmBuffer
	enum eAccBoolean Emphasis; //<! Buffer has emphasis
	U32 ElapsedTime; //<! elapsed time to do the transform in microseconds
	U32 PTSflag; //<! PTSflag[b0..1] = PTS - Validity / PTSflag[b16] = PTS[b32]
	U32 PTS; //<! PTS[b0..b31]

	U16 FrameStatus[ACC_MAX_DEC_FRAME_STATUS]; /* Decoder Specific Parameters */
} MME_AudioParserFrameStatus_t;

#endif /* _AUDIO_DECODER_PARSERTYPES_H_ */

