/**
*** @file : Dvd_driver/MMEplus/Interface/Transformer/JPEG_DecoderTypes.h
***
*** @brief : Structures and types for the MME JPEG decoder
***
*** @par OWNER: ST Microelectronics, JPEG Decoder Team
***
*** @author : Sudipto Paul, Roland Schaufler
***
*** @par SCOPE:
***
*** @date : 2003-09-24
***
*** &copy; 2003 ST Microelectronics. All Rights Reserved.
**/

#ifndef __JPEG_VIDEOTRANSFORMERTYPES_H__
#define __JPEG_VIDEOTRANSFORMERTYPES_H__

#include "mme.h"

/* Common driver error constants */
#define JPEGDEC_ERROR_ID 0
#define JPEGDEC_ERROR_BASE (JPEG_ERROR_ID << 16)

#define JPEGDEC_MME_TRANSFORMER_NAME "JPEG_Transformer"

#define JPEGDEC_MME_API_VERSION "1.0"

/* Structure for GetTransformerCapability() */
/* This structure is currently not used on the target because its */
/* implemenation of the MME API layer does not support it yet! */
typedef struct
{
	/* for further expansion... */
	unsigned int dummy;
} JPEGDEC_TransformerInfo_t;

/* Structure for the Transformer Initialization */
typedef struct
{
	/* The X(0) coardinate for subregion decoding */
	unsigned int xvalue0;
	/* The X(1) coardinate for subregion decoding */
	unsigned int xvalue1;
	/* The Y(0) coardinate for subregion decoding */
	unsigned int yvalue0;
	/* The Y(1) coardinate for subregion decoding */
	unsigned int yvalue1;

	/* Output width of the decoded image. Used for IDCT select and resizing(if done in the core ) */
	int outputWidth;
	/* Output height of the decoded image. Used for IDCT select and resizing(if done in the core ) */
	int outputHeight;

	/* TRUE if Rotatedegree is other than zero degree Rotation clockwise/anticlockwise is required */
	int ROTATEFLAG;
	/* degree of rotation clockwise/anti-clockwise || 90 | 180 | 270 */
	int Rotatedegree;
	/* Flag set TRUE when vertical flip of the picture is required */
	int VerticalFlip;
	/* Flag set TRUE when vertical flip of the picture is required */
	int HorizantalFlip;

	/* the pitch. Valid only if ROTATEFLAG & 0x80000000 */
	int Pitch;
} JPEGDEC_InitParams_t;

/* Structure for the databuffers passed with TRANSFORM commands */
typedef struct
{
	/* Steering of size/widht/height of output picture */
	JPEGDEC_InitParams_t outputSettings;
} JPEGDEC_TransformParams_t;

typedef enum
{
	JPEG_NO_ERROR,
	UNDEFINED_HUFF_TABLE,
	UNSUPPORTED_MARKER,
	UNABLE_ALLOCATE_MEMORY,
	NON_SUPPORTED_SAMP_FACTORS,
	BAD_PARAMETER,
	DECODE_ERROR,
	BAD_RESTART_MARKER,
	UNSUPPORTED_COLORSPACE,
	BAD_SOS_SPECTRAL,
	BAD_SOS_SUCCESSIVE,
	BAD_HEADER_LENGHT,
	BAD_COUNT_VALUE,
	BAD_DHT_MARKER,
	BAD_INDEX_VALUE,
	BAD_NUMBER_HUFFMAN_TABLES,
	BAD_QUANT_TABLE_LENGHT,
	BAD_NUMBER_QUANT_TABLES,
	BAD_COMPONENT_COUNT,
	DIVIDE_BY_ZERO_ERROR,
	NOT_JPG_IMAGE,
	UNSUPPORTED_ROTATION_ANGLE,
	UNSUPPORTED_SCALING,
	INSUFFICIENT_OUTPUTBUFFER_SIZE
} JPEGDEC_ErrorCodes_t;

/* Structure for return parameters of TRANSFORM commands */
typedef struct
{
	/* bytes written to the o/p buffer sent back to the host */
	unsigned int bytes_written;
	/* Scaled image output height */
	int decodedImageHeight;
	/* Scaled image output width */
	int decodedImageWidth;
	unsigned int Total_cycle;
	unsigned int DMiss_Cycle;
	unsigned int IMiss_Cycle;
	JPEGDEC_ErrorCodes_t ErrorType;
} JPEGDEC_TransformReturnParams_t;

typedef struct
{
	unsigned int caps_len;
} JPEGDEC_Capability_t;

#endif /* __JPEG_VIDEOTRANSFORMERTYPES_H__ */
