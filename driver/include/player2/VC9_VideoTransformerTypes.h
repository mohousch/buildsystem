/*
** @file : VC9_VideoTransformerTypes.h
**
** @brief : VC9 Video Decoder specific types
**
** @par OWNER: Didier SIRON / Didier SIRON
**
** @author : Didier SIRON / Didier SIRON
**
** @par SCOPE:
**
** @date : June 8th 2004
**
** &copy; 2004 ST Microelectronics. All Rights Reserved.
*/

#ifndef __VC9_TRANSFORMERTYPES_H
#define __VC9_TRANSFORMERTYPES_H

#include "stddefs.h" // U32, BOOL...

/*
** Identifies the name of the transformer
*/
#define VC9_MME_TRANSFORMER_NAME "VC9_TRANSFORMER"

/*
** Identifies the version of the MME API for the firmware
*/
#if (VC9_MME_VERSION == 21)
#define VC9_MME_API_VERSION "2.1"
#elif (VC9_MME_VERSION == 20)
#define VC9_MME_API_VERSION "2.0"
#elif (VC9_MME_VERSION == 19)
#define VC9_MME_API_VERSION "1.9"
#elif (VC9_MME_VERSION == 18)
#define VC9_MME_API_VERSION "1.8"
#elif (VC9_MME_VERSION == 17)
#define VC9_MME_API_VERSION "1.7"
#elif (VC9_MME_VERSION == 16)
#define VC9_MME_API_VERSION "1.6"
#elif (VC9_MME_VERSION == 15)
#define VC9_MME_API_VERSION "1.5"
#else
#define VC9_MME_API_VERSION "1.4"
#endif

/*
** Definition used to size the VC9_CommandStatus_t.CEHRegisters array
** where values of CEH registers will be stored
*/
#if VC9_MME_VERSION >= 17
#define VC9_NUMBER_OF_CEH_INTERVALS 32
#endif /* VC9_MME_VERSION >= 17 */

#ifdef VC1DEC_MB_DEBUG_INFO
#define VC1DEC_PIC_SEQ_DEBUG_INFO
#endif

/*
** VC9_DecodingError_t :
** Errors returned by the VC9 decoder in MME_CommandStatus_t.Error when
** CmdCode is equal to MME_TRANSFORM.
** These errors are bitfields, and several bits can be raised at the
** same time.
*/
typedef enum
{
	/* The firmware has been sucessfull */
	VC9_DECODER_NO_ERROR = 0,

	/* The firmware decoded to much MBs:
	 - the VC9_CommandStatus_t.Status doesn't locate these erroneous MBs
	 because the firmware can't know where are these extra MBs.
	 - MME_CommandStatus_t.Error can also contain VC9_DECODER_ERROR_MB_OVERFLOW.
	*/
	VC9_DECODER_ERROR_MB_OVERFLOW = 1,

	/* The firmware encountered error(s) that were recovered:
	 - VC9_CommandStatus_t.Status locates the erroneous MBs.
	 - MME_CommandStatus_t.Error can also contain VC9_DECODER_ERROR_RECOVERED.
	*/
	VC9_DECODER_ERROR_RECOVERED = 2,

	/* The firmware encountered an error that can't be recovered:
	 - VC9_CommandStatus_t.Status has no meaning.
	 - MME_CommandStatus_t.Error is equal to VC9_DECODER_ERROR_NOT_RECOVERED.
	*/
	VC9_DECODER_ERROR_NOT_RECOVERED = 4,

	/* The firmware task is hanged and doesn't get back to watchdog task even after maximum time
	 alloted has lapsed:
	- VC9_CommandStatus_t.Status has no meaning.
	- MME_CommandStatus_t.Error is equal to VC9_DECODER_ERROR_TASK_TIMEOUT.
	*/
	VC9_DECODER_ERROR_TASK_TIMEOUT = 8
} VC9_DecodingError_t;

