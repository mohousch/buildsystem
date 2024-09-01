#ifndef _MP2AE_ENCODER_TYPES_H_
#define _MP2AE_ENCODER_TYPES_H_

#include "acc_mmedefines.h"

typedef struct
{
	int Mode;
	enum eAccBoolean CrcOn;
	enum eAccEmphasis Emphasis;
	enum eAccBoolean Copyrighted;
	enum eAccBoolean Original;
} tMMEMp2aeConfig;

typedef struct
{
	U32 Status;
} MME_Mp2aeStatus_t;

#endif //_MP2AE_ENCODER_TYPES_H_

