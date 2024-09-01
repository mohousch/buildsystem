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
 @File chip.c
 @brief

*/
#include <linux/kernel.h> /* Kernel support */
#include <linux/delay.h>
#include <linux/i2c.h>

#include "D3501.h"
#include "ywtuner_ext.h"
#include "tuner_def.h"
#include "ioarch.h"
#include "ioreg.h"
#include "tuner_interface.h"
#include "chip_0367ter.h"
#include "stv0367ofdm_init.h"
#include "D0367_inner.h"

extern IOARCH_HandleData_t IOARCH_Handle[TUNER_IOARCH_MAX_HANDLES];

extern int debug_fe7162;
#define _DEBUG if (debug_fe7162)

#define REPEATER_ON 1
#define REPEATER_OFF 0
#define WAITFORLOCK 1

#define I2C_HANDLE(x) (*((YWI2C_Handle_T *)x))

/* ----------------------------------------------------------------------------
Name: ChipUpdateDefaultValues_0367ter

Description:update the default values of the RegMap chip

Parameters:hChip ==> handle to the chip
 :: pRegMap ==> pointer to

Return Value:YW_NO_ERROR if ok, YWHAL_ERROR_INVALID_HANDLE otherwise
---------------------------------------------------------------------------- */
YW_ErrorType_T ChipUpdateDefaultValues_0367ter(TUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, U16 RegAddr, unsigned char Data, S32 reg)
{
	YW_ErrorType_T error = YW_NO_ERROR;
	// S32 reg;
	if (DeviceMap != NULL)
	{
// for(reg=0;reg<DeviceMap->Registers;reg++)
		{
			DeviceMap->RegMap[reg].Address = RegAddr;
			DeviceMap->RegMap[reg].ResetValue = Data;
			DeviceMap->RegMap[reg].Value = Data;
		}
	}
	else
		error = YWHAL_ERROR_INVALID_HANDLE;
	return error;
}

/* ----------------------------------------------------------------------------
Name: ChipSetOneRegister_0367ter()

Description: Writes Value to the register specified by RegAddr

Parameters:hChip ==> handle to the chip
 RegAddr ==>Address of the register
 Value ==>Value to be written to register
Return Value: ST_NO_ERROR (SUCCESS)
---------------------------------------------------------------------------- */
YW_ErrorType_T ChipSetOneRegister_0367ter(TUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, U16 RegAddr, U8 Data)
{
	S32 regIndex;
	if (DeviceMap)
	{
		regIndex = ChipGetRegisterIndex(DeviceMap, IOHandle, RegAddr);
		if ((regIndex >= 0) && (regIndex < DeviceMap->Registers))
		{
			DeviceMap->RegMap[regIndex].Value = Data;
			ChipSetRegisters_0367ter(DeviceMap, IOHandle, RegAddr, 1);
		}
	}
	else
		return YWHAL_ERROR_INVALID_HANDLE;
	return DeviceMap->Error;
}

void D0367ter_write(TUNER_IOREG_DeviceMap_t *DeviceMap,
					IOARCH_Handle_t IOHandle,
					unsigned char *pcData, int nbdata)
{
	D0367_write(DeviceMap, IOHandle, pcData, nbdata);
}

void D0367ter_read(TUNER_IOREG_DeviceMap_t *DeviceMap,
				   IOARCH_Handle_t IOHandle,
				   unsigned char *pcData, int NbRegs)
{
	D0367_read(DeviceMap, IOHandle, pcData, NbRegs);
}

