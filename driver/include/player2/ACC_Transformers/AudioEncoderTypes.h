//////////////////////////////////////////////////////////////////////
//File : ACC_Multicom/ACC_Transformers/AudioEncoderTypes.h
//
//Author : Sebastien Clapie (sebastien.clapie@st.com)
//
//creation date : 2005/05/20
//
//comments : C file (derived from MME + version 1.0)
//
//version : 1.0
//////////////////////////////////////////////////////////////////////

#ifndef AUDIOENCODERTYPES_H
#define AUDIOENCODERTYPES_H

#include "mme.h"

//Param stucture for initialising the audio encoder transformer
typedef struct _MMEAudioEncodeInitParams
{
	//MME_UINT id_enc;
	MME_UINT bytesToSkipBeginScatterPage; //skip this number of bytes at the beginning of a new scatterpage (for PES header)
	MME_UINT channels; //number of audio channel of input data
	MME_UINT bitsPerSample; //16 24 or 32 bits per sample
	MME_UINT outputBitrate; //number of bits per second created by encoder
	MME_UINT maxNumberSamplesPerChannel; //maximum number of samples per channel to be send with 1 transform call

} MMEAudioEncodeInitParams;

//audio transform structure
typedef struct _MMEAudioTransformParams
{
	//Until here compatible with MME_BufferPassingConvention_t
	MME_UINT numberOutputSamples; //desired number of output samples (SRC!!)
} MMEAudioTransformParams;

//Return Parameters

typedef struct _MMEAudioReturnParams
{
	MME_UINT numberInputBytesLeft; //number of input sample bytes that could not be encoded
	MME_UINT numberOutputBytes; //number of bytes written from encoder to scatter pages
	MME_UINT byteStartAddress; //byte address of first byte written to the buffers
	MME_UINT ElapsedTime; //<! elapsed time to do the transform in microseconds

} MMEAudioReturnParams;

#endif //AUDIOENCODERTYPES_H

