/**
*** @file : H263Decoder\H263Dec_Interface\H263Dec_TransformerTypes.h
***
*** @brief : Structures and types for the MME H263 decoder
***
*** @par OWNER: ST Microelectronics
***
*** @author : Sudipto Paul
***
*** @par SCOPE:
***
*** @date : 2006-04-27
***
*** &copy; 2003 ST Microelectronics. All Rights Reserved.
**/

#ifndef H263_DECODERTYPES_H
#define H263_DECODERTYPES_H

#include "mme.h"

#define H263_TRANSFORMER_NAME "H263_TRANSFORMER"

/* To Disable the MME_GLOBAL_TRANSFORM command */
#ifdef DISABLE_GLOBAL_TRANSFORM
#undef DISABLE_GLOBAL_TRANSFORM
#endif

#define MAX_NO_GOB 18 // Set for CIF picture
#undef PROFILE_ON

typedef enum
{
	MOTION_COMPENSATED, // Motion compensation with the previous frame motion vector,forming the predicted frame and adding with zero residual
	MACROBLOCK_REPLACEMENT // Macroblock replacement from the prvious frome at the same position
} Error_ConcealmentType_t;

typedef enum
{
	H263_NO_ERROR, // NO Error
	WRONG_START_CODE_EMULATION, // Bit should be always 1
	WRONG_DISTINCTION_MARKER , // Distinction with H261 always 0
	SPLIT_SCREEN_NOT_SUPPORTED , // Split screen not supported in this version
	DOC_CAMERA_INDICATOR_NOT_SUPPORTED, // Document camera indicator not supported
	PICTURE_FREEZE_NOT_SUPPORTED, // Picture freeze not supported in this version
	UNSUPPORTED_RESOLUTION, // Maximum resolution supported is CIF
	ANNEX_D_NOT_SUPPORTED, // Unrestricted Motion vector mode not supported in this version
	ANNEX_E_NOT_SUPPORTED, // Syntax based arithmetic coding not supported
	ANNEX_F_NOT_SUPPORTED, // Advanced prediction mode not supported
	ANNEX_G_NOT_SUPPORTED, // PB-Frame mode not supported
	CPM_MODE_NOT_SUPPORTED, // CPM mode not supported
	INVALID_UFEP, // Invalid UFEP bit
	ANNEX_N_NOT_SUPPORTED, // Reference picture selection mode not supported
	ANNEX_R_NOT_SUPPORTED, // Independent Segment mode decoding not supported
	ANNEX_S_NOT_SUPPORTED, // Alternate Inter VLC mode not supported
	ANNEX_I_NOT_SUPPORTED, // Advanced Intra coding mode not supported
	ANNEX_J_NOT_SUPPORTED, // Deblocking Filter mode not supported
	ANNEX_K_NOT_SUPPORTED, // Slice structure mode not supported
	ANNEX_T_NOT_SUPPORTED, // Modified Quantization mode not supported
	CORRUPT_MARKER_BIT, // Corrupted markler bit
	ANNEX_P_NOT_SUPPORTED, // Reference picture resampling mode not supported
	ANNEX_Q_NOT_SUPPORTED, // Reduced resolution update mode not supported
	INVALID_P_TYPE, // Ptype not valid
	UNSUPPORTED_RECTANGULAR_SLICE, // Rectangular slice order not supported
	ARBITARY_SLICE_ORDER, // Arbitary slice order not supported
	PICTURE_SC_ERROR, // Could not find the begining of the picture layer
#ifndef DISABLE_GLOBAL_TRANSFORM
	MEMORY_ALLOCATION_FAILURE // Unable to allocate memory for firmware internal usage
#endif
} H263D_ErrorCodes_t;

/* Structure for return parameters of TRANSFORM commands */
typedef struct
{
	H263D_ErrorCodes_t ErrorType; // To return the encountered error type
	unsigned int EndofStream; // Flag set to 1 if the end of stream is detected by the transformer
#ifdef PROFILE_ON
	unsigned int Decoding_cycle;
	unsigned int DMiss_cycle;
	unsigned int IMiss_cycle;
#endif
} H263D_DecodeReturnParams_t;

/* Transformer Cpabiltity */
typedef struct
{
	unsigned int caps_len;
} H263D_Capability_t;

#ifndef DISABLE_GLOBAL_TRANSFORM
/* Global Transform Parameters */
typedef struct
{
	unsigned int PictureWidhth; // Picture Width parsed from the packet header
	unsigned int PictureHeight; // Picture Height parsed from the packet header
	Error_ConcealmentType_t ErrorConceal_type; // Error concealment type in the firmware
} H263D_GlobalParams_t ;

typedef struct
{
	H263D_ErrorCodes_t ErrorType;
} H263D_GlobalReturnParams_t ;
#else
typedef struct
{
	unsigned int PictureWidth; // Picture Width parsed from the packet header
	unsigned int PictureHeight; // Picture Height parsed from the packet header
	Error_ConcealmentType_t ErrorConceal_type; // Error concealment type in the firmware
} H263D_InitParams_t ;
#endif

typedef struct
{
	unsigned int GobLossFlag;
	unsigned int MB_start;
	unsigned int MB_end;
	unsigned int Discard_bits;
} PacketLossParams_t;

typedef struct
{
	unsigned int Quant;
	unsigned int Width;
	unsigned int Height;
	unsigned int FirstPacketLossFlag;
	unsigned int FrameType;
	PacketLossParams_t LossParams[MAX_NO_GOB];
} H263D_TransformParams_t;

#endif /*#ifndef H263_DECODERTYPES_H*/
