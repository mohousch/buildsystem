/*****************************************************************************/
/* COPYRIGHT (C) 2009 STMicroelectronics - All Rights Reserved */
/* ST makes no warranty express or implied including but not limited to, */
/* any warranty of */
/* */
/* (i) merchantability or fitness for a particular purpose and/or */
/* (ii) requirements, for a particular purpose in relation to the LICENSED */
/* MATERIALS, which is provided "AS IS", WITH ALL FAULTS. ST does not */
/* represent or warrant that the LICENSED MATERIALS provided here */
/* under is free of infringement of any third party patents, */
/* copyrights,trade secrets or other intellectual property rights. */
/* ALL WARRANTIES, CONDITIONS OR OTHER TERMS IMPLIED BY LAW ARE */
/* EXCLUDED TO THE FULLEST EXTENT PERMITTED BY LAW */
/* */
/*****************************************************************************/
/**
 @File chip.h
 @brief

*/
#ifndef _CHIP_0367TER_H
#define _CHIP_0367TER_H

/* includes ---------------------------------------------------------------- */

/* STAPI (ST20) requirements */

#include "ioarch.h"
#include "ioreg.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

//S32 ChipGetRegisterIndex(TUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, U16 RegId);
YW_ErrorType_T
ChipUpdateDefaultValues_0367ter(TUNER_IOREG_DeviceMap_t *DeviceMap,
								IOARCH_Handle_t IOHandle,
								U16 RegAddr,
								unsigned char Data, S32 reg);
YW_ErrorType_T
ChipSetOneRegister_0367ter(TUNER_IOREG_DeviceMap_t *DeviceMap,
						   IOARCH_Handle_t IOHandle,
						   U16 RegAddr, U8 Data);
int ChipGetOneRegister_0367ter(TUNER_IOREG_DeviceMap_t *DeviceMap,
							   IOARCH_Handle_t IOHandle, U16 RegAddr);
YW_ErrorType_T
ChipSetField_0367ter(TUNER_IOREG_DeviceMap_t *DeviceMap,
					 IOARCH_Handle_t IOHandle,
					 U32 FieldId, int Value);
U8 ChipGetField_0367ter(TUNER_IOREG_DeviceMap_t *DeviceMap,
						IOARCH_Handle_t IOHandle, U32 FieldId);

//void ChipWaitOrAbort(TUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, U32 delay_ms);
//void ChipWaitOrAbort_0367ter(TUNER_ScanTaskParam_T *Inst, U32 delay_ms);

YW_ErrorType_T
ChipSetFieldImage_0367ter(TUNER_IOREG_DeviceMap_t *DeviceMap,
						  IOARCH_Handle_t IOHandle,
						  U32 FieldId, S32 Value);
S32 ChipGetFieldImage_0367ter(TUNER_IOREG_DeviceMap_t *DeviceMap,
							  IOARCH_Handle_t IOHandle, U32 FieldId);

//U16 ChipGetRegAddress(U32 FieldId);
//int ChipGetFieldMask(U32 FieldId);
//int ChipGetFieldSign(U32 FieldId);
//int ChipGetFieldPosition(U8 Mask);
//int ChipGetFieldBits(int mask, int Position);

YW_ErrorType_T
ChipSetRegisters_0367ter(TUNER_IOREG_DeviceMap_t *DeviceMap,
						 IOARCH_Handle_t IOHandle,
						 int FirstRegAddr, int NbRegs);
YW_ErrorType_T
ChipGetRegisters_0367ter(TUNER_IOREG_DeviceMap_t *DeviceMap,
						 IOARCH_Handle_t IOHandle,
						 int FirstRegAddr, int NbRegs);//, unsigned char *RegsVal)

void ChipWaitOrAbort_0367ter(BOOL bForceSearchTerm, U32 delay_ms);//lwj

/* IO repeater/passthru function format */

//#define STX0367ter_LOCK_TIME_OUT 1000// add

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* H_CHIP */

/* End of chip.h */

