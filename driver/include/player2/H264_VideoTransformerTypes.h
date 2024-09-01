/*
** @file : H264_VideoTransformerTypes.h
**
** @brief : H264 Video Decoder specific types
**
** @par OWNER: Jerome Audu / Vincent Capony
**
** @author : Jerome Audu / Vincent Capony
**
** @par SCOPE:
**
** @date : 2005-09-12
**
** &copy; 2003 ST Microelectronics. All Rights Reserved.
*/

#ifndef __H264_TRANSFORMERTYPES_H
#define __H264_TRANSFORMERTYPES_H

/*
** Identifies the name of the transformer
*/
#define H264_MME_TRANSFORMER_NAME "H264_TRANSFORMER"

/*
Definition used to size some arrays used in this API
*/
#define H264_MAX_NUMBER_OF_REFERENCES_FRAME 16

/*
Definition used to size some arrays used in this API
*/

#define H264_MAX_NUMBER_OF_REFERENCES_FIELD 32

/*
Definition used to size the H264_CommandStatus_fmw_t::CEHRegisters array
where values of CEH registers will be stored
*/
#define H264_NUMBER_OF_CEH_INTERVALS 32

#ifdef H264DEC_MB_DEBUG_INFO
#define H264DEC_PIC_SEQ_DEBUG_INFO
#define H264DEC_SLICE_DEBUG_INFO
#endif

#ifdef H264DEC_SLICE_DEBUG_INFO
#define H264DEC_PIC_SEQ_DEBUG_INFO
#endif

/*
** Identifies the version of the MME API for both preprocessor and firmware
*/
#if (H264_MME_VERSION==64)
#define H264_MME_API_VERSION "6.4"
#elif (H264_MME_VERSION==63)
#define H264_MME_API_VERSION "6.3"
#elif (H264_MME_VERSION==62)
#define H264_MME_API_VERSION "6.2"
#elif (H264_MME_VERSION==61)
#define H264_MME_API_VERSION "6.1"
#elif (H264_MME_VERSION==60)
#define H264_MME_API_VERSION "6.0"
#elif (H264_MME_VERSION==59)
#define H264_MME_API_VERSION "5.9"
#elif (H264_MME_VERSION==58)
#define H264_MME_API_VERSION "5.8"
#elif (H264_MME_VERSION==57)
#define H264_MME_API_VERSION "5.7"
#elif (H264_MME_VERSION==56)
#define H264_MME_API_VERSION "5.6"
#elif (H264_MME_VERSION==55)
#define H264_MME_API_VERSION "5.5"
#elif (H264_MME_VERSION==54)
#define H264_MME_API_VERSION "5.4"
#elif (H264_MME_VERSION==53)
#define H264_MME_API_VERSION "5.3"
#elif (H264_MME_VERSION==52)
#define H264_MME_API_VERSION "5.2"
#elif (H264_MME_VERSION==51)
#define H264_MME_API_VERSION "5.1"
#elif (H264_MME_VERSION==50)
#define H264_MME_API_VERSION "5.0"
#elif (H264_MME_VERSION==42)
#define H264_MME_API_VERSION "4.2"
#elif (H264_MME_VERSION==41)
#define H264_MME_API_VERSION "4.1"
#elif (H264_MME_VERSION==40)
#define H264_MME_API_VERSION "4.0"
#elif (H264_MME_VERSION==31)
#define H264_MME_API_VERSION "3.1"
#elif (H264_MME_VERSION==30)
#define H264_MME_API_VERSION "3.0"
#elif (H264_MME_VERSION==20)
#define H264_MME_API_VERSION "2.0"
#else
#define H264_MME_API_VERSION "1.0"
#endif

/*
** H264_HorizontalDeciFactor _t :
** Identifies the horizontal decimation factor
*/
typedef enum
{
	HDEC_1 = 0x00000000, /* no resize */
	HDEC_2 = 0x00000001, /* H/2 resize */
	HDEC_4 = 0x00000002, /* H/4 resize */
	HDEC_8 = 0x00000003, /* H/8 resize for FGT */
	HDEC_ADVANCED_2 = 0x00000101, /* Advanced H/2 resize using improved 8-tap filters */
	HDEC_ADVANCED_4 = 0x00000102 /* Advanced H/2 resize using improved 8-tap filters */

} H264_HorizontalDeciFactor_t;

