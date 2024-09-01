/// $Id: Audio_DecoderTypes.h,v 1.3 2003/11/26 17:03:07 lassureg Exp $
/// @file : ACC_Multicom/ACC_Transformers/Audio_DecoderTypes.h
///
/// @brief : Generic Audio Decoder types for Multicom
///
/// @par OWNER: Gael Lassure
///
/// @author : Gael Lassure
///
/// @par SCOPE:
///
/// @date : 2004-11-1
///
/// &copy; 2003 ST Microelectronics. All Rights Reserved.
///

#ifndef _AUDIO_DECODERTYPES_H_
#define _AUDIO_DECODERTYPES_H_

#define DRV_MULTICOM_AUDIO_DECODER_VERSION 0x090228

#include "mme.h"
#include "acc_mmedefines.h"

// Number of frames to decode per MME transform operation
#define AUDIO_DECODER_NUM_FRAMES_PER_TRANSFORM 1
#define AUDIO_DECODER_NUM_PAGES_PER_BUFFER 4
#define AUDIO_DECODER_NUM_BUFFERS_PER_SENDBUFFER AUDIO_STREAMING_NUM_BUFFERS_PER_SENDBUFFER
#define ACC_AUDIO_DECODER_NB_MAX_INPUT_BUFFER 1

#define ACC_MAX_DEC_CONFIG_SIZE 23
#define ACC_MAX_BUFFER_PARAMS 2
#define ACC_MAX_DEC_FRAME_PARAMS 4
#define ACC_MAX_DEC_FRAME_STATUS 4

#define ACC_MAX_MIXING_OUPUT_CONF 4

#define ACC_NBMAX_AUDIODECODER 32
enum eAccDecoderId
{
	ACC_PCM_ID = (ACC_DECODER_MT << 8) ,
	ACC_AC3_ID,
	ACC_MP2A_ID,
	ACC_MP3_ID,
	ACC_DTS_ID,
	ACC_MLP_ID,
	ACC_LPCM_ID, /* LPCM DVD Video/Audio */
	ACC_SDDS_ID,
	ACC_WMA9_ID,
	ACC_OGG_ID,
	ACC_MP4A_AAC_ID,
	ACC_REALAUDIO_ID,
	ACC_HDCD_ID,
	ACC_AMRWB_ID,
	ACC_WMAPROLSL_ID,
	ACC_DDPLUS_ID,
	ACC_GENERATOR_ID,
	ACC_SPDIFIN_ID,
	ACC_TRUEHD_ID,
	/* Do not edit beyond this comment */
	ACC_LAST_DECODER_ID
};

#define ACC_DEC_PCMPROCESS_ID ( ACC_DEC_POSTPROCESS_MT << 8 )

enum eAccDecPcmProcId
{
	ACC_CMC_ID = ACC_DEC_PCMPROCESS_ID,
	ACC_DeEMPH_ID,
	ACC_TSXT_VCR_ID,
	ACC_OMNI_VCR_ID,
	ACC_STWD_VCR_ID,
	ACC_CUSTOM_VCR_ID,
	ACC_AC3Ex_ID,
	ACC_CUSTOM_MAIN_ID,
	ACC_PLII_ID,
	ACC_CSII_ID,
	ACC_CHSYN_ID,
	ACC_NEO_ID,
	ACC_TSXT_ID,
	ACC_OMNI_ID,
	ACC_STWD_ID,
	ACC_CUSTOM_N22_ID,
	ACC_DB_HEADPHONE_ID,
	ACC_ST_HEADPHONE_ID,
	ACC_CUSTOM_HEADPHONE_ID,
	ACC_DMIX_ID,
	ACC_KOKPS_ID, /* Pitch Shift */
	ACC_KOKVC_ID, /* Voice Cancel */
	ACC_TEMPO_ID,
	ACC_CUSTOM_KOKMUSIC_ID,
	ACC_DECPCMPRO_BASSMGT_ID,
	ACC_DECPCMPRO_EQUALIZER_ID,
	ACC_DECPCMPRO_CUSTOM_ID,
	ACC_DECPCMPRO_DCREMOVE_ID,
	ACC_DECPCMPRO_DELAY_ID,
	ACC_PCMPRO_SFC_ID,
	ACC_PCMPRO_SIGANALYSIS_ID,
	ACC_ES2PES_ENCODER_ID,
	ACC_ENCODERPP_ID,
	ACC_SPDIFOUT_ID,
	ACC_DECPCMPRO_BTSC_ID,
	ACC_VIQ_ID,

