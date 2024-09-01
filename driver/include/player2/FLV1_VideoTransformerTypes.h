/*
 * @file : MMEplus/Interface/Transformer/FLV1_VideoTransformerTypes.h
 *
 * @brief : FLV1 Video Decoder specific types for MME
 *
 *
 * &copy; 2008 ST Microelectronics. All Rights Reserved.
 *
*/

#ifndef _FLV1_VIDEOTRANSFORMERTYPES_H_
#define _FLV1_VIDEOTRANSFORMERTYPES_H_

#define FLV1_MME_TRANSFORMER_NAME "FLV1_TRANSFORMER"

#define FLV1_MME_API_VERSION "0.1"

#define FLV1DEC_TRANSFORMER_RELEASE "FLV1 Firmware 0.1.0" /*Done on 31st July'09*/

#ifndef WIN32
#define USE_BITSTREAM_HANDLER
#endif /* WIN32 */

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

#ifndef DEFINED_UL64
#define DEFINED_UL64
typedef signed int UL64;
#endif

/* FLV1_LumaAddress_t :
 Defines the address type for the FLV1 decoded Luma data's
*/
typedef U8 *FLV1_LumaAddress_t;

/* FLV1_ChromaAddress_t :
 Defines the address type for the FLV12 decoded Chroma data's
*/
typedef U8 *FLV1_ChromaAddress_t;

/* FLV1_CompressedData_t :
 Defines the address type for the MPEG2 compressed data
*/
typedef U8 *FLV1_CompressedData_t;

typedef enum
{
	FLV1_NO_ERROR = 0, /* No Error condition detected -- Sucessfull decoding */
	FLV1_DECODER_ERROR_TASK_TIMEOUT /* Decoder task timeout has occurred */
} FLV1_ERROR_CODES;

typedef enum _FLV1CODECTYPE_T_
{
	FLV1 = 0
} FLV1_CodecType_t;

typedef struct
{
	FLV1_CodecType_t codec_id;
	/*-------------------------------*/
#ifndef SEND_GLOBAL_PARAMS_CMD
	/*-------------------------------*/
	U32 CodedWidth;
	U32 CodedHeight;
	/*-------------------------------*/
#endif /* SEND_GLOBAL_PARAMS_CMD */
	/*-------------------------------*/
} FLV1_InitTransformerParam_t;

typedef struct
{
	U32 StructSize; /* Size of the structure in bytes */
	U32 EnableDeblocking;
	U32 PicType ;
	U32 H263Flv;
	U32 ChromaQscale;
	U32 Qscale;
	U32 Dropable ;
	U32 index;
	U32 size_in_bits;

	/*------------------------------*/
#ifdef USE_BITSTREAM_HANDLER
	/*------------------------------*/
	U32 PictureStartOffset ;
	U32 PictureStopOffset ;
	/*------------------------------*/
#endif /* USE_BITSTREAM_HANDLER */
	/*------------------------------*/
} FLV1_ParamPicture_t;

typedef struct
{
	U32 StructSize; /* Size of the structure in bytes */
	FLV1_ParamPicture_t PictureParameters; /* Picture parameters */
	U32 AdditionalFlags; /* Additonal Flags for future uses */
} FLV1_TransformParam_t;

/*-------------------------------*/
#ifdef SEND_GLOBAL_PARAMS_CMD
/*-------------------------------*/
typedef struct
{
	U32 StructSize; /* Size of the structure in bytes */
	U32 CodedWidth;
	U32 CodedHeight;
} FLV1_SetGlobalParamSequence_t;
/*-------------------------------*/
#endif /* SEND_GLOBAL_PARAMS_CMD */
/*-------------------------------*/

typedef struct
{
	FLV1_ERROR_CODES Errortype;
	/*unsigned int pm_cycles;*/
	/*unsigned int pm_dmiss;*/
	/*unsigned int pm_imiss;*/
	/*unsigned int pm_bundles;*/
	/*unsigned int pm_nop_bundles;*/
} FLV1_ReturnParams_t;

#endif /* _FLV1_VIDEOTRANSFORMERTYPES_H_ */

