/// $Id: Mixer_ProcessorTypes.h,v 1.3 2003/11/26 17:03:09 lassureg Exp $
/// @file : ACC_Multicom/ACC_Transformers/Mixer_ProcessorTypes.h
///
/// @brief : Mixer Processor specific types for Multicom
///
/// @par OWNER: Gael Lassure
///
/// @author : Gael Lassure
///
/// @par SCOPE:
///
/// @date : 2004-11-30
///
/// &copy; 2004 ST Microelectronics. All Rights Reserved.
///

#ifndef _AUDIOMIXER_PROCESSORTYPES_H_
#define _AUDIOMIXER_PROCESSORTYPES_H_

#ifndef DRV_MULTICOM_MIXER_VERSION
#define DRV_MULTICOM_MIXER_VERSION 0x070105
#endif

#include "mme.h"
#include "PcmProcessing_DecoderTypes.h"
#include "Pcm_PostProcessingTypes.h"
#include "acc_mmedefines.h"

// Mixer state
enum eMixerState
{
	MIXER_INPUT_NOT_RUNNING,
	MIXER_INPUT_RUNNING,
	MIXER_INPUT_FADING_OUT,
	MIXER_INPUT_MUTED,
	MIXER_INPUT_PAUSED,
	MIXER_INPUT_FADING_IN
};

// Number of frames to decode per MME transform operation
#define NUM_FRAMES_PER_TRANSFORM 1

#define ACC_MIXER_MAX_NB_INPUT 4
#define ACC_MIXER_MAX_NB_OUTPUT 4
#define ACC_MIXER_MAX_INPUT_PAGES_PER_STREAM 16 // max nb of pages we want to send per input stream for each transform
#define ACC_MIXER_MAX_OUTPUT_PAGES_PER_STREAM 1 // max nb of pages we want to send per output stream for each transform

#define ACC_MIXER_MAX_IAUDIO_NB_INPUT 8

#define ACC_MIXER_MAIN_CHANNEL_NB (ACC_MAIN_CSURR + 1)

#define MAX_ACC_VOL 0xFFFF
#define MIN_ACC_VOL 0x0
#define ACC_0dB_uQ0_16 0xFFFF
#define ACC_0dB_uQ3_13 0x2000
#define ACC_ALPHA_DONT_CARE 0x0001 /* set the old Alpha field to this value if using bd structures with alpha per channel */

#define ACC_MIXER_BEEPTONE_MAX_NTONE 4
#define BEEPTONE_GET_FREQ(x) (((x & 0x3f) * 4) * (((((100 << 16) + (10 << 8) + 1) >> (8 * (x>>6))))&0xff))
#define BEEPTONE_SET_FREQ(x) ((x>=2560) ? (((x / 400) & 0x3F) + 0x80) : ((x>=256) ? (((x/40) & 0x3F) + 0x40) : (((x/4)&0x3F))))

//Mixer Configuration Type IDs :
enum eAccMixerId
{
	ACC_RENDERER_MIXER_ID,
	// ?? if any other mixing technology ??

	// All postprocessing type
	ACC_RENDERER_MIXER_POSTPROCESSING_ID,

	// Generic Output Config
	ACC_RENDERER_MIXER_OUTPUT_CONFIG_ID,

	// BD Iaudio Config
	ACC_RENDERER_MIXER_BD_IAUDIO_ID,

	// BD Input Gain Config
	ACC_RENDERER_MIXER_BD_GAIN_ID,

	// BD input panning Config
	ACC_RENDERER_MIXER_BD_PANNING_ID,

	// General BD Config
	ACC_RENDERER_MIXER_BD_GENERAL_ID,

	// Do not edit below this line
	ACC_RENDERER_LAST_MIXER_ID
};

//Mixer Configuration Type IDs :
enum eAccMixerStatusId
{
	ACC_MIXER_EXTENDED_OUTPUT_ID,
};

enum eMixerDataBufferType
{
	ACC_MIXER_DATABUFFER_FLAG_REGULAR = 0,
	ACC_MIXER_DATABUFFER_FLAG_IAUDIO_MONO = 1,
	ACC_MIXER_DATABUFFER_FLAG_IAUDIO_STEREO = 2,
	ACC_MIXER_DATABUFFER_FLAG_COMPRESSED_DATA = 4
};

