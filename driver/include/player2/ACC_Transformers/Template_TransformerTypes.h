/*
 $Id: Exp $
 @file : ACF_Multicom/ACC_Transformers/Template_TransformerTypes.h

 @brief : APIs of the Template Transformer

 @par OWNER: Gael Lassure

 @author : Gael Lassure

 @par SCOPE:

 @date : 2005/05/20

 &copy; 2005 ST Microelectronics. All Rights Reserved.
*/

#ifndef _TEMPLATE_TRANSFORMERTYPES_H_
#define _TEMPLATE_TRANSFORMERTYPES_H_

#include "mme.h"

// Additional transformer capability structure
typedef struct
{
	MME_UINT StructSize; // Size of this structure
} tTemplateCapabilityInfo;

enum eTemplateParams
{
	TEMPLATE_PARAM0,
	TEMPLATE_PARAM1,
	TEMPLATE_PARAM2,
	TEMPLATE_PARAM3,

	//
	TEMPLATE_NB_PARAMS
};

// Transformer Parameters transmitted once in a while
typedef struct
{
	MME_UINT Id; // Structure identification
	MME_UINT StructSize;

	MME_UINT Config[TEMPLATE_NB_PARAMS];
} tTemplateGlobalParams;

//
enum
{
	TEMPLATE_MEMCOPY,
	TEMPLATE_PSEUDOPROCESSING,
};

typedef struct
{
	MME_UINT ExecutionMode: 1; // TEMPLATE_MEMCOPY or TEMPLATE_DUMMYPROCESSING
	MME_UINT TimeSlice : 7; // Processing TimeSlice in ms
	MME_UINT CpuMHz10 : 6; // Speed of the CPU in parts of 10MHz
	MME_UINT MIPS10 : 6; // Number of MIPS in parts of 10MIPS spent by the processing
	MME_UINT MBps10 : 6; // Number of Bytes per second in parts of 10MB
	MME_UINT Reserved : 6;
} tTemplateExecutionInit;

// Macros to convert absolute X values (X being MIPS, MHZ or MBPs) into parts of 10X
#define MIPS10(x) (x / 10)
#define MBPS10(x) (x / 10)
#define MHZ10(x) (x / 10)

// Macros to convert parts of 10X into X
#define MIPS(x10) (x10 * 10)
#define MBPS(x10) (x10 * 10)
#define MHZ(x10) (x10 * 10)

typedef union
{
	MME_UINT Word32; // Allows to reset the init to an absolute value.
	tTemplateExecutionInit BitFields; // Allows to set each independ field info.
} uTemplateExecutionInit;

// Params that are only given at init
typedef struct
{
	MME_UINT structsize; // Sizeof this struct.
	uTemplateExecutionInit InitConfig; // Example of special initialisation params.
} tTemplateInitSpecificParams;

typedef struct
{
	tTemplateInitSpecificParams InitP;
	tTemplateGlobalParams GlobalP;
} tTemplateInitParams;

typedef struct
{
	// Params that are given to each TRANSFORM command
	MME_UINT CommandType;

} tTemplateTransformParams;

typedef struct
{
	// Params fed back by each TRANSFORM command
	MME_UINT Feedback;
	MME_UINT ElapsedTime;
} tTemplateTransformStatus;

#endif // _TEMPLATE_TRANSFORMERTYPES_H_