/*
** H264_VerticalDeciFactor _t :
** Identifies the vertical decimation factor
*/
typedef enum
{
	VDEC_1 = 0x00000000, /* no resize */
	VDEC_2_PROG = 0x00000004, /* V/2 , progressive resize */
	VDEC_2_INT = 0x00000008, /* V/2 , interlaced resize */
	VDEC_ADVANCED_2_PROG = 0x00000204, /* Advanced V/2, Progressive resize */
	VDEC_ADVANCED_2_INT = 0x00000208 /* Advanced V/2, Interlace resize */
} H264_VerticalDeciFactor_t;

/*
** H264_MainAuxEnable_t :
** Used for enabling Main/Aux outputs
*/
typedef enum
{
	AUXOUT_EN = 0x00000010, /* enable decimated reconstruction */
	MAINOUT_EN = 0x00000020, /* enable main reconstruction */
	AUX_MAIN_OUT_EN = 0x00000030 /* enable both main & decimated reconstruction */
} H264_MainAuxEnable_t;

/*
** H264_DecodingMode_t :
** Identifies the decoding mode.
*/
typedef enum
{
	H264_NORMAL_DECODE = 0,
	H264_NORMAL_DECODE_WITHOUT_ERROR_RECOVERY = 1,
	H264_DOWNGRADED_DECODE_LEVEL1 = 2,
	H264_DOWNGRADED_DECODE_LEVEL2 = 4,
	H264_ERC_MAX_DECODE = 8
} H264_DecodingMode_t;

typedef enum
{
	H264_ADDITIONAL_FLAG_NONE = 0,
	H264_ADDITIONAL_FLAG_FGT = 1, /* request firwmare to generate suplementary data
 Later used for Film Grain Technology support */
	H264_ADDITIONAL_FLAG_CEH = 2, /* Request firmware to return values of the CEH registers */
	H264_ADDITIONAL_FLAG_FILL_BUFFER = 4, /* Fill output buffer with a uniform color */
	H264_ADDITIONAL_FLAG_GNBVD67106 = 8, /* To fix GNBVD67106 Casema bug. */
	H264_ADDITIONAL_FLAG_RASTER = 64 /* Output storage of Auxillary reconstruction in Raster format. */
} H264_AdditionalFlags_t;

/*
** H264_DecoderProfileComformance_t :
** To setup the decoder in the correct mode to decode a stream according to the
characteristics of the incoming bitstream.
In the general case it will be equal to the profil_idc extracted from the bistream. Depending on the
values of constraint_setX_flags (with X=0..3) it can be different and modified accordingly in order
for the decoder to know how to parse/interpret the bitstream syntax and semantic.
*/
typedef enum
{
	H264_BASELINE_PROFILE = 66,
	H264_MAIN_PROFILE = 77,
	H264_HIGH_PROFILE = 100
} H264_DecoderProfileComformance_t;

/*
** H264_ReferenceType_t :
** Identifies the type of reference picture
*/
typedef enum
{
	H264_SHORT_TERM_REFERENCE, /* Identifies a short term reference picture */
	H264_LONG_TERM_REFERENCE, /* Identifies a long term reference picture */
	H264_UNUSED_FOR_REFERENCE /* Identifies picture unused for reference */
} H264_ReferenceType_t;

/*
** H264_PictureDescriptorType_t :
** Identifies the type of the picture descriptor
** Information computed:
** EITHER from the following stream info:
** - frame_mbs_only_flag (SPS)
** - mb_adaptive_frame_field_flag (SPS)
** - field_pic_flag (Slice)
** - bottom_field_flag (Slice)
** OR: artificially created by the host
*/
typedef enum
{
	H264_PIC_DESCRIPTOR_FRAME, /* Identifies a frame picture descriptor */
	H264_PIC_DESCRIPTOR_AFRAME, /* Identifies an mbaff frame picture descriptor */
	H264_PIC_DESCRIPTOR_FIELD_TOP, /* Identifies a field top picture descriptor */
	H264_PIC_DESCRIPTOR_FIELD_BOTTOM /* Identifies a field bottom picture descriptor */
} H264_PictureDescriptorType_t;

/*
** H264_LumaAddress_t :
** Defines the address type for the H264 decoded Luma data's
*/
typedef U32 *H264_LumaAddress_t;

