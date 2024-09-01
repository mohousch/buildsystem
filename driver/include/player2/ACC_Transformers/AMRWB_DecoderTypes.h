#ifndef _AMRWB_DECODERTYPES_H_
#define _AMRWB_DECODERTYPES_H_

#include "PcmProcessing_DecoderTypes.h"
#include "acc_mmedefines.h"
#include "Audio_DecoderTypes.h"
enum eAMRWBConfigidx
{
	AMRWB_Mode,
	AMRWB_Extension,
	AMRWB_MonoDecStereo,
	AMRWB_LimiterOn,
	AMRWB_NB_CONFIG_ELEMENTS
};
typedef struct
{
	enum eAccDecoderId DecoderId;
	U32 StructSize;

	U8 Config[AMRWB_NB_CONFIG_ELEMENTS];
	//config[0]: Word16 mode; /* AMR_WB core mode: 0..8 */
	//config[1]: Word16 extension; /* 0=AMRWB, 1=mono, 2=stereo20%, 3=stereo25% */
	//config[2]: enum eBoolean mono_dec_stereo;
	//config[3]: enum eBoolean limiter_on;

	U16 st_mode; /* stereo mode 0..13 (not used, for ericsson cmd line?) */
	U16 fscale; /* Frequency scaling */
	U16 bfi[4]; /* Bad frame indicator */
	U32 fs;
} MME_LxAmrwbConfig_t;

#endif //_AMRWB_DECODERTYPES_H_