/*
** VC9_HorizontalDeciFactor _t :
** Identifies the horizontal decimation factor
*/
typedef enum
{
	VC9_HDEC_1 = 0x00000000, /* no resize */
	VC9_HDEC_2 = 0x00000001, /* H/2 resize */
	VC9_HDEC_4 = 0x00000002, /* H/4 resize */
	VC9_HDEC_ADVANCED_2 = 0x00000101, /* Advanced H/2 resize using improved 8-tap filters */
	VC9_HDEC_ADVANCED_4 = 0x00000102 /* Advanced H/4 resize using improved 8-tap filters */
} VC9_HorizontalDeciFactor_t;

/*
** VC9_VerticalDeciFactor _t :
** Identifies the vertical decimation factor
*/
typedef enum
{
	VC9_VDEC_1 = 0x00000000, /* no resize */
	VC9_VDEC_2_PROG = 0x00000004, /* V/2 , progressive resize */
	VC9_VDEC_2_INT = 0x00000008, /* V/2 , interlaced resize */
	VC9_VDEC_ADVANCED_2_PROG = 0x00000204, /* Advanced V/2 , progressive resize */
	VC9_VDEC_ADVANCED_2_INT = 0x00000208 /* Advanced V/2 , interlaced resize */
} VC9_VerticalDeciFactor_t;

/*
** VC9_AdditionalFlags_t :
** Identifies the different flags that will be passed to VC9 firmware
*/
#if VC9_MME_VERSION >= 17
typedef enum
{
	VC9_ADDITIONAL_FLAG_NONE = 0,
	VC9_ADDITIONAL_FLAG_CEH = 2, /* Request firmware to return values of the CEH registers */
#if (VC9_MME_VERSION >= 18)
	VC9_ADDITIONAL_FLAG_RASTER = 64, /* Output storage of Auxillary reconstruction in Raster format. */
#endif
} VC9_AdditionalFlags_t;
#endif /* VC9_MME_VERSION >= 17 */

/*
** VC9_MainAuxEnable_t :
** Used for enabling Main/Aux outputs
*/
typedef enum
{
	VC9_AUXOUT_EN = 0x00000010, /* enable decimated reconstruction */
	VC9_MAINOUT_EN = 0x00000020, /* enable main reconstruction */
	VC9_AUX_MAIN_OUT_EN = 0x00000030 /* enable both main & decimated reconstruction */
} VC9_MainAuxEnable_t;

/*
** VC9_DecodingMode_t :
** Identifies the decoding mode.
*/
typedef enum
{
	VC9_NORMAL_DECODE = 0,
	VC9_NORMAL_DECODE_WITHOUT_ERROR_RECOVERY = 1,
	VC9_DOWNGRADED_DECODE_LEVEL1 = 2,
	VC9_DOWNGRADED_DECODE_LEVEL2 = 4
} VC9_DecodingMode_t;

/*
** VC9_Profile_t :
** Identifies the stream profile.
*/
typedef enum
{
	VC9_PROFILE_SIMPLE,
	VC9_PROFILE_MAIN,
	VC9_PROFILE_ADVANCE
} VC9_Profile_t;

/*
** VC9_PictureSyntax_t :
** Identifies the syntax of the picture to decode.
*/
typedef enum
{
	VC9_PICTURE_SYNTAX_PROGRESSIVE,
	VC9_PICTURE_SYNTAX_INTERLACED_FRAME,
	VC9_PICTURE_SYNTAX_INTERLACED_FIELD
} VC9_PictureSyntax_t;

/*
** VC9_PictureType_t :
** Identifies the type of the picture to decode.
*/
typedef enum
{
	VC9_PICTURE_TYPE_NONE,
	VC9_PICTURE_TYPE_I,
	VC9_PICTURE_TYPE_P,
	VC9_PICTURE_TYPE_B,
	VC9_PICTURE_TYPE_BI,
	VC9_PICTURE_TYPE_SKIP
} VC9_PictureType_t;

/*
** VC9_LumaAddress_t :
** Defines the address type for the VC9 decoded Luma data's
** Bits 0 to 7 shall be set to 0
*/
typedef U32 *VC9_LumaAddress_t;

/*
** VC9_ChromaAddress_t :
** Defines the address type for the VC9 decoded Chroma data's
** Bits 0 to 7 shall be set to 0
*/
typedef U32 *VC9_ChromaAddress_t;