/*
** H264_ChromaAddress_t :
** Defines the address type for the H264 decoded Chroma data's
*/
typedef U32 *H264_ChromaAddress_t;

/*
** H264_MBStructureAddress_t :
** Defines the address type for the H264 macro block structure data's
*/
typedef U32 *H264_MBStructureAddress_t;

/*
** H264_BitsField32_t :
** Defines a bits field over 32 bits
*/
typedef U32 H264_BitsField32_t;

/*
** H264_RefPictList_t :
** Defines a reference picture list.
*/

typedef struct
{
	U32 RefPiclistSize; /* Number of elements in the RefPicList
 */
	/* Array of indices in for a
	 H264_PictureDescritor_t array. It
	 indicates the reference pictures to
	 be used for the incoming decodes.
	 see section
	 "Initialisation process for reference picture
	 lists" of the H264 specifications */
	U32 RefPicList[H264_MAX_NUMBER_OF_REFERENCES_FIELD];
} H264_RefPictList_t;

/*
** H264_HostData_t :
** Defines the data resulting from the slice header 0 host processing. The target consumer is the
** firmware.
*/
typedef struct
{
	S32 PictureNumber; /* Defines the picture number. Depends on the type of
 the picture, this field may be set to a PicNum or
 or to the LonTermPicNum. (see the section "Decoding
 process for picture numbers" in the H264
 specifications.
 */

	S32 PicOrderCntTop ; /* [-(2^31) , +(2^31-1)] Picture order count of the
 Top field. (no meaning if the current picture is a
 bottom field).
 This is the result of the <<Decoding process for
 picture order count>> process
 */
	S32 PicOrderCntBot ; /* [-(2^31) , +(2^31-1)] Picture order count of the
 Bottom field. (no meaning if the current picture is a
 top field).
 This is the result of the <<Decoding process for
 picture order count>> process
 */
	S32 PicOrderCnt ; /* [-(2^31) , +(2^31-1)] Picture order count of the
 current picture. (if curent picture is frame,
 PicOrderCnt = min (PicOrderCntTop, PicOrderCntBot).
 This is the result of the <<Decoding process for
 picture order count>> process
 */
	H264_PictureDescriptorType_t DescriptorType; /* picture descriptor */

	H264_ReferenceType_t ReferenceType; /* The type of reference */
	U32 IdrFlag; /*
 1 => The picture is an IDR,
 O => The picture is not an IDR,
 */
	U32 NonExistingFlag; /*
 1 => The frame is a non-existing frame
 0 => The frame is an existing frame
 */
} H264_HostData_t;

/*
** H264_DecodedBufferAddress_t :
** Defines the addresses where the decoded picture/additional info related to the MB
** structures will be stored
*/
typedef struct
{
	H264_LumaAddress_t Luma_p; /* address for the decoded Luma block */
	H264_ChromaAddress_t Chroma_p; /* address for the decoded Chroma's block */
	H264_LumaAddress_t LumaDecimated_p; /* address for the decimated Luma block */
	H264_ChromaAddress_t ChromaDecimated_p; /* address for the decimated Chroma's block */
	H264_MBStructureAddress_t MBStruct_p; /* address for MB structure data's */
} H264_DecodedBufferAddress_t;

/*
** H264_PictureDescriptor_t :
** picture descriptor
*/
typedef struct
{
	H264_HostData_t HostData; /* data resulting from the Slice header 0
 host processing.
 */
	U32 FrameIndex; /* Index in a H264_ReferenceFrameAddress_t array
 */
} H264_PictureDescriptor_t;

/*
** H264_ReferenceFrameAddress_t :
** reference frame addresses.
*/
typedef struct
{
	H264_DecodedBufferAddress_t DecodedBufferAddress; /* Address where the decoded picture will be stored
 (Luma and Chroma samples and information related
 to the MB decription
 */
} H264_ReferenceFrameAddress_t;

