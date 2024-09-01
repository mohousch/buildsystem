/*
** @file : MPEG2_VideoTransformerTypes.h
**
** @brief : MPEG2 Video Decoder specific types
**
** @par OWNER: Akash Goel / Akash Goel
**
** @author : Regis Sochard / Regis Sochard
**
** @par SCOPE:
**
** @date : April 20th 2004
**
** &copy; 2004 ST Microelectronics. All Rights Reserved.
*/

#ifndef __MPEG2_VIDEO_TRANSFORMER_TYPES_H
#define __MPEG2_VIDEO_TRANSFORMER_TYPES_H

#define MPEG2_MME_TRANSFORMER_NAME "MPEG2_TRANSFORMER"

/*
MPEG2_MME_VERSION:
Identifies the MME API version of the MPEG2 firmware.
If wants to build the firmware for old MME API version, change this string correspondingly.
*/
#ifndef MPEG2_MME_VERSION
#define MPEG2_MME_VERSION 43 /* Latest MME API version */
#endif

#if (MPEG2_MME_VERSION >= 11)
#define MME11
#endif

#if (MPEG2_MME_VERSION==46)
#define MPEG2_MME_API_VERSION "4.6"
#elif (MPEG2_MME_VERSION==45)
#define MPEG2_MME_API_VERSION "4.5"
#elif (MPEG2_MME_VERSION==44)
#define MPEG2_MME_API_VERSION "4.4"
#elif (MPEG2_MME_VERSION==43)
#define MPEG2_MME_API_VERSION "4.3"
#elif (MPEG2_MME_VERSION==42)
#define MPEG2_MME_API_VERSION "4.2"
#elif (MPEG2_MME_VERSION==32)
#define MPEG2_MME_API_VERSION "3.2"
#elif (MPEG2_MME_VERSION==30)
#define MPEG2_MME_API_VERSION "3.0"
#elif (MPEG2_MME_VERSION==20)
#define MPEG2_MME_API_VERSION "2.0"
#else
#define MPEG2_MME_API_VERSION "1.1"
#endif

#define MPEG2_Q_MATRIX_SIZE 64

#define MPEG2_DECODER_ID 0xCAFE
#define MPEG2_DECODER_BASE (MPEG2_DECODER_ID << 16)
/*
** Definition used to size the MPEG2_CommandStatus_t.CEHRegisters array
** where values of CEH registers will be stored
*/
#if (MPEG2_MME_VERSION >= 42)
#define MPEG2_NUMBER_OF_CEH_INTERVALS 32
#endif /* MPEG2_MME_VERSION >= 42 */

/*
** Definitions of the "mpeg_decoding_flags", according to the bit position, inside the 32 bits word
*/
#define MPEG_DECODING_FLAGS_TOP_FIELD_FIRST 0x0001 /* position at bit 1 : used to
 determine the type of picture */
#define MPEG_DECODING_FLAGS_FRAME_PRED_FRAME_DCT 0x0002 /* position at bit 2 : used for
 parsing progression purpose only */
#define MPEG_DECODING_FLAGS_CONCEALMENT_MOTION_VECTORS 0x0004 /* position at bit 3 : used for
 parsing progression purpose only */
#define MPEG_DECODING_FLAGS_Q_SCALE_TYPE 0x0008 /* position at bit 4 : used for the
 Inverse Quantisation process */
#define MPEG_DECODING_FLAGS_INTRA_VLC_FORMAT 0x0010 /* position at bit 5 : used for the
 selection of VLC tables when
 decode the DCT coefficients */
#define MPEG_DECODING_FLAGS_ALTERNATE_SCAN 0x0020 /* position at bit 6 : used for the
 Inverse Scan process */
#define MPEG_DECODING_FLAGS_DEBLOCKING_FILTER_ENABLE 0x0040 /* position at bit 7 : used to
 enable-disable the deblocking
 filter post-processing */

#define MPEG_DECODING_FLAGS_PROGRESSIVE_FRAME 0x0040 /* position at bit 7 used for post processing*/