/*****************************************************
**FUNCTION :: ChipSetRegisters_0367ter
**ACTION :: Set values of consecutive's registers (values are taken in RegMap)
**PARAMS IN :: hChip ==> Handle to the chip
** FirstReg ==> Id of the first register
** NbRegs ==> Number of register to write
**PARAMS OUT:: NONE
**RETURN :: Error
*****************************************************/
YW_ErrorType_T ChipSetRegisters_0367ter(TUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, int FirstRegAddr, int NbRegs)
{
	unsigned char data[100], nbdata = 0;
	int i;
	//unsigned int Size;
	//TUNER_ScanTaskParam_T *Inst = NULL;
	//TUNER_IOREG_DeviceMap_t *RepeaterDeviceMap = NULL;
	//IOARCH_Handle_t RepeaterIOHandle = 0xffff;
	S32 firstRegIndex = -1;
	DeviceMap->Error = 0;
	firstRegIndex = ChipGetRegisterIndex(DeviceMap, IOHandle, FirstRegAddr);
	if (((firstRegIndex >= 0) && ((firstRegIndex + NbRegs - 1) < DeviceMap->Registers)) == FALSE)
	{
		//printk("ChipSetRegisters_0367ter############DeviceMap->Registers = %d,firstRegIndex= %d,NbRegs=%d\n",DeviceMap->Registers,
		// firstRegIndex,NbRegs);////
		DeviceMap->Error = YWHAL_ERROR_BAD_PARAMETER;
		return DeviceMap->Error;
	}
#if 0
	if (IOARCH_Handle[IOHandle].IORoute == TUNER_IO_REPEATER)
	{
		for (i = 0; i < YWTUNERi_MAX_TUNER_NUM/*TUNER_1*/; i++)
		{
			Inst = TUNER_GetScanInfo(i);
			if (Inst->DriverParam.Ter.TunerIOHandle == IOHandle)
			{
				RepeaterDeviceMap = &Inst->DriverParam.Ter.Demod_DeviceMap;
				RepeaterIOHandle = Inst->DriverParam.Ter.DemodIOHandle;
				break;
			}
		}
		if (i == YWTUNERi_MAX_TUNER_NUM)
		{
			//printk("DeviceMap->Error YWHAL_ERROR_INVALID_HANDLE=====\n");////
			DeviceMap->Error = YWHAL_ERROR_INVALID_HANDLE;
			return DeviceMap->Error;
		}
	}
#endif
	if (DeviceMap)
	{
		{
			if (NbRegs < 70)
			{
				switch (DeviceMap->Mode)
				{
					case IOREG_MODE_SUBADR_16:
						data[nbdata++] = MSB(FirstRegAddr); /* 16 bits sub addresses */
					case IOREG_MODE_SUBADR_8:
						data[nbdata++] = LSB(FirstRegAddr); /* 8 bits sub addresses */
					case IOREG_MODE_NOSUBADR:
						for (i = 0; i < NbRegs; i++)
							data[nbdata++] = DeviceMap->RegMap[firstRegIndex + i].Value;; /* fill data buffer */
						break;
					default:
						//printk("Error %d\n", __LINE__);
						DeviceMap->Error = YWHAL_ERROR_INVALID_HANDLE;
						return YWHAL_ERROR_INVALID_HANDLE;
				}
				//lwj add for test
#if 0
				int j = 0;
				printk("FirsetRegister = 0x%0x", FirstRegAddr);
				for (j = 0 ; j < nbdata; j++)
				{
					printk(" data = 0x%02x,", data[j]);
				}
				printk("\n");
#endif
				//lwj add for test end
				//if(IOARCH_Handle[IOHandle].IORoute == TUNER_IO_REPEATER)
				//{
				// ChipSetField_0367ter(RepeaterDeviceMap, RepeaterIOHandle,F367_I2CT_ON, 1); /* Set repeater ON */
				//}
#if 0
				DeviceMap->Error = YWI2C_WriteWithStop(I2C_HANDLE(&IOARCH_Handle[IOHandle].ExtDeviceHandle), data, nbdata, DeviceMap->Timeout, &Size);
				if (DeviceMap->Error == YWI2C_ERROR_WRITE_FAILURE)
				{
					YWOS_TaskSleep(2);
				}
#else
				D0367ter_write(DeviceMap, IOHandle, data, nbdata);
#endif
				//if(IOARCH_Handle[IOHandle].IORoute == TUNER_IO_REPEATER)
				//{
				// ChipSetField_0367ter(RepeaterDeviceMap, RepeaterIOHandle,F367_I2CT_ON, 0); /* Set repeater OFF */
				//}
				//#endif
			}
			else
				DeviceMap->Error = YWHAL_ERROR_FEATURE_NOT_SUPPORTED;
		}
	}
	else
		return YWHAL_ERROR_INVALID_HANDLE;
	_DEBUG
	if (DeviceMap->Error != 0)
	{
		printk("d0367ter DeviceMap->Error=%d,FirstRegAddr=%x\n", DeviceMap->Error, FirstRegAddr); //for test
		YWOSTRACE((YWOS_TRACE_ERROR, "d0367ter DeviceMap->Error=%d,FirstRegAddr=%x\n", DeviceMap->Error, FirstRegAddr)); //for test
	}
	return DeviceMap->Error;
}