enum eAccRendererProcId
{
	ACC_RENDERER_BASSMGT_ID = (ACC_RENDERER_PROCESS_MT << 8), // BassMgt + Volume Control + Delay
	ACC_RENDERER_EQUALIZER_ID,
	ACC_RENDERER_TEMPO_ID,
	ACC_RENDERER_DCREMOVE_ID,
	ACC_RENDERER_DOLBYHEADPHONE_ID,
	ACC_RENDERER_STHEADPHONE_ID,
	ACC_RENDERER_CUSTOMHEADPHONE_ID,
	ACC_RENDERER_CUSTOMMIXERPOSTPROC_ID,
	ACC_RENDERER_DELAY_ID,
	ACC_RENDERER_SFC_ID,
	ACC_RENDERER_ENCODERPP_ID,
	ACC_RENDERER_SPDIFOUT_ID,
	ACC_RENDERER_CMC_ID,
	ACC_RENDERER_DMIX_ID,
	ACC_RENDERER_BTSC_ID,
	ACC_RENDERER_VIQ_ID,
	ACC_LAST_RENDERERPROCESS_ID
};

enum eAccMixerCapabilityFlags
{
	ACC_MIXER_PREPROCESSING,
	ACC_MIXER_PROCESSING,
	ACC_MIXER_POSTPROCESSING,
	ACC_MIXER_INT_AUDIO
	// ACC_MIXER_LIMITER
};

//! Additional decoder capability structure
typedef struct
{
	U32 StructSize; //!< Size of this structure
	U32 MixerCapabilityFlags;
	U32 PcmProcessorCapabilityFlags[2];
} MME_LxMixerTransformerInfo_t;

enum eMixerInputType
{
	ACC_MIXER_LINEARISER,
	ACC_MIXER_BEEPTONE_GENERATOR,
	ACC_MIXER_PINKNOISE_GENERATOR,
	ACC_MIXER_IAUDIO,
	ACC_MIXER_COMPRESSED_DATA,

	// do not edit further
	ACC_NB_MIXER_INPUT_TYPE,
	ACC_MIXER_INVALID_INPUT
};

typedef struct
{
	U8 InputId; //!< b[7..4] :: Input Type | b[3..0] :: Input Number
	U8 NbChannels; //!< Interleaving of the input pcm buffers
	U16 Alpha; //!< Mixing Coefficient for Each Input
	U16 Mono2Stereo; //!< [enum eAccBoolean] Mono 2 Stereo upmix of a mono input
	U16 Reserved; //!< Inserted by Compiler for alignment issue
	enum eAccWordSizeCode WordSize; //!< Input WordSize : ACC_WS32 / ACC_WS16
	enum eAccAcMode AudioMode; //!< Channel Configuration
	enum eAccFsCode SamplingFreq; //!< Sampling Frequency
	enum eAccMainChannelIdx FirstOutputChan; //!< To which output channel is mixer the 1st channel of this input stream
	enum eAccBoolean AutoFade;
	U32 Config; //!< Special config (applicable to generators);
} MME_MixerInputConfig_t;

typedef struct
{
	enum eAccMixerId Id; //!< ID of this processing
	U32 StructSize; //!< Size of this structure
	MME_MixerInputConfig_t Config[ACC_MIXER_MAX_NB_INPUT];
} MME_LxMixerInConfig_t;

// get/set compressed data output identifier from the MME_LxMixerInConfig_t structure
#define AUDIOMIXER_SET_COMPRESSED_DATA_OUTPUT_ID(input_config, out_id) ((input_config)->Config |= (out_id & 0xFFFF))

#define AUDIOMIXER_GET_COMPRESSED_DATA_OUTPUT_ID(input_config) (((input_config)->Config>>0) & 0xFFFF)

#define AUDIOMIXER_OVERRIDE_OUTFS 0x1FF

typedef struct
{
	enum eAccMixerId Id; //!< ID of this processing
	U32 StructSize; //!< Size of this structure

	unsigned int NbOutputSamplesPerTransform : 16; //!< Number of Output samples per transform

	//!< Override OutSfreq when InputParams->OutputSamplingFreq == AUDIOMIXER_OVERRIDE_OUTFS
	unsigned int MainFS : 8; //! SamplingFrequency of the Main Output (enum eAccFsCode)
	unsigned int Reserved : 8;
} MME_LxMixerOutConfig_t;

typedef struct
{
	enum eAccMixerId Id; //!< Id of the PostProcessing structure.
	U16 StructSize; //!< Size of this structure
	U8 DigSplit; //!< Point of split between Dig output and Main/Aux output
	U8 AuxSplit; //!< Point of split between Main output and Aux output

	MME_BassMgtGlobalParams_t BassMgt;
	MME_EqualizerGlobalParams_t Equalizer;
	MME_TempoGlobalParams_t TempoControl;
	MME_DCRemoveGlobalParams_t DCRemove;
	MME_DelayGlobalParams_t Delay;
	MME_EncoderPPGlobalParams_t Encoder;
	MME_SfcPPGlobalParams_t Sfc;
	MME_CMCGlobalParams_t CMC;
	MME_DMixGlobalParams_t Dmix;
	MME_SpdifOutGlobalParams_t Spdifout;
	MME_BTSCGlobalParams_t Btsc;
	MME_VIQGlobalParams_t Viq;
} MME_LxPcmPostProcessingGlobalParameters_t; //!< PcmPostProcessings Params

