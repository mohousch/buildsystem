///
/// @file : MMEplus/Interface/Transformer/DivX_DecoderTypes.h
///
/// @brief : MPEG4 Video Decoder specific types for MME
///
/// @par OWNER: Milorad Neskovic
///
/// @author : Jim Hauxwell, Milorad Neskovic
///
/// @par SCOPE:
///
/// @date : 2004-02-18
///
/// &copy; 2002 ST Microelectronics. All Rights Reserved.
///

#ifndef _DIVXHD_DECODERTYPES_H_
#define _DIVXHD_DECODERTYPES_H_

#define DIVXHD_MME_TRANSFORMER_NAME "MPEG4P2_720P_TRANSFORMER"

#include "mme.h"

#include "DivX_decoderTypes.h"

#define DIVXHDDECHD_MME_API_VERSION "1.1"

typedef unsigned int *DivXHD_CompressedData_t;

/*
** AVS_LumaAddress_t :
** Defines the address type for the AVS decoded Luma data’s
** Bits 0 to 7 shall be set to 0
*/
typedef unsigned int *DivXHD_LumaAddress_t;

/*
** AVS_ChromaAddress_t :
** Defines the address type for the AVS decoded Chroma data’s
** Bits 0 to 7 shall be set to 0
*/
typedef unsigned int *DivXHD_ChromaAddress_t;

/* MB struct for DC MB in B picture */
typedef unsigned int *DivXHD_MBStructureAddress_t;

typedef struct _DIVXHDDECODEDBUFFERADDRESS_T_
{
	DivXHD_LumaAddress_t Luma_p;
	DivXHD_ChromaAddress_t Chroma_p;
	DivXHD_LumaAddress_t LumaDecimated_p;
	DivXHD_ChromaAddress_t ChromaDecimated_p;
	DivXHD_MBStructureAddress_t MBStruct_p;
} DivXHD_DecodedBufferAddress_t;

typedef struct _DIVXREFPICLISTADDRESS_T_
{
	DivXHD_LumaAddress_t BackwardRefLuma_p;
	DivXHD_ChromaAddress_t BackwardRefChroma_p;
	DivXHD_LumaAddress_t ForwardRefLuma_p;
	DivXHD_ChromaAddress_t ForwardRefChroma_p;
} DivXHD_RefPicListAddress_t;

typedef PICTURETYPE DIVXHD_PICTURETYPE;
typedef ERROR_CODES DIVXHD_ERROR_CODES;

#if 0
// should match mp4_vars.h (I,P,B,S_VOP)
typedef enum _DIVX_HD_PICTURETYPE_T_
{
	IFRAME = 0,
	PFRAME,
	BFRAME,
	SFRAME
} DIVXHD_PICTURETYPE;

typedef enum _DIVXHD_ERROR_CODES_T_
{
	MPEG4P2_NO_ERROR, /* No Error condition detected -- Sucessfull decoding */
	MPEG4P2_ILLEGAL_MB_NUM_PCKT, /* Illegal MB no while processing multiple slices */
	MPEG4P2_CBPY_DAMAGED, /* CBPY of MB corrupted */
	MPEG4P2_CBPC_DAMAGED, /* CBPC of MB corrupted */
	MPEG4P2_ILLEGAL_MB_TYPE, /* Illegal MB Type detected */
	MPEG4P2_DC_DAMAGED, /* DC value damaged for block detected */
	MPEG4P2_AC_DAMAGED, /* AC value damaged for block detected */
	MPEG4P2_MISSING_MARKER_BIT, /* Marker bit disabled detected */
	MPEG4P2_HEADER_DAMAGED, /* VOL/VOP/GOP/User data corrupted */
	MPEG4P2_INVALID_PICTURE_SIZE, /* W/H detected during VOL header parsing doenot match with the W/H during Intialization */
	MPEG4P2_GMC_NOT_SUPPORTED, /* GLOBAL Motion Compensation not supported */
	MPEG4P2_QPEL_NOT_SUPPORTED, /* QPEL not supported */
	MPEG4P2_FIRMWARE_TIME_OUT_ENCOUNTERED /* Timeout in F/W */
} DIVXHD_ERROR_CODES;

#endif

typedef enum
{
	DIVXHD_HDEC_1 = 0x00000000, /* no H resize */
	DIVXHD_HDEC_2 = 0x00000001, /* H/2 resize */
	DIVXHD_HDEC_4 = 0x00000002, /* H/4 resize */
	DIVXHD_HDEC_RESERVED = 0x00000003, /* reserved H resize */
	DIVXHD_HDEC_ADVANCED_2 = 0x00000101, /* Advanced H/2 resize using improved 8-tap filters */
	DIVXHD_HDEC_ADVANCED_4 = 0x00000102 /* Advanced H/4 resize using improved 8-tap filters */

} DIVXHD_HorizontalDeciFactor_t;

