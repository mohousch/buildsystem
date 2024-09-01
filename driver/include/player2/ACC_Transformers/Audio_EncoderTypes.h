#ifndef AUDIO_ENCODERTYPES_H
#define AUDIO_ENCODERTYPES_H

#define DRV_MULTICOM_AUDIO_ENCODER_VERSION 0x060203

#include "mme.h"
#include "PcmProcessing_DecoderTypes.h"
#include "Pcm_PostProcessingTypes.h"
#include "acc_mmedefines.h"

#define AUDIO_ENCODER_NUM_BUFFERS_PER_SENDBUFFER AUDIO_STREAMING_NUM_BUFFERS_PER_SENDBUFFER
#define ACC_ENCODER_ID_BITPOS 4

#define ACC_GENERATE_ENCODERID(x) (ACC_ENCODER_OUTPUT_CONFIG_ID + (x<<ACC_ENCODER_ID_BITPOS))
#define ACC_EXTRACT_ENCODER_ID(x) (x >> ACC_ENCODER_ID_BITPOS)
#define ACC_EXTRACT_ENCODER_CONFIG_ID(x) (x & ((1<<ACC_ENCODER_ID_BITPOS) -1))

// Max among all encoder for number of bytes required for specific config
#define NB_ENCODER_CONFIG_ELEMENT 48

// Max among all encoder for number of bytes required for specific return params
#define MAX_NB_ENCODER_TRANSFORM_RETURN_ELEMENT 200

enum eAccEncStatusError
{
	ACC_ENCODER_NO_ERROR,
	ACC_ENCODER_SFC_WRONG_NB_INPUT_SPL,
	ACC_ENCODER_SFC_WRONG_NB_INPUT_SPL_LBUFFER,
	ACC_ENCODER_SFC_INTERNAL_ERROR,
	ACC_ENCODER_CODEC_INTERNAL_ERROR,
	ACC_ENCODER_COPY_DATA_BUFFER_ERROR,
	ACC_ENCODER_WRONG_NB_BUFFERS,
	ACC_ENCODER_UNABLE_TO_ADD_BUFFER,

	// do no edit below this line
	ACC_ENCODER_LAST_STATUS_ERROR,
};

typedef struct
{
	U32 PtsFlags;
	U32 PTS;
} MME_AudioEncoderPts_t;

enum eAccEncGlobalParamsId
{
	ACC_ENCODER_PCMPREPROCESSOR_ID,
	ACC_ENCODER_OUTPUT_CONFIG_ID,
	ACC_ENCODER_INPUT_CONFIG_ID,

	// do no edit below this line
	ACC_ENCODER_LAST_ID,
};

enum eAccEncoderId
{
	ACC_DDCE_ID,
	ACC_MP2E_ID,
	ACC_MP3E_ID,
	ACC_AACE_ID,
	ACC_WMAE_ID,
	ACC_DTSE_ID,
	ACC_DDCE51_ID,
	ACC_MP3ELC_ID,

	// Do not edit below this line.
	ACC_LAST_AUDIOENCODER_ID
};

enum eMmeAudioEncodeSfcMode
{
	AENC_SFC_TICK_MODE,
	AENC_SFC_AUTO_NOUT_MODE,
	AENC_SFC_RESERVED_MODE
};

enum eMmeAudioEncoderTimerId
{
	AENC_TIMERID_PREPRO,
	AENC_TIMERID_ENCODE,
	AENC_TIMERID_TRANSF,
	AENC_TIMERID_RDBUFF,
	AENC_TIMERID_WRBUFF,
	AENC_TIMERID_WTBUFF,

	AENC_NB_TIMERID
};

enum eSfcConfigIdx
{
	SFC_MODE, // Selection of the SFC mode see previous definition
	SFC_FILTER_SELECTION, // Choose among compiled filters
	SFC_FILT_32BIT_MODE, // Higher Precision, more expensive
	SFC_REFRESH_RATE, // Ratio reevaluation rate, def. 1 (used in Tick Mode only)
	SFC_CONST_RAT_OUT,
	/* Do not edit beyond this comment */
	SFC_NB_CONFIG_ELEMENTS
};

enum eAccEncoderPcmId
{
	ACC_ENCODER_SFC_ID,
	ACC_ENCODER_DCREMOVE_ID,
	ACC_ENCODER_RESAMPLEX2_ID,
	// do not edit beyond this line
	ACC_LAST_ENCODER_PCMPROCESS_ID
};

typedef struct
{
	enum eAccEncoderPcmId Id; //!< ID of this processing
	U32 StructSize; //!< Size of this structure
	enum eAccProcessApply Apply;
	U8 Config[SFC_NB_CONFIG_ELEMENTS];
} MME_SfcGlobalParams_t;

typedef struct
{
	enum eAccEncGlobalParamsId Id; //!< ID of this processing
	U32 StructSize; //!< Size of this structure
	U32 ChannelConfig; //!< number of audio channel at input + channel routing info
	enum eAccWordSizeCode WordSize; //!< 16/32 bit buffers...
	enum eAccFsCode SamplingFreq; //!< Sampling Frequency
} MME_AudEncInConfig_t;