/*
** VC9_CompressedData_t :
** Defines the address type for the VC9 compressed data
*/
typedef U32 *VC9_CompressedData_t;

/*
** VC9_MBStructureAddress_t :
** Defines the address type for the VC9 macro block structure data's
** Bits 0 to 2 shall be set to 0
*/
typedef U32 *VC9_MBStructureAddress_t;

/*
** VC9_DecodedBufferAddress_t :
** Defines the addresses where the decoded picture/additional info related to the MB
** structures will be stored
*/
typedef struct
{
	VC9_LumaAddress_t Luma_p; /* address for the decoded Luma block */
	VC9_ChromaAddress_t Chroma_p; /* address for the decoded Chroma's block */
	VC9_LumaAddress_t LumaDecimated_p; /* address for the decimated Luma block */
	VC9_ChromaAddress_t ChromaDecimated_p; /* address for the decimated Chroma's block */
	VC9_MBStructureAddress_t MBStruct_p; /* address for MB structure data's, the size of this
 buffer shall be "Picture Size in MB" * 16 bytes */
} VC9_DecodedBufferAddress_t;

/*
** VC9_RefPicListAddress_t :
** Defines the addresses where the two reference pictures will be stored
*/
typedef struct
{
	VC9_LumaAddress_t BackwardReferenceLuma_p; /* address of the backward reference Luma */
	VC9_ChromaAddress_t BackwardReferenceChroma_p; /* address of the backward reference Chroma */
	VC9_MBStructureAddress_t BackwardReferenceMBStruct_p; /* address of the backward reference
 MB structure data's */
	VC9_PictureSyntax_t BackwardReferencePictureSyntax; /* Syntax of the backward reference */
	VC9_LumaAddress_t ForwardReferenceLuma_p; /* address of the forward referenced Luma */
	VC9_ChromaAddress_t ForwardReferenceChroma_p; /* address of the forward referenced Chroma */
	VC9_PictureSyntax_t ForwardReferencePictureSyntax; /* Syntax of the forward reference */
} VC9_RefPicListAddress_t;

/*
** VC9_EntryPointParam_t :
** In Simple and Main Profile, that parameters are found in the pseudo-sequence layer.
** In Advance Profile, that parameters are found in the entry point layer.
*/
typedef struct
{
	/* ------------------------------ */
	/* Relevant in all profiles */

	BOOL LoopfilterFlag; /* in [0..1] . */
	BOOL FastuvmcFlag; /* in [0..1] . */
	BOOL ExtendedmvFlag; /* in [0..1] . */
	U32 Dquant; /* in [0..2] . */
	BOOL VstransformFlag; /* in [0..1] . */
	BOOL OverlapFlag; /* in [0..1] . */
	U32 Quantizer; /* in [0..3] . */
	/* ------------------------------ */

	/* ------------------------------ */
	/* Relevant only in advance profile */

	BOOL ExtendedDmvFlag; /* in [0..1]. Relevant only if ExtendedMvFlag = 1. */
	BOOL PanScanFlag; /* in [0..1] . */
	BOOL RefdistFlag; /* in [0..1] . */
	/* ------------------------------ */

} VC9_EntryPointParam_t;

/*
** VC9_SliceParam_t :
** That structure is relevant only in advance profile.
** It describes the characteristics of a slice.
*/
typedef struct
{
	/* Address of the slice start code */
	VC9_CompressedData_t SliceStartAddrCompressedBuffer_p;
	/* Address of the slice, i.e. value of the SLICE_ADDR field inside the slice header */
	U32 SliceAddress;

} VC9_SliceParam_t;

/*
** VC9_StartCodesParam_t :
** That structure is relevant only in advance profile.
** It describes the start codes of the current picture.
*/
/* Maximum number of slices = maximum number of MBs lines = 1088 / 16 */
#define MAX_SLICE 68
typedef struct
{
	/* Number of element in SliceArray */
	U32 SliceCount;
	/* Describe each slice inside the frame */
	VC9_SliceParam_t SliceArray[MAX_SLICE];

} VC9_StartCodesParam_t;