/*****************************************************
**FUNCTION :: ChipSetField_0367ter
**ACTION :: Set value of a field in the chip
**PARAMS IN :: hChip ==> Handle to the chip
** FieldId ==> Id of the field
** Value ==> Value to write
**PARAMS OUT:: NONE
**RETURN :: Error
*****************************************************/
YW_ErrorType_T
ChipSetField_0367ter(TUNER_IOREG_DeviceMap_t *DeviceMap,
					 IOARCH_Handle_t IOHandle,
					 U32 FieldId, int Value)
{
	int regValue;
	int mask;
	int Sign;
	int Bits;
	int Pos;
	if (DeviceMap)
	{
		//if(!DeviceMap->Error)
		{
			if (FieldId <= 0xffff)
			{
				ChipSetOneRegister_0367ter(DeviceMap, IOHandle, FieldId, Value);
			}
			else
			{
				regValue = ChipGetOneRegister_0367ter(DeviceMap, IOHandle, (FieldId >> 16) & 0xFFFF); /* Read the register */
				Sign = ChipGetFieldSign(FieldId);
				mask = ChipGetFieldMask(FieldId);
				Pos = ChipGetFieldPosition(mask);
				Bits = ChipGetFieldBits(mask, Pos);
				if (Sign == CHIP_SIGNED)
					Value = (Value > 0) ? Value : Value + (Bits); /* compute signed fieldval */
				Value = mask & (Value << Pos); /* Shift and mask value */
				regValue = (regValue & (~mask)) + Value; /* Concat register value and fieldval */
				//printk("(FieldId >> 16) & 0xFFFF = %x, regValue = %x\n",
				// (FieldId >> 16) & 0xFFFF, regValue);
				ChipSetOneRegister_0367ter(DeviceMap, IOHandle, (FieldId >> 16) & 0xFFFF, regValue); /* Write the register */
			}
		}
		//else
		//DeviceMap->Error = YWHAL_ERROR_BAD_PARAMETER;
	}
	else
		return YWHAL_ERROR_INVALID_HANDLE;
	_DEBUG
	if (DeviceMap->Error != 0)
	{
		printk("d0376ter chip.c DeviceMap->Error=%d,FirstRegAddr=%x\n", DeviceMap->Error, FieldId); //for test
		YWOSTRACE((YWOS_TRACE_ERROR, "d0376ter chip.c DeviceMap->Error=%d,FirstRegAddr=%x\n", DeviceMap->Error, FieldId)); //for test
	}
	return DeviceMap->Error;
}

