///
/// @file : AVS_DecoderTypes.h
///
/// @brief : AVS Video Decoder specific types for MME
///
/// @par OWNER:
///
/// @author : Sudipto Paul
///
/// @par SCOPE:
///
/// @date : 2006-11-23
///
/// &copy; 2006 ST Microelectronics. All Rights Reserved.
///

#ifndef _AVS_DECODERTYPES_H_
#define _AVS_DECODERTYPES_H_

#include "mme.h"
#ifdef __ST200__
#include "delta.h"
#endif

#define AVSDECSD_MME_TRANSFORMER_NAME "AVSSD_TRANSFORMER"

#define AVSDECSD_MME_API_VERSION "2.0"

/*
** AVS_CompressedData_t :
** Defines the address type for the AVS compressed data
*/
typedef U32 *AVS_CompressedData_t;

/*
** AVS_LumaAddress_t :
** Defines the address type for the AVS decoded Luma data’s
** Bits 0 to 7 shall be set to 0
*/
typedef U32 *AVS_LumaAddress_t;

/*
** AVS_ChromaAddress_t :
** Defines the address type for the AVS decoded Chroma data’s
** Bits 0 to 7 shall be set to 0
*/
typedef U32 *AVS_ChromaAddress_t;

typedef U32 *AVS_MBStructureAddress_t;

typedef struct _AVSDECODEDBUFFERADDRESS_T_
{
	AVS_LumaAddress_t Luma_p;
	AVS_ChromaAddress_t Chroma_p;
	AVS_LumaAddress_t LumaDecimated_p;
	AVS_ChromaAddress_t ChromaDecimated_p;
	AVS_MBStructureAddress_t MBStruct_p;
} AVS_DecodedBufferAddress_t;

/*
** AVS_RefPicListAddress_t :
** Defines the addresses where the two reference pictures’ data will be stored
*/
typedef struct _AVSREFPICLISTADDRESS_T_
{
	AVS_LumaAddress_t BackwardRefLuma_p;
	AVS_ChromaAddress_t BackwardRefChroma_p;
	AVS_LumaAddress_t ForwardRefLuma_p;
	AVS_ChromaAddress_t ForwardRefChroma_p;
} AVS_RefPicListAddress_t;

/*
** AVS_PictureType_t :
** Identifies the type of the picture to be decoded.
*/
typedef enum _AVS_PICTURETYPE_T_
{
	AVS_PICTURE_TYPE_I = 0,
	AVS_PICTURE_TYPE_P,
	AVS_PICTURE_TYPE_B,
	AVS_PICTURE_TYPE_NONE
} AVS_PictureType_t;

/* Structure reuired by the FW for correcting the INTRA MB's in the second pass on B picture */
typedef struct _INTRA_MB_STRUCT_T_
{
	U32 mb_x; /* Horizantal coardinate of the Intra Mb of B picture */
	U32 mb_y; /* Vertical coardinate of the Intra Mb in B picture */
	S16 Res_blk[6][64]; /* Residual block */
	S8 Pred_mode_Luma_blk[4]; /* Prediction modes of the 4 block in an Intra MB of a B picture */
	S8 Pred_mode_Chroma; /* Prediction mode of chroma block(Cb/Cr) in an Intra MB of a B picture */
	S8 cbp; /* Coded block Pattern */
	S32 curr_mb_no; /* Current MB Number */
	S32 coded_coeff[6]; /* Number of coefficient in the block are coded */
	S32 First_MB_In_Current_Slice; /* Flag for the first MB in Slice */
} AVS_IntraMBstruct_t;

typedef struct _AVSVIDEODECODEINITPARAMS_T_
{
	AVS_CompressedData_t CircularBufferBeginAddr_p;
	AVS_CompressedData_t CircularBufferEndAddr_p;
	AVS_IntraMBstruct_t *IntraMB_struct_ptr;
	MME_UINT Width;
	MME_UINT Height;
} MME_AVSVideoDecodeInitParams_t;

/*
** AVS_FrameSyntax_t :
** Identifies the syntax of the Frame to decode.
*/
typedef enum _AVS_FRAMESYNTAX_T_
{
	AVS_INTERLACED_FRAME = 0,
	AVS_PROGRESSIVE_FRAME
} AVS_FrameSyntax_t;

