/// $Id: MP2a_DecoderTypes.h,v 1.11 2003/11/26 17:03:08 lassureg Exp $
/// @file : MMEPlus/Interface/Transformers/MP2a_DecoderTypes.h
///
/// @brief : MP2 Audio Decoder specific types for MME
///
/// @par OWNER: Gael Lassure
///
/// @author : Gael Lassure
///
/// @par SCOPE:
///
/// @date : 2003-07-04
///
/// &copy; 2003 ST Microelectronics. All Rights Reserved.
///

#ifndef _MP2a_DECODERTYPES_H_
#define _MP2a_DECODERTYPES_H_

#define DRV_MME_MP2a_DECODER_VERSION 0x031030

#include "PcmProcessing_DecoderTypes.h"
#include "acc_mmedefines.h"
#include "Audio_DecoderTypes.h"

// Number of frames to decode per MME transform operation
#define NUM_FRAMES_PER_TRANSFORM 1

enum eAccMP2aStatus
{
	ACC_MPEG2_OK = 0,
	ACC_MPEG_BA_CRC_ERROR = (1 << 0),
	ACC_MPEG_MC_CRC_ERROR = (1 << 1),
	ACC_MPEG_EXT_CRC_ERROR = (1 << 2),
	ACC_MPEG_MC_EXT_BAD_SYNCWORD_ERROR = (1 << 3),
	ACC_MPEG_MC_LAYER1_NOT_SUPPORTED = (1 << 4),
	ACC_MPEG_FB_ERROR = (1 << 5),
	ACC_MPEG_LAYER1_NOT_SUPPORTED = (1 << 6),
	ACC_MPEG_LAYER2_NOT_SUPPORTED = (1 << 7),
	ACC_MPEG_ILLEGAL_LAYER = (1 << 8),
	ACC_MPEG_ID_RESERVED = (1 << 9),
	ACC_MPEG_CENTER_UNDEFINED = (1 << 10),
	ACC_MPEG_AUG_CRC_ERROR = (1 << 11),
	ACC_MPEG_MC_ERROR = (1 << 12), //ACC_MPEG_MC_EXT_BAD_SYNCWORD_ERROR,
	ACC_MPEG_SAMPLING_FREQ_RESERVED = (1 << 13),
	ACC_MPEG_BITRATE_INDEX_INVALID = (1 << 14),
	ACC_MPEG_EMPHASIS_RESERVED = (1 << 15),
	ACC_MPEG_EXT_ID_RESERVED = (1 << 16),
	ACC_MPEG_MULTILINGUAL_WARNING = (1 << 17),
	ACC_MPEG_PREDICTION_ON_WARNING = (1 << 18),
	ACC_MPEG_BAD_SYNCWORD_ERROR = (1 << 19)
};

enum eMP2aCapabilityFlags
{
	ACC_MP2_MC_L1,
	ACC_MP2_MC_L2,
	ACC_MP2_71_L2,
	ACC_MP2_DAB
};

enum eMP3CapabilityFlags
{
	ACC_MP1_L3,
	ACC_MP2_L3,
	ACC_MP25_L3,
	ACC_MP3_PRO
};

enum eMpgAacCapability
{
	ACC_AAC_LC,
	ACC_AAC_SSR,
	ACC_AAC_SBR,
	ACC_AAC_PS,
};

enum eMP2aConfigIdx
{
	MP2a_CRC_ENABLE,
	MP2a_LFE_ENABLE,
	MP2a_DRC_ENABLE,
	MP2a_MC_ENABLE,
	MP2a_NIBBLE_ENABLE,
	MP3_FREE_FORMAT,
	MP3_MUTE_ON_ERROR,
	MP3_DECODE_LAST_FRAME,
	/* Do not edit below this comment */
	MP2a_NB_CONFIG_ELEMENTS
};

enum eAacFormatType
{
	AAC_ADTS_FORMAT,
	AAC_ADIF_FORMAT,
	AAC_MP4_FILE_FORMAT,
	AAC_LOAS_FORMAT,
	AAC_RAW_FORMAT,
	AAC_BSAC_RAW_FORMAT,
	AAC_RESERVED_FORMAT, // for future use.
	AAC_AUTO_TYPE
};

enum eAacConfigIdx
{
	AAC_CRC_ENABLE, //! Activate CRC checking
	AAC_DRC_ENABLE, //! Activate postprocessing : DRC / DTS Transcoding
	AAC_SBR_ENABLE, //! Activate HE - AAC decoding : SBR(HE - AACv1) and PS (HE - AACv2)
	AAC_FORMAT_TYPE, //! Specify the encapsulation format :: ADTS,LOAS,ADIF, MP4, Raw ( + SamplingFrequency)
	AAC_DRC_CUT, //! DRC cut factor (portion of the attenuation factor for high level signal)
	AAC_DRC_BOOST, //! DRC boost factor (portion of the gain factor for low level signal)
	AAC_DUAL_MONO, //! Force the copy of the mono onto L/R when stereo output.
	/* Do not edit below this comment */
	AAC_NB_CONFIG_ELEMENTS
};