/*
** VC9_IntensityComp_t :
** Defines values related to the intensity compensation
*/
#define VC9_LUMSCALE_SHIFT 0
#define VC9_LUMSCALE_MASK (0x3f << VC9_LUMSCALE_SHIFT)
#define VC9_LUMSHIFT_SHIFT 16
#define VC9_LUMSHIFT_MASK (0x3f << VC9_LUMSHIFT_SHIFT)
/* Value to set in order to indicate that there is no intensity compensation */
#define VC9_NO_INTENSITY_COMP 0xffffffff

typedef struct
{
	/* Intensity comp parameters for forward top field reference (pass 1) */
	U32 ForwTop_1;
	/* Intensity comp parameters for forward top field reference (pass 2) */
	U32 ForwTop_2;
	/* Intensity comp parameters for forward bottom field reference (pass 1) */
	U32 ForwBot_1;
	/* Intensity comp parameters for forward bottom field reference (pass 2) */
	U32 ForwBot_2;
	/* Intensity comp parameters for backward top field reference (pass 1) */
	U32 BackTop_1;
	/* Intensity comp parameters for backward top field reference (pass 2) */
	U32 BackTop_2;
	/* Intensity comp parameters for backward bottom field reference (pass 1) */
	U32 BackBot_1;
	/* Intensity comp parameters for backward bottom field reference (pass 2) */
	U32 BackBot_2;
} VC9_IntensityComp_t;

