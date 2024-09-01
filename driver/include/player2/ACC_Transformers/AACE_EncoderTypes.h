#ifndef _AACE_ENCODER_TYPES_H_
#define _AACE_ENCODER_TYPES_H_

#include "acc_mmedefines.h"
enum eEncoderAACSubType
{
	ACC_ENCODER_AACE_ADTS,
	ACC_ENCODER_AACE_LOAS,

	// do not edit below this item
	ACC_LAST_ENCODER_AAC_SUBTYPE
};

typedef struct
{
	enum eEncoderAACSubType SubType;

	unsigned long quantqual;
	int VbrOn;
} tMMEAaceConfig;

typedef struct
{
	U32 Status;
} MME_AaceStatus_t;

#endif //_MP3ELC_ENCODER_TYPES_H_