/*
** AVS_MainAuxEnable_t :
** Used for enabling Main/Aux outputs
*/
typedef enum _AVS_MAINAUXENABLE_T_
{
	AVS_AUXOUT_EN = 0x00000010, /* enable decimated reconstruction */
	AVS_MAINOUT_EN = 0x00000020, /* enable main reconstruction */
	AVS_AUX_MAIN_OUT_EN = 0x00000030 /* enable both main & decimated reconstruction */
} AVS_MainAuxEnable_t;

/*
** AVS_HorizontalDeciFactor _t :
** Identifies the horizontal decimation factor
*/
typedef enum _AVS_HORIZONTALDECIFACTOR_T_
{
	AVS_HDEC_1 = 0x00000000, /* no resize */
	AVS_HDEC_ADVANCED_2 = 0x00000101, /* Advanced H/2 resize using improved 8-tap filters */
	AVS_HDEC_ADVANCED_4 = 0x00000102 /* Advanced H/4 resize using improved 8-tap filters */
} AVS_HorizontalDeciFactor_t;

/*
** AVS_VerticalDeciFactor _t :
** Identifies the vertical decimation factor
*/
typedef enum _AVS_VERTICALDECIFACTOR_T_
{
	AVS_VDEC_1 = 0x00000000, /* no resize */
	AVS_VDEC_ADVANCED_2_PROG = 0x00000204, /* V/2 , progressive resize */
	AVS_VDEC_ADVANCED_2_INT = 0x000000208 /* V/2 , interlaced resize */
} AVS_VerticalDeciFactor_t;

/*
** AVS_SeqSyntax_t :
** Identifies the syntax of the Sequence to decode.
*/
typedef enum _AVS_SEQSYNTAX_T_
{
	AVS_INTERLACED_SEQ = 0, /* Interlaced sequence */
	AVS_PROGRESSIVE_SEQ /* Progressive sequence*/
} AVS_SeqSyntax_t;

/*
** AVS_PicStruct_t :
** Identifies the picture structure (Frame / Field picture)
*/
typedef enum _AVS_PICSTRUCT_T_
{
	AVS_FIELD_PIC = 0, /* Coded data of the two fields of current picture
 appears in sequential order */
	AVS_FRAME_PIC /* Coded data of the two fields of current picture
 appears in alternate order */
} AVS_PicStruct_t;

/*
** AVS_SkipMode_t :
** Identifies the coding type of skip mode
*/
typedef enum _AVS_SKIPMODE_T_
{
	AVS_SKIPMODE_FROM_MBTYPE = 0,
	AVS_SKIPMODE_FROM_RLCODING
} AVS_SkipMode_t;

/*
** AVS_PicRef_t :
** Identifies the reference picture flag
*/
typedef enum _AVS_PICREF_T_
{
	AVS_PICREF_SPECIFIED = 0,
	AVS_PICREF_DEFAULT
} AVS_PicRef_t;

/*
** AVS_DecodingMode_t :
** Identifies the decoding mode.
*/
typedef enum _AVS_DECODINGMODE_T_
{
	AVS_NORMAL_DECODE = 0,
	/* Other values to be added later */
} AVS_DecodingMode_t;

/* AVS_SliceParam_t :
** Structure describing the slice used for error concealment */

typedef struct _AVS_SLICEPARAM_T_
{
	AVS_CompressedData_t SliceStartAddrCompressedBuffer_p;
	MME_UINT SliceAddress;
} AVS_SliceParam_t;

/* AVS_StartCodecsParam_t :
** It describes the slice start codes of the current picture
*/

#define MAX_SLICE 68 /* Maximum number of slices = maximum number of MB lines == 1088 */ /* 16 */
typedef struct _AVS_STARTCODESPARAM_T_
{
	MME_UINT SliceCount;
	AVS_SliceParam_t SliceArray[MAX_SLICE];
} AVS_StartCodecsParam_t;

/*
** AVS_FieldSyntax_t :
** Identifies the syntax of the interlaced field to decode.
*/
typedef enum _AVS_FIELDSYNTAX_T_
{
	AVS_INTERLACED_TOP_FIELD = 0,
	AVS_INTERLACED_BOT_FIELD = 1
} AVS_FieldSyntax_t;

/*
** AVS_SetGlobalParamSequence_t :
** Parameters to be sent along with the MME_SET_GLOBAL_TRANSFORM_PARAMS command
*/
typedef struct _AVS_SETGLOBALPARAMSEQUENCE_T_
{
	MME_UINT Width;
	MME_UINT Height;
	AVS_SeqSyntax_t Progressive_sequence; /* Not Used */
} MME_AVSSetGlobalParamSequence_t;

