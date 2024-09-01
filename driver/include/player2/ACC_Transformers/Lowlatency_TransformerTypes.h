/*
 $Id: Exp $
 @file : ACC_Multicom/ACC_Transformers/Lowlatency_TransformerTypes.h

 @brief : Lowlatency Multicom APIs

 @par OWNER: Gael Lassure

 @par SCOPE:

 @date : 2008/09/10

 &copy; 2008 ST Microelectronics. All Rights Reserved.
*/

#ifndef _LOWLATENCY_TRANSFORMERTYPES_H_
#define _LOWLATENCY_TRANSFORMERTYPES_H_

/* A bug in older versions of this file made any code that expanded
 * LOWLATENCY_API_VERSION fail to compile. We therefore introduce the
 * first LOWLATENCY_API_EPOCH in order to indicate that it is safe to
 * expand LOWLATENCY_API_VERSION.
 */
#define LOWLATENCY_API_EPOCH 1
#define LOWLATENCY_API_VERSION 0x100430

#include "Audio_DecoderTypes.h"
#include "PcmProcessing_DecoderTypes.h"
#include "Spdifin_DecoderTypes.h"

#define LL_NB_QUEUED_CMD 4 //!<
#define LL_STACK_SIZE 32768

#define LL_NB_INPUT 2
#define LL_NB_OUTPUT 4

#define LL_MT_NAME "LOWLATENCY_MT"

//!< Low Latency transformer capability structure :: inherit from AudioDecoder / SPDIFin
typedef MME_LxAudioDecoderHDInfo_t MME_LowlatencyCapabilityInfo_t;

enum eIOState
{
	IO_DISCONNECT = 0,
	IO_CONNECT = 1,
};

#define LL_FILE_DEVICE 0xF
#define LL_ALSAINPUT_FLAG 0xA15A1
#define LL_ALSAOUTPUT_FLAG 0xA15A0

typedef struct
{
MME_UINT DevId :
	4; //!< hw device id / set to 0xF if File IO.
MME_UINT CardId :
	4; //!< sound card id / set to 0 if file IO.
MME_UINT MTId :
	8; //!< set to LL_MT
MME_UINT ExtId :
	16; //!< set to 0 [reserved for future use]
}
tLLId;

enum eLLDeviceType
{
	PLAYBACK_DEVICE = 0,
	CAPTURE_DEVICE = 1,
	REPLAY_DEVICE = 2,
	// do not edit beyond this definition
	LL_NB_DEVICE
};

typedef struct
{
	MME_UINT Connect : 1; //!< Disconnected / Connected
	MME_UINT Source : 1; //!< source 0 / Source 1
	MME_UINT Sink : 2; //!< Output chain 0 to 3
	MME_UINT TimeOut : 8; //!< TimeOut waiting for next block in ms

	MME_UINT DeviceType : 2; //!< use enum eCapture
	//!< if the driver sends the input as file inplace of ALSA actual capture (REPLAY_DEVICE)
	//!< then ALSA physical IO will not be open , a queue of SB is expected instead.
	MME_UINT SBOutEnable : 1; //!< To Set if the driver sends output buffer, ALSA I/O is returned to host through SB out.
	MME_UINT Reserved : 13; //!< TBD
	MME_UINT Layout : 4; //!< set and used by the FW internally (cascaded from SpdifinConfig).
}
tLLIOConnect;

typedef struct
{
	MME_UINT NbChannels : 8; //!< Interleaving of the input pcm buffers
	MME_UINT WordSize : 8; //!< Input WordSize : ACC_WS32
	MME_UINT AudioMode : 8; //!< Channel Configuration : ACC_MODE_ID
	MME_UINT SamplingFreq : 8; //!< Sampling Frequency : ACC_FS_Reserved (same as source).
}
tLLIOParams;

typedef struct
{
	MME_UINT Latency : 8; //!< Target Latency in ms { 20 ms for PCM bypass, 60ms for game mode , 100 ms otherwise }
	int SilenceThresh: 8; //!< Threshold for Silence detection in dBFS; if 0 , silence is not detected.
	MME_UINT Reserved : 16;

	MME_UINT SilencePeriod; //!< Period in ms to report signal below SilenceThreshold ; if 0, silence is not detected.
}
tLLInitFeatures;

typedef struct
{
	tLLId ID; //!< IO Id.
	MME_UINT StructSize; //!< sizeof(this)

	tLLIOParams Params;
	tLLIOConnect Connect; //!< Connection Params;

	U32 Config; //!< TBD
}
MME_LowLatencyIO_t;

#define ACC_LOWLATENCY_PP_ID (ACC_PCMPROCESS_MAIN_MT << 8)

typedef struct
{
	MME_UINT ID; //!< ACC_LOWLATENCY_PP_ID
	MME_LxPcmProcessingGlobalParams_t PP;
}
MME_LowLatencyPP_t;
//!< Transformer Parameters transmitted once in a while

