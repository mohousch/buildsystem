#ifndef _MP3E_ENCODER_TYPES_H_
#define _MP3E_ENCODER_TYPES_H_

#include "acc_mmedefines.h"
typedef struct
{
	long bandWidth; /* BandWidth */
	enum eAccBoolean fIntensity; /* Intensity Stereo */
	enum eAccBoolean fVbrMode; /* VBR Mode */
	int vbrQuality; /* VBR Quality */
	enum eAccBoolean fFullHuffman;
	enum eAccBoolean paddingMode;
	enum eAccBoolean fCrc;
	enum eAccBoolean privateBit;
	enum eAccBoolean copyRightBit;
	enum eAccBoolean originalCopyBit;
	enum eAccBoolean EmphasisFlag;
	enum eAccBoolean downmix;
} tMMEMp3eConfig;

typedef struct
{
	U32 Status;
} MME_Mp3eStatus_t;

#endif //_MP3E_ENCODER_TYPES_H_

