/// $Id: WMA_DecoderTypes.h,v 1.4 2004/03/08 18:20:14 lassureg Exp $
/// @file : MMEPlus/Interface/Transformers/WMA_DecoderTypes.h
///
/// @brief : WMA MME Decoder Types
///
/// @par OWNER: Gael Lassure
///
/// @author : Gael Lassure
///
/// @par SCOPE:
///
/// @date : 2004-02-18
///
/// &copy; 2004 ST Microelectronics. All Rights Reserved.
///

#ifndef _WMA_DECODERTYPES_H_
#define _WMA_DECODERTYPES_H_

#define DRV_MME_WMA_DECODER_VERSION 0x040705

#include "Pcm_PostProcessingTypes.h"
#include "acc_mmedefines.h"

// Number of output frames to decode per MME transform operation
#define AUDIO_DECODER_NUM_FRAMES_PER_TRANSFORM 1
#define ACC_AUDIO_DECODER_NB_MAX_INPUT_BUFFER 1

#define WMA_DECODER_MAX_BUFFER_PARAMS AUDIO_MAX_BUFFER_PARAMS

//#define ACC_MAX_DEC_CONFIG_SIZE 21
#define ACC_MAX_DEC_FRAME_PARAMS 4
#define ACC_MAX_BUFFER_PARAMS 2
//#define ACC_MAX_DEC_FRAME_STATUS 4

enum eWmaCapabilityFlags
{
	ACC_WMA9_STD,
	ACC_WMA9_PRO,
	ACC_WMA9_LOSSLESS,
	ACC_WMA9_VOICE
};

/****************************************************************************/
typedef struct sMME_WmaAudioStreamInfo
{
	U16 nVersion;
	U16 wFormatTag;
	U32 nSamplesPerSec;
	U32 nAvgBytesPerSec;
	U32 nBlockAlign;
	U16 nChannels;
	U16 nEncodeOpt;
	U32 nSamplesPerBlock;
	U32 dwChannelMask;
	U32 nBitsPerSample;
	U16 wValidBitsPerSample; // actual valid data depth in the decoded stream
	U16 wStreamId;
} MME_WmaAudioStreamInfo_t;

typedef struct
{
	enum eAccDecoderId DecoderId;
	U32 StructSize;

	U16 MaxNbPages;
	U16 MaxPageSize;
	U32 MaxBitRate; //!< Max bitrate present in ASF header.
	U32 NbSamplesOut; //!< Number of Output Samples per WMA Decoder call.

	enum eAccBoolean NewAudioStreamInfo;
	MME_WmaAudioStreamInfo_t AudioStreamInfo; //!< Audio stream specific properties present in ASF header.
} MME_LxWmaConfig_t;

enum eWmaFrameParams
{
	// WMA_BUFFER_PARAMS_N
	// ...

	// do not edit below this line
	WMA_FRAME_PARAMS_NB_ELEMENTS
};

/*enum eWmaBufferParams
{
 // WMA_DATA_BLOCK_PRESENT, //!< indicates whether a new data block is present in given page
 // WMA_ALIGN_COUNT,
 // WMA_ALIGN_INFO,
 // WMA_BLOCK_COUNT,
 WMA_BUFFER_TYPE, //!< indicates that the given buffer is the last packet of the file
 WMA_SKIP_FRAMES, //!< indicates which frame in current Packet to skip

 // do not edit below this line
 WMA_BUFFER_PARAMS_NB_ELEMENTS
};*/

enum eWmaBufferType
{
	WMA_BLOCK,
	WMA_LAST_BLOCK,
	WMA_ASF_HEADER,
	WMA_PES_BLOCK
};

/*typedef struct
{
 U32 StructSize; //!< Size of the BufferParams array.
 U32 BufferParams[WMA_DECODER_MAX_BUFFER_PARAMS]; //!< Decoder Specific Parameters
} MME_LxWmaTransformerBufferParams_t;*/

enum eWmaStatus
{
	WMA_DECODER_RUNNING,
	WMA_STATUS_EOF,
	WMA_STATUS_BAD_STREAM
};

typedef struct
{
	U8 Status;
	U8 CurrentPacket;
	U16 NbSamplesLeft;
	U32 TotalNumberSamplesOut;
} tMMEWmaStatus;

#endif /* _AUDIO_DECODER_DECODERTYPES_H_ */
