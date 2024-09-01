#ifndef _DEBUG_TYPES_H_
#define _DEBUG_TYPES_H_

#include "mme.h"
#include "acc_mmedefines.h"
#include "Audio_DecoderTypes.h"
#include "PcmProcessing_DecoderTypes.h"
#include "Audio_EncoderTypes.h"
#include "Audio_ParserTypes.h"
#include "AudioMixer_ProcessorTypes.h"
#include "Audio_GeneratorTypes.h"
#include "Mixer_ProcessorTypes.h"

#include "Pcm_PostProcessingTypes.h"
#include "Spdifin_DecoderTypes.h"
typedef union
{
	MME_LxAudioDecoderInitParams_t *Dec;
	MME_LxPcmProcessingInitParams_t *PP;
	MME_AudioGeneratorInitParams_t *Gen;
	MME_LxMixerTransformerInitParams_t *Mixer;
	MME_LxMixerTransformerInitBDParams_t *BDMixer;
	MME_AudioParserInitParams_t *Parser;
	MME_AudioEncoderInitParams_t *Enc;
	MME_GenericParams_t *Generic;
	char *Char;
	unsigned char *UChar;
} uInitParams;

typedef union
{
	MME_LxAudioDecoderGlobalParams_t *DecGP;
	MME_LxAudioDecoderFrameParams_t *DecFP;
	//MME_LxAudioDecoderBufferParams_t * DecBP;
	MME_StreamingBufferParams_t *DecBP;
	MME_PcmProcessingFrameParams_t *PcmFP;
	MME_LxMixerTransformerFrameDecodeParams_t *MixFP;
	MME_LxMixerBDTransformerGlobalParams_t *MixGP;

	MME_GenericParams_t *Generic;
	char *Char;
	unsigned char *UChar;
	unsigned int UINT; // to ensure no warning cast.
} uCmdParams;

typedef struct
{
	char Text[64];
	int Value;
} tSizeOf;

#define E(x) {#x, sizeof(x)}
static tSizeOf uc_SizeOf[] =
{
	E(MME_Command_t),
	E(MME_DataBuffer_t),
	E(MME_CommandStatus_t),
	E(MME_ScatterPage_t),

	E(MME_LxAudioDecoderInitParams_t),
	E(MME_LxPcmProcessingInitParams_t),
	E(MME_AudioGeneratorInitParams_t),
	E(MME_LxMixerTransformerInitParams_t),
	E(MME_AudioParserInitParams_t),
	E(MME_AudioEncoderInitParams_t),
	E(MME_LxDecConfig_t),
	E(MME_DMixGlobalParams_t),
	E(MME_SpdifOutGlobalParams_t),
	E(MME_FatpipeGlobalParams_t),
	E(MME_LimiterGlobalParams_t),
	E(MME_BTSCGlobalParams_t),
	E(MME_VIQGlobalParams_t),
	E(MME_SpdifinConfig_t),
	// Do not edit below this line
	{"", 0}
};

#endif // _DEBUG_TYPES_H_