/*****************************************************
**FUNCTION :: ChipGetRegisters_0367ter
**ACTION :: Get values of consecutive's registers (values are writen in RegMap)
**PARAMS IN :: hChip ==> Handle to the chip
** FirstReg ==> Id of the first register
** NbRegs ==> Number of register to read
**PARAMS OUT:: NONE
**RETURN :: Error
*****************************************************/
YW_ErrorType_T ChipGetRegisters_0367ter(TUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, int FirstRegAddr, int NbRegs)//, unsigned char *RegsVal)
{
	unsigned char data[100], nbdata = 0;
	int i;
	//unsigned int Size;
	//TUNER_ScanTaskParam_T *Inst = NULL;
	//TUNER_IOREG_DeviceMap_t *RepeaterDeviceMap = NULL;
	//IOARCH_Handle_t RepeaterIOHandle = 0xffff;
	S32 firstRegIndex;
	DeviceMap->Error = 0;
	firstRegIndex = ChipGetRegisterIndex(DeviceMap, IOHandle, FirstRegAddr);
#if 0
	if (IOARCH_Handle[IOHandle].IORoute == TUNER_IO_REPEATER)
	{
		for (i = 0; i < YWTUNERi_MAX_TUNER_NUM/*TUNER_1*/; i++)
		{
			Inst = TUNER_GetScanInfo(i);
			if (Inst->DriverParam.Ter.TunerIOHandle == IOHandle)
			{
				RepeaterDeviceMap = &Inst->DriverParam.Ter.Demod_DeviceMap;
				RepeaterIOHandle = Inst->DriverParam.Ter.DemodIOHandle;
				break;
			}
		}
		if (i == YWTUNERi_MAX_TUNER_NUM)
		{
			DeviceMap->Error = YWHAL_ERROR_INVALID_HANDLE;
			return DeviceMap->Error;
		}
	}
#endif
	if (DeviceMap)
	{
		//if(!DeviceMap->Error)
		{
			if (NbRegs < 70)
			{
				switch (DeviceMap->Mode)
				{
					case IOREG_MODE_SUBADR_16:
						data[nbdata++] = MSB(FirstRegAddr); /* for 16 bits sub addresses */
					case IOREG_MODE_SUBADR_8:
						data[nbdata++] = LSB(FirstRegAddr); /* for 8 bits sub addresses */
#if 0
						if (IOARCH_Handle[IOHandle].IORoute == TUNER_IO_REPEATER)
						{
							ChipSetField_0367ter(RepeaterDeviceMap, RepeaterIOHandle, F367_I2CT_ON, 1); /* Set repeater ON */
						}
#endif
						//DeviceMap->Error = YWI2C_WriteWithStop(I2C_HANDLE(&IOARCH_Handle[IOHandle].ExtDeviceHandle), data, nbdata, 100, &Size); /* write data buffer */
						//if (DeviceMap->Error == YWI2C_ERROR_WRITE_FAILURE)
						//{
						// YWOS_TaskSleep(1);
						//}
						D0367ter_write(DeviceMap, IOHandle, data, nbdata);
#if 0
						if (IOARCH_Handle[IOHandle].IORoute == TUNER_IO_REPEATER)
						{
							ChipSetField_0367ter(RepeaterDeviceMap, RepeaterIOHandle, F367_I2CT_ON, 0); /* Set repeater ON */
						}
#endif
					case IOREG_MODE_NOSUBADR:
#if 0
						if (IOARCH_Handle[IOHandle].IORoute == TUNER_IO_REPEATER)
						{
							ChipSetField_0367ter(RepeaterDeviceMap, RepeaterIOHandle, F367_I2CT_ON, 1); /* Set repeater ON */
						}
#endif
						//DeviceMap->Error |= YWI2C_ReadWithStop(I2C_HANDLE(&IOARCH_Handle[IOHandle].ExtDeviceHandle), data, NbRegs, 100, &Size); /* write data buffer */
						//if (DeviceMap->Error == YWI2C_ERROR_READ_FAILURE)
						//{
						// YWOS_TaskSleep(1);
						//}
						D0367ter_read(DeviceMap, IOHandle, data, NbRegs);
#if 0
						if (IOARCH_Handle[IOHandle].IORoute == TUNER_IO_REPEATER)
						{
							ChipSetField_0367ter(RepeaterDeviceMap, RepeaterIOHandle, F367_I2CT_ON, 0); /* Set repeater ON */
						}
#endif
						break;
					default:
						DeviceMap->Error = YWHAL_ERROR_FEATURE_NOT_SUPPORTED;
						return YWHAL_ERROR_FEATURE_NOT_SUPPORTED;
				}
				/* Update RegMap structure */
				for (i = 0; i < NbRegs; i++)
				{
					if (!DeviceMap->Error)
					{
						DeviceMap->RegMap[i + firstRegIndex].Value = data[i];
					}
				}
			}
			else
			{
				DeviceMap->Error = YWHAL_ERROR_FEATURE_NOT_SUPPORTED;
				// printk(" ChipGetRegisters_0367ter DeviceMap->Error YWHAL_ERROR_FEATURE_NOT_SUPPORTED ===\n");
			}
		}
	}
	else
		return YWHAL_ERROR_INVALID_HANDLE;
	if (DeviceMap->Error != 0)
	{
		YWOSTRACE((YWOS_TRACE_ERROR, "ChipGetRegisters_0367ter DeviceMap->Error=%d,FirstRegAddr=%x,NbRegs=%d\n", DeviceMap->Error, FirstRegAddr, NbRegs)); //for test
	}
	return DeviceMap->Error;
}