/*
** VC9_TransformParam_t :
** VC9 specific parameters required for feeding the firmware on a picture decoding
** command order.
**
** Naming convention:
** -----------------
** VC9_TransformParam_t
** | |
** | |-> Only the data required by the firmware are
** | concerned by this structure.
** |
** |-------------> structure dealing with the parameters of the TRANSFORM
** command when using the MME_SendCommand()
**
**
** Not relevant fields shall be set to zero.
*/
typedef struct
{
	/* ------------------------------ */
	/* Relevant in all profiles */

	/* Begin address of the circular buffer. Bits 0 to 2 shall be equal to 0 */
	VC9_CompressedData_t CircularBufferBeginAddr_p;
	/* End address of the circular buffer. Bits 0 to 2 shall be equal to 1 */
	VC9_CompressedData_t CircularBufferEndAddr_p;
	/* Start address of the compressed picture in the input buffer */
	VC9_CompressedData_t PictureStartAddrCompressedBuffer_p;
	/* Stop address of the compressed picture in the input buffer */
	VC9_CompressedData_t PictureStopAddrCompressedBuffer_p;
	/* Addresses where the decoded picture will be stored (Luma, Chroma, MBs of the picture) */
	VC9_DecodedBufferAddress_t DecodedBufferAddress;
	/* Addresses where the backward and forward reference pictures are stored */
	VC9_RefPicListAddress_t RefPicListAddress;
	/* Enable Main and/or Aux outputs */
	VC9_MainAuxEnable_t MainAuxEnable;
	/* Horizontal decimation factor */
	VC9_HorizontalDeciFactor_t HorizontalDecimationFactor;
	/* Vertical decimation factor */
	VC9_VerticalDeciFactor_t VerticalDecimationFactor;
	/* Specifies the decoding mode (normal or downgraded) */
	VC9_DecodingMode_t DecodingMode;
#if VC9_MME_VERSION >= 17
	VC9_AdditionalFlags_t AdditionalFlags; /* Additonal Flags for future uses */
#else
	U32 AdditionalFlags; /* Additonal Flags for future uses */
#endif /* VC9_MME_VERSION >= 17 */
	/* Anti Emulation Byte Removal Flag.
	 Shall be set to TRUE when data comes from MPEG2-TS, or for an Advance Profile (AP) stream.
	 Shall be set to FALSE when data comes from a Simple/Main Profile (SP/MP) stream in ASF */
	BOOL AebrFlag;
	VC9_Profile_t Profile;
	VC9_PictureSyntax_t PictureSyntax;
	/* Type of the picture. */
	VC9_PictureType_t PictureType;
	BOOL FinterpFlag; /* in [0..1]. Comes from the (pseudo)sequence layer */
	/* Size of the encoded picture.
	 In simple/main profile: it comes from the pseudo-sequence layer.
	 In advance profile: if it is not defined in the entry point layer, it comes from the sequence
	 layer (MAX_CODED_WIDTH/MAX_CODED_HEIGHT), otherwise from the entry point layer
	 (CODED_WIDTH/CODED_HEIGHT) */
	U32 FrameHorizSize; /* in [17..1920], expressed in pixels */
	U32 FrameVertSize; /* in [2..1088], expressed in pixels */
	VC9_IntensityComp_t IntensityComp;
	BOOL RndCtrl; /* in [0..1], Comes from the picture layer (AP) or computed
 using the picture type (SP/MP) */
	/* NumeratorBFraction and DenominatorBFraction are computed using the BFRACTION syntax element
	 in the frame picture layer. If BFRACTION select the 1/5 fraction, then NumeratorBFraction
	 shall be set to 1 and DenominatorBFraction shall be set to 5.
	 Shall be set to 0 for a non B/BI picture. */
	U32 NumeratorBFraction; /* in [1..7]. */
	U32 DenominatorBFraction; /* in [2..8]. */
	/* ------------------------------ */

	/* ------------------------------ */
	/* Relevant only in simple/main profile */

	BOOL MultiresFlag; /* in [0..1] . Comes from the pseudo-sequence layer */
	/* The two following values are computed using the RESPIC syntax element found in the picture layer.
	 For a B and Bi picture, they shall have the same value as for the forward reference picture */
	BOOL HalfWidthFlag; /* in [0..1] */
	BOOL HalfHeightFlag; /* in [0..1] */
	BOOL SyncmarkerFlag; /* in [0..1] . Comes from the pseudo-sequence layer */
	BOOL RangeredFlag; /* in [0..1] . Comes from the pseudo-sequence layer */
	U32 Maxbframes; /* in [0..7] . Comes from the pseudo-sequence layer */
	BOOL ForwardRangeredfrmFlag; /* in [0..1]. Comes from the picture layer. This is the value of the
 RANGEREDFRM flag of the forward reference picture.
 Shall be set to FALSE if it is not present in the bitstream, or if
 the current picture is a I/Bi picture */
	/* ------------------------------ */

	/* ------------------------------ */
	/* Relevant only in advance profile */

	BOOL PostprocFlag; /* in [0..1] . Comes from the sequence layer */
	BOOL PulldownFlag; /* in [0..1] . Comes from the sequence layer */
	BOOL InterlaceFlag; /* in [0..1] . Comes from the sequence layer */
	BOOL PsfFlag; /* in [0..1] . Comes from the sequence layer */
	BOOL TfcntrFlag; /* in [0..1] . Comes from the sequence layer */
	VC9_StartCodesParam_t StartCodes;
	U32 BackwardRefdist; /* in [0..16]. Comes from the picture layer. This is the value of the
 REFDIST syntax element in the backward reference picture.
 Shall be set to 0 if the current picture is not a B picture */
	U32 FramePictureLayerSize; /* Size of the frame picture layer, in bits, i.e. size of the
 INTERLACE FIELD PICTURE FIELD1() syntax element up to the
 FIELDPIC() syntax element (not included).
 Relevant only for interlace field pictures. */
	BOOL TffFlag; /* in [0..1]. Comes from the frame picture layer */
	BOOL SecondFieldFlag; /* in [0..1]. Set to TRUE only if we are decoding the second field
 of an interlace field picture */
	U32 RefDist; /* in [0..16]. Comes from the frame picture layer.
 Shall be set to 0 if the current picture is not a I or P interlace field
 picture, or if REFDIST_FLAG syntax element is equal to 0 */
	/* ------------------------------ */

	VC9_EntryPointParam_t EntryPoint;
#if (VC9_MME_VERSION >= 15)
	S32 CurrentPicOrderCount; /* Current Picture order count, only used in trick mode*/
	S32 ForwardPicOrderCount; /* Forward Picture order count, only used in trick mode*/
	S32 BackwardPicOrderCount; /* Backward Picture order count, only used in trick mode*/
#endif
} VC9_TransformParam_t;

