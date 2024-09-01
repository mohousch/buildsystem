/// $Id: Spdifin_DecoderTypes.h,v 1.5 2003/11/26 17:03:08 lassureg Exp $
/// @file : ACC_Multicom/ACC_Transformers/Spdifin_DecoderTypes.h
///
/// @brief : Spdifin Audio Decoder specific types for Multicom
///
/// @par OWNER: Gael Lassure
///
/// @author : Gael Lassure
///
/// @par SCOPE:
///
/// @date : 2007-07-04
///
/// &copy; 2007 ST Microelectronics. All Rights Reserved.
///

#ifndef _SPDIFIN_DECODERTYPES_H_
#define _SPDIFIN_DECODERTYPES_H_

#define SPDIFIN_API_VERSION 0x100415

#include "Audio_DecoderTypes.h"
#include "PcmProcessing_DecoderTypes.h"
#include "DolbyDigital_DecoderTypes.h"
#include "acc_mmedefines.h"

enum eSpdifinCapabilityFlags
{
	ACC_SPDIFIN_DD,
	ACC_SPDIFIN_DTS,
	ACC_SPDIFIN_MPG,
	ACC_SPDIFIN_WMA,
};

enum eIecConfigIdx
{
	IEC_SFREQ, /// Input Sampling Frequency
	IEC_NBSAMPLES, /// Number of Samples per transform
	IEC_DEEMPH, /// Describe quite few Flags : should be cast in tSpdifinFlags....
	IEC_LOOKAHEAD, /// NbSamples to LookAhead
	IEC_NB_CONFIG_ELEMENTS
};

#define IEC_FLAGS IEC_DEEMPH

enum eHDMILayout
{
	HDMI_HBRA = 1, // High BitRate Audio : DTSHD MA, TrueHD (single core)
	HDMI_LAYOUT0 = 2, // IEC60958, IEC61937, DTSHD HR , DD+
	HDMI_HBRA2 = 5, // High BitRate Audio : DTSHD MA, TrueHD (dual core :: enabling coprocessor for the PcmProcessings)
	HDMI_LAYOUT1 = 8, // PCM 8ch, Audio Mode provided through frame buffer
};

typedef struct
{
	unsigned int Emphasis : 1;
	unsigned int EnableAAC : 1;
	unsigned int ForceLookAhead : 1;
	unsigned int UserDecCfg : 1; // Set to apply the provided DEC CONFIG
	unsigned int Layout : 4; // enum eHDMILayout
	unsigned int BlockSizeInms : 8;
	unsigned int HDMIAudioMode : 8; // used if Layout == HDMI_PCM8CH
	unsigned int ReportCodecStatus : 1; // Set to enable the reporting of codec specific status.
	unsigned int ExtendedStatus : 1; // Set to enable the reporting of extended FrameStatus definition
	unsigned int Reserved : 4;
	unsigned int DisableDetection : 1; // Set to bypass compressed data as PCM.
	unsigned int DisableFade : 1; // Set to disable the fade in/out operation.
} MME_SpdifinFlags_t;

typedef union
{
	MME_SpdifinFlags_t Bits;
	unsigned int U32;
} MME_SpdifInFlags_u;

typedef struct
{
	enum eAccDecoderId DecoderId;
	U32 StructSize;

	U32 Config[IEC_NB_CONFIG_ELEMENTS];
	U8 DecConfig[DD_NB_CONFIG_ELEMENTS];

} MME_SpdifinConfig_t;

#define SPDIFIN_NBMAX_PAGES 32

/*typedef struct
{
 U32 StructSize; //!< Size of the BufferParams array.
 U32 BufferParams[STREAMING_BUFFER_PARAMS_NB_ELEMENTS]; //!< Decoder Specific Parameters
} MME_SpdifinBufferParams_t;*/

enum eMulticomSpdifinStatus
{
	SPDIFIN_DECODER_RUNNING,
	SPDIFIN_STATUS_EOF,
};

enum eMulticomSpdifinPC
{
	SPDIFIN_NULL_DATA_BURST,
	SPDIFIN_AC3,
	SPDIFIN_SMPTE_338M,
	SPDIFIN_PAUSE_BURST,
	SPDIFIN_MP1L1,
	SPDIFIN_MP1L2L3,
	SPDIFIN_MP2MC,
	SPDIFIN_MP2AAC,
	SPDIFIN_MP2L1LSF,
	SPDIFIN_MP2L2LSF,
	SPDIFIN_MP2L3LSF,
	SPDIFIN_DTS1,
	SPDIFIN_DTS2,
	SPDIFIN_DTS3,
	SPDIFIN_ATRAC,
	SPDIFIN_ATRAC2_3,
	SPDIFIN_DTS4 = 17,
	SPDIFIN_DDPLUS = 21,
	SPDIFIN_DTRUEHD = 22,
	SPDIFIN_IEC60958 = 32,
	SPDIFIN_IEC60958_DTS14 = 33, // 4096 sample frames
	SPDIFIN_IEC60958_DTS16 = 34,
	SPDIFIN_IEC60958_DTS1_14 = 35, // 512 sample frames
	SPDIFIN_IEC60958_DTS1_16 = 36,
	SPDIFIN_IEC60958_DTS2_14 = 37, // 1024 sample frames
	SPDIFIN_IEC60958_DTS2_16 = 38,
	SPDIFIN_IEC60958_DTS3_14 = 39, // 2048 sample frames
	SPDIFIN_IEC60958_DTS3_16 = 40,
	SPDIFIN_DTSES_61_MATRIX = 41,
	SPDIFIN_DTSES_61_DISCRETE = 42,
	SPDIFIN_DTSES_71_DISCRETE = 43,
	SPDIFIN_DTS_96k_24b = 44,
	SPDIFIN_DTSHD_HIGH_RES = 45,
	SPDIFIN_DTSHD_MASTERAUDIO = 46,
	SPDIFIN_RESERVED

};

enum eMulticomSpdifinState
{
	SPDIFIN_STATE_RESET,
	SPDIFIN_STATE_PCM_BYPASS,
	SPDIFIN_STATE_COMPRESSED_BYPASS,
	SPDIFIN_STATE_UNDERFLOW,
	SPDIFIN_STATE_COMPRESSED_MUTE,
	SPDIFIN_STATE_SWITCH_TO_PCMBYPASS,

	// do not add entries below this line.
	SPDIFIN_STATE_INVALID
};

enum eSpdifinFrameStatus
{
	SPDIFIN_STATUS,
	SPDIFIN_CURRENT_STATE,
	SPDIFIN_CURRENT_PC,
	SPDIFIN_NBSAMPLES_LEFT
};

typedef struct
{
// First U16 extended definition
	U16 Status : 1; // eMulticomSpdifinStatus
	U16 IsLossless : 1;
	U16 Silence : 1; // report Silence (VuM < SilenceThresh) for more than requested period.
	U16 LFE : 1;
	U16 SamplingFreq : 4;
	U16 NbFront : 4;
	U16 NbRear : 4;

// Second U16 extended definition
	U16 CurrentState : 4; // eMulticomSpdifinState
	U16 Display : 4; // maps to DTSHDSubStream->Field.Display / DTSHDSubStream->Bits.{1:isDTSHD,2:isDTSES}
	U16 Reserved2 : 8;

// Third U16 extended definition
	U16 PC; // eMulticomSpdifinPC

// Fourth U16 extended definition
	U16 NbSamplesLeft; // Number of samples already decoded but not yet output.
} tMMESpdifinStatus;

#endif /* _SPDIFIN_DECODERTYPES_H_ */