	/* Do not edit beyond this comment */
	ACC_LAST_DECPCMPROCESS_ID
};

#include "PcmProcessing_DecoderTypes.h"

enum eAccPacketCode
{
	ACC_ES,
	ACC_PES_MP2,
	ACC_PES_MP1,
	ACC_PES_DVD_VIDEO,
	ACC_PES_DVD_AUDIO,
	ACC_IEC_61937,
	ACC_ASF,
	ACC_WAV,
	ACC_WMA_ST_FILE,
	ACC_UNKNOWN_PACKET
};

enum eStreamingBufferParams
{
	STREAMING_BUFFER_TYPE, //!< indicates that the given buffer is the last packet of the file
	STREAMING_SKIP_FRAMES, //!< indicates which frame in current Packet to skip
	STREAMING_PCM_DESCRIPTION, //!< describe PCM Buffers content (Sfreq, NbChannels, Interleaving, AudioMode)
	STREAMING_RESERVED_2,
	// do not edit below this line
	STREAMING_BUFFER_PARAMS_NB_ELEMENTS
};

enum eStreamingBufferType
{
	STREAMING_DEC_RUNNING,
	STREAMING_DEC_EOF
};

enum eStreamingFrameStatus
{
	STREAMING_STATUS_RUNNING,
	STREAMING_STATUS_EOF,
	STREAMING_STATUS_BAD_STREAM
};

/* Channel Map Description Structure :- */
typedef struct
{
	U32 LeftRightPair : 1; // L & R
	U32 Center : 1; // C
	U32 Lfe : 1; // Lfe
	U32 SurroundPair : 1; // Ls & Rs
	U32 CenterSurround : 1; // Cs
	U32 FrontHighPair : 1; // Lh & Rh / VHpr
	U32 RearSurroundPair : 1; // Lsr & Rsr /RSpr
	U32 HighCenter : 1; // Ch /Cvh
	U32 TopSurround : 1; // Oh/Ts
	U32 CenterLRPair : 1; // Lc & Rc /Cpr
	U32 FrontWidePair : 1; // Lw & Rw /Wpr
	U32 SideSurroundPair : 1; // Lss & Rss / SDpr
	U32 Lfe2 : 1; // Lfe2
	U32 HighSurroundPair : 1; // Lhs & Rhs
	U32 CenterHighRear : 1; // Chr
	U32 HighRearPair : 1; // Lhr & Rhr
	U32 CenterLowFront : 1; // Clf
	U32 FrontLowPair : 1; // Llf & Rlf
	U32 LtRtPair : 1; // Lt & Rt
	U32 SideDirectsPair : 1; // Lsd/Rsd
	U32 LsExRsExPair : 1; // Ls/Rs are encoded as Dolby Surround encoded channels
	U32 LbinRbinPair : 1; // L/R are encoded for binaural playback

	U32 Reserved : 10; // Reserved for future channels,Must Be 0
} MME_ChannelMapInfo_t;

typedef union
{
	MME_ChannelMapInfo_t bits;
	U32 u32;
} MME_ChannelMapInfo_u;

typedef struct
{
	enum eAccDecoderId DecoderId ; //!< ID of the deocder.
	U32 StructSize ; //!< Size of this structure
	MME_ChannelMapInfo_u ChannelMap ; //!< Description of the encoded input channels.
	U32 BitDepth : 3; //!< eAccBitdepth :Encoded bit resolution
	U32 EncSampleRate : 5; //!< Original Encoding Sample rate
	U32 Reserved : 24; //!< Codec specific ... check DecoderID.
} tADFrameStatus_t;

typedef struct
{
	enum eAccDecoderId DecoderId; // ID of the deocder.
	U32 StructSize; //!< Size of this structure
	MME_ChannelMapInfo_t ChannelMap; // Description of the encoded input channels.
	U32 Reserved : 32; // Reserved for Future, Must Be 0

} tTemplateFrameStatus_t;

