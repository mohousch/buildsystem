/// $Id: RealAudio_DecoderTypes.h,v 1.2 2003/11/26 17:03:08 lassureg Exp $
/// @file : MMEPlus/Interface/Transformers/RealAudio_DecoderTypes.h
///
/// @brief : Real Audio Audio Decoder specific types for MME
///
/// @par OWNER: Gael Lassure
///
/// @author : Pankaj
///
/// @par SCOPE:
///
/// @date :
///
/// &copy; 2003 ST Microelectronics. All Rights Reserved.
///

#ifndef _REALAUDIO_DECODERTYPES_H_
#define _REALAUDIO_DECODERTYPES_H_

#include "acc_mmedefines.h"
#include "Audio_DecoderTypes.h"

/* Defines the Maximum Scatter Pages expected in Real Audio in one Databuffer of a Transform Command */
#define REAL_NB_SCATTERPAGES (4) // 4 * 256 may be used

enum eRACapabilityFlags
{
	ACC_RA_COOK,
	ACC_RA_LSD,
	ACC_RA_AAC
};

enum
{
	ACC_HXR_OK = 0,
	ACC_HXR_FAIL,
	ACC_HXR_FOURCC_INCORRECT,
	ACC_HXR_MDPR_NOT_FOUND,
	ACC_HXR_INVALID_PARAMETER,
	ACC_HXR_OUTOFMEMORY,
	ACC_HXR_INVALID_FILE,
	ACC_HXR_INVALID_VERSION,
	ACC_HXR_UNSUPPORTED_AUDIO,
	ACC_HXR_DEC_NOT_FOUND,
	ACC_HXR_NOT_SUPPORTED,
	ACC_HXR_ZERO_MDPR_LEN,
	ACC_HXR_NOTIMPL,
	ACC_HXR_GECKO2_INVALID_SIDEINFO,
	ACC_HXR_INVALID_CHANNEL_MAP,
};

typedef struct
{
	enum eAccDecoderId DecoderId;
	U32 StructSize;
	U32 CodecType;
	U16 usNumChannels;
	U16 usFlavorIndex;
	U32 NbSample2Conceal;
	U32 ulSampleRate;
	U32 ulBitsPerFrame;
	U32 ulGranularity;
	U32 ulOpaqueDataSize;
	U32 Features;
} MME_LxRealAudioConfig_t;

typedef struct
{
	U32 StructSize; //!< Size of this structure
	MME_LxRealAudioConfig_t RealAudoConfig; //!< Private Config of MP2 Decoder
	MME_LxPcmProcessingGlobalParams_t PcmParams; //!< PcmPostProcessings Params
} MME_LxRealAudioTransformerGlobalParams_t;

#endif /* _REALAUDIO_DECODERTYPES_H_ */