/*
** MPEG2_VerticalDeciFactor_t :
** Identifies the vertical decimation factor
*/
typedef enum
{
	DIVXHD_VDEC_1 = 0x00000000, /* no V resize */
	DIVXHD_VDEC_2_PROG = 0x00000004, /* V/2 , progressive resize */
	DIVXHD_VDEC_2_INT = 0x00000008, /* V/2 , interlaced resize */
	DIVXHD_VDEC_ADVANCED_2_PROG = 0x00000204, /* Advanced V/2 , progressive resize */
	DIVXHD_VDEC_ADVANCED_2_INT = 0x00000208 /* Advanced V/2 , interlaced resize */

} DIVXHD_VerticalDeciFactor_t;

/*
** MPEG2_MainAuxEnable_t :
** Used for enabling Main/Aux outputs
*/
typedef enum
{
	DIVXHD_AUXOUT_EN = 0x00000010, /* enable decimated reconstruction */
	DIVXHD_MAINOUT_EN = 0x00000020, /* enable main reconstruction */
	DIVXHD_AUX_MAIN_OUT_EN = 0x00000030 /* enable both main & decimated reconstruction */
} DIVXHD_MainAuxEnable_t;

typedef struct _DIVXHDVIDEODECODERETURNPARAMS_T_
{
	DIVXHD_ERROR_CODES Errortype;
	DIVXHD_PICTURETYPE picturetype;
	MME_UINT Memory_Usage;
	/* profiling info */
	MME_UINT pm_cycles;
	MME_UINT pm_dmiss;
	MME_UINT pm_imiss;
	MME_UINT pm_bundles;
	MME_UINT pm_pft;
} MME_DivXHDVideoDecodeReturnParams_t;

typedef struct _DIVXHDVIDEODECODEINITPARAMS_T_
{
	DivXHD_CompressedData_t CircularBufferBeginAddr_p;
	DivXHD_CompressedData_t CircularBufferEndAddr_p;
	MME_UINT width; //!< width predetermined from the AVI file
	MME_UINT height; //!< height predetermined from the AVI file
	MME_UINT codec_version; //!< width predetermined from the AVI file
} MME_DivXHDVideoDecodeInitParams_t;

typedef struct _DIVX_SETGLOBALPARAMSEQUENCE_T_
{
	int shape;
	int time_increment_resolution;
	int interlaced;
	int sprite_usage;
	int quant_type;
	int load_intra_quant_matrix;
	unsigned char intra_quant_matrix[64];
	unsigned char nonintra_quant_matrix[64];
	int load_nonintra_quant_matrix;
	int quarter_pixel;
	char resync_marker_disable; /* HE */
	char data_partitioning; /* HE */
	char reversible_vlc; /* HE */
	int short_video_header;//shv
	int num_mb_in_gob; // shv
	int num_gobs_in_vop; // shv
} MME_DivXHDSetGlobalParamSequence_t;

typedef struct _DIVXHDVIDEODECODEPARAMS_T_
{
	DivXHD_CompressedData_t PictureStartAddr_p;
	DivXHD_CompressedData_t PictureEndAddr_p;
	DivXHD_DecodedBufferAddress_t DecodedBufferAddr;
	DivXHD_RefPicListAddress_t RefPicListAddr;
	DIVXHD_MainAuxEnable_t MainAuxEnable; /* enable Main and/or Aux outputs */
	DIVXHD_HorizontalDeciFactor_t HorizontalDecimationFactor; /* Horizontal decimation factor */
	DIVXHD_VerticalDeciFactor_t VerticalDecimationFactor; /* Vertical decimation factor */
	int prediction_type;
	int quantizer;
	int rounding_type;
	int fcode_for;
	int vop_coded;
	int use_intra_dc_vlc;
	int trb_trd;
	int trb_trd_trd;
	int intra_dc_vlc_thr;
	int fcode_back;
	int shape_coding_type;
	int bit_skip_no;
	char alternate_vertical_scan_flag;
	void *UserData_p;
} MME_DivXHDVideoDecodeParams_t;

typedef struct
{
	unsigned int caps_len;
} MME_DivXHD_Capability_t;

#endif /* _DIVXHD_DECODERTYPES_H_ */