/*
** H264_RefPictListAddress_t :
** Defines the addresses of the reference picture list
*/
typedef struct
{

	/* The 2 following fields are common for the initial_P_list0_p, initial_B_list0_p and
	 initial_B_list1_p
	*/
	H264_PictureDescriptor_t PictureDescriptor[H264_MAX_NUMBER_OF_REFERENCES_FIELD]; /* Array of picture descriptor */

	H264_BitsField32_t ReferenceDescriptorsBitsField; /* Bits field (32bits) that indicates
 whether a reference frame is coded
 frame (bit = 0) or fields (bits = 1).
 A one by one correspondance exists
 between the bits of
 "ReferenceDescriptorsBitsField" and
 the elements listed by the next
 field (ReferenceFrameAddress_p), with
 the less significant bit
 corresponding to the first
 element in "ReferenceFrameAddress_p"
 */
	/* Array of addresses of the reference frame
	 used for the current decode.
	 These addresses corresponds to the addresses
	 that are transmitted to the hardware IP for
	 setting its the reference frames.
	 Array with a fixed size (16 elements).
	*/
	H264_ReferenceFrameAddress_t ReferenceFrameAddress[H264_MAX_NUMBER_OF_REFERENCES_FRAME];

	H264_RefPictList_t InitialPList0; /* Initial reference idx list0 used for
 decoding a P slice
 */
	H264_RefPictList_t InitialBList0; /* Initial reference idx list0 used for
 decoding a B slice
 */
	H264_RefPictList_t InitialBList1; /* Initial reference idx list1 used for
 decoding a B slice
 */
} H264_RefPictListAddress_t;

/*
** H264_ScalingList_t :
** Structure used to defines scaling matrices. The management of the lists is under the
 responsibility of the host.
 The elements of the lists are the result of the complete processing relative to the management of
 scaling lists as described in the H264 specification. In particular inference rules regarding
 presence/non presence of the different syntax elements inside the bitstream either at PPS or SPS level
 shall be fullfilled.
 The firmware will assume that when sending its first command the Host will force a first update by setting the
 ScalingListUpdated to 1.
 For main profile stream, the lists corresponds to Flat_4x4[] and Flat_8x8[] lists as they
 are defined in H264 specification
*/
typedef struct
{
	U32 ScalingListUpdated; /* [0, 1] ScalingListUpdated equals 1 specifies that the
 scaling list have been updated since last sending.
 ScalingListUpdated equals 1 specifies that the scaling
 lists remain unchanged since last sendingScalingList[][]
 */

	U32 FirstSixScalingList[6][16];
	U32 NextTwoScalingList[2][64];

} H264_ScalingList_t;

/*
** H264_SetGlobalParamSPS_t :
** H264 SPS parameters required by the firmware for performing a picture decode.
** These parameters comes from the SPS host processing.
**
** Naming convention:
** -----------------
** H264_SetGlobalParamSPS_t
** | | |
** | | |-> Only the SPS data required by the firmware are
** | | concerned by this structure.
** | |
** | |-----> The command parameters concern the SPS buffers update
** |
** |
** |-------------> This structure defines the command parameter required when
** using the MME_SendCommand() function.
*/
typedef struct
{
	H264_DecoderProfileComformance_t DecoderProfileConformance; /* To setup the conformance profile of
 the decoder for the decoding of the bitstream
 */
	U32 level_idc ; /* [10..51] Level of the stream.Used to know the maximum
 ** number of MV for each MB.
 */

	U32 log2_max_frame_num_minus4 ; /* [0..12] used for parsing progression purpose only
 */

	U32 pic_order_cnt_type; /* [0..2] used for parsing progression purpose only
 */

	U32 log2_max_pic_order_cnt_lsb_minus4 ; /* [0..12] used for parsing progression purpose only
 */

	U32 delta_pic_order_always_zero_flag; /* [0..1] used for parsing progression purpose only
 */

	U32 pic_width_in_mbs_minus1 ; /* [0..255] used to compute the neighbouring MBs.
 */

	U32 pic_height_in_map_units_minus1; /* [0..255] used to compute the max MB address, which is
 ** used to check if a MB address is valid.
 */

	U32 frame_mbs_only_flag ; /* [0..1] Indicates whether all pictures are coded
 frame. Used to decode the slice layer.
 */

	U32 mb_adaptive_frame_field_flag; /* [0..1] Indicates whether switching between frame and
 field decoding mode at MB layer is enabled.
 Must be set to 0 when not coded in the
 bitstream.Used to compute the neighbouring
 MBs.
 */
	U32 direct_8x8_inference_flag ; /* [0..1] Specifies the method used in the decoding
 process to determine the prediction values
 of the co-located 4x4 sub-macroblock
 partitions
 */
	U32 chroma_format_idc; /* [0..1] for High Profile. Specifies the chroma
 sampling relative to the luma sampling
 Shall be set to 1 when decoding a "main profile"
 stream
 */
} H264_SetGlobalParamSPS_t ;

