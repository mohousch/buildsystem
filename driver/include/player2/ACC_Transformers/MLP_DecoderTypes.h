/// $Id: MLP_DecoderTypes.h,v 1.6 2003/11/26 17:03:08 lassureg Exp $
/// @file : MMEPlus/Interface/Transformers/MLP_DecoderTypes.h
///
/// @brief : MLP Audio Decoder specific types for MME
///
/// @par OWNER: Sudhir
///
/// @author : Sudhir
///
/// @par SCOPE:
///
/// @date : 2003-07-23
///
/// &copy; 2002 ST Microelectronics. All Rights Reserved.
///

#ifndef _MLP_DECODERTYPES_H_
#define _MLP_DECODERTYPES_H_

#define DRV_MME_MLP_DECODER_VERSION 0x031124

#include "acc_mmedefines.h"
#include "Audio_DecoderTypes.h"

enum eMLPCapabilityFlags
{
	ACC_MLP_DVDA,
	ACC_DOLBY_LOSSLESS
};
enum eMLPConfig
{
	MLP_CRC_ENABLE,
	MLP_DRC_ENABLE,
	MLP_DOWNMIX_2_0,
	MLP_CHANNEL_ASSIGNMENT,
	MLP_AUX_ENABLE,
	MLP_SERIAL_ACCESS_UNITS,

	/* Do not edit beyond this line */
	MLP_NB_CONFIG_ELEMENTS
};

typedef struct
{
	enum eAccDecoderId DecoderId;
	U32 StructSize;
	U8 Config[MLP_NB_CONFIG_ELEMENTS];

} MME_LxMlpConfig_t;

// Macros to be used by Host
#define MLPMME_SET_DRC(x, drc) x[MLP_DRC_ENABLE] = ((x[MLP_DRC_ENABLE] & 0xFE) | drc)
#define MLPMME_SET_PP(x, pp ) x[MLP_DRC_ENABLE] = ((x[MLP_DRC_ENABLE] & 0xFD) | (pp << 1))
#define MLPMME_SET_CHREDIR(x, chredir) x[MLP_DRC_ENABLE] = ((x[MLP_DRC_ENABLE] & 0xFB) | (chredir << 2))

// Macros to be used by Companion
#define MLPMME_GET_DRC(x) (enum eBoolean) ((x[MLP_DRC_ENABLE] >> 0) &1)
#define MLPMME_GET_PP(x) (enum eBoolean) ((x[MLP_DRC_ENABLE] >> 1) &1)
#define MLPMME_GET_CHREDIR(x) (enum eBoolean) ((x[MLP_DRC_ENABLE] >> 2) &1)

#endif /* _MLP_DECODERTYPES_H_ */