/*
** MPEG2_AdditionalFlags_t :
** Identifies the different flags that will be passed to MPEG2 firmware
*/
typedef enum
{
	MPEG2_ADDITIONAL_FLAG_NONE = 0,
	MPEG2_ADDITIONAL_FLAG_DEBLOCKING_ENABLE = 1,
	MPEG2_ADDITIONAL_FLAG_DERINGING_ENABLE = 2,
	MPEG2_ADDITIONAL_FLAG_TRANSCODING_H264 = 4,
	MPEG2_ADDITIONAL_FLAG_CEH = 8,
	MPEG2_ADDITIONAL_FLAG_FIRST_FIELD = 16,
	MPEG2_ADDITIONAL_FLAG_SECOND_FIELD = 32,
	MPEG2_ADDITIONAL_FLAG_RASTER = 64, /* Output storage of Auxillary reconstruction in Raster format. */
	MPEG2_ADDITIONAL_FLAG_PP_DEBUG = 0x80000000
} MPEG2_AdditionalFlags_t;

/*
** MPEG2_HorizontalDeciFactor _t :
** Identifies the horizontal decimation factor
*/
typedef enum
{
	MPEG2_HDEC_1 = 0x00000000, /* no H resize */
	MPEG2_HDEC_2 = 0x00000001, /* H/2 resize */
	MPEG2_HDEC_4 = 0x00000002, /* H/4 resize */
	MPEG2_HDEC_RESERVED = 0x00000003, /* reserved H resize */
	MPEG2_HDEC_ADVANCED_2 = 0x00000101, /* Advanced H/2 resize using improved 8-tap filters */
	MPEG2_HDEC_ADVANCED_4 = 0x00000102 /* Advanced H/4 resize using improved 8-tap filters */

} MPEG2_HorizontalDeciFactor_t;

/*
** MPEG2_VerticalDeciFactor_t :
** Identifies the vertical decimation factor
*/
typedef enum
{
	MPEG2_VDEC_1 = 0x00000000, /* no V resize */
	MPEG2_VDEC_2_PROG = 0x00000004, /* V/2 , progressive resize */
	MPEG2_VDEC_2_INT = 0x00000008, /* V/2 , interlaced resize */
	/* removed as per the MME_API_DOC Version 3.1
	 MPEG2_VDEC_RESERVED = 0x00000012, */ /* reserved H resize */
	MPEG2_VDEC_ADVANCED_2_PROG = 0x00000204, /* Advanced V/2 , progressive resize */
	MPEG2_VDEC_ADVANCED_2_INT = 0x00000208 /* Advanced V/2 , interlaced resize */

} MPEG2_VerticalDeciFactor_t;

/*
** MPEG2_MainAuxEnable_t :
** Used for enabling Main/Aux outputs
*/
typedef enum
{
	MPEG2_AUXOUT_EN = 0x00000010, /* Enable decimated reconstruction */
	MPEG2_MAINOUT_EN = 0x00000020, /* Enable main reconstruction */
	MPEG2_AUX_MAIN_OUT_EN = 0x00000030 /* Enable both main & decimated reconstruction */
} MPEG2_MainAuxEnable_t;

/*
** MPEG2_PictureCodingType_t :
** Identifies the Picture Prediction : used of no, one or two reference pictures
*/
typedef enum
{
	MPEG2_FORBIDDEN_PICTURE = 0x00000000, /* forbidden coding type */
	MPEG2_INTRA_PICTURE = 0x00000001, /* intra (I) picture coding type */
	MPEG2_PREDICTIVE_PICTURE = 0x00000002, /* predictive (P) picture coding type */
	MPEG2_BIDIRECTIONAL_PICTURE = 0x00000003, /* bidirectional (B) picture coding type */
	MPEG2_DC_INTRA_PICTURE = 0x00000004, /* dc intra (D) picture coding type */
	MPEG2_RESERVED_1_PICTURE = 0x00000005, /* reserved coding type */
	MPEG2_RESERVED_2_PICTURE = 0x00000006, /* reserved coding type */
	MPEG2_RESERVED_3_PICTURE = 0x00000007 /* reserved coding type */
} MPEG2_PictureCodingType_t;

/*
** MPEG2_PictureStructure_t :
** Identifies the Picture Type
*/
typedef enum
{
	MPEG2_RESERVED_TYPE = 0x00000000, /* identifies a reserved structure type */
	MPEG2_TOP_FIELD_TYPE = 0x00000001, /* identifies a top field picture type */
	MPEG2_BOTTOM_FIELD_TYPE = 0x00000002, /* identifies a bottom field picture type */
	MPEG2_FRAME_TYPE = 0x00000003 /* identifies a frame picture type */
} MPEG2_PictureStructure_t;

