/// $Id: Ping_TransformerTypes.h,v Exp $
/// @file : ACC_Multicom/ACC_Transformers/Ping_TransformerTypes.h
///
/// @brief : Ping Transformer APIs
///
/// @par OWNER: Gael Lassure
///
/// @author : Gael Lassure
///
/// @par SCOPE:
///
/// @date : 2008 - 02 - 20
///
/// &copy; 2008 ST Microelectronics. All Rights Reserved.
///

#ifndef _PING_TRANSFORMERTYPES_H_
#define _PING_TRANSFORMERTYPES_H_

#define DRV_MULTICOM_PING_TRANSFORMER_VERSION 0x080220

#include "mme.h"
#include "acc_mmedefines.h"

#define PING_TF_NAME "PING_CPU_"
#define RELEASE_LENGTH 64

//! Additional PING capability structure

typedef struct
{
	MME_UINT Info[RELEASE_LENGTH];
} MME_PingInfo_t;

#define AUDIORELEASE_TF_NAME "AUDIORELEASE"

enum eAudioFeatureReleaseTag
{
	AUDIOFEATURE_DDPLUS_TAG,
	AUDIOFEATURE_DDCO_TAG,
	AUDIOFEATURE_DOLBY_PULSE_TAG,
	AUDIOFEATURE_DOLBY_VOLUME_TAG,
	AUDIOFEATURE_TRUEHD_TAG,
	AUDIOFEATURE_DTSHD_TAG,
	AUDIOFEATURE_DTSE_TAG,
	AUDIOFEATURE_HEAAC_TAG,
	AUDIOFEATURE_WMAPRO_TAG,
	AUDIOFEATURE_SRS_TRUSURROUNDXT_TAG,
	AUDIOFEATURE_SRS_TRUSURROUNDHD_TAG,
	AUDIOFEATURE_SRS_TRUVOLUME_TAG,
	AUDIOFEATURE_SRS_WOWHD_TAG,
	AUDIOFEATURE_NB_RELEASE_TAG
};

#define AUDIOFEATURE_RELEASE_TAG_SIZE 32
typedef struct
{
	char ReleaseTag[AUDIOFEATURE_NB_RELEASE_TAG][AUDIOFEATURE_RELEASE_TAG_SIZE];
} tAudioReleaseInfo;

#endif // _PING_TRANSFORMERTYPES_H_