//! Additional decoder capability structure
typedef struct
{
	U32 StructSize; //!< Size of this structure
	U32 DecoderCapabilityFlags; //!< 1 bit per decoder
	U32 StreamDecoderCapabilityFlags; //!< 1 bit per decoder
	U32 DecoderCapabilityExtFlags[2]; //!< 4 bit per decoder (for DecoderId < 0x10)
	U32 PcmProcessorCapabilityFlags[AUDIO_DECODER_NB_PCMCAPABILITY_FIELD]; //!< 1 bit per pcm processing
} MME_LxAudioDecoderInfo_t;

typedef struct
{
	U32 StructSize; //!< Size of this structure
	U32 DecoderCapabilityFlags; //!< 1 bit per decoder
	U32 StreamDecoderCapabilityFlags; //!< 1 bit per decoder
	U32 DecoderCapabilityExtFlags[4]; //!< 4 bit per decoder (for DecoderId < 0x10)
	U32 PcmProcessorCapabilityFlags[AUDIO_DECODER_NB_PCMCAPABILITY_FIELD]; //!< 1 bit per pcm processing
} MME_LxAudioDecoderHDInfo_t;

#define ADECCAP_SIZE (sizeof(MME_LxAudioDecoderInfo_t))
//#define MIN_ADECCAP_SIZE (ADECCAP_SIZE - 2 * sizeof(U32)) // In min_adeccap_size :: exclude the addition ExtFlags
#define MIN_ADECCAP_SIZE ADECCAP_SIZE

typedef struct
{
	enum eAccDecoderId DecoderId;
	U32 StructSize;

	U8 Config[(ACC_MAX_DEC_CONFIG_SIZE - 2) * sizeof(U32) ];

} MME_LxDecConfig_t;

typedef struct
{
	U32 StructSize; //!< Size of this structure
	U32 DecConfig[ACC_MAX_DEC_CONFIG_SIZE]; //!< Private Config of Audio Decoder
	MME_LxPcmProcessingGlobalParams_t PcmParams; //!< PcmPostProcessings Params

} MME_LxAudioDecoderGlobalParams_t;

typedef struct
{
	unsigned int BlockWise : 1;
	unsigned int TimerEnable : 1;
	unsigned int StreamBase : 1;
	unsigned int Watch : 1;
	unsigned int OutputWS : 4;
	unsigned int Align16 : 8;
	unsigned int RsplEnable : 1;
	unsigned int PacketType : 1;
	unsigned int Align32 : 14;
} tADFeatures;

typedef union
{
	unsigned int u32;
	tADFeatures Bits;
} uADFeatures;

// Macros to be used by host
#define AUDIODEC_SET_BLOCKWISE(x,bk) x->BlockWise.Bits.BlockWise = bk; // x->BlockWise = ((x->BlockWise & 0xFFFFFFFE) | (bk ))
#define AUDIODEC_SET_TIMER_ENABLE(x,tm) x->BlockWise.Bits.TimerEnable = tm; // x->BlockWise = ((x->BlockWise & 0xFFFFFFFD) | (tm << 1))
#define AUDIODEC_SET_STREAMBASE(x,str) x->BlockWise.Bits.StreamBase = str;// x->BlockWise = ((x->BlockWise & 0xFFFFFFFB) | (str << 2))
#define AUDIODEC_SET_WATCH(x,w) x->BlockWise.Bits.Watch = w; // x->BlockWise = ((x->BlockWise & 0xFFFFFFF7) | (w << 3))
#define AUDIODEC_SET_OUTPUT_WS(x,ws) x->BlockWise.Bits.OutputWS = ws; // x->BlockWise = ((x->BlockWise & 0xFFFFFF0F) | (ws << 4))
#define AUDIODEC_SET_RSPL_ENABLE(x,rsp) x->BlockWise.Bits.RsplEnable = rsp;// x->BlockWise = ((x->BlockWise & 0xFFFEFFFF) | (rsp << 16))
#define AUDIODEC_SET_PACKET_TYPE(x,pkt) x->BlockWise.Bits.PacketType = pkt;// x->BlockWise = ((x->BlockWise & 0xF0FFFFFF) | (pkt << 24))