/*
** MPEG2_DecodingMode_t :
** Identifies the decoding mode.
*/
typedef enum
{
	MPEG2_NORMAL_DECODE = 0,
	MPEG2_NORMAL_DECODE_WITHOUT_ERROR_RECOVERY = 1,
	MPEG2_DOWNGRADED_DECODE_LEVEL1 = 2,
	MPEG2_DOWNGRADED_DECODE_LEVEL2 = 4
					 /* TBC */
} MPEG2_DecodingMode_t;

/*
** MPEG2_LumaAddress_t :
** Defines the address type for the MPEG2 decoded Luma data's
*/
typedef U32 *MPEG2_LumaAddress_t;

/*
** MPEG2_ChromaAddress_t :
** Defines the address type for the MPEG2 decoded Chroma data's
*/
typedef U32 *MPEG2_ChromaAddress_t;

/*
** MPEG2_CompressedData_t :
** Defines the address type for the MPEG2 compressed data
*/
typedef U32 *MPEG2_CompressedData_t;

typedef void *MPEG2_GenericParam_t;

typedef struct
{
	U32 PicStdConfig;
	U32 PictureStructure;
	U32 PictureWidthWord;
	U32 PictureHeightWord;
	U32 Decimation ;
} MPEG2_DebugPicture_t;

typedef struct
{
	U32 SMB_WORD_0 ;
	U32 SMB_WORD_1 ;
	U32 MbType;
	U32 LmbWord2;
	U32 EnableWeightedPrediction;
	U32 InterRefIndex0;
	U32 InterRefIndex1;
	U32 MvCount;
	U32 MotionVectors0;
	U32 MotionVectors1;
	U32 MotionVectors2;
	U32 MotionVectors3;
	U32 ErrorFlag;
	U32 BhStatus;
} MPEG2_DebugMb_t;

typedef struct
{
	U32 MBInfo; /* MPEG-2 decoded macroblock parameters packed in a single word */
	S32 MV[2][2]; /* MV[TOP(FRAME)/BOTTOM][foward/backward] */
} XcodeMPEG2_MBDescr_t;

/*done on 26th Sept as per MME API 3.0 Version */
typedef enum
{
	MPEG2_LOAD_INTRA_QUANTIZER_MATRIX_FLAG = 1,
	MPEG2_LOAD_NON_INTRA_QUANTIZER_MATRIX_FLAG = 2
} MPEG2_DefaultMatrixFlags_t ;

/*
** MPEG2_MBDescr_t :
** Defines the address type for the MPEG2 macro block description
*/
typedef void *MPEG2_MBDescr_t;

/*
** MPEG2_DecodedBufferAddress_t :
** Defines the addresses where the decoded pictures will be stored
*/
typedef struct
{
	U32 StructSize; /* Size of the structure in bytes */
	MPEG2_LumaAddress_t DecodedLuma_p; /* address of the decoded Luma's block */
	MPEG2_ChromaAddress_t DecodedChroma_p; /* address of the decoded Chroma's block */
	U32 DecodedTemporalReferenceValue; /* temporal_reference value of the decoded
 (current) picture */
	MPEG2_LumaAddress_t DecimatedLuma_p; /* address of the decimated Luma's block */
	MPEG2_ChromaAddress_t DecimatedChroma_p; /* address of the decimated Chroma's block */
	MPEG2_MBDescr_t MBDescr_p; /* address of the buffer where to store data
 related to every MBs of the picture */
} MPEG2_DecodedBufferAddress_t;

typedef struct
{
	BOOL IPF_DeringingLumaOnly;
	BOOL IPF_DeblockingLumaOnly;
	U32 IPF_MONOTH;
	U32 IPF_ADRTH;
	U32 IPF_ADRC;
	U32 IPF_ADRID;
	U32 IPF_ADRTQ_INTRA;
	U32 IPF_ADRTQ_INTER;
} MPEG2_PostProDebug_t;

/*
** MPEG2_RefPicListAddress_t :
** Defines the addresses where the two reference pictures will be stored
*/
typedef struct
{
	U32 StructSize; /* Size of the structure in bytes */
	MPEG2_LumaAddress_t BackwardReferenceLuma_p; /* address of the backward referenced Luma */
	MPEG2_ChromaAddress_t BackwardReferenceChroma_p; /* address of the backward referenced Chroma */
	U32 BackwardTemporalReferenceValue; /* temporal_reference value of the backward
 reference picture */
	MPEG2_LumaAddress_t ForwardReferenceLuma_p; /* address of the forward referenced Luma */
	MPEG2_ChromaAddress_t ForwardReferenceChroma_p; /* address of the forward referenced Chroma */
	U32 ForwardTemporalReferenceValue; /* temporal_reference value of the forward
 reference picture */
} MPEG2_RefPicListAddress_t;

