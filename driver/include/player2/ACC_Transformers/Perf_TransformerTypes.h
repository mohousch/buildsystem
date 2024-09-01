#ifndef _PERF_TRANSFORMERTYPES_H_
#define _PERF_TRANSFORMERTYPES_H_

#include "mme.h"

#define DRV_MULTICOM_PERFLOG_VERSION 0x090617

#define NB_TRANSFORMERS_TO_LOG 4
#define NB_LOG_ENTRIES 128

// Insert Timing Info here after
typedef struct _sBufferLog
{
	unsigned int NbSamples: 16;
	unsigned int FsCode : 8;
	unsigned int NbChan : 4;
	unsigned int Offset : 4;
	unsigned int AudioMode: 16;
	unsigned int Reserved : 16;
} tBufferLog;

typedef struct _sChainLog
{
	unsigned int ChainIdx : 4;
	unsigned int Extended : 4;
	unsigned int Counting : 1;
	unsigned int Upsampling: 2;
	unsigned int Flags : 5; // for future use.
	unsigned int InputIdx : 4;
	unsigned int OutputIdx : 4;
	unsigned int FlagsExt : 8; // for future use
} tChainLog;

typedef union
{
	tChainLog Extended;
	unsigned int Base;
} uChainLog;

typedef struct _sTimingLog
{
	char Name[8];
	tChainLog Chain;
	unsigned int Clk;
	unsigned int BusReads;
	unsigned int BusWrites;
	tBufferLog Input;
	tBufferLog Output;
} tTimingLog;

#define TIMELOG_REPORT_SIZE 32

enum eTimeLogReport
{
	TIMELOG_REPORT_MTNAME,
	TIMELOG_REPORT_MT_DEFINED_1,
	TIMELOG_REPORT_MT_DEFINED_2,
	TIMELOG_REPORT_MT_DEFINED_3,
	TIMELOG_REPORT_MT_DEFINED_4,
	TIMELOG_REPORT_MT_DEFINED_5,
	TIMELOG_REPORT_MT_DEFINED_6,
	TIMELOG_REPORT_MT_DEFINED_7,
	TIMELOG_REPORT_FLAGS,
	TIMELOG_REPORT_EMUTE,
	TIMELOG_REPORT_10,
	TIMELOG_REPORT_11,
	TIMELOG_NB_REPORT
};

typedef union
{
	short u16[TIMELOG_REPORT_SIZE / sizeof(short)];
	int u32[TIMELOG_REPORT_SIZE / sizeof(int)];
	char txt[TIMELOG_REPORT_SIZE];
} uReport;

typedef struct _sTimingLogControl
{
	unsigned int InUse;
	unsigned int NbUsedEntries;

	// 4 strings of 32 chars to display text info from the transformer directly
	uReport Report[TIMELOG_NB_REPORT];
} tTimingLogControl;

#define PERF_TIMELOG_ID 0x106 // read LOG

typedef enum
{
	POSTMORTEM_RUNNING,
	POSTMORTEM_CRASH,
	POSTMORTEM_LIVELOCK,
	POSTMORTEM_DEADLOCK,

	POSTMORTEM_TRAPPED, //!< Transient state to ensure we catch exceptions within the exception handler.
} MME_TimeLogPostMortemStatus_t;

/*!
 *
 * This structure is held in shared memory and directly addressed by both host
 * and co-processor. For this reason we stick rigidly to 32-bit types to ensure
 * there is no scope for structure packing issues.
 */
//
//! This structure
typedef struct
{
	unsigned int StructSize; //!< Size of the structure in bytes (don't read beyond this point)
	unsigned int Version; //!< Interface version used for the dump (DRV_MULTICOM_AUDIO_POSTMORTEM_VERSION)

	char FirmwareRelease[32];
	char BuildMachine[32];
	char UserName[16];
	char CompileDate[12]; //!< Always 'Mmm dd yyyy' plus '\0'
	char CompileTime[12]; //!< Always 'hh:mm:ss' plus '\0' but we must be on 32-bit boundary

	MME_TimeLogPostMortemStatus_t Status; //!< Report the nature of the remaining values
	char Message[128]; //!< Message describing the observed failure

	unsigned int PC; //!< Program Counter of faulting (or hung) process
	unsigned int SP; //!< Stack pointer of faulting process
	unsigned int LINK; //!< Link register of faulting process
	unsigned int PSW; //!< Processor Status Word
	unsigned int Regs[64];
	unsigned int BranchBits;

	unsigned int BackTrace[32]; //!< PC values found by searching the stack for pointer in the .text section

} MME_TimeLogPostMortem_t;

typedef struct
{
	unsigned int StructSize;

	//! Once converted to a virtual address this should become an MME_TimeLogPostMortem_t pointer.
	unsigned int PostMortemPhysicalAddress;
} MME_TimeLogTransformerInfo_t;

typedef struct
{
	MME_UINT ID;
	MME_UINT StructSize;
	tTimingLogControl Control[NB_TRANSFORMERS_TO_LOG];
	tTimingLog Log [NB_TRANSFORMERS_TO_LOG][NB_LOG_ENTRIES];
} MME_TimeLogStatusParams_t;

typedef struct
{
	MME_UINT ID;
	MME_UINT StructSize;
	MME_UINT Param; // PERFLOG_LOG/PERF_ENABLE
} MME_TimeLogGlobalParams_t;

#endif // _PERF_TRANSFORMERTYPES_H_