// Macros to be used by companion
#define AUDIODEC_GET_BLOCKWISE(x) (x->BlockWise.Bits.BlockWise) // ((x->BlockWise ) & 0x1) // Extract BlockWise from InitParams
#define AUDIODEC_GET_TIMER_ENABLE(x) (x->BlockWise.Bits.TimerEnable) // ((x->BlockWise >> 1) & 0x1) // Extract TimerEnable from InitParams
#define AUDIODEC_GET_STREAMBASE(x) (x->BlockWise.Bits.StreamBase) // ((x->BlockWise >> 2) & 0x1) // Extract StreamBase Enable from InitParams
#define AUDIODEC_GET_WATCH(x) (x->BlockWise.Bits.Watch) // ((x->BlockWise >> 3) & 0x1) // Extract Watch Enable from InitParams
#define AUDIODEC_GET_OUTPUT_WS(x) (x->BlockWise.Bits.OutputWS) // ((x->BlockWise >> 4) & 0xF) // Extract outputWordSize from InitParams
#define AUDIODEC_GET_RSPL_ENABLE(x) (x->BlockWise.Bits.RsplEnable) // ((x->BlockWise >>16) & 0x1) // Extract ResamplingEnable from InitParams
#define AUDIODEC_GET_PACKET_TYPE(x) (x->BlockWise.Bits.PacketType) // ((x->BlockWise >>24) & 0xF) // Extract PacketType from InitParams

typedef struct
{
	U32 StructSize; //!< Size of this structure

	//! System Init
	enum eAccProcessApply CacheFlush; //!< If ACC_DISABLED then the cached data aren't sent back to DDR
	uADFeatures BlockWise; //!< use above macros to read this field
	enum eFsRange SfreqRange;

	U8 NChans[ACC_MIX_MAIN_AND_AUX];
	U8 ChanPos[ACC_MIX_MAIN_AND_AUX];

	//! Decoder specific parameters
	MME_LxAudioDecoderGlobalParams_t GlobalParams;

} MME_LxAudioDecoderInitParams_t;

typedef struct
{
	enum eAccBoolean Restart; //!< Restart decoder as if 1st frame
	U16 Cmd; //!< (enum eAccCmdCode) Play/Skip/Mute/Pause
	U16 PauseDuration; //!< Duration of Pause in sample
	U32 FrameParams[ACC_MAX_DEC_FRAME_PARAMS]; //!< Decoder Specific Parameters
	uMME_BufferFlags PtsFlags; //<! Flag[b0..1] = PTS - Validity / PTSflag[b16] = PTS[b32]
	U32 PTS; //<! PTS[b0..b31]
} MME_LxAudioDecoderFrameParams_t;

#ifdef _BEFORE_BL026_ // legacy driver support

typedef struct
{
	U32 Param0 : 16;
	U32 PTS_DTS_FLAG : 2; // macro was ( flags >> 16) & 0x3
	U32 Reserved0 : 6;
	U32 PTS_Bit32 : 1; // macro was ( flags >> 23) & 0x1
	U32 Reserved1 : 7;

} tMME_ADBufferFlags;

typedef struct
{
	tMME_ADBufferFlags Bits;
	U32 Base;
} uMME_ADBufferFlags; // use AudioDec specific flags definition (legacy drivers).

#else // Any baseline beyond BL026

#define uMME_ADBufferFlags uMME_BufferFlags // use generic Flags definition :: given in acc_mmedefines.h

#define ADEC_SET_BUFFER_TYPE(buf, type) do { \
		uBufferFlags ad_flags; \
		ad_flags.U32 = buf[WMAPROLSL_BUFFER_TYPE]; \
		ad_flags.Bits.FrameType = type; \
		buf[WMAPROLSL_BUFFER_TYPE] = ad_flags.U32; \
	} while(0)

#endif

typedef struct
{
	uMME_ADBufferFlags PTSflags;
	U32 PTS;
} MME_ADBufferParams_t;

