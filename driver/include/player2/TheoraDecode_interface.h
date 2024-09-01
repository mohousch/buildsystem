/**
*** @file TheoraDecode_interface.h
*** @brief MME API interface ST200 Theora decoder transformer
*** @author Manish Sikarwar/Akash goel
*** @date 14-Mar-2009
*** &copy; (c) 2009 ST Microelectronics. All Rights Reserved.
**/

#ifndef _THEORADECODE_INTERFACE_H_
#define _THEORADECODE_INTERFACE_H_

#define THEORADEC_MME_TRANSFORMER_NAME "THEORADEC_TRANSFORMER"

#ifndef THEORADEC_MME_VERSION
#define THEORADEC_MME_VERSION 30 /*Latest MME API version*/
#endif

#if (THEORADEC_MME_VERSION==30)
#define THEORADEC_MME_API_VERSION "0.3"
#elif (THEORADEC_MME_VERSION==20)
#define THEORADEC_MME_API_VERSION "0.2"
#else
#define THEORADEC_MME_API_VERSION "0.1"
#endif

/* Common signed types */
#ifndef DEFINED_U32
#define DEFINED_U32
typedef unsigned int U32;
typedef unsigned int BOOL ;
#endif

#ifndef DEFINED_S32
#define DEFINED_S32
typedef signed int S32;
#endif

#ifndef DEFINED_S8
#define DEFINED_S8
typedef unsigned char U8;
#endif

#ifndef DEFINED_U16
#define DEFINED_U16
typedef signed short U16;
#endif

#ifndef DEFINED_S16
#define DEFINED_S16
typedef signed short S16;
#endif

/* VP3 transformer */

typedef enum
{
	THEORA_NO_ERROR, /* No Error condition detected -- Sucessfull decoding */
	THEORA_DECODING_ERROR, /* Error condition detected -- in decoding */
	THEORA_DECODER_ERROR_TASK_TIMEOUT /* Decoder task timeout has occurred */
	/* More Error Codes to be added later */
} THEORA_ERROR_CODES;

typedef struct
{
	U32 Data; /* Pointer to start of the header */
	U32 Size ; /* Size of the header in bytes */
} THEORA_HeaderPacket_t;

typedef struct
{
	U32 StructSize; /* Size of the structure in bytes */
	U32 PictureStartOffset ;
	U32 PictureStopOffset ;
} THEORA_ParamPicture_t;

typedef struct
{
#if (THEORADEC_MME_VERSION < 20)
	U32 CodedWidth;
	U32 CodedHeight;
	U32 CodecVersion; // To determine flipped image (s->theora < 0x030200)

	U8 theora_tables;
	U32 filter_limit_values[64];
	U32 coded_ac_scale_factor[64];
	U16 coded_dc_scale_factor[64];

	U8 base_matrix[384][64];
	U8 qr_count[2][3];
	U8 qr_size [2][3][64];
	U16 qr_base[2][3][64];

	S32 hti;
	U32 hbits;
	S32 entries;
	S32 huff_code_size;
	U16 huffman_table[80][32][2];
#else
	THEORA_HeaderPacket_t InfoHeader;
	THEORA_HeaderPacket_t CommentHeader;
	THEORA_HeaderPacket_t SetUpHeader;
#if (THEORADEC_MME_VERSION >= 30)
	U32 CoefficientBuffer;
#endif
#endif

} THEORA_InitTransformerParam_t;

typedef struct
{
	U32 StructSize; /* Size of the structure in bytes */
	THEORA_ParamPicture_t PictureParameters; /* Picture parameters */
	U32 AdditionalFlags; /* Additonal Flags for future uses */
} THEORA_TransformParam_t;

typedef struct
{
	THEORA_ERROR_CODES Errortype;
	/* profiling info */
	U32 pm_cycles;
	U32 pm_dmiss;
	U32 pm_imiss;
	U32 pm_bundles;
	U32 pm_nop_bundles;
} THEORA_ReturnParams_t;

typedef struct
{
	U32 display_buffer_size; /* Omega2 frame buffer size (luma+chroma) */
	U32 packed_buffer_size; /* packed-data buffer size (for VIDA) */
	U32 packed_buffer_total; /* number of packed buffers (for VIDA) */
} THEORA_CapabilityParams_t;

#endif /* _THEORADECODE_INTERFACE_H_ */

