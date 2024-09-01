/*
 * @file : MMEplus/Interface/Transformer/VP6_VideoTransformerTypes.h
 *
 * @brief : VP6 Video Decoder specific types for MME
 *
 *
 * &copy; 2002 ST Microelectronics. All Rights Reserved.
 *
*/

#ifndef _VP6_DECODERTYPES_H_
#define _VP6_DECODERTYPES_H_

#define VP6_MME_TRANSFORMER_NAME "VP6_TRANSFORMER"

#define VP6DEC_TRANSFORMER_RELEASE "VP6 Firmware 0.0.a" /*Done on 28th March 09*/

#ifndef VP6DEC_MME_VERSION
#define VP6DEC_MME_VERSION 20 /*Latest MME API version*/
#endif

#if (VP6DEC_MME_VERSION==20)
#define VP6_MME_API_VERSION "0.2"
#else
#define VP6_MME_API_VERSION "0.1"
#endif

#ifndef DEFINED_U32
#define DEFINED_U32
typedef unsigned int U32;
typedef unsigned int BOOL ;
#endif

/* Common signed types */
#ifndef DEFINED_S8
#define DEFINED_S8
typedef unsigned char U8;
#endif

#ifndef DEFINED_S16
#define DEFINED_S16
typedef signed short S16;
#endif

#ifndef DEFINED_S32
#define DEFINED_S32
typedef signed int S32;
#endif

/* VP6_LumaAddress_t :
 Defines the address type for the VP6 decoded Luma data's
*/
typedef U8 *VP6_LumaAddress_t;

/* VP6_ChromaAddress_t :
 Defines the address type for the VP6 decoded Chroma data's
*/
typedef U8 *VP6_ChromaAddress_t;

/* VP6_CompressedData_t :
 Defines the address type for the VP6 compressed data
*/
typedef U8 *VP6_CompressedData_t;

typedef enum
{
	VP6_NO_ERROR, /* No Error condition detected -- Sucessfull decoding */
	VP6_DECODER_ERROR_TASK_TIMEOUT /* Decoder task timeout has occurred */
	/* More Error Codes to be added later */
} VP6_ERROR_CODES;

typedef enum
{
	VP6f = 0,
	VP6 = 1,
	NONE
} VP6_CODEC_TYPE;

typedef struct
{
	U32 CodedWidth;
	U32 CodedHeight;
#if (VP6DEC_MME_VERSION > 10)
	U32 CodecVersion;
#endif
} VP6_InitTransformerParam_t;

typedef enum
{
	VP6_FRAME_CURRENT = 0,
	VP6_FRAME_PREVIOUS = 1,
	VP6_FRAME_GOLDEN = 2
} vp6_frame_t;

typedef struct
{
	U32 StructSize; /* Size of the structure in bytes */
	BOOL KeyFrame ;
	U32 SampleVarianceThreshold;
	U32 Deblock_Filtering;
	U32 Quant;
	U32 MaxVectorLength;
	U32 FilterMode;
	U32 FilterSelection;
	S32 high;
	U32 bits;
	U32 code_word;
	U32 PictureStartOffset ;
	U32 PictureStopOffset ;
} VP6_ParamPicture_t;

typedef struct
{
	U32 StructSize; /* Size of the structure in bytes */
	VP6_ParamPicture_t PictureParameters; /* Picture parameters */
	U32 AdditionalFlags; /* Additonal Flags for future uses */
} VP6_TransformParam_t;

typedef struct
{
	VP6_ERROR_CODES Errortype;
	/* profiling info */
	U32 pm_cycles;
	U32 pm_dmiss;
	U32 pm_imiss;
	U32 pm_bundles;
	U32 pm_nop_bundles;
} VP6_ReturnParams_t;

typedef struct
{
	U32 display_buffer_size; /* Omega2 frame buffer size (luma+chroma) */
	U32 packed_buffer_size; /* packed-data buffer size (for VIDA) */
	U32 packed_buffer_total; /* number of packed buffers (for VIDA) */
} VP6_CapabilityParams_t;

#endif /* _VP6_DECODERTYPES_H_ */

