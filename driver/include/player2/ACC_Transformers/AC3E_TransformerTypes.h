/*
 $Id: Exp $
 @file : ACC_Multicom/ACC_Transformers/AC3e_TransformerTypes.h

 @brief : AC3 Audio Encoder + SFC specific types for Multicom

 @par OWNER: Charles Averty

 @author : Sebastien Clapie

 @par SCOPE:

 @date : 2005/05/20

 &copy; 2005 ST Microelectronics. All Rights Reserved.
*/

#ifndef _AC3E_TRANSFORMERTYPES_H_
#define _AC3E_TRANSFORMERTYPES_H_

#include "mme.h"

#include "acc_api.h"

/*! Enum to be used for choosing AC3 bitrate */
enum eAccDdceBitrate
{
	ACC_DDCE_96_KBPS = 6,
	ACC_DDCE_112_KPBS,
	ACC_DDCE_128_KPBS,
	ACC_DDCE_160_KPBS,
	ACC_DDCE_192_KPBS,
	ACC_DDCE_224_KPBS,
	ACC_DDCE_256_KPBS,
	ACC_DDCE_320_KPBS,
	ACC_DDCE_384_KPBS,
	ACC_DDCE_448_KPBS
};

/*! Number of frames to process per MME transform operation*/
#define NUM_FRAMES_PER_TRANSFORM 1

/*!Additional transformer capability structure*/
typedef struct
{
	MME_UINT StructSize; /*!< Size of this structure*/
} MMEAudioEncodeTransformerInfo;

/*! Transformer Parameters transmitted once in a while*/
typedef struct
{
	MME_UINT StructSize;

} MMEAudioEncodeGlobalParams;

#include "AudioEncoderTypes.h"

#endif // _AC3E_TRANSFORMERTYPES_H_