/*
** H264_SetGlobalParamPPS_t :
** H264 PPS parameters required by the firmware for performing a picture decode.
** These parameter results from the PPS host processing.
**
** Naming convention:
** -----------------
** H264_SetGlobalParamPPS_t
** | | |
** | | |-> Only the PPS data required by the firmware are
** | | concerned by this structure.
** | |
** | |-----> The command parameters concern the PPS buffers update
** |
** |
** |-------------> This structure defines the command parameter required when
** using the MME_SendCommand() function.
**
*/
typedef struct
{

	U32 entropy_coding_mode_flag ; /* [0..1] Used to decode the slice layer, the MB
 layer, the residual data (each time a syntax
 element has two formats seperated by a vertical
 bar; for rbsp_slice_trailing_bits(); .).
 */

	U32 pic_order_present_flag ; /* [0..1] Used to decode the slice layer.
 */

	U32 num_ref_idx_l0_active_minus1 ; /* [0..31] Specifies the maximum reference index
 minus 1 in reference picture list 0 that are used
 to decode the picture. Can be overridden at slice
 layer.Used to decode the slice layer
 (pred_weight_table(), mb_pred())
 */

	U32 num_ref_idx_l1_active_minus1 ; /* [0..31] Specifies the maximum reference index
 minus 1 in reference picture list 1 that are used
 to decode the picture. Can be overridden at slice
 layer.Used to decode the slice layer
 (pred_weight_table(), mb_pred())
 */

	U32 weighted_pred_flag ; /* [0..1] Indicates whether weighted prediction is
 applied to P slices.Used to decode the slice
 layer, to compute the MV.
 */

	U32 weighted_bipred_idc ; /* [0..2] If 0, indicates that weighted prediction
 is not applied to B slices.If 1, indicates that
 explicit weighted prediction is applied to B
 slices.If 2, indicates that implicit weighted
 prediction is applied to B slices.Used to decode
 the slice layer, to compute the MV.
 */

	S32 pic_init_qp_minus26 ; /* [-26..25] Specifies the initial value minus 26
 of QPY for each slice.Used to compute QPY which
 is given to the HW MB engine.
 */

	S32 chroma_qp_index_offset ; /* [-12..12] Specifies the offset that shall be
 added to QPY and QSY for addressing the table of
 QPC values.Given to the HW MB engine.
 */

	U32 deblocking_filter_control_present_flag ; /* [0..1] Specifies whether a set of parameters
 controlling the characteristics of the deblocking
 filter is indicated in the slice header.Used to
 decode the slice layer.Given to the HW MB engine.
 */

	U32 constrained_intra_pred_flag ; /* [0..1] Indicates whether the intra prediction
 only uses neighbouring intra coded MBs.Given to
 the HW MB engine.
 */
	U32 transform_8x8_mode_flag ; /* [0..1]Indicates when equal to 1 that the 8x8
 transform decodig process may be in use. When equal
 to 0 specifiesthat the 8x8 transform decoding
 process is not in use.
 Shall be set to 0 when decoding a main profile
 stream
 */
	H264_ScalingList_t ScalingList;

	S32 second_chroma_qp_index_offset ; /* [-12..+12]. Specifies the offset that shall be
 added to QPy et Qs for addressing the table of QPc
 values for the Cr chroma component respectively.
 Shall be set to NULL when decoding a "main
 profile" stream
 */
} H264_SetGlobalParamPPS_t;

/*
** H264_SetGlobalParam_t :
** H264 global transform parameters required by the firmware for performing a picture decode.
*/
typedef struct
{
	H264_SetGlobalParamSPS_t H264SetGlobalParamSPS; /* SPS for next picture */
	H264_SetGlobalParamPPS_t H264SetGlobalParamPPS; /* PPS for next picture */
} H264_SetGlobalParam_t ;

