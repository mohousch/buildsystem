#ifndef RV89DECODE_INTERFACEH
#define RV89DECODE_INTERFACEH

#ifdef __cplusplus
extern "C"
{
#endif

#include "stddefs.h"

#define RV89DEC_MME_TRANSFORMER_NAME "RV89Decoder"

#if (RV89DEC_MME_VERSION > 12)
#define RV89DEC_MME_API_VERSION "1.3"
#elif (RV89DEC_MME_VERSION > 11)
#define RV89DEC_MME_API_VERSION "1.2"
#elif (RV89DEC_MME_VERSION > 10)
#define RV89DEC_MME_API_VERSION "1.1"
#endif

typedef U8 *RV89Dec_ChromaAddress_t;

typedef U8 *RV89Dec_LumaAddress_t;

typedef enum
{
	RV89DEC_INTRAPIC, /* I Frame */
	RV89DEC_INTERPIC, /* P Frame */
	RV89DEC_TRUEBPIC /* B frame */
} RV89Dec_PicType_t;

typedef enum
{
	RV89DEC_NO_ERROR,
	RV89DEC_TIMEOUT_ERROR,
	RV89DEC_RUN_TIME_INVALID_PARAMS,
	RV89DEC_FEATURE_NOT_IMPLEMENTED,
	RV89DEC_MEMORY_UNDERFLOW_ERROR,
	RV89DEC_MEMORY_TRANSLATION_ERROR,
	RV89DEC_TASK_CREATION_ERROR,
	RV89DEC_UNKNOWN_ERROR
} RV89Dec_ErrorCodes_t;

typedef enum
{
	RV89DEC_FID_REALVIDEO30,
	RV89DEC_FID_RV89COMBO
} RV89Dec_fid_t;

typedef enum
{
	RV89DEC_BFRAME_REGULAR_DEBLOCK,
	RV89DEC_BFRAME_FAST_DEBLOCK,
	RV89DEC_BFRAME_NO_DEBLOCK
} RV89Dec_BFrameDeblockingMode_t;

typedef struct
{
	BOOL is_valid; /* TRUE if valid segment */
	U32 offset; /* Segment Offset (in bytes) from
 start of compressed data */
} RV89Dec_Segment_Info;

typedef struct
{
	U8 *pStartPtr; /* Start address of input buffer containing compressed
 picture data (8 byte aligned) */
	U8 *pEndPtr; /* End address of input buffer */
	U32 PictureOffset; /* Compressed picture data offset from start of buffer */
	U32 PictureSize; /* Size of compressed picture */
	U32 NumSegments; /* Number of segments */
	RV89Dec_Segment_Info *pSegmentInfo;
} RV89Dec_InputBuffer_t;

typedef struct
{
	RV89Dec_LumaAddress_t pLuma; /* Address of the frame buffer containing the
 luma data of a picture */
	RV89Dec_ChromaAddress_t pChroma; /* Address of the frame buffer containing
 the chroma data of a picture */

} RV89Dec_OutputBuffer_t;

//#ifdef ENABLE_IPRED
typedef struct
{
	S16 QuantBuff[64 * 6 + 4]; /* Must be 8 byte aligned */
	U8 MBIntraTypes[16]; /* Must be 4 bytes aligned */
	U8 IsSubBlockEmpty[24]; /* Must be 4 byte aligned */
	U32 MBnum; /* U32 is required for alignment otherwaise U16 is sufficient */
	U32 Yoffset;
} RV89Dec_IntraMBInfo_t;
//#endif

typedef struct
{
	U32 MaxWidth;
	U32 MaxHeight;
	RV89Dec_fid_t StreamFormatIdentifier;
#if (RV89DEC_MME_VERSION > 11)
	RV89Dec_BFrameDeblockingMode_t BFrameDeblockingMode;
#endif
	BOOL isRV8;
	U32 NumRPRSizes;
	U32 RPRSize[18];
//#ifdef ENABLE_IPRED
	RV89Dec_IntraMBInfo_t *pIntraMBInfo;
//#endif
} RV89Dec_InitTransformerParam_t;

typedef struct
{
	U32 caps_len;
} RV89Dec_caps_len_t;

/* This structure is as big as returned via caps_len */
typedef struct
{
	U8 *caps;
} RV89Dec_caps_t;

typedef struct
{
	RV89Dec_InputBuffer_t InBuffer;
	RV89Dec_OutputBuffer_t Outbuffer; /* Output buffer in Omega-2 format */
	RV89Dec_OutputBuffer_t PrevMinusOneRefFrame;
	RV89Dec_OutputBuffer_t PrevRefFrame;
	RV89Dec_OutputBuffer_t CurrDecFrame; /* Decoded picture buffer in raster format */
//#ifdef ENABLE_DELTA_HW
	RV89Dec_OutputBuffer_t PrevMinusOneRefFrameOmega2;
	RV89Dec_OutputBuffer_t PrevRefFrameOmega2;
	RV89Dec_OutputBuffer_t CurrDecFrameOmega2;
//#endif
#ifdef SKIP_B_FRAME
	BOOL isSkipped;
#endif

} RV89Dec_TransformParams_t;

typedef struct
{
	U32 DecodeMBCycles;
	U32 DeblockCycles;
	U32 PostfilterCycles;
	U32 Deblock_PrePostCycles;
	U32 Smooth_PrePostCycles;
	U32 decMBheaderCycles;
	U32 decCoeffsCycles;
	U32 recMBCycles;
} RV89Dec_ProfilingFunctions_t;

typedef struct
{
	RV89Dec_PicType_t PicType;
	U32 OutputWidth;
	U32 OutputHeight;
	RV89Dec_ErrorCodes_t ErrorCode;
#ifdef RV89DEC_PROFILING_ENABLE
#ifdef FUNCTION_LEVEL_PROFILING
	RV89Dec_ProfilingFunctions_t ProfilingData;
#endif
	U32 Cycles;
	U32 Bundles;
	U32 ICacheMiss;
	U32 DCacheMiss;
	U32 NopBundles;
#endif
} RV89Dec_TransformStatusAdditionalInfo_t;

#ifdef __cplusplus
}
#endif

#endif /* RV89DECODE_INTERFACEH */