enum eAacTscOutFmt
{
	AAC_TSC_32be, // 7FFE 0000 8001 0000 ...
	AAC_TSC_32le, // 0000 FE7F 0000 0180 ...
	AAC_TSC_16be, // 7FFE 8001 ...
	AAC_TSC_16le // FE7F 0180 ...
};

enum eAacParserStatus
{
	AAC_RUNNING,
	AAC_EOF,
};

enum eACCMpegMode
{
	MODE_STEREO,
	MODE_JOINT_STEREO,
	MODE_DUAL_CHANNEL,
	MODE_SINGLE_CHANNEL
};

typedef struct
{
	U8 ParserStatus;
	U8 DecoderStatus;
	U16 TranscoderStatus;
	U16 ConvSyncStatus;
	U16 TranscoderFreq;
} tAacStatus;

// Macros to be used by Host
#define AACMME_SET_DRC(x, drc) ((x[AAC_DRC_ENABLE] & 0xFE) | drc)
#define AACMME_SET_TRANSCODE(x, tsc,fmt) x[AAC_DRC_ENABLE] = ((x[AAC_DRC_ENABLE] & 0xF1) | (tsc<<1) | (fmt<<2))

// Macros to be used by Companion
#define AACMME_GET_DRC_ENABLE(x) ((x[AAC_DRC_ENABLE]>>0) & 0x1)
#define AACMME_GET_TRANSCODE_ENABLE(x) ((x[AAC_DRC_ENABLE]>>1) & 0x1)
#define AACMME_GET_TRANSCODE_OUTFMT(x) ((x[AAC_DRC_ENABLE]>>2) & 0x3)

#define AACMME_GET_TYPE(x) ((x[AAC_FORMAT_TYPE]>>0) &0xF)
#define AACMME_GET_TYPE_PARAM(x) ((x[AAC_FORMAT_TYPE]>>4) &0xF) /* Sfreq in case of Raw Stream*/
#define AACMME_SET_TYPE_PARAM(x,sf) x[AAC_FORMAT_TYPE] = ((x[AAC_FORMAT_TYPE]& 0xF) + (sf << 4))

#define ACCMME_ENABLE_SBR(x) x[AAC_SBR_ENABLE] |= 1 //! Enable SBR (HE - AACv1) when fs(AAC) <= 24kHz
#define ACCMME_ENABLE_SBR96(x) x[AAC_SBR_ENABLE] |= 2 //! Enable SBR when fs(AAC) > 24kHz
#define ACCMME_ENABLE_PS(x) x[AAC_SBR_ENABLE] |= 4 //! Enable PS Decoding (HE - AACv2)

typedef struct
{
	enum eAccDecoderId DecoderId;
	U32 StructSize;

	U8 Config[MP2a_NB_CONFIG_ELEMENTS];
	//config[0]: enum eAccBoolean CrcOff; //!< Disable the CRC Checking
	//config[1]: enum eAccBoolean LfeEnable; //!< Enable the Lfe Processing
	//config[2]: enum eAccBoolean DrcEnable; //!< Enable the Dynamic Range Compression processing
	//config[3]: enum eAccBoolean McEnable; //!< Enable the MultiChannel Decoding
} MME_LxMp2aConfig_t;

typedef struct
{
	U32 StructSize; //!< Size of this structure

	MME_CMCGlobalParams_t CMC; //!< Common DMix Config : could be used internally by decoder or by generic downmix
	MME_TsxtGlobalParams_t TSXT; //!< TsXT Configuration
	MME_OmniGlobalParams_t OMNI; //!< Omni Configuration
	MME_StwdGlobalParams_t STWD; //!< WideSurround Configuration
	MME_DMixGlobalParams_t DMix; //!< Generic DMix : valid for main and vcr output

} MME_LxMP2aPcmProcessingGlobalParams_t;

typedef struct
{
	U32 StructSize; //!< Size of this structure
	MME_LxMp2aConfig_t Mp2aConfig; //!< Private Config of MP2 Decoder
	MME_LxPcmProcessingGlobalParams_t PcmParams; //!< PcmPostProcessings Params
} MME_LxMp2aTransformerGlobalParams_t;

#endif /* _MP2a_DECODERTYPES_H_ */
