/// $Id: AC3_DecoderTypes.h,v 1.6 2003/11/26 17:03:07 lassureg Exp $
/// @file : MMEPlus/Interface/Transformers/AC3_DecoderTypes.h
///
/// @brief : AC3 Audio Decoder specific types for MME
///
/// @par OWNER: Stephan Bergmann
///
/// @author : Stephan Bergmann
///
/// @par SCOPE:
///
/// @date : 2003-03-18
///
/// &copy; 2002 ST Microelectronics. All Rights Reserved.
///

#ifndef _AC3_DECODERTYPES_H_
#define _AC3_DECODERTYPES_H_

#define DRV_MME_AC3_DECODER_VERSION 0x031030

//#include "MMEplus/Interface/mme_api.h"

#include "acc_mmedefines.h"

enum eCompMode
{
	CUSTOM_MODE_0,
	CUSTOM_MODE_1,
	LINE_OUT,
	RF_MODE
};

// Number of frames to decode per MME transform operation
#define NUM_FRAMES_PER_TRANSFORM 1

//! Additional decoder capability structure
typedef struct
{
	U32 StructSize; //!< Size of this structure
} MME_LxAc3TransformerInfo_t;

typedef struct
{
	enum eAccBoolean LfeEnable;
	enum eCompMode CompressMode;
	enum eAccAcMode StereoMode;
	enum eAccBoolean VcrRequested;

	int LDR;
	int HDR;

} MME_LxAc3Config_t;

typedef struct
{
	U32 StructSize; //!< Size of this structure

	MME_LxAc3Config_t LxAc3Config;

	enum eAccAcMode DownMix[ACC_MIX_MAIN_AND_AUX];
	enum eAccDualMode DualMode;

} MME_LxAc3TransformerGlobalParams;

typedef struct
{
	U32 StructSize; //!< Size of this structure

	enum eAccBoolean HighCostVcr;
	enum eAccBoolean CrcOff;
	enum eAccMainChannelIdx ChanPtr[6];

	int RPC; // not used in the decoder
	int PcmScale; // not used in the decoder
	enum eAccBoolean BlockWise;

	MME_LxAc3TransformerGlobalParams GlobalParams;

} MME_LxAc3TransformerInitParams_t;

//! This structure must be passed when sending the TRANSFORM command to the decoder

typedef struct
{
	U32 NumInputBuffers; //!< input MME_DataBuffer_t structures, even if it's zero
	U32 NumOutputBuffers; //!< output MME_DataBuffer_t structures, even if it's zero
	MME_DataBuffer_t buffers[NUM_FRAMES_PER_TRANSFORM * 2]; //!< The input/output buffers
	// after NumInputBuffers input buffers the output buffers are also here
	// U32 NumberOfFrames; //!< AC3 frames contained in these buffers

	int SkipMuteCmd;

} MME_LxAc3TransformerFrameDecodeParams_t;

typedef struct
{

	enum eAccBoolean PcmDownScaled; /* TRUE if signal has been downscaled before downmix */
	int RemainingBlocks; /* not 0 if BlockDecoding requested */
	int NbOutSamples; /* Report the actual number of samples that have been output */
	enum eAccAcMode AudioMode; /* Channel Output configuration */
} MME_LxAc3TransformerFrameStatus_t;

#endif // _AC3_DECODERTYPES_H_
