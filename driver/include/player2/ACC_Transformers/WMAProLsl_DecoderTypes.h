/// @file : MMEPlus/Interface/Transformers/WMAProLsl_DecoderTypes.h
///
/// @brief : WMAProLsl MME Decoder Types
///
/// @par OWNER: previr rangroo
///
/// @author : previr rangroo
///
/// @par SCOPE:
///
/// @date : 2006-01-03
///
/// &copy; 2004 ST Microelectronics. All Rights Reserved.
///

#ifndef _WMAPROLSL_DECODERTYPES_H_
#define _WMAPROLSL_DECODERTYPES_H_

#define DRV_MME_WMAPROLSL_DECODER_VERSION 0x060103

#include "Audio_DecoderTypes.h"
#include "Pcm_PostProcessingTypes.h"
#include "acc_mmedefines.h"

// Number of output frames to decode per MME transform operation
#define AUDIO_DECODER_NUM_FRAMES_PER_TRANSFORM 1
#define ACC_AUDIO_DECODER_NB_MAX_INPUT_BUFFER 1

#define WMAPROLSL_DECODER_MAX_BUFFER_PARAMS AUDIO_MAX_BUFFER_PARAMS

//#define DECOPT_CHANNEL_DOWNMIXING 0x0001
//#define DECOPT_DRC 0x0002
//#define DECOPT_INTERPOLATED_DOWNSAMPLE 0x0004
//#define DECOPT_HALF_TRANSFORM 0x0008
//#define DECOPT_HALF_UP_TRANSFORM 0x0010
//#define DECOPT_2X_TRANSFORM 0x0020
//#define DECOPT_REQUANTTO16 0x0040
//#define DECOPT_DOWNSAMPLETO44OR48 0x0080
//#define DECOPT_LTRTDOWNMIX 0x0100

/****************************************************************************/
typedef struct sMME_WmaProLslAudioStreamInfo
{
	U16 nVersion;
	U16 wFormatTag;
	U32 nSamplesPerSec;
	U32 nAvgBytesPerSec;
	U32 nBlockAlign;
	U16 nChannels;
	U16 nEncodeOpt;
	U16 nAdvancedEncodeOpt;
	U16 DummyAlign; // for PC platform compatibility.
	U32 nAdvancedEncodeOpt2;
	U32 nSamplesPerBlock;
	U32 dwChannelMask;
	U32 nBitsPerSample;
	U16 wValidBitsPerSample; // actual valid data depth in the decoded stream
	U16 wStreamId;
}
MME_WmaProLslAudioStreamInfo_t;

typedef struct
{
	enum eAccDecoderId DecoderId;
	U32 StructSize;

	U16 MaxNbPages;
	U16 MaxPageSize;
	U32 _DUMMY; //MaxBitRate;//!< Max bitrate present in ASF header.
	U32 NbSamplesOut; //!< Number of Output Samples per WMA Decoder call.

	enum eAccBoolean NewAudioStreamInfo;
	MME_WmaProLslAudioStreamInfo_t AudioStreamInfo; //!< Audio stream specific properties present in ASF header.
	U16 nDecoderFlags;
	U16 nDRCSetting;
	int iPeakAmplitudeRef;
	int iRmsAmplitudeRef;
	int iPeakAmplitudeTarget;
	int iRmsAmplitudeTarget;

	U32 nInterpResampRate;
}
MME_LxWmaProLslConfig_t;

enum eWmaProLslCapabilityFlags
{
	ACC_WMA_STD,
	ACC_WMA_PRO,
	ACC_WMA_LOSSLESS,
	ACC_WMA_VOICE
};

enum eWmaProLslFrameParams
{
	// WMA_BUFFER_PARAMS_N
	// ...

	// do not edit below this line
	WMAPROLSL_FRAME_PARAMS_NB_ELEMENTS
};

/*enum eWmaProLslBufferParams
{
 // WMA_DATA_BLOCK_PRESENT, //!< indicates whether a new data block is present in given page
 // WMA_ALIGN_COUNT,
 // WMA_ALIGN_INFO,
 // WMA_BLOCK_COUNT,
 WMAPROLSL_BUFFER_TYPE, //!< indicates that the given buffer is the last packet of the file
 WMAPROLSL_SKIP_FRAMES, //!< indicates which frame in current Packet to skip

 // do not edit below this line
 WMAPROLSL_BUFFER_PARAMS_NB_ELEMENTS
};*/

#ifdef _BEFORE_BL026_
#define WMAPROLSL_BUFTYPE_POS 24
#define WMAPROLSL_GET_BUFFER_TYPE(buf) (buf[WMAPROLSL_BUFFER_TYPE] >> WMAPROLSL_BUFTYPE_POS);
//#define WMAPROLSL_SET_BUFFER_TYPE(buf,type) buf[WMAPROLSL_BUFFER_TYPE] = ((buf[WMAPROLSL_BUFFER_TYPE] & 0x00FFFFFF) | (type << WMAPROLSL_BUFTYPE_POS))
#define WMAPROLSL_SET_BUFFER_TYPE(buf,type) ((buf[WMAPROLSL_BUFFER_TYPE] & 0xFF00FFFF) | (type << WMAPROLSL_BUFTYPE_POS))

#else

#define WMAPROLSL_GET_BUFFER_TYPE(buf) STREAMING_GET_BUFFER_TYPE(buf)
#define WMAPROLSL_SET_BUFFER_TYPE(buf, type) STREAMING_SET_BUFFER_TYPE(buf, type)

#define WMAPROLSL_BUFTYPE_POS_OLDAPI 24
#define WMAPROLSL_GET_BUFFER_TYPE_OLDAPI(buf) (buf[WMAPROLSL_BUFFER_TYPE] >> WMAPROLSL_BUFTYPE_POS_OLDAPI);

#endif

enum eWmaProLslBufferType
{
	WMAPROLSL_BLOCK,
	WMAPROLSL_LAST_BLOCK,
	WMAPROLSL_ASF_HEADER,
	WMAPROLSL_PES_BLOCK
};

/*typedef struct
{
 U32 StructSize; //!< Size of the BufferParams array.
 U32 BufferParams[WMAPROLSL_DECODER_MAX_BUFFER_PARAMS]; //!< Decoder Specific Parameters
} MME_LxWmaProLslTransformerBufferParams_t;*/

enum eWmaProLslStatus
{
	WMAPROLSL_DECODER_RUNNING,
	WMAPROLSL_STATUS_EOF
};

typedef struct
{
	U8 Status;
	U8 CurrentPacket;
	U16 NbSamplesLeft;
	U32 TotalNumberSamplesOut;
} tMMEWmaProLslStatus;

#endif /* _AUDIO_DECODER_DECODERTYPES_H_ */
