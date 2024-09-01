///
/// @file : MMEPlus/Interface/Transformers/MP2A_TransformerTypes.h
///
/// @brief : Audio Decoder specific types for MME
///
/// @par OWNER: Ritesh Kumar Pila
///
/// @author : Ritesh Kumar Pila
///
/// @par SCOPE:
///
/// @date : 2003-07-09
///
/// &copy; 2002 ST Microelectronics. All Rights Reserved.
///

#ifndef MP2A_TRANSFORMERTYPES_INC
#define MP2A_TRANSFORMERTYPES_INC

//#include "MMEplus/Interface/mme_api.h"

// Number of frames to decode per MME transform operation
#define NUM_FRAMES_PER_TRANSFORM 1

//! Additional decoder capability structure
typedef struct
{
	U32 StructSize; //!< Size of this structure
} MME_MP2aTransformerInfo_t;

typedef struct
{
	U32 StructSize; //!< Size of this structure
} MME_MP2aTransformerInitParams_t;

typedef struct
{
	U32 StructSize; //!< Size of this structure
} MME_MP2aTransformerGlobalTransformParameters;

//! This structure must be passed when sending the TRANSFORM command to the decoder
typedef struct
{
	U32 NumInputBuffers; //!< input MME_DataBuffer_t structures, even if it's zero
	U32 NumOutputBuffers; //!< output MME_DataBuffer_t structures, even if it's zero
	MME_DataBuffer_t buffers[NUM_FRAMES_PER_TRANSFORM * 2]; //!< The input/output buffers
	// after NumInputBuffers input buffers the output buffers are also here
	U32 NumberOfFrames; //!< MP2 frames contained in these buffers

} MME_MP2aTransformerFrameDecodeParams_t;

#endif // MP2A_TRANSFORMERTYPES_INC