/*
** H264_TransformParam_fmw_t :
** H264 specific parameters required for feeding the firmware on a picture decoding
** command order.
**
** Naming convention:
** -----------------
** H264_TransformParam_fmw_t
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
	U32 *PictureStartAddrBinBuffer; /* start address in the bin buffer */
	U32 *PictureStopAddrBinBuffer; /* stop address in the bin buffer */
	U32 MaxSESBSize; /* Maximum SEBSize in bytes */
	H264_MainAuxEnable_t MainAuxEnable; /* enable Main and/or Aux outputs */
	H264_HorizontalDeciFactor_t HorizontalDecimationFactor; /* Horizontal decimation factor */
	H264_VerticalDeciFactor_t VerticalDecimationFactor; /* Vertical decimation factor */
	H264_DecodingMode_t DecodingMode; /* Specifies the decoding mode
 (normal or downgraded) */
	H264_AdditionalFlags_t AdditionalFlags; /* To specify additional processing
 e.g.FGT */
	H264_HostData_t HostData; /* data resulting from the Slice header 0
 host processing.
 */

	H264_DecodedBufferAddress_t DecodedBufferAddress; /* Address where the decoded picture will be stored
 (Luma and Chroma samples and information related
 to the MB decription
 */
	H264_RefPictListAddress_t InitialRefPictList ; /* initialized reference picture list index
 */
} H264_TransformParam_fmw_t;

/*
** H264_InitTransformerParam_fmw_t:
** Parmeters required for a firmware transformer initialization.
**
** Naming convention:
** -----------------
** H264_InitTransformerParam_fmw_t
** |
** |-------------> This structure defines the parameter required when
** using the MME_InitTransformer() function.
*/

typedef struct
{
	/* Begin address of the circular bin buffer. Bits 0 to 4 shall be equal to 0 */
	U32 *CircularBinBufferBeginAddr_p;
	/* End address of the circular bin buffer. Bits 0 to 4 shall be equal to 1 */
	U32 *CircularBinBufferEndAddr_p;
} H264_InitTransformerParam_fmw_t;

/*
** H264_TransformerCapability_fmw_t:
** Parameters required for a firmware transformer initialization.
**
** Naming convention:
** -----------------
** H264_TransformerCapability_fmw_t
** |
** |-------------> This structure defines generic parameter required by the
** MME_TransformerCapability_t in the MME API strucure definition.
*/
typedef struct
{
	U32 SD_MaxMBStructureSize; /* Max Size in bytes of the MB structure for SD decode (for one frame) */
	U32 HD_MaxMBStructureSize; /* Max Size in bytes of the MB structure for HD decode (for one frame) */
	U32 MaximumFieldDecodingLatency90KHz; /* Maximum field decoding latency expressed in 90kHz unit */
} H264_TransformerCapability_fmw_t;

/**********************************************************/

/*
** H264_CommandStatus_fmw_t :
** H264 specific parameters required for performing a picture decode.
** When performing a Picture decode command (MME_TRANSFORM) on the DeltaPhi,
** the video driver will use this structure for
** MME_CommandStatus_t.AdditionalInfo_p
**
*/

/* The picture to decode is divided into a maximum of
 H264_STATUS_PARTITION * H264_STATUS_PARTITION areas in order to locate
 the decoding errors.
*/
#define H264_STATUS_PARTITION 6

typedef struct
{
	ST_ErrorCode_t ErrorCode;
#if (H264_MME_VERSION >= 42)
	U32 DecodeTimeInMicros;
#endif /* H264_MME_VERSION >= 42 */

#if (H264_MME_VERSION >= 53)
#ifdef TEMPORAL_DIRECT_MODE_OFF_FOR_ECHOSTAR
	BOOL IsDirectTempModePresentAndOffFlag;
#endif
#endif

	U8 Status[H264_STATUS_PARTITION][H264_STATUS_PARTITION];

#if (H264_MME_VERSION >= 50)
	U32 CEHRegisters[H264_NUMBER_OF_CEH_INTERVALS];
#endif /* H264_MME_VERSION >= 50 */

#ifdef H264DEC_PIC_SEQ_DEBUG_INFO
	void *DebugBufferPtr;
#endif

} H264_CommandStatus_fmw_t;

/* --------------------------------------- */
/* Errors returned by the H264 decoder in MME_CommandStatus_t.Error when
 CmdCode is equal to MME_TRANSFORM.
 These errors are bitfields, and several bits can be raised at the
 same time. */

