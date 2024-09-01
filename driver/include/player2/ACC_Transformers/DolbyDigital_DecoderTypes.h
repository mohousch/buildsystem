/// $Id: DolbyDigital_DecoderTypes.h,v 1.5 2003/11/26 17:03:08 lassureg Exp $
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

#ifndef _DOLBY_DIGITAL_DECODERTYPES_H_
#define _DOLBY_DIGITAL_DECODERTYPES_H_

#define DRV_MULTICOM_DOLBY_DIGITAL_DECODER_VERSION 061004

#include "Audio_DecoderTypes.h"
#include "PcmProcessing_DecoderTypes.h"
#include "acc_mmedefines.h"

// Number of frames to decode per MME transform operation
#define NUM_FRAMES_PER_TRANSFORM 1

enum eAccAc3Status
{
	ACC_AC3_OK,
	ACC_AC3_ERROR_BSI
	// ...
};

enum eAc3CapabilityFlags
{
	ACC_DOLBY_DIGITAL_Ex,
	ACC_DOLBY_DIGITAL_PLUS
};

typedef struct
{
	unsigned int Upsample : 2; /* 0 = Use Metadta, 1 = Always Upsample, 2 = Never Upsample */
	unsigned int AC3Endieness : 1; /* 0 = Big Endian, 1 = Little Endian */
	unsigned int DitherAlgo : 1; /* 0 = DCV Algo, 1 = DEC71 Algo (Useful only in 7.1 build) */
	unsigned int Enable71Dmix : 1; /* 0 = Cert. Mode, 1 = En 7.1 DMix (Useful only in 7.1 build) */
	unsigned int DisableLfeDmix : 1; /* 0 = Normal DDP dmix, 1 = Disable LFE dmix to L&R (RF Mode) */
	unsigned int Disable71Decoder: 1; /* 0 = 7.1 Decoding, 1 = 5.1 Decoding (Useful only in 7.1 build) */
	unsigned int DisableAC3Out : 1; /* 0 = Ac3 o/p generated, 1 = AC3 O/P disabled */
	unsigned int ReportMetaData : 1; // If Set, then report the metadata info to the PcmStatus.
	unsigned int ReportFrameInfo : 1; // If set, then report the Frame Info to the PcmStatus.(tAC3FrameStatus_t)
	unsigned int Reserved : 22; /* 0 = 5.1 Decoding, 1 = 7.1 Decoding (Useful only in 7.1 build) */
} tDDPfeatures;

typedef union
{
	unsigned char ddp_output_settings;
	tDDPfeatures ddp_features;
} uDDPfeatures;

enum eDDConfigIdx
{
	DD_CRC_ENABLE,
	DD_LFE_ENABLE,
	DD_COMPRESS_MODE,
	DD_HDR,
	DD_LDR,
	DD_HIGH_COST_VCR,
	DD_VCR_REQUESTED,
	DDP_FRAMEBASED_ENABLE,
	DDP_OUTPUT_SETTING, // Bit[0..1] :: UPSAMPLE_PCMOUT_ENABLE ; BIT[2] :: LITTLE_ENDIAN_DDout
	/* Do not edit beyond this comment */
	DD_NB_CONFIG_ELEMENTS
};

enum eDDCompMode
{
	DD_CUSTOM_MODE_0,
	DD_CUSTOM_MODE_1,
	DD_LINE_OUT,
	DD_RF_MODE,
	/* Do not edit beyond this element */
	DD_LAST_COMP_MODE
};

typedef struct
{
	enum eAccDecoderId DecoderId;
	U32 StructSize;

	U8 Config[DD_NB_CONFIG_ELEMENTS];
	//config[0]:enum eAccBoolean CrcOff; //!< Disable the CRC Checking
	//config[1]:enum eAccBoolean LfeEnable; //!< Enable the Lfe Processing
	//config[2]:enum eDDCompMode CompMode; //!< Compression Mode (LineOut / Custom 0,1/ RF)
	//config[3]:U8 HDR; //!< Q8 HDR Coefficient : 1.0 <=> 0xFF
	//config[4]:U8 LDR; //!< Q8 LDR Coefficient : 1.0 <=> 0xFF
	//config[5]:enum eAccBoolean HighCostVcr; //!< Dual Decoding Simulatneaous Multichannel + Stereo
	//config[6]:enum eAccBoolean VcrRequested;//!< If ACC_MME_TRUE then process VCR downmix internally
	tCoeff15 PcmScale; //!< Q15 scaling coeff (applied in freq domain) 1.0 <=> 0x7FFF
} MME_LxAc3Config_t;

typedef struct
{
	U32 StructSize; //!< Size of this structure

	MME_CMCGlobalParams_t CMC; //!< Common DMix Config : could be used internally by decoder or by generic downmix
	MME_TsxtGlobalParams_t TSXT; //!< TsXT Configuration
	MME_OmniGlobalParams_t OMNI; //!< Omni Configuration
	MME_StwdGlobalParams_t STWD; //!< WideSurround Configuration
	MME_DMixGlobalParams_t DMix; //!< Generic DMix : valid for main and vcr output

} MME_LxAc3PcmProcessingGlobalParams_t;

typedef struct
{
	U32 StructSize;//!< Size of this structure
	MME_LxAc3Config_t Ac3Config; //!< Private Config of MP2 Decoder
	MME_LxPcmProcessingGlobalParams_t PcmParams; //!< PcmPostProcessings Params

} MME_LxAc3TransformerGlobalParams_t;

typedef struct
{
	enum eAccDecoderId DecoderId; //!< ID of the deocder.
	U32 StructSize; //!< Size of this structure
	MME_ChannelMapInfo_t ChannelMap; //!< Description of the encoded input channels.
	U32 BitDepth : 3; //!< eAccBitdepth :Encoded bit resolution
	U32 EncSampleRate : 5; //!< Original Encoding Sample rate in eFsCode
	U32 ConvSyncStatus : 5; //!< DDPI_DCV_SYNC_STATUS_NOCHANGE_NOSYNC = 0,
	//!< DDPI_DCV_SYNC_STATUS_NOCHANGE_INSYNC = 2,
	//!< DDPI_DCV_SYNC_STATUS_FOUND = 4,
	//!< DDPI_DCV_SYNC_STATUS_FOUNDEARLY = 8,
	//!< DDPI_DCV_SYNC_STATUS_LOST = 16
	U32 TranscoderStatus : 3; //!< DDPI_DCV_CONV_STATUS_NO_DATA = 0,
	//!< DDPI_DCV_CONV_STATUS_FRMSET_OK = 2,
	//!< DDPI_DCV_CONV_STATUS_FRMSET_ERR = 4
	U32 DecoderStatus : 16;

} tAC3FrameStatus_t;

typedef struct
{
	U16 DecoderStatus;
	U16 TranscoderStatus;
	U16 ConvSyncStatus;
	U16 TranscoderFreq;
} tMMEDDPlusStatus;

#endif /* _DOLBY_DIGITAL_DECODERTYPES_H_ */