#ifdef VC1DEC_MB_DEBUG_INFO
typedef struct
{
	U32 ResDecodingParam;
	U32 ResDecodingAndDeblockingParam;
	U32 Mv[5];
	U32 LumaMbType;
	U32 ChromaMbType;
	U32 HwcfgLumaPicBufIdxPart1_0;
	U32 HwcfgLumaPicBufIdxPart3_2;
	U32 HwcfgChromaPicBufIdxPart1_0;
	U32 HwcfgChromaPicBufIdxPart3_2;
	U32 TbandTd;
	U32 MbErrorStatus;
} VC9_DebugMb_t;
#endif

#ifdef VC1DEC_PIC_SEQ_DEBUG_INFO
typedef struct
{
	U32 PicCfg;
	U32 PicStdCfg;
	U32 Fip;
	U32 Decimation;
	U32 RefFieldFrame;
	U32 BilinearFlag;
	U32 RndCtrl;
	U32 IntNotProgFlags;
	U32 WeightedPrediction;
	U32 PicSizeInMbs;
} VC9_DebugPicture_t;
#endif
/*
** VC9_InitTransformerParam_fmw_t:
** Parmeters required for a firmware transformer initialization.
**
** Naming convention:
** -----------------
** VC9_InitTransformerParam_fmw_t
** |
** |-------------> This structure defines the parameter required when
** using the MME_InitTransformer() function.
*/
typedef struct
{
	U32 Dummy;
} VC9_InitTransformerParam_fmw_t;

/*
** VC9_TransformerCapability_t:
** Parameters required for a firmware transformer initialization.
**
** Naming convention :
** -------------------
** VC9_TransformerCapability_t
** |
** |-------------> This structure defines generic parameter required by the
** MME_TransformerCapability_t in the MME API strucure definition.
*/
typedef struct
{
	U32 MaximumFieldDecodingLatency90KHz; /* Maximum field decoding latency expressed
 in 90kHz unit */
} VC9_TransformerCapability_t;

/*
** The picture to decode is divided into a maximum of
** VC9_STATUS_PARTITION * VC9_STATUS_PARTITION areas in order to locate
** the decoding errors.
*/
#define VC9_STATUS_PARTITION 6

/*
** VC9_CommandStatus_t:
** Defines the VC9 decoding command status.
** When performing a Picture decode command (MME_TRANSFORM) on the Delta-Mu,
** the video driver will use this structure for MME_CommandStatus_t.AdditionalInfo_p
**
** Naming convention :
** -------------------
** VC9_CommandStatus_t
** |
** |-----------> This structure defines generic parameter required by the
** MME_CommandStatus_t in the MME API strucure definition.
*/
typedef struct
{
	VC9_DecodingError_t ErrorCode;
	U8 Status[VC9_STATUS_PARTITION][VC9_STATUS_PARTITION];
#if VC9_MME_VERSION >= 16
	// DecodeTimeInMicros is for validation/debug purpose
	// only. This field is not updated by the H264 decoder firmware itself
	// but by the host/firmware communication layer used in validation environment.
	U32 DecodeTimeInMicros;
#endif /* VC9_MME_VERSION >= 16 */
#if VC9_MME_VERSION >= 17
	// CEHRegisters[] is an array where values of the
	// Contrast Enhancement Histogram (CEH) registers will be stored.
	// CEHRegisters[0] will correspond to register MBE_CEH_0_7, CEHRegisters[1] will
	// correspond to register MBE_CEH_8_15., CEHRegisters[2], correspond to register
	// MBE_CEH_16_23.
	// Note that elements of this array will be updated only if
	// VC9_TransformParam_t.AdditionalFlags will have the flag
	// VC9_ADDITIONAL_FLAG_CEH set. They will remain unchanged otherwise.
	U32 CEHRegisters[VC9_NUMBER_OF_CEH_INTERVALS];
#endif /* VC9_MME_VERSION >= 17 */

#ifdef VC1DEC_PIC_SEQ_DEBUG_INFO
	void *DebugBufferPtr;
#endif
} VC9_CommandStatus_t;

#endif /* #ifndef __VC9_TRANSFORMERTYPES_H */

