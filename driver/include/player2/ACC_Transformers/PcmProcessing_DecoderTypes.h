/// $Id: PcmProcessing_DecoderTypes.h,v 1.9 2003/11/26 17:03:09 lassureg Exp $
/// @file : ACC_Multicom/ACC_Transformers/PcmProcessing_DecoderTypes.h
///
/// @brief : Decoder PostProcessing specific types for Multicom
///
/// @par OWNER: Gael Lassure
///
/// @author : Gael Lassure
///
/// @par SCOPE:
///
/// @date : 2004-11-11
///
/// &copy; 2003 ST Microelectronics. All Rights Reserved.
///

#ifndef _PCMPROCESSING_DECODERTYPES_H_
#define _PCMPROCESSING_DECODERTYPES_H_

#include "acc_mmedefines.h"
#include "mme.h"

#include "Pcm_PostProcessingTypes.h"

#define DRV_PCMPROCESSING_MULTICOM_VERSION 0x090415

#define PP_MT_NAME "MME_TRANSFORMER_TYPE_AUDIO_PCMPROCESSOR"

// Number of frames to decode per MME transform operation
#define NUM_FRAMES_PER_TRANSFORM 1
#define NUM_MAX_PAGES_PER_TRANSFORM 16

//! Additional decoder capability structure
typedef struct
{
	U32 StructSize; //!< Size of this structure
	U32 DecoderCapabilityFlags;
	U32 StreamDecoderCapabilityFlags; //!< 1 bit per decoder
	U32 DecoderCapabilityExtFlags[2];
	U32 PcmProcessorCapabilityFlags[2];
} MME_LxPcmProcessingInfo_t;

typedef struct
{
	U16 StructSize; //!< Size of this structure
	U8 DigSplit; //!< Point of split between Dig output and Main/Aux output
	U8 AuxSplit; //!< Point of split between Main output and Aux output
	MME_CMCGlobalParams_t CMC; //!< Common DMix Config : could be used by decoder or by generic downmix
	MME_TsxtGlobalParams_t TsxtVcr; //!< Trusurround XT Configuration for VCR output
	MME_DMixGlobalParams_t DMix; //!< Generic DMix : valid for main and vcr output
	MME_AC3ExGlobalParams_t Ac3Ex; //!< Ac3EX post - processing ( can be applied to any input whether AC3 or not)
	MME_DeEmphGlobalParams_t DeEmph; //!< DeEmphasis Filtering
	MME_PLIIGlobalParams_t Plii; //!< Dolby PrologicII
	MME_TsxtGlobalParams_t TsxtMain; //!< TruSurround XT Configuration for Main Output

#ifdef USE_NEW_PP
	MME_OmniGlobalParams_t OmniVcr; //!< ST OmniSurround for VCR output
	MME_StwdGlobalParams_t StWdVcr; //!< ST WideSurround for VCR output
	MME_CSIIGlobalParams_t Csii; //!< SRS CircleSurround II
	MME_ChsynGlobalParams_t ChSyn; //!< ST Channel Synthesis
	MME_OmniGlobalParams_t OmniMain; //!< ST OmniSurround for Main output
	MME_StwdGlobalParams_t StWdMain; //!< ST WideSurround for Main output

	MME_BassMgtGlobalParams_t BassMgt;
	MME_EqualizerGlobalParams_t Equalizer;
	MME_DCRemoveGlobalParams_t DcRemove;
	MME_DelayGlobalParams_t Delay;

	MME_SigAnalysisGlobalParams_t SigAnalysis;
	MME_EncoderPPGlobalParams_t Encoder;
	MME_SpdifOutGlobalParams_t Spdifout;
#endif

	MME_KokPSGlobalParams_t PitchShift; //!< Kok Pitch Shift control
	MME_KokVCGlobalParams_t VoiceCancel; //!< Kok Voice Cancel control
	MME_TempoGlobalParams_t TempoCtrl; //!< Tempo Control
	MME_es2pesGlobalParams_t Es2Pes; //!< ES2pes
	MME_SfcPPGlobalParams_t Sfc;
	MME_NEOGlobalParams_t Neo; //!< DTS Neo 6
	MME_BTSCGlobalParams_t Btsc;
	MME_VIQGlobalParams_t Viq; //!< SRS VIQ
} MME_LxPcmProcessingGlobalParams_t;

typedef struct
{
	U32 Id;
	U32 StructSize;
	U8 Config[1];
} MME_PcmProcessingStatusTemplate_t;

typedef struct
{
	U32 BytesUsed; // Amount of this structure already filled
	MME_PcmProcessingStatusTemplate_t PcmStatus; // To be replaced with MME_WaterMStatus_t like structures
} MME_PcmProcessingFrameExtStatus_t;

typedef struct
{
	U32 BytesUsed; // Amount of this structure already filled
	MME_WaterMStatus_t PcmStatus; // To be replaced with MME_WaterMStatus_t like structures
} MME_WMFrameStatus_t;

#include "Audio_DecoderTypes.h"
#include "LPCM_DecoderTypes.h"

// APIs to manage special status request from SetGlobal ---> will be returned through CmdStatus->DecStatus / CmdStatus->FrameStatus[2-3]
enum ePPRequest
{
	PP_NO_REQUEST,
	PP_GET_OUTPUTRANGE,