/*****************************************************
**FUNCTION :: ChipGetOneRegister_0367ter
**ACTION :: Get the value of one register
**PARAMS IN :: hChip ==> Handle to the chip
** reg_id ==> Id of the register
**PARAMS OUT:: NONE
**RETURN :: Register's value
*****************************************************/
int ChipGetOneRegister_0367ter(TUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, U16 RegAddr)
{
	//unsigned char data = 0xFF, dataTab[80];
	U8 data = 0xFF;
	S32 regIndex = -1;
	if (DeviceMap)
	{
		//if(DeviceMap->Mode != IOREG_MODE_NOSUBADR)
		{
			if (ChipGetRegisters_0367ter(DeviceMap, IOHandle, RegAddr, 1) == YW_NO_ERROR)
			{
				regIndex = ChipGetRegisterIndex(DeviceMap, IOHandle, RegAddr);
				data = DeviceMap->RegMap[regIndex].Value;
			}
		}
	}
	else
		DeviceMap->Error = YWHAL_ERROR_INVALID_HANDLE;
	return (data);
}

/* ----------------------------------------------------------------------------
Name: ChipGetField_0367ter()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
U8 ChipGetField_0367ter(TUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, U32 FieldId)
{
	S32 value = 0xFF;
	S32 sign, mask, pos, bits;
	if (DeviceMap != NULL)
	{
		//if(!DeviceMap->Error) //lwj remove for BUG:有的时候CPU负担太重的时候，I2C回的会比较慢，这个时候会出现i2c错误，但我们可以sleep(1ms)，接着做下一次的i2c。
		{
			//整体读取
			if (FieldId <= 0xffff)
			{
				value = ChipGetOneRegister_0367ter(DeviceMap, IOHandle, FieldId); /* Read the register */
			}
			else
			{
				/* I2C Read : register address set-up */
				sign = ChipGetFieldSign(FieldId);
				mask = ChipGetFieldMask(FieldId);
				pos = ChipGetFieldPosition(mask);
				bits = ChipGetFieldBits(mask, pos);
				value = ChipGetOneRegister_0367ter(DeviceMap, IOHandle, FieldId >> 16); /* Read the register */
				value = (value & mask) >> pos; /* Extract field */
				if ((sign == CHIP_SIGNED) && (value & (1 << (bits - 1))))
					value = value - (1 << bits); /* Compute signed value */
			}
		}
	}
	return value;
}