/*
** AVS_DecodingError_t :
** Status of the decoding process
*/
typedef enum
{
	/* The firmware has been sucessful */
	AVS_DECODER_NO_ERROR = 0,
#ifdef ERC
	/* The firmware decoded too much MBs:
	 - the AVS_CommandStatus_t.Status doesn't locate these erroneous MBs
	 because the firmware can't know where are these extra MBs.
	 - MME_CommandStatus_t.Error can also contain AVS_DECODER_ERROR_MB_OVERFLOW.
	*/
	AVS_DECODER_ERROR_MB_OVERFLOW = 1,

	/* The firmware encountered error(s) that were recovered:
	 - AVS_CommandStatus_t.Status locates the erroneous MBs.
	 - MME_CommandStatus_t.Error can also contain AVS_DECODER_ERROR_RECOVERED.
	*/
	AVS_DECODER_ERROR_RECOVERED = 2,

	/* The firmware encountered an error that can't be recovered:
	 - AVS_CommandStatus_t.Status has no meaning.
	 - MME_CommandStatus_t.Error is equal to AVS_DECODER_ERROR_NOT_RECOVERED.
	*/
	AVS_DECODER_ERROR_NOT_RECOVERED = 4,
#endif
	/* The firmware task is hanged and doesn't get back to watchdog task even after maximum time
	 alloted has lapsed:
	 - AVS_CommandStatus_t.Status has no meaning.
	 - MME_CommandStatus_t.Error is equal to AVS_DECODER_ERROR_TASK_TIMEOUT.
	*/
	AVS_DECODER_ERROR_TASK_TIMEOUT = 8
} AVS_DecodingError_t;

typedef struct _AVSVIDEODECODERETURNPARAMS_T_
{

	AVS_PictureType_t picturetype;
	/* profiling info */
	MME_UINT pm_cycles;
	MME_UINT pm_dmiss;
	MME_UINT pm_imiss;
	MME_UINT pm_bundles;
	MME_UINT pm_pft;
	AVS_DecodingError_t Errortype;
	MME_UINT Memory_usage;
} MME_AVSVideoDecodeReturnParams_t;

/*
** AVS_VideoDecodeCapabilityParams_t :
** Transformer capability parameters.
*/
typedef struct _AVS_VIDEODECODECAPABILITYPARAMS_T_
{
	MME_UINT api_version; /* Omega2 frame buffer size (luma+chroma) */
} MME_AVSVideoDecodeCapabilityParams_t;

typedef struct _AVSVIDEODECODEPARAMS_T_
{
	AVS_CompressedData_t PictureStartAddr_p;
	AVS_CompressedData_t PictureEndAddr_p;
	AVS_DecodedBufferAddress_t DecodedBufferAddr;
	AVS_RefPicListAddress_t RefPicListAddr;
	AVS_FrameSyntax_t Progressive_frame; /* Not Used */
	AVS_MainAuxEnable_t MainAuxEnable; /* Not Used */
	AVS_HorizontalDeciFactor_t HorizontalDecimationFactor; /* Not Used */
	AVS_VerticalDeciFactor_t VerticalDecimationFactor; /* Not Used */
	U8 AebrFlag; /* Not Used */
	AVS_PicStruct_t Picture_structure;
	AVS_PicStruct_t Picture_structure_bwd; /* Not Used */
	MME_UINT Fixed_picture_qp;
	MME_UINT Picture_qp;
	AVS_SkipMode_t Skip_mode_flag;
	MME_UINT Loop_filter_disable;
	AVS_PictureType_t FrameType;
	S32 alpha_offset;
	S32 beta_offset;
	AVS_PicRef_t Picture_ref_flag;
	S32 tr;
	S32 imgtr_next_P;
	S32 imgtr_last_P;
	S32 imgtr_last_prev_P;
	AVS_FieldSyntax_t field_flag;
	U32 topfield_pos;
	U32 botfield_pos;
	AVS_StartCodecsParam_t StartCodecs; /* Not Used */
	AVS_DecodingMode_t DecodingMode;/* Not Used */
	MME_UINT AdditionalFlags;/* Not Used */
	MME_UINT CEH_returnvalue_flag;/* Not Used */

} MME_AVSVideoDecodeParams_t;

#endif /* _AVS_DECODERTYPES_H_ */