/*
** MPEG2_ChromaFormat_t :
** Identifies the Type of Luma and Chroma Pixels of the Decoded Picture
*/
typedef enum
{
	MPEG2_CHROMA_RESERED = 0x00000000, /* unsupported chroma type */
	MPEG2_CHROMA_4_2_0 = 0x00000001, /* chroma type 4:2:0 */
	MPEG2_CHROMA_4_2_2 = 0x00000002, /* chroma type 4:2:2 */
	MPEG2_CHROMA_4_4_4 = 0x00000003 /* chroma type 4:4:4 */
} MPEG2_ChromaFormat_t;

/*
** MPEG2_IntraDcPrecision_t :
** Identifies the Intra DC Precision
*/
typedef enum
{
	MPEG2_INTRA_DC_PRECISION_8_BITS = 0x00000000, /* 8 bits Intra DC Precision */
	MPEG2_INTRA_DC_PRECISION_9_BITS = 0x00000001, /* 9 bits Intra DC Precision */
	MPEG2_INTRA_DC_PRECISION_10_BITS = 0x00000002, /* 10 bits Intra DC Precision */
	MPEG2_INTRA_DC_PRECISION_11_BITS = 0x00000003 /* 11 bits Intra DC Precision */
} MPEG2_IntraDCPrecision_t;

/*
** MPEG2_SetGlobalParamSequence_t :
** MPEG2 parameters required by the firmware for performing a picture decode.
** These parameters comes from the "video_sequence" host processing.
**
** Naming convention :
** -------------------
** MPEG2_SetGlobalParamSequence_t
** | | |
** | | |-> Only the sequence data required by the firmware are
** | | concerned by this structure.
** | |
** | |-----> The command parameters concern the sequence buffers update
** |
** |
** |-------------> This structure defines the command parameter required when
** using the MME_SendCommand() function.
*/
typedef struct
{
	U32 StructSize; /* Size of the structure in bytes */
	BOOL MPEGStreamTypeFlag; /* Type of the bitstream MP1/MP2
 = 0 for MPEG1 coded stream,
 = 1 for MPEG2 coded stream */
	U32 horizontal_size; /* horizontal size of the video picture : based
 on the two elements "horizontal_size_value"
 and "horizontal_size_extension" */
	U32 vertical_size; /* vertical size of the video picture : based
 on the two elements "vertical_size_value"
 and "vertical_size_extension" */
	U32 progressive_sequence; /* used to compute the mb_height */
	MPEG2_ChromaFormat_t chroma_format; /* used for parsing progression purpose only */

#if(MPEG2_MME_VERSION >= 30)
	U32 MatrixFlags; /* set of MPEG2_LoadMatrixFlags_t specifying the origin
 of the quantizer matrix ,Added as per 3.0 MME API version */
#endif

	U8 intra_quantiser_matrix[MPEG2_Q_MATRIX_SIZE]; /* used by the HW accelerator for Inverse */
	U8 non_intra_quantiser_matrix[MPEG2_Q_MATRIX_SIZE]; /* used by the HW accelerator for Inverse
 Quantisation process : array of 64 bytes */
	U8 chroma_intra_quantiser_matrix[MPEG2_Q_MATRIX_SIZE]; /* used by the HW accelerator for Inverse
 Quantisation process : array of 64 bytes */
	U8 chroma_non_intra_quantiser_matrix[MPEG2_Q_MATRIX_SIZE]; /* used by the HW accelerators for Inverse
 Quantisation process : array of 64 bytes */
} MPEG2_SetGlobalParamSequence_t;