#define H264_DECODER_ID 0xABCE
#define H264_DECODER_BASE (H264_DECODER_ID << 16)

/*
** H264_DecodingError_t :
** Errors returned by the H264 decoder in MME_CommandStatus_t.Error when
** CmdCode is equal to MME_TRANSFORM.
** These errors are bitfields, and several bits can be raised at the
** same time.
*/
typedef enum
{
	/* The firmware was successful. */
	H264_DECODER_NO_ERROR = (H264_DECODER_BASE + 0),

	/* The firmware decoded to much MBs:
	 - the H264_CommandStatus_t.Status doesn't locate these erroneous MBs
	 because the firmware can't know where are these extra MBs.
	 - MME_CommandStatus_t.Error can also contain H264_DECODER_ERROR_RECOVERED.
	*/
	H264_DECODER_ERROR_MB_OVERFLOW = (H264_DECODER_BASE + 1),

	/* The firmware encountered error(s) that were recovered:
	 - H264_CommandStatus_t.Status locates the erroneous MBs.
	 - MME_CommandStatus_t.Error can also contain H264_DECODER_ERROR_MB_OVERFLOW.
	*/
	H264_DECODER_ERROR_RECOVERED = (H264_DECODER_BASE + 2),

	/* The firmware encountered an error that can't be recovered:
	 - H264_CommandStatus_t.Status has no meaning.
	 - MME_CommandStatus_t.Error is equal to H264_DECODER_ERROR_NOT_RECOVERED.
	*/
	H264_DECODER_ERROR_NOT_RECOVERED = (H264_DECODER_BASE + 4),

	/* The firmware encountered an error GNBvd42696 */
	/* This is encountered on deltamu blue only */
	H264_DECODER_ERROR_GNBvd42696 = (H264_DECODER_BASE + 8),

	/* The firmware task doesn't complete within allowed time limit:
	 - H264_CommandStatus_t.Status has no meaning.
	 - MME_CommandStatus_t.Error is equal to H264_DECODER_ERROR_TASK_TIMEOUT.
	*/
	H264_DECODER_ERROR_TASK_TIMEOUT = (H264_DECODER_BASE + 16)
} H264_DecodingError_t;
/* --------------------------------------- */

#ifdef H264DEC_PIC_SEQ_DEBUG_INFO
typedef struct
{
	U32 PicCfg;
	U32 Fip;
	U32 Decimation;
	U32 RefFieldFrame;
	U32 HwcfgPicMbdescrOffsets;
	U32 PicSizeInMbs;
} H264Dec_DebugPicture_t;
#endif

#ifdef H264DEC_SLICE_DEBUG_INFO
typedef struct
{
	U32 DisableDeblockingFilterIdc;
	U32 SliceAlphaC0OffsetDiv2;
	U32 SliceBetaOffsetDiv2;
	U32 HwcfgSliDirectMode;
	U32 EnableWeightedPrediction;
	U32 NumRefIdxL0ActiveMinus1;
	U32 NumRefIdxL1ActiveMinus1;
	U32 LumaLogWeightDenom;
	U32 ChromaLogWeightDenom;
	U32 LumaWeightL0Flags;
	U32 ChromaWeightL0Flags;
	U32 LumaWeightL1Flags;
	U32 ChromaWeightL1Flags;
} H264Dec_DebugSlice_t;
#endif

#ifdef H264DEC_MB_DEBUG_INFO
typedef struct
{
	U32 ResDecodingParam;
	U32 ResDecodingAndDeblockingParam;
	U32 LumaMbType;
	U32 ChromaMbType;
	U32 IntraLumaPredMode0;
	U32 IntraLumaPredMode1;
	U32 IntraChromaPredMode;
	U32 HwcfgLumaPicBufIdxPart1_0;
	U32 HwcfgLumaPicBufIdxPart3_2;
	U32 RefIndexPart1_0;
	U32 RefIndexPart3_2;
	U32 TbandTdPart1_0;
	U32 TbandTdPart3_2;
	U32 MbErrorStatus;
	U32 Mv[32];
} H264Dec_DebugMb_t;
#endif

/************************************************************/

#endif /* #ifndef __H264_TRANSFORMERTYPES_H */

