#ifndef _DTSE_ENCODER_TYPES_H_
#define _DTSE_ENCODER_TYPES_H_

#include "acc_mmedefines.h"

enum eMmmeDtseConfMasks
{
	//Wav File header on/off (default off) else don't (raw "2ch 16bit pcm")
	MME_DTSE_ADD_WAV_HEADER = 1,

	// Use Frame Based of
	MME_DTSE_FRAMEBASE_FRAMEBUFFER = 2,

	//Endianess of stream
	//The SyncWord for DTS is
	//7F FE 80 01
	//b3b2
	//0 0 : FE 7F 01 80
	//0 1 : 01 80 FE 7F
	//1 0 : 7F FE 80 01
	//1 1 : 80 01 7F FE
	MME_DTSE_BYTE_ORDER_0 = 4,
	MME_DTSE_BYTE_ORDER_1 = 8,

	//If set to '1' Compressed mode without padding
	MME_DTSE_NO_PADDING = 16
};

enum eMMEDtseByteOrder
{
	MME_DTSE_16BIT_LITTLEENDIAN,
	MME_DTSE_32BIT_LITTLEENDIAN,
	MME_DTSE_32BIT_BIGENDIAN,
	MME_DTSE_32BIT_SWAPPED16
};

// Same as above but with bitfields
typedef struct
{
	unsigned int AddWavHeader : 1; //! Adds a WAV header to the very first encoded frame and maintain the sample count.
	unsigned int FrameBaseFrameBuffer : 1; //! The Output FrameBuffer is managed as frame base (single page).

	unsigned int ByteOrder : 2; //! Specify the byte format according to enum eDtseByteOrder

	unsigned int DisablePadding : 1; //! Disable the '0' padding at the end of the frames.
} tMMEDtseOption;

typedef struct
{
	U32 Config;
} tMMEDtseConfig;

typedef struct
{
	U32 Status;
} MME_DtseStatus_t;

#endif //_DTSE_ENCODER_TYPES_H_