/*
** MPEG2_ParamPicture_t :
** MPEG2 parameters required by the firmware for performing a picture decode.
** These parameters comes from the "picture_header" and "picture_coding_extension" host processing.
**
** Naming convention :
** -------------------
** MPEG2_ParamPicture_t
** | |
** | |-> Only the picture data required by the firmware are
** | concerned by this structure.
** |
** |-----> The command parameters concern the picture buffers update
*/
typedef struct
{
	U32 StructSize; /* Size of the structure in bytes */
	MPEG2_PictureCodingType_t picture_coding_type; /* prediction type of the picture */
	U32 forward_horizontal_f_code; /* used for Motion Vector decoding and
 for parsing progression purpose */
	U32 forward_vertical_f_code; /* used for Motion Vector decoding and
 for parsing progression purpose */
	U32 backward_horizontal_f_code; /* used for Motion Vector decoding and
 for parsing progression purpose */
	U32 backward_vertical_f_code; /* used for Motion Vector decoding and
 for parsing progression purpose */
	MPEG2_IntraDCPrecision_t intra_dc_precision; /* used for the Inverse Quantisation process */
	MPEG2_PictureStructure_t picture_structure; /* used to determine the type of picture */
	U32 mpeg_decoding_flags; /* group of the six boolean values
 "top_field_first", "frame_pred_frame_dct",
 "concealment_motion_vectors", "q_scale_type",
 "intra_vlc_format" and " alternate_scan " :
 see the definitions below */
} MPEG2_ParamPicture_t;

/*
** MPEG2_TransformParam_t :
** MPEG2 specific parameters required for feeding the firmware on a picture decoding
** command order.
**
** Naming convention :
** -------------------
** MPEG2_TransformParam_t
** | |
** | |-> Only the data required by the firmware are
** | concerned by this structure.
** |
** |-------------> structure dealing with the parameters of the TRANSFORM
** command when using the MME_SendCommand()
**
*/
typedef struct
{
	U32 StructSize; /* Size of the structure in bytes */
	MPEG2_CompressedData_t PictureStartAddrCompressedBuffer_p; /* start address of the compressed
 frame in the input buffer */
	MPEG2_CompressedData_t PictureStopAddrCompressedBuffer_p; /* stop address of the compressed
 frame in the input buffer */
	MPEG2_DecodedBufferAddress_t DecodedBufferAddress; /* Address where the decoded picture will be stored
 (Luma and Chroma samples of the picture) */
	MPEG2_RefPicListAddress_t RefPicListAddress; /* Address where the backward and forward reference
 pictures will be stored (Luma and Chroma
 samples) */
	MPEG2_MainAuxEnable_t MainAuxEnable; /* enable Main and/or Aux outputs */
	MPEG2_HorizontalDeciFactor_t HorizontalDecimationFactor; /* Horizontal decimation factor */
	MPEG2_VerticalDeciFactor_t VerticalDecimationFactor; /* Vertical decimation factor */
	MPEG2_DecodingMode_t DecodingMode; /* Specifies the decoding mode
 (normal or downgraded) */
#if (MPEG2_MME_VERSION < 20)
	U32 AdditionalFlags; /* Additonal Flags for future uses */
#else
	MPEG2_AdditionalFlags_t AdditionalFlags; /* Additonal Flags for deblocking/deringing */
#endif

	MPEG2_ParamPicture_t PictureParameters; /* Picture parameters */
} MPEG2_TransformParam_t;

/*
** MPEG2_InitTransformerParam_t:
** Parameters required for a firmware transformer initialization.
**
** Naming convention :
** -------------------
** MPEG2_InitTransformerParam_t
** |
** |-------------> This structure defines the parameter required when
** using the MME_InitTransformer() function.
*/
typedef struct
{
	U32 InputBufferBegin; /* Begin address of the input circular buffer */
	U32 InputBufferEnd; /* End address of the input circular buffer */
} MPEG2_InitTransformerParam_t;

/*
** MPEG2_TransformerCapability_t:
** Parameters required for a firmware transformer initialization.
**
** Naming convention :
** -------------------
** MPEG2_TransformerCapability_t
** |
** |-------------> This structure defines generic parameter required by the
** MME_TransformerCapability_t in the MME API strucure definition.
*/
typedef struct
{
	U32 StructSize; /* Size of the structure in bytes */
	U32 MPEG1Capability; /* Define if the decoder is MPEG1 capable */
	MPEG2_ChromaFormat_t SupportedFrameChromaFormat; /* Supported Chroma Format of the
 Frame Picture */
	MPEG2_ChromaFormat_t SupportedFieldChromaFormat; /* Supported Chroma Format of the
 Field Picture */
	U32 MaximumFieldDecodingLatency90KHz; /* Maximum field decoding latency expressed
 in 90kHz unit */
} MPEG2_TransformerCapability_t;

/*
** The picture to decode is divided into a maximum of
** MPEG2_STATUS_PARTITION * MPEG2_STATUS_PARTITION areas in order to locate
** the decoding errors.
*/
#define MPEG2_STATUS_PARTITION 6

