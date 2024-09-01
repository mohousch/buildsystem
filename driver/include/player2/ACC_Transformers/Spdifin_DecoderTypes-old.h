/// $Id: Spdifin_DecoderTypes.h,v 1.5 2003/11/26 17:03:08 lassureg Exp $
/// @file : ACC_Multicom/ACC_Transformers/Spdifin_DecoderTypes.h
///
/// @brief : Spdifin Audio Decoder specific types for Multicom
///
/// @par OWNER: Gael Lassure
///
/// @author : Gael Lassure
///
/// @par SCOPE:
///
/// @date : 2007-07-04
///
/// &copy; 2007 ST Microelectronics. All Rights Reserved.
///

#ifndef _SPDIFIN_DECODERTYPES_H_
#define _SPDIFIN_DECODERTYPES_H_

#include "mme.h"
#include "Audio_DecoderTypes.h"
#include "PcmProcessing_DecoderTypes.h"
#include "DolbyDigital_DecoderTypes.h"
#include "acc_mmedefines.h"

enum eSpdifinCapabilityFlags
{
	ACC_SPDIFIN_DD,
	ACC_SPDIFIN_DTS,
	ACC_SPDIFIN_MPG,
	ACC_SPDIFIN_WMA,
};

enum eIecConfigIdx
{
	IEC_SFREQ,
	IEC_NBSAMPLES,
	IEC_DEEMPH,
	IEC_Reserved,
	/* Do not edit beyond this comment */
	IEC_NB_CONFIG_ELEMENTS
};

typedef struct
{
	enum eAccDecoderId DecoderId;
	U32 StructSize;

	U32 Config[IEC_NB_CONFIG_ELEMENTS];
	U8 DecConfig[DD_NB_CONFIG_ELEMENTS];

} MME_SpdifinConfig_t;

#define SPDIFIN_NBMAX_PAGES 32

typedef struct
{
	U32 StructSize; //!< Size of the BufferParams array.
	U32 BufferParams[STREAMING_BUFFER_PARAMS_NB_ELEMENTS]; //!< Decoder Specific Parameters
} MME_SpdifinBufferParams_t;

enum eMulticomSpdifinStatus
{
	SPDIFIN_DECODER_RUNNING,
	SPDIFIN_STATUS_EOF,
};

typedef struct
{
	U32 Status;
	U32 NbSamplesLeft;
	U32 TotalNumberSamplesOut;
} tMMESpdifinStatus;
#endif /* _SPDIFIN_DECODERTYPES_H_ */