	// do not edit beyond.
	PP_NB_REQUEST
};

enum ePPFrameStatus
{
	PP_FRAMESTATUS0,
	PP_FRAMESTATUS1,
	PP_FRAMESTATUS_OUTPUTRANGE,
	PP_FRAMESTATUS_CMC_MAIN,
};

typedef struct
{
	unsigned int MainFS : 8; //! SamplingFrequency of the Main Output (enum eAccFsCode)
	unsigned int MainRange : 2; //! Range of the Main Ouput (enum eMixerOutRange)
	unsigned int AuxRange : 2; //! Range of the Auxilliary Ouput (enum eMixerOutRange)
	unsigned int SpdifRange : 2; //! Range of the Spdif Output (enum eMixerOutRange)
	unsigned int HdmiRange : 2; //! Range of the Hdmi-Output (enum eMixerOutRange)

	// warning this is limited to 16bits
} tOutputRange;

typedef struct
{
	unsigned int Dmix : 8;
	unsigned int DualMode : 4;
	unsigned int Reserved : 4;
	// warning this is limited to 16bits
} tCMCcfg;

typedef union
{
	unsigned int u32;
	tOutputRange Outputs;
	tCMCcfg CMC;
} uFrameStatus;

typedef struct
{
	enum eAccDecoderId DecoderId;
	U32 StructSize;

	U32 NbChannels; //!< Interleaving of the input pcm buffers
	enum eAccWordSizeCode WordSize; //!< Input WordSize : ACC_WS32 / ACC_WS16
	enum eAccAcMode AudioMode; //!< Channel Configuration
	enum eAccFsCode SamplingFreq; //!< Sampling Frequency
	U32 Request: 4; //!< fill SetGlobalStatus according to enum ePPRequest
	U32 Config: 24; //!< should be set to 0 , For Future use
} MME_PcmInputConfig_t;

typedef struct
{
	U32 StructSize; //!< Size of this structure
	MME_PcmInputConfig_t PcmConfig ; //!< Private Config of Audio Decoder
	MME_LxPcmProcessingGlobalParams_t PcmParams ; //!< PcmPostProcessings Params
} MME_PcmProcessorGlobalParams_t ;

typedef struct
{
	U32 StructSize; //!< Size of this structure

	//! System Init
	enum eAccProcessApply CacheFlush; //!< If ACC_DISABLED then the cached data aren't sent back to DDR
	U32 BlockWise ; //!< Perform the decoding blockwise instead of framewise
	enum eFsRange SfreqRange;

	U8 NChans[ACC_MIX_MAIN_AND_AUX];
	U8 ChanPos[ACC_MIX_MAIN_AND_AUX];

	//! Decoder specific parameters
	MME_PcmProcessorGlobalParams_t GlobalParams;

} MME_LxPcmProcessingInitParams_t;

// Ensure PcmProcessing TRANSFORMs applie and return the same params / status as a LPCM / PCM Decoder
#define MME_PcmProcessingFrameParams_t MME_LxAudioDecoderFrameParams_t
#define MME_PcmProcessingFrameStatus_t MME_LxAudioDecoderFrameStatus_t
#define PP_SPECIAL_DECSTATUS 0xC0DE0000 // should be set to PPFrameStatus->DecStatus with 4lsb specifying how to decode PPFrameStatus->FrameStatus[0]

enum ePPCmdCode
{
	PP_CMD_PLAY = ACC_CMD_PLAY,
	PP_CMD_MUTE = ACC_CMD_MUTE,
	PP_CMD_SKIP = ACC_CMD_SKIP,
	PP_CMD_PAUSE = ACC_CMD_PAUSE,
	PP_COPROCESSOR,

	// Do not edit beyond this entry
	PP_LAST_CMD
};

//! This structure must be passed when sending the TRANSFORM command to the decoder within the U32 FrameParams[ACC_MAX_DEC_FRAME_PARAMS];
typedef struct
{
	unsigned int Sfreq : 8; // enum eAccFsCode;
	unsigned int AcMode : 8; // enum eAccAcMode;
	unsigned int NCh : 4; // NbChannels in input buffer.
	unsigned int Reserved: 12; // for future use
} tPPInputConfig;

typedef union
{
	tPPInputConfig bits;
	U32 u32;
} uPPInputConfig;

enum ePPFrameParams
{
	PP_FRAMEP_EMUTE, // uEmergencyMute
	PP_FRAMEP_FORMAT, // uPPInputConfig
	PP_FRAMEP_CONFIG,
	PP_FRAMEP_RESERVED,
};

// Output Buffer Properties for any output chains
typedef struct
{
	U32 Id; //<! Output Chain ID
	U32 StructSize;
	int NbOutSamples; //<! Report the actual number of samples that have been output
	int NbChannels; //<! Number of Interlaced Channels in Output Buffer
	enum eAccAcMode AudioMode; //<! Channel Output configuration
	enum eAccFsCode SamplingFreq; //<! Sampling Frequency of Output Buffer

} MME_PcmProcessingOutputChainStatus_t;

#endif // _PCMPROCESSING_DECODERTYPES_H_

