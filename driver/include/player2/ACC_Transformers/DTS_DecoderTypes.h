/// $Id: DTS_DecoderTypes.h,v 1.2 2003/11/26 17:03:08 lassureg Exp $
/// @file : MMEPlus/Interface/Transformers/Dts_DecoderTypes.h
///
/// @brief : DTS Audio Decoder specific types for MME
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

#ifndef _DTS_DECODERTYPES_H_
#define _DTS_DECODERTYPES_H_

#define DRV_MME_DTS_DECODER_VERSION 0x060220 //0x051112

#include "PcmProcessing_DecoderTypes.h"
#include "acc_mmedefines.h"
#include "Audio_DecoderTypes.h"

#define DTS_DEFAULT_NBSAMPLES_PER_TRANSFORM 2048

enum eDTSCapabilityFlags
{
	ACC_DTS_ES,
	ACC_DTS_96K,
	ACC_DTS_HD,
	ACC_DTS_XLL,
};

#define DTS_GET_NBBLOCKS(n) (n >> 8)
#define DTS_GET_NBSAMPLES(n) (n << 8)

enum eDtsConfigIdx
{
	DTS_CRC_ENABLE,
	DTS_LFE_ENABLE,
	DTS_DRC_ENABLE,
	DTS_ES_ENABLE,
	DTS_96K_ENABLE,
	DTS_NBBLOCKS_PER_TRANSFORM,
	DTS_XBR_ENABLE,
	DTS_XLL_ENABLE,
	DTS_MIX_LFE,
	DTS_LBR_ENABLE,
	/* Do not edit below this comment */
	DTS_NB_CONFIG_ELEMENTS
};

//enum eDtshdConfigIdx
//{
//	DTS_CRC_ENABLE,
//	DTS_LFE_ENABLE,
//	DTS_DRC_ENABLE,
//	DTS_ES_ENABLE,
//	DTS_96K_ENABLE,
//	DTS_NBBLOCKS_PER_TRANSFORM,
//	DTS_XBR_ENABLE,
//	DTS_XLL_ENABLE,
//	DTS_MIX_LFE,
//	DTS_LBR_ENABLE,
//	/* Do not edit below this comment */
//	DTSHD_NB_CONFIG_ELEMENTS
//};

typedef struct
{
	U32 DRC : 7; // DRCPercentage; Values from 0 to 100
	U32 SampleFreq : 8; //
	U32 DialogNorm : 1; // Enable / Disable Dialog Normalisation
	U32 MultipleAsset : 1; // Enable / Disable Multiple Asset Decoding
	U32 AudioPresentation: 4; // No of Audio Presentations
	U32 SndField : 1; // Sound Field Select ( 0/1)
	U32 DefaultSpkMap : 1; // Enable / Disable Default Speaker Remapping
	U32 BitDepth : 1; // Select Bit Depth ( 24 / 16 )
	U32 AnalogComp : 1; // Enable / Disable Analog Compensation During DMix
	U32 ReportDmixTable : 1; // If Set, then report the Dmix table from metadata to the PcmStatus.
	U32 ReportRev2Meta : 1; // If Set, then report the DialNorm/DRC info from metadata to the PcmStatus.
	U32 Only96From192K : 1; // Disable decoding of 192K only 96K O/P produced.
	U32 ReportFrameInfo : 1; // If set, then report the Frame Info to the PcmStatus.(tDTSFrameStatus_t)
	U32 Reserved : 3; // Reserved for future use

	U32 Features; // Reserved for future extns. Should be 0.
}
PostProcessings_t;

typedef struct
{
	enum eAccDecoderId DecoderId;
	U32 StructSize;

	U8 Config[DTS_NB_CONFIG_ELEMENTS];

	PostProcessings_t PostProcessing;

	U8 FirstByteEncSamples;
	U32 Last4ByteEncSamples;
	U16 DelayLossLess;

} MME_LxDtsConfig_t;

typedef struct
{
	U32 StructSize; //!< Size of this structure
	MME_LxDtsConfig_t DtsConfig; //!< Private Config of MP2 Decoder
	MME_LxPcmProcessingGlobalParams_t PcmParams; //!< PcmPostProcessings Params
} MME_LxDtsTransformerGlobalParams_t;

enum eDTSHD
{
	DTS,
	DTSHD_96k,
	DTSHD_HR,
	DTSHD_MA
};

enum eDTSES
{
	DTS_51,
	DTS_61_MATRIX,
	DTS_61_DISCRETE,
	DTS_71_DISCRETE
};

typedef struct
{
	U32 BitDepth : 3; //!< eAccBitdepth :Encoded bit resolution
	U32 EncSampleRate : 5; //!< Original Encoding Sample rate
	U32 IsMultAsset : 1; //!< Is Multiple Asset Available
	U32 AmbisonicAudio : 1; //!< Is Audio I/P Ambisonic
	U32 ReservedDetail : 2;
	U32 nAudioPresentation : 4; //!< No of Audio Presentations Available

	U32 CoreSubstream : 1; // enum DTS_EXTSUBSTREAM
	U32 XXCH : 1;
	U32 X96 : 1;
	U32 XCH : 1;

	U32 ExSubStreamCORE : 1;
	U32 ExSubStreamXBR : 1;
	U32 ExSubStreamXXCH : 1;
	U32 ExSubStreamX96 : 1;

	U32 ExSubStreamLBR : 1;
	U32 ExSubStreamXLL : 1;
	U32 ExSubStreamAUX1 : 1;
	U32 ExSubStreamAUX2 : 1;

	U32 isDTSHD : 2; // enum eDTSHD
	U32 isDTSES : 2; // enum eDTSES
} tDTSSubStreamsDetails;

typedef struct
{
	U32 BitDepth : 3; //!< eAccBitdepth :Encoded bit resolution
	U32 EncSampleRate : 5; //!< Original Encoding Sample rate

	U32 IsMultAsset : 1; //!< Is Multiple Asset Available
	U32 AmbisonicAudio : 1; //!< Is Audio I/P Ambisonic
	U32 ReservedDetail : 2;
	U32 nAudioPresentation : 4; //!< No of Audio Presentations Available

	U32 SubStreams : 12;

	U32 Display : 4;
} tDTSSubStreams;

typedef union
{
	tDTSSubStreamsDetails Bits;
	tDTSSubStreams Fields;
	U32 u32;
} uDTSHDSubStreams;

typedef struct
{
	enum eAccDecoderId DecoderId; //!< ID of the deocder.
	U32 StructSize; //!< Size of this structure
	MME_ChannelMapInfo_t ChannelMap; //!< Description of the encoded input channels.

	uDTSHDSubStreams BSI;
} tDTSFrameStatus_t;

#endif /* _DTS_DECODERTYPES_H_ */
