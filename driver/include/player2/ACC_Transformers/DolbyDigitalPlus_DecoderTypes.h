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

#ifndef _DOLBY_DIGITAL_PLUS_DECODERTYPES_H_
#define _DOLBY_DIGITAL_PLUS_DECODERTYPES_H_

#define DRV_MULTICOM_DOLBY_DIGITAL_PLUS_DECODER_VERSION 061004

#include "Audio_DecoderTypes.h"
#include "PcmProcessing_DecoderTypes.h"
#include "DolbyDigital_DecoderTypes.h"
#include "acc_mmedefines.h"

// Number of frames to decode per MME transform operation
#define NUM_FRAMES_PER_TRANSFORM 1

enum eAccDDPStatus
{
	ACC_DDPLUS_OK,
	ACC_DDPLUS_ERROR_BSI
	// ...
};

#define ACC_DDPLUS_STREAM (1<<16)
/*enum eAc3CapabilityFlags
{
 ACC_DOLBY_DIGITAL_Ex,
 ACC_DOLBY_DIGITAL_PLUS
};*/

//enum eDDPConfigIdx
//{
//	DDP_CRC_ENABLE,
//	DDP_FRAMEBASED_ENABLE,
//	DDP_LFE_ENABLE,
//	DDP_COMPRESS_MODE,
//	DDP_HDR,
//	DDP_LDR,
//	DDP_STEREO_MODE,
//	DDP_DUAL_MODE,
//	DDP_KMODE,
//	DDP_OUTPUT_SETTING, // Bit[0..1] :: UPSAMPLE_PCMOUT_ENABLE ; BIT[2] :: LITTLE_ENDIAN_DDout
//
//	/* Do not edit beyond this comment */
//	DDP_NB_CONFIG_ELEMENTS
//};

enum eDDPLUSCompMode
{
	DDP_CUSTOM_MODE_0,
	DDP_CUSTOM_MODE_1,
	DDP_LINE_OUT,
	DDP_RF_MODE,

	/* Do not edit beyond this element */
	DDP_LAST_COMP_MODE
};

/*enum eDDPlusBufferParams
{
 DDPLUS_BUFFER_TYPE, //!< indicates that the given buffer is the last packet of the file
 DDPLUS_SKIP_FRAMES, //!< indicates which frame in current Packet to skip

 // do not edit below this line
 DDPLUS_BUFFER_PARAMS_NB_ELEMENTS
};*/

enum eDDPlusBufferType
{
	DDPLUS_NORMAL_BUFFER,
	DDPLUS_LAST_BUFFER
};

typedef struct
{
	U8 Status; //!< Status based on enum eOvStatus

} tMMEDDPLUSStatus;

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
} MME_LxDDPConfig_t;

typedef struct
{
	U32 StructSize; //!< Size of this structure

	MME_CMCGlobalParams_t CMC; //!< Common DMix Config : could be used internally by decoder or by generic downmix
	MME_TsxtGlobalParams_t TSXT; //!< TsXT Configuration
	MME_OmniGlobalParams_t OMNI; //!< Omni Configuration
	MME_StwdGlobalParams_t STWD; //!< WideSurround Configuration
	MME_DMixGlobalParams_t DMix; //!< Generic DMix : valid for main and vcr output

} MME_LxDDPPcmProcessingGlobalParams_t;

typedef struct
{
	U32 StructSize;//!< Size of this structure
	MME_LxDDPConfig_t DDPConfig; //!< Private Config of MP2 Decoder
	MME_LxPcmProcessingGlobalParams_t PcmParams; //!< PcmPostProcessings Params

} MME_LxDDPTransformerGlobalParams_t;

// Macros to be used by Host
/* (dmixEnable == ACC_TRUE) => SET DOWNMIXING
 (dmixEnable == ACC_FALSE) => DISABLE DOWNMIXING */
#define DDPLUS_SET_DMIX71(Config, dmixEnable) ((dmixEnable) ? (Config[DDP_OUTPUT_SETTING] &= ~(1 << 3)) : \
					       (Config[DDP_OUTPUT_SETTING] |= (1 << 3)) )
// Macros to be used by Companion
#define DDPLUS_GET_DMIX71(Config) ((Config[DDP_OUTPUT_SETTING] & (1 << 3)) ? ACC_FALSE : ACC_TRUE)

#endif /* _DOLBY_DIGITAL_PLUS_DECODERTYPES_H_ */
