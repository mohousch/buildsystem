#ifndef _TRUEHD_DECODERTYPES_H_
#define _TRUEHD_DECODERTYPES_H_

//#define DRV_MME_MLP_DECODER_VERSION 0x031124
#define DRV_MME_TRUEHD_DECODER_VERSION 0x00001 //(TO ask)

#include "acc_mmedefines.h"
#include "Audio_DecoderTypes.h"

enum eTRUEHDCapabilityFlags
{
	ACC_DOLBY_TRUEHD_LOSSLESS
};

typedef struct
{
	unsigned int dialrefSupplied : 1; /* Used to enable dialref providede by user */
	unsigned int DecodingMode : 2; /* 0 = 8_channel_deoding, 1 = 6_channel_deocding , 2 = 2_channel_decoding */
	unsigned int ReportFrameInfo : 1; // If set, then report the Frame Info to the PcmStatus.(tTrueHDFrameStatus_t)
	unsigned int Reserved : 28; /* Reserved bits */
} tTrueHDfeatures;

typedef union
{
	unsigned char TrueHD_Dec_Settings;
	tTrueHDfeatures TrueHD_Features;
} uTrueHDfeatures;

enum eTrueHDConfig
{
	//TRUEHD_CRC_ENABLE,
	//TRUEHD_ARCHIVE,
	TRUEHD_BITFIELD_FEATURES, // Future use
	TRUEHD_DRC_ENABLE,
	TRUEHD_PP_ENABLE, // Enables postprocessing
	TRUEHD_DIALREF,
	TRUEHD_LDR,
	TRUEHD_HDR,
	TRUEHD_SERIAL_ACCESS_UNITS,
	/* Do not edit beyond this line */
	TRUEHD_NB_CONFIG_ELEMENTS
};

typedef struct
{
	enum eAccDecoderId DecoderId;
	U32 StructSize;

	U8 Config[TRUEHD_NB_CONFIG_ELEMENTS];
} MME_LxTruehdConfig_t;

// Macros to be used by Host // TO ASK what is this
//
//#define TRUEHDMME_SET_DRC(x, drc) x[TRUEHD_DRC_ENABLE] = ((x[TRUEHD_DRC_ENABLE] & 0xFE) | drc)
//#define TRUEHDMME_SET_PP(x, pp ) x[TRUEHD_DRC_ENABLE] = ((x[TRUEHD_DRC_ENABLE] & 0xFD) | (pp << 1))
//#define TRUEHDMME_SET_CHREDIR(x, chredir) x[TRUEHD_DRC_ENABLE] = ((x[TRUEHD_DRC_ENABLE] & 0xFB) | (chredir << 2))
//
////
////// Macros to be used by Companion
//#define TRUEHDMME_GET_DRC(x) (enum eBoolean) ((x[TRUEHD_DRC_ENABLE] >> 0) &1)
//#define TRUEHDMME_GET_PP(x) (enum eBoolean) ((x[TRUEHD_DRC_ENABLE] >> 1) &1)
//#define TRUEHDMME_GET_CHREDIR(x) (enum eBoolean) ((x[TRUEHD_DRC_ENABLE] >> 2) &1)

enum eFormatType
{
	FBA_Format,
	FBB_Format,
};

typedef struct
{
	enum eAccDecoderId DecoderId; // ID of the deocder.
	U32 StructSize; //!< Size of this structure
	MME_ChannelMapInfo_t ChannelMap; // Description of the encoded input channels.

	U32 BitDepth : 3; //!< eAccBitdepth :Encoded bit resolution
	U32 EncSampleRate : 5; //!< Original Encoding Sample rate
	U32 FormatType : 1; // eFormatType : 0 = FBA(TrueHD type) , 1 = FBB (MLP Type)
	U32 SubStreams : 4; // Upto 16 SubStreams are possible but decoder can decode upto first 3 sub streams only
	U32 DRCEnable : 1; // DRC is enable in the stream or not
	U32 ChannelModifier : 2; // Indicates that L-R/Ls-Rs are encoded for PLII/Dolby Surround Ex.
	U32 Gain_Required : 3; // The gain_required bit field contains the amount of makeup gain that a product must apply for the Dolby TrueHD clip protection system.
	U32 Reserved : 13;
} tTrueHDFrameStatus_t;

#endif /* _MLP_DECODERTYPES_H_ */