/* All the following structures may apply to Blu Ray Mixer */

typedef struct
{
	U32 InputId; //!< Input Number
	U16 AlphaPerChannel[ACC_MIXER_MAIN_CHANNEL_NB]; //!< Mixing coefficient for each channel (applies for DD+ primary stream)
} MME_LxMixerInGainSet_t; //!< GainSet params

typedef struct
{
	enum eAccMixerId Id; //!< Id of the GainSet structure.
	U32 StructSize; //!< Size of this structure
	MME_LxMixerInGainSet_t GainSet[ACC_MIXER_MAX_NB_INPUT];
} MME_LxMixerGainSet_t;

typedef struct
{
	U32 InputId; //!< Input Number
	U16 Panning[ACC_MIXER_MAIN_CHANNEL_NB]; //!< Panning coefficient for each channel (applies for mono secondary stream)
} MME_LxMixerInPanningSet_t;

typedef struct
{
	enum eAccMixerId Id; //!< Id of the PanningSet structure.
	U32 StructSize; //!< Size of this structure
	MME_LxMixerInPanningSet_t PanningSet[ACC_MIXER_MAX_NB_INPUT];
} MME_LxMixerPanningSet_t;

typedef struct
{
	enum eAccMixerId Id; //!< Id of the BluRay General structure.
	U32 StructSize; //!< Size of this structure
	U16 PostMixGain;
	U16 Reserved;
	enum eAccBoolean GainSmoothEnable;
	enum eAccBoolean OutputLimiterEnable;
} MME_LxMixerBDGeneral_t;

typedef struct
{
	U8 InputId; //!< Input Number
	U8 NbChannels; //!< Interleaving of the input pcm buffers
	U16 Alpha; //!< Mixing Coefficient for Each Input
	U16 Panning[ACC_MIXER_MAIN_CHANNEL_NB]; //!< Panning coefficient for each channel
	enum eAccBoolean Play; //!< Play/Stop for each input
} MME_MixerIAudioInputConfig_t;

typedef struct
{
	enum eAccMixerId Id; //!< ID of this processing
	U32 StructSize; //!< Size of this structure
	U32 NbInteractiveAudioInput;
	MME_MixerIAudioInputConfig_t ConfigIAudio[ACC_MIXER_MAX_IAUDIO_NB_INPUT];
} MME_LxMixerInIAudioConfig_t;

/* end of Blu Ray specific structures */

typedef struct
{
	U32 StructSize; //!< Size of this structure
	MME_LxMixerInConfig_t InConfig; //!< Specific configuration of input
	MME_LxMixerOutConfig_t OutConfig; //!< output specific configuration information
	MME_LxPcmPostProcessingGlobalParameters_t PcmParams; //!< PcmPostProcessings Params
} MME_LxMixerTransformerGlobalParams_t;

typedef struct
{
	U32 StructSize; //!< Size of this structure
	MME_LxMixerInConfig_t InConfig; //!< Specific configuration of input
	MME_LxMixerGainSet_t InGainConfig; //!< Specific configuration of input gains
	MME_LxMixerPanningSet_t InPanningConfig; //!< Specific configuration of input panning
	MME_LxMixerInIAudioConfig_t InIaudioConfig; //!< Specific configuration of iaudio input
	MME_LxMixerBDGeneral_t InBDGenConfig; //!< some geenral config for BD mixer
	MME_LxMixerOutConfig_t OutConfig; //!< output specific configuration information
	MME_LxPcmPostProcessingGlobalParameters_t PcmParams; //!< PcmPostProcessings Params
} MME_LxMixerBDTransformerGlobalParams_t;

typedef struct
{
	unsigned int NbChanMain : 4;
	unsigned int StrideMain : 4;

	unsigned int NbChanAux : 4;
	unsigned int StrideAux : 4;

	unsigned int NbChanDig : 4;
	unsigned int StrideDig : 4;

	unsigned int NbChanHdmi : 4;
	unsigned int StrideHdmi : 4;

} MME_OutChan_t;

