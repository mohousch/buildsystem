/*
 * @file : MMEplus/Interface/Transformer/DivX_DecoderTypes.h
 *
 * @brief : MPEG4 Video Decoder specific types for MME
 *
 * @par OWNER: Milorad Neskovic
 *
 * @author : Jim Hauxwell, Milorad Neskovic
 *
 * @par SCOPE:
 *
 * @date : 2004-02-18
 *
 * &copy; 2002 ST Microelectronics. All Rights Reserved.
 *
*/

#ifndef _DIVX_DECODERTYPES_H_
#define _DIVX_DECODERTYPES_H_

#define DIVXDEC_MME_TRANSFORMER_NAME "DIVX_TRANSFORMER"
#define MPEG4P2_MME_TRANSFORMER_NAME "MPEG4P2_TRANSFORMER"

#include "mme.h"

#if (MPEG4P2_MME_VERSION >= 12)
#define MPEG4P2_MME_API_VERSION "1.2"
#else
#define MPEG4P2_MME_API_VERSION "1.1"
#endif

#define DIVXDEC_MME_API_VERSION "1.1"

/* DivX transformer */
/*--------------------- */
/* should match mp4_vars.h (I,P,B,S_VOP)*/
typedef enum _PICTURETYPE_T_
{
	IFRAME = 0,
	PFRAME,
	BFRAME,
	SFRAME
} PICTURETYPE;

typedef struct _DIVXVIDEODECODEINITPARAMS_T_
{
	MME_UINT width; /*!< width predetermined from the AVI file */
	MME_UINT height; /*!< height predetermined from the AVI file */
	MME_UINT codec_version; /*!< width predetermined from the AVI file */
} MME_DivXVideoDecodeInitParams_t;

#if (MPEG4P2_MME_VERSION >= 12)
typedef enum ERROR_CODES_T_
{
	MPEG4P2_NO_ERROR, /* No Error condition detected -- Sucessfull decoding */
	MPEG4P2_HEADER_DAMAGED, /* VOL/VOP/GOP/User data corrupted */
	MPEG4P2_NEW_PRED_NOT_SUPPORTED,
	MPEG4P2_SCALABILITY_NOT_SUPPORTED,
	MPEG4P2_VOP_SHAPE_NOT_SUPPORTED,
	MPEG4P2_STATIC_SPRITE_NOT_SUPPORTED,
	MPEG4P2_GMC_NOT_SUPPORTED, /* GLOBAL Motion Compensation not supported */
	MPEG4P2_QPEL_NOT_SUPPORTED, /* QPEL not supported */
	MPEG4P2_FIRMWARE_TIME_OUT_ENCOUNTERED, /* Timeout in F/W */
	MPEG4P2_ILLEGAL_MB_NUM_PCKT, /* Illegal MB no while processing multiple slices */
	MPEG4P2_CBPY_DAMAGED, /* CBPY of MB corrupted */
	MPEG4P2_CBPC_DAMAGED, /* CBPC of MB corrupted */
	MPEG4P2_ILLEGAL_MB_TYPE, /* Illegal MB Type detected */
	MPEG4P2_DC_DAMAGED, /* DC value damaged for block detected */
	MPEG4P2_AC_DAMAGED, /* AC value damaged for block detected */
	MPEG4P2_MISSING_MARKER_BIT, /* Marker bit disabled detected */
	MPEG4P2_INVALID_PICTURE_SIZE,
} ERROR_CODES;
#else /* MME API < 12 */
typedef enum ERROR_CODES_T_
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
} ERROR_CODES;
#endif

typedef struct _DIVXVIDEODECODERETURNPARAMS_T_
{
	ERROR_CODES Errortype;
	PICTURETYPE picturetype;
	/* profiling info */
	MME_UINT pm_cycles;
	MME_UINT pm_dmiss;
	MME_UINT pm_imiss;
	MME_UINT pm_bundles;
	MME_UINT pm_pft;
} MME_DivXVideoDecodeReturnParams_t;

typedef struct
{
	MME_UINT display_buffer_size; /* Omega2 frame buffer size (luma+chroma) */
	MME_UINT packed_buffer_size; /* packed-data buffer size (for VIDA) */
	MME_UINT packed_buffer_total; /* number of packed buffers (for VIDA) */
	MME_UINT supports_311; /* If non-zero indicates support for DivX 311 */
} MME_DivXVideoDecodeCapabilityParams_t;

typedef struct _DIVXVIDEODECODEPARAMS_T_
{
	MME_UINT StartOffset; /*!< Only used when input buffer is circular */
	MME_UINT EndOffset; /*!< Only used when input buffer is circular */
} MME_DivXVideoDecodeParams_t;

/*! DIVX Picture Coding type, the numeric values are guaranteed to correspond to the MPEG4 standard */
typedef enum
{
	DIVX_PCT_INTRA = 0,
	DIVX_PCT_PREDICTIVE = 1,
	DIVX_PCT_BIDIRECTIONAL = 2,
	DIVX_PCT_SPRITE = 3
} MME_DIVXPictureCodingType;

#endif /* _DIVX_DECODERTYPES_H_ */