typedef struct
{
	U32 Id; //!< ==> use ACC_GENERATE_ENCODERID(*eAccEncoderId*) macro
	U32 StructSize; //!< Size of this structure
	U16 outputBitrate; //!< number of bits per second created by encoder
	enum eAccFsCode SamplingFreq; //!< Sampling frequency of the audio to encode (after SRC)
	U8 Config[NB_ENCODER_CONFIG_ELEMENT]; //Specific encoder configuration
} MME_AudEncOutConfig_t;

typedef struct
{
	enum eAccEncGlobalParamsId Id; //!< Id of the PostProcessing structure.
	U32 StructSize; //!< Size of this structure

	MME_SfcGlobalParams_t Sfc;
	MME_Resamplex2GlobalParams_t Resamplex2;
} MME_AudEncPreProcessingConfig_t; //!< PcmPostProcessings Params

/*param stucture for initialising the audio encoder transformer*/
typedef struct
{
	U32 StructSize; //!< Size of this structure
	MME_AudEncInConfig_t InConfig; //!< Specific Configuration for Mixer
	MME_AudEncPreProcessingConfig_t PcmParams; //!< PrePostProcessings Params
	MME_AudEncOutConfig_t OutConfig; //!< Output specific configuration information
} MME_AudioEncoderGlobalParams_t;

typedef struct
{
	U32 StructSize; //!< Size of this structure
	U32 EncoderCapabilityFlags;
	U32 EncoderCapabilityExtFlags;
	U32 DebugAdress;
} MME_AudioEncoderInfo_t;

//Define bit positions for option flags
//for MME_AudioEncoderInitParams_t.OptionFlags1
enum AudioEncoderInitParamsOptionFlags
{
	AEIPOF1_SEND_BACK_BUFFERS_IMMEDIATELY,
	AEIPOF1_START_UNUSED_BITS
};

typedef struct
{
	U32 StructSize;
	U32 BytesToSkipBeginScatterPage;
	U32 maxNumberSamplesPerChannel;
	MME_AudioEncoderGlobalParams_t GlobalParams;
	U32 OptionFlags1;
} MME_AudioEncoderInitParams_t;

/*audio transform structure*/

typedef struct
{
	//U32 numberInputBuffers; //!< number inputBuffers = 1
	//U32 numberOutputBuffers; //!< number outputBuffers = 0
	//MME_DataBuffer_t dataBuffer;

	//!<Until here compatible with MME_BufferPassingConvention_t

	U32 numberOutputSamples; //!< desired number of output samples (SRC!!)
	U32 EndOfFile;
	MME_AudioEncoderPts_t PtsInfo;
} MME_AudioEncoderTransformParams_t;

typedef struct
{
	U32 numberInputBuffers; //!< number inputBuffers = 1
	U32 numberOutputBuffers; //!< number outputBuffers = 0
	MME_DataBuffer_t dataBuffer[AUDIO_ENCODER_NUM_BUFFERS_PER_SENDBUFFER];

	//!<Until here compatible with MME_BufferPassingConvention_t
} MME_AudioEncoderBufferParams_t;

/*Return Parameters*/

typedef struct
{
	U32 StructSize;

	U32 numberInputBytesLeft; //!< number of input sample bytes that could not be encoded
	U32 numberOutputBytes; //!< number of bytes written from encoder to scatter pages
	U32 byteStartAddress; //!< address of the first access unit.
	enum eAccEncStatusError EncoderStatus; //!< MT_Encoder Status
	U32 BitRate; //Returns actual bitrate chosen by packet
	U32 PreProElapsedTime; //Elapsed time (us) in SFC
	U32 EncodeElapsedTime; //Elapsed time (us) in codec
	U32 TransfElapsedTime; //Elapsed time (us) in Transform
	U32 RdBuffElapsedTime; //Elapsed time (us) reading data
	U32 WrBuffElapsedTime; //Elapsed time (us) writing data
	U32 WtBuffElapsedTime; //Elapsed time (us) waiting for available buffer
	U32 ElapsedTime[AENC_NB_TIMERID];

	U32 SpecificEncoderStatusBArraySize; //!< Number of bytes allocated for the encoder status (must be set by host)
	U8 SpecificEncoderStatusBArray[MAX_NB_ENCODER_TRANSFORM_RETURN_ELEMENT]; //!< Array of bytes for the encoder to report status
} MME_AudioEncoderStatusParams_t;

typedef struct
{
	U32 StructSize;
	enum eAccEncStatusError EncoderStatus; //Encoder Status (see eAccEncStatusError)
	enum eAccBoolean LastBuffer; //Will be set to one if this buffer corresponds to EOF
	MME_AudioEncoderPts_t PtsInfo; //Pts for the the start byte of a starting frame
} MME_AudioEncoderStatusBuffer_t;

#endif /*AUDIOENCODERTYPES_H*/