/*****************************************************
**FUNCTION :: ChipSetFieldImage_0367ter
**ACTION :: Set value of a field in RegMap
**PARAMS IN :: hChip ==> Handle to the chip
** FieldId ==> Id of the field
** Value ==> Value of the field
**PARAMS OUT:: NONE
**RETURN :: Error
*****************************************************/
YW_ErrorType_T ChipSetFieldImage_0367ter(TUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, U32 FieldId, S32 Value)
{
	S32 regIndex,
		mask,
		sign,
		bits, regAddress,
		pos;
	if (DeviceMap != NULL)
	{
		//if(!DeviceMap->Error)
		{
			regAddress = ChipGetRegAddress(FieldId);
			regIndex = ChipGetRegisterIndex(DeviceMap, IOHandle, regAddress);
			if ((regIndex >= 0) && (regIndex < DeviceMap->Registers))
			{
				sign = ChipGetFieldSign(FieldId);
				mask = ChipGetFieldMask(FieldId);
				pos = ChipGetFieldPosition(mask);
				bits = ChipGetFieldBits(mask, pos);
				if (sign == CHIP_SIGNED)
					Value = (Value > 0) ? Value : Value + (1 << bits); /* compute signed fieldval */
				Value = mask & (Value << pos); /* Shift and mask value */
				DeviceMap->RegMap[regIndex].Value = (DeviceMap->RegMap[regIndex].Value & (~mask)) + Value; /* Concat register value and fieldval */
			}
			else
				DeviceMap->Error = YWHAL_ERROR_INVALID_HANDLE;
		}
	}
	else
		return YWHAL_ERROR_INVALID_HANDLE;
	return DeviceMap->Error;
}

/*****************************************************
**FUNCTION :: ChipGetFieldImage_0367ter
**ACTION :: get the value of a field from RegMap
**PARAMS IN :: hChip ==> Handle of the chip
** FieldId ==> Id of the field
**PARAMS OUT:: NONE
**RETURN :: field's value
*****************************************************/
S32 ChipGetFieldImage_0367ter(TUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, U32 FieldId)
{
	S32 value = 0xFF;
	S32 regIndex,
		mask,
		sign,
		bits, regAddress,
		pos;
	if (DeviceMap != NULL)
	{
		//if(!DeviceMap->Error)
		{
			regAddress = ChipGetRegAddress(FieldId);
			regIndex = ChipGetRegisterIndex(DeviceMap, IOHandle, regAddress);
			if ((regIndex >= 0) && (regIndex < DeviceMap->Registers))
			{
				sign = ChipGetFieldSign(FieldId);
				mask = ChipGetFieldMask(FieldId);
				pos = ChipGetFieldPosition(mask);
				bits = ChipGetFieldBits(mask, pos);
				if (!DeviceMap->Error)
					value = DeviceMap->RegMap[regIndex].Value;
				value = (value & mask) >> pos; /* Extract field */
				if ((sign == CHIP_SIGNED) && (value & (1 << (bits - 1))))
					value = value - (1 << bits); /* Compute signed value */
			}
		}
		/* else
		 DeviceMap->Error = YWHAL_ERROR_INVALID_HANDLE;*/
	}
	return value;
}

/*****************************************************
**FUNCTION :: ChipWait_Or_Abort
**ACTION :: wait for n ms or abort if requested
**PARAMS IN ::
**PARAMS OUT:: NONE
**RETURN :: NONE
*****************************************************/
/* 2009-11-27@11:25:16 D02SH Remark
void ChipWaitOrAbort(TUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,U32 delay_ms)
{
 YWOS_TaskSleep(delay_ms);
}
*/
#if 0
void ChipWait_MS(U32 delay_ms)
{
	YWOS_TaskSleep(delay_ms);
	//U8 vid = 8;
	//usleep (delay_ms*1000);
}
#endif

/*****************************************************
**FUNCTION :: ChipWait_Or_Abort
**ACTION :: wait for n ms or abort if requested
**PARAMS IN ::
**PARAMS OUT:: NONE
**RETURN :: NONE
*****************************************************/
void ChipWaitOrAbort_0367ter(BOOL bForceSearchTerm, U32 delay_ms)
{
	//YWOS_TaskSleep(delay_ms);
	U32 i = 0;
	while ((bForceSearchTerm == FALSE) && (i++ < (delay_ms / 10))) msleep(10);
	i = 0;
	while ((bForceSearchTerm == FALSE) && (i++ < (delay_ms % 10))) msleep(1);
}