typedef union
{
	MME_SpdifinConfig_t *Dec;
	MME_LowLatencyPP_t *PP;
	MME_LowLatencyIO_t *IO;
	tLLIOParams *IOParams;
	tLLIOConnect *IOConnect;
	tLLId *pID;
	unsigned long *p32;
	unsigned short *p16;
	unsigned char *p8;
	void *v;

	// extension of conversion to data
	unsigned int u32;
	tLLId uID;
} uLLtype;

typedef struct
{
	MME_UINT StructSize;
	MME_SpdifinConfig_t SPDIFin; //!< Optional : SPDIFin configuration
	MME_LowLatencyIO_t IOCfg[LL_NB_INPUT + LL_NB_OUTPUT]; //!< Optional : IO connections
	MME_LowLatencyPP_t PcmParams; //!< PcmPostProcessings Params
}
MME_LowlatencyGlobalParams_t;

//!< Transformer Parameters provided at InitTransformer
typedef struct
{
	MME_UINT TimeOut; //!< TimeOut for the local execution thread in ms
	MME_UINT BlockSize; //!< Granularity of the I/O DMA transfers in ms
	tLLInitFeatures Features; //!< BitFields :: TBD
	MME_UINT Reserved[3]; //!< Reserved for future use.
}
tLowlatencyInitP;
typedef struct
{
	MME_UINT StructSize; //!< Sizeof this struct
	tLowlatencyInitP InitParams;
	MME_LowlatencyGlobalParams_t GlobalParams; //!< Global Processing Params
}
MME_LowlatencyInitParams_t;

//!< Transformer Parameters transmitted within every transform command
typedef struct
{
	unsigned int Restart : 1; //!< Restart decoder as if 1st frame
	unsigned int RestartPP : 1; //!< Restart PcmProcessings
	unsigned int RestartReserved: 30;
	U16 Cmd; //!< (enum eAccCmdCode) Play/Skip/Mute/Pause
	U16 Reserved; //!< For future use.
}
MME_LowlatencyTransformParams_t;

//!< Transformer Status returned by every transform command
typedef MME_LxAudioDecoderFrameExtendedStatus_t MME_LowlatencyTransformStatus_t;

//!< Additional definition : PES definition for ALSA LogPackets
//!< Warning :: this stream description may reveal endianness issues....
typedef struct
{
	unsigned int ID : 32; //!< 0x000001AA

	unsigned int Length : 16; //!<
	unsigned int Flags : 8; //!< 0x80 --> default value....
	unsigned int PTS_ExtFlags : 8; //!< 0x81 --> PTS(bit 7) Present / Pes Ext Flag(bit 0) = 1

	unsigned int HeaderLength : 8; //!< 0x17 --> Packet Header Length (comprising PTS and PES Extension)
	unsigned int PTS_MSB : 8;
	unsigned int PTS_TopHalf : 16;

	unsigned int PTS_BottomHalf: 16;
	unsigned int PES_ExtFlags : 8; //!< 0x80 --> PrivateDataFlag(bit 7) = 1 / ExtFlag2(bit 0) = 0
	// From here starts the 128 bits of private data
	unsigned int PData_NbChans : 4; //!< NbChannels...
	unsigned int PData_Sfreq : 4; //!< Sfreq Code of the audio

	// From here Private Data are aligned on 32 bits word
	unsigned int PData_NbFrames : 20; //!< NbFrames reported by snd_pcm_available
	unsigned int PData_ReturnOnWait: 1; // return on snd_pcm_wait()
	unsigned int PData_ReturnOnHTS : 1; // Return On snd_pcm_htimestamp()
	unsigned int PData_Restart : 1; // Record the restart of the pcm-reader (LL_ANALYSE_STATE)
	unsigned int PData_Drop : 1; // Record the call to snd_pcm_drop on pcm-reader
	unsigned int PData_Layout : 4; // Input player Layout (HDMI_LAYOUT0/1/HBRA)
	unsigned int PData_Reserved0 : 4;
	unsigned int PData_NbNew : 20; //!< Nb New Frames logged in this packet (others where logged in previous packet)
	unsigned int PData_Committed : 12; //!< Nb Frames commited at previous lap
	unsigned int PData_Timing : 32; //!< Timing of this snd_pcm_available...

	unsigned int PData_Mapped : 24; //!<
	unsigned int StuffingByte : 8; //!< align PES Header to 32bits.

	// Total Packet Header is 32 bytes long....

	//!< symbolic payload indication... definitely variable size.. could be 0 as well
	unsigned int Payload[1];
} MME_AlsaPacket_t;

#define SIZEOF_PKTHDR (sizeof(MME_AlsaPacket_t) - sizeof(unsigned int) /* sizeof(Payload[1]) */)

#endif // _LOWLATENCY_TRANSFORMERTYPES_H_