/*typedef struct
{
 U32 StructSize;
 U32 BufferParams[ACC_MAX_BUFFER_PARAMS]; //!< Decoder Specific Parameters
} MME_LxAudioDecoderBufferParams_t;*/

typedef struct
{
	U32 StructSize;

	int DecStatus;
	int DecRemainingBlocks;
	enum eAccAcMode DecAudioMode;
	enum eAccFsCode DecSamplingFreq;

	int NbOutSamples; //<! Report the actual number of samples that have been output
	enum eAccAcMode AudioMode; //<! Channel Output configuration
	enum eAccFsCode SamplingFreq; //<! Sampling Frequency of Output PcmBuffer
	enum eAccBoolean Emphasis; //<! Buffer has emphasis
	U32 ElapsedTime; //<! elapsed time to do the transform in microseconds
	uMME_BufferFlags PTSflag; //<! PTSflag[b0..1] = PTS - Validity / PTSflag[b16] = PTS[b32]
	U32 PTS; //<! PTS[b0..b31]

	U16 FrameStatus[ACC_MAX_DEC_FRAME_STATUS]; /* Decoder Specific Parameters */
} MME_LxAudioDecoderFrameStatus_t;

typedef struct
{
	MME_LxAudioDecoderFrameStatus_t DecStatus;
	MME_PcmProcessingFrameExtStatus_t PcmStatus;
} MME_LxAudioDecoderFrameExtendedStatus_t;

typedef struct
{
	MME_LxAudioDecoderFrameStatus_t DecStatus;
	MME_WMFrameStatus_t PcmStatus;

} MME_LxAudioDecoderFrameWMStatus_t;

//PrimaryAudioGain of MME_LxAudioDecoderMixingMetadata_t exchanged with SecondaryAudioPanCoeff of MME_MixingOutputConfiguration_t by AG
#define ADEC_SET_BUFFER_TYPE(buf, type) do { \
		uBufferFlags ad_flags; \
		ad_flags.U32 = buf[WMAPROLSL_BUFFER_TYPE]; \
		ad_flags.Bits.FrameType = type; \
		buf[WMAPROLSL_BUFFER_TYPE] = ad_flags.U32; \
	} while(0)

// The following structures shall be used if the decoded audio stream contains some mixing metadata (Blu Ray case)
typedef struct
{
	enum eAccAcMode AudioMode; //! Primary Channel Output configuration
	U16 PrimaryAudioGain[ACC_MAIN_CSURR + 1]; //! unsigned Q3.13 gain to be applied to each channel of primary stream
	U16 SecondaryAudioPanCoeff[ACC_MAIN_CSURR + 1]; //! unsigned Q3.13 panning coefficients to be applied to secondary mono stream
} MME_MixingOutputConfiguration_t;

typedef struct
{
	U32 Id; //! Id of this processing structure: ACC_MIX_METADATA_ID
	U32 StructSize; //! Size of this structure
	U16 PostMixGain; //! unsigned Q3.13 gain to be applied to output of mixed primary and secondary
	U16 NbOutMixConfig; //! Number of mix output configurations
} MME_LxAudioDecoderMinimalMixingMetadata_t;

typedef struct
{
	MME_LxAudioDecoderMinimalMixingMetadata_t MinStruct; //! Minimal mandatory structure
	MME_MixingOutputConfiguration_t MixOutConfig[ACC_MAX_MIXING_OUPUT_CONF]; //! This array is extensible according to NbOutMixConfig
} MME_LxAudioDecoderMixingMetadata_t;

typedef struct
{
	U32 BytesUsed; // Amount of this structure already filled
	MME_LxAudioDecoderMixingMetadata_t PcmStatus;
} MME_MixMetadataFrameStatus_t;

typedef struct
{
	MME_LxAudioDecoderFrameStatus_t DecStatus;
	MME_MixMetadataFrameStatus_t PcmStatus;

} MME_LxAudioDecoderFrameMixMetadataStatus_t;

// registering function to plug-in new AudioDecoder
extern MME_ERROR AudioDecoder_registerDLL(void *f_params); // tAudioDLL * f_params

#endif /* _AUDIO_DECODER_DECODERTYPES_H_ */