typedef struct
{
	U32 StructSize; //!< Size of this structure

	//! System Init
	enum eAccProcessApply CacheFlush; //!< If ACC_DISABLED then the cached
	//!< data aren't sent back at the end of the command

	//! Mixer Init Params
	U32 NbInput;
	U32 MaxNbOutputSamplesPerTransform;

	U32 OutputNbChannels; //! To cast to MME_OutChan_t in order to access extended API.
	enum eAccFsCode OutputSamplingFreq; //! Set to 0x0FF if Master Clock is Clock of Input[0]
	//! Set to 0x1FF if Master Clock is overridden by OutputConfig
	//! Mixer specific global parameters
	MME_LxMixerTransformerGlobalParams_t GlobalParams;

} MME_LxMixerTransformerInitParams_t;

typedef struct
{
	U32 StructSize; //!< Size of this structure

	//! System Init
	enum eAccProcessApply CacheFlush; //!< If ACC_DISABLED then the cached
	//!< data aren't sent back at the end of the command

	//! Mixer Init Params
	U32 NbInput;
	U32 MaxNbOutputSamplesPerTransform;

	U32 OutputNbChannels; //! To cast to MME_OutChan_t in order to access extended API.
	enum eAccFsCode OutputSamplingFreq;

	//! Mixer specific global parameters
	MME_LxMixerBDTransformerGlobalParams_t GlobalParams;

} MME_LxMixerTransformerInitBDParams_t;

enum eMixerCommand
{
	MIXER_STOP,
	MIXER_PLAY,
	MIXER_FADE_OUT, ///!< Invalid command. for compatibility only
	MIXER_MUTE,
	MIXER_PAUSE,
	MIXER_FADE_IN, ///!< Invalid command. for compatibility only
	MIXER_FOFI,
	MIXER_LAST_COMMAND
};

typedef struct
{
	U16 Command; //!< enum eMixerCommand : Play / Mute / Pause / Fade in - out ...
	U16 StartOffset; //!< start offset of the given input [when exiting of Pause / Underflow ]
	U32 PTSflags;
	U32 PTS;
} tMixerFrameParams;

//! This structure must be passed when sending the TRANSFORM command to the decoder
typedef struct
{
	tMixerFrameParams InputParam[ACC_MIXER_MAX_NB_INPUT]; //!< Input Params attached to given input buffers
} MME_LxMixerTransformerFrameDecodeParams_t;

typedef struct
{
	enum eMixerState State;
	U32 BytesUsed;
	U32 NbInSplNextTransform;
} MME_MixerInputStatus_t;

typedef struct
{
	U32 StructSize;
	// System report
	U32 ElapsedTime; //<! elapsed time to do the transform in microseconds
	MME_MixerInputStatus_t InputStreamStatus[ACC_MIXER_MAX_NB_INPUT];
} MME_LxMixerTransformerSetGlobalStatusParams_t;

typedef struct
{
	U32 StructSize;

	// Mixed signal Properties
	enum eAccAcMode MixerAudioMode; //<! Channel Output configuration
	enum eAccFsCode MixerSamplingFreq;//<! Sampling Frequency of Output PcmBuffer

	int NbOutSamples; //<! Report the actual number of samples that have been output
	enum eAccAcMode AudioMode; //<! Channel Output configuration
	enum eAccFsCode SamplingFreq; //<! Sampling Frequency of Output PcmBuffer

	enum eAccBoolean Emphasis; //<! Buffer has emphasis
	U32 ElapsedTime; //<! elapsed time to do the transform in microseconds

	U32 PTSflag; //<! PTSflag[b0..1] = PTS - Validity / PTSflag[b16] = PTS[b32]
	U32 PTS; //<! PTS[b0..b31]

	// Input consumption feedback
	MME_MixerInputStatus_t InputStreamStatus[ACC_MIXER_MAX_NB_INPUT];

} MME_LxMixerTransformerFrameDecodeStatusParams_t;

typedef struct
{
	U32 Id;
	U32 StructSize;
	U8 Config[1];
} MME_MixerStatusTemplate_t;

typedef struct
{
	U32 BytesUsed; // Amount of this structure already filled
	MME_MixerStatusTemplate_t MixExtStatus; // To be replaced with MME_MixerOutputExtStatus_t like structures
} MME_MixerFrameExtStatus_t;

typedef struct
{
	U32 BytesUsed; // Amount of this structure already filled
	MME_PcmProcessingOutputChainStatus_t Main;
	MME_PcmProcessingOutputChainStatus_t Aux;
	MME_PcmProcessingOutputChainStatus_t Spdif;
	MME_PcmProcessingOutputChainStatus_t HDMI;
} MME_MixerFrameOutExtStatus_t;

typedef struct
{
	MME_LxMixerTransformerFrameDecodeStatusParams_t MixStatus;
	MME_MixerFrameExtStatus_t MixExtStatus;
} MME_LxMixerTransformerFrameMixExtendedParams_t;

#endif /* _AUDIOMIXER_PROCESSORTYPES_H_ */