/*
** MPEG2_CommandStatus_t:
** Defines the MPEG2 decoding command status.
** MPEG2specific parameters required for performing a picture decode.
** When performing a Picture decode command (MME_TRANSFORM) on the Delta-Mu,
** the video driver will use this structure for MME_CommandStatus_t.AdditionalInfo_p
**
** Naming convention :
** -------------------
** MPEG2_CommandStatus_t
** |
** |-----------> This structure defines generic parameter required by the
** MME_CommandStatus_t in the MME API strucure definition.
*/
typedef struct
{
	U32 StructSize; /* Size of the structure in bytes */
	U8 Status[MPEG2_STATUS_PARTITION][MPEG2_STATUS_PARTITION];
	ST_ErrorCode_t ErrorCode;

#if (MPEG2_MME_VERSION>=32)
	U32 DecodeTimeInMicros;
#endif /* MPEG2_MME_VERSION>=32 */

#if (MPEG2_MME_VERSION>=42)
	/* CEHRegisters[] is an array where values of the
	 Contrast Enhancement Histogram (CEH) registers will be stored. CEHRegisters[0]
	 will correspond to register MBE_CEH_0_7, CEHRegisters[1] will correspond
	 to register MBE_CEH_8_15., CEHRegisters[2], correspond to register MBE_CEH_16_23….
	 Note that elements of this array will be updated only if
	 MPEG2_TransformParam_t.AdditionalFlags will have the flag
	 MPEG2_ADDITIONAL_FLAG_CEH set. They will remain unchanged otherwise. */
	U32 CEHRegisters[MPEG2_NUMBER_OF_CEH_INTERVALS];
#endif /* MPEG2_MME_VERSION >= 42 */

#ifdef MPEG2_PIC_SEQ_DEBUG_INFO
	MPEG2_GenericParam_t *DebugBufferPtr;
#endif

#ifdef PROFILE_MPEG2_FIRMWARE
	U32 pm_cycles;
	U32 pm_dmiss;
	U32 pm_imiss;
	U32 pm_bundles;
	U32 pm_nop_bundles;
#endif

} MPEG2_CommandStatus_t;

/*
** MPEG2_DecodingError_t :
** Errors returned by the MPEG2 decoder in MME_CommandStatus_t.Error when
** CmdCode is equal to MME_TRANSFORM.
** These errors are bitfields, and several bits can be raised at the
** same time.
*/
typedef enum
{
	/* The firmware was successful. */
	MPEG2_DECODER_NO_ERROR = (MPEG2_DECODER_BASE + 0),

	/* The firmware decoded too much MBs:
	 - the MPEG2_CommandStatus_t.Status doesn't locate these erroneous MBs
	 because the firmware can't know where are these extra MBs.
	 - MME_CommandStatus_t.Error can also contain MPEG2_DECODER_ERROR_RECOVERED.
	*/
	MPEG2_DECODER_ERROR_MB_OVERFLOW = (MPEG2_DECODER_BASE + 1),

	/* The firmware encountered error(s) that were recovered:
	 - MPEG2_CommandStatus_t.Status locates the erroneous MBs.
	 - MME_CommandStatus_t.Error can also contain MPEG2_DECODER_ERROR_MB_OVERFLOW.
	*/
	MPEG2_DECODER_ERROR_RECOVERED = (MPEG2_DECODER_BASE + 2),

	/* The firmware encountered an error that can't be recovered:
	 - MPEG2_CommandStatus_t.Status has no meaning.
	 - MME_CommandStatus_t.Error is equal to MPEG2_DECODER_ERROR_NOT_RECOVERED.
	*/
	MPEG2_DECODER_ERROR_NOT_RECOVERED = (MPEG2_DECODER_BASE + 4),
	/* The firmware task is hanged and doesn't get back to watchdog task even after maximum time
	 alloted has lapsed:
	- MPEG2_CommandStatus_t.Status has no meaning.
	- MME_CommandStatus_t.Error is equal to MPEG2_DECODER_ERROR_TASK_TIMEOUT.
	*/
	MPEG2_DECODER_ERROR_TASK_TIMEOUT = (MPEG2_DECODER_BASE + 8)
} MPEG2_DecodingError_t;

#endif /* #ifndef __MPEG2_VIDEO_TRANSFORMER_TYPES_H */

