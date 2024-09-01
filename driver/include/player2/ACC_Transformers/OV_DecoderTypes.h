/// $Id: OV_DecoderTypes.h,v 1.4 2004/03/08 18:20:14 lassureg Exp $
/// @file : ACC_MME/ACC_Transformers/OV_DecoderTypes.h
///
/// @brief : OV MME Decoder Types
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

#ifndef _OV_DECODERTYPES_H_
#define _OV_DECODERTYPES_H_

#define DRV_MME_OV_DECODER_VERSION 0x050105

#include "PcmProcessing_DecoderTypes.h"
#include "acc_mmedefines.h"

// Number of output frames to decode per MME transform operation
#define AUDIO_DECODER_NUM_FRAMES_PER_TRANSFORM 1
#define ACC_AUDIO_DECODER_NB_MAX_INPUT_BUFFER 1
#define OV_DECODER_NUM_BUFFERS_PER_SENDBUFFER AUDIO_STREAMING_NUM_BUFFERS_PER_SENDBUFFER
#define OV_DECODER_MAX_BUFFER_PARAMS AUDIO_MAX_BUFFER_PARAMS

typedef struct
{
	MME_UINT StructSize; //!< Size of this structure
	MME_UINT DecoderCapabilityFlags;
	MME_UINT DecoderCapabilityExtFlags;
	MME_UINT PcmProcessorCapabilityFlags;

} MME_LxOvTransformerInfo_t;

/****************************************************************************/

typedef struct
{
	enum eAccDecoderId DecoderId;
	MME_UINT StructSize;

	U16 MaxNbPages;
	U16 MaxPageSize;

	MME_UINT NbSamplesOut; //!< Number of Output Samples per Ov Decoder call.

} MME_LxOvConfig_t;

typedef struct
{
	MME_UINT StructSize; //!< Size of this structure
	MME_LxOvConfig_t DecConfig; //!< Ov Decoder Config :
	MME_LxPcmProcessingGlobalParams_t PcmParams; //!< PcmPostProcessings Params

} MME_LxOvTransformerGlobalParams_t;

enum eOvFrameParams
{
	// OV_BUFFER_PARAMS_N
	// ...

	// do not edit below this line
	OV_FRAME_PARAMS_NB_ELEMENTS
};
/*enum eOvBufferParams
{
 OV_BUFFER_TYPE, //!< indicates that the given buffer is the last packet of the file
 OV_SKIP_FRAMES, //!< indicates which frame in current Packet to skip

 // do not edit below this line
 OV_BUFFER_PARAMS_NB_ELEMENTS
};*/

enum eOvBufferType
{
	OV_NORMAL_BUFFER,
	OV_LAST_BUFFER
};

/*typedef struct
{
 MME_UINT StructSize; //!< Size of the BufferParams array.
 MME_UINT BufferParams[OV_DECODER_MAX_BUFFER_PARAMS]; //!< Decoder Specific Parameters
} MME_LxOvTransformerBufferParams_t;*/

enum eOvStatus
{
	OV_DECODER_RUNNING,
	OV_STATUS_EOF
};

typedef struct
{
	U8 Status; //!< Status based on enum eOvStatus

} tMMEOvStatus;

#endif /* _AUDIO_DECODER_DECODERTYPES_H_ */
