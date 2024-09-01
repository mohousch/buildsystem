
/**********************************文件头部注释***********************************/
//
//
// Copyright (C), 2005-2010, AV Frontier Tech. Co., Ltd.
//
//
// 文件名： ioreg.h
//
// 创建者： chm
//
// 创建时间： 12-Oct-2005
//
// 文件描述：
//
// 修改记录： 日 期 作 者 版本 修定
// --------- --------- ----- -----
//
/**********************************************************************************/

/* define to prevent recursive inclusion */
#ifndef __TUNER_IOREG_H
#define __TUNER_IOREG_H

#define FIELD_TYPE_UNSIGNED 0
#define FIELD_TYPE_SIGNED 1

/* includes --------------------------------------------------------------- */

/* default timeout (for all I/O (I2C) operations) */
#define IOREG_DEFAULT_TIMEOUT 100

#define LSB(X) ((X & 0xFF))
#define MSB(Y) ((Y>>8)& 0xFF)

/* public types ------------------------------------------------------------ */

typedef enum
{
	IOREG_NO = 0,
	IOREG_YES = 1
} TUNER_IOREG_Flag_t;

/* register and address width: 8bits x 8addresses, 16x16 or 32x32 */
typedef enum
{
	REGSIZE_8BITS,
	REGSIZE_16BITS,
	REGSIZE_32BITS
} TUNER_IOREG_RegisterSize_t;

/* Addressing mode - 8 bit address, 16 bit address, no address (0) */
typedef enum
{
	IOREG_MODE_SUBADR_8, /* <addr><reg8><data><data>... */
	IOREG_MODE_SUBADR_16, /* <addr><reg8><reg8>data><data>... */
	IOREG_MODE_SUBADR_16_POINTED, /*Used in 899*/
	IOREG_MODE_NOSUBADR, /* <addr><data><data>... */
	IOREG_MODE_NO_R_SUBADR
} TUNER_IOREG_Mode_t;

typedef struct
{
	U32 Address; /* Address */
	U32 ResetValue; /* Default (reset) value */
	U32 Value; /* Current value */
} TUNER_IOREG_Register_t;

typedef enum STTUNER_IOREG_Pointed_e
{
	IOREG_NOT_POINTED = 0,
	IOREG_POINTED
} STTUNER_IOREG_Pointed_T;

typedef struct
{
	S32 Reg; /* Register index */
	U32 Pos; /* Bit position */
	U32 Bits; /* Bit width */
	U32 Type; /* Signed or unsigned */
	U32 Mask; /* Mask compute with width and position */
} TUNER_IOREG_Field_t;

typedef struct
{
	S32 RegExtClk; /* external clock value */
	U32 Timeout; /* I/O timeout */
	U32 Registers; /* number of registers, e.g. 65 for stv0299 */
	U32 Fields; /* number of register fields, e.g. 113 for stv0299 */
	//U32 WrStart;
	TUNER_IOREG_Register_t *RegMap; /* register map list */
	TUNER_IOREG_Field_t *FieldMap; /* register field list */
	TUNER_IOREG_Flag_t RegTrigger; /* trigger scope enable */
	TUNER_IOREG_RegisterSize_t RegSize; /* width of registers in this list */
	TUNER_IOREG_Mode_t Mode; /* Addressing mode */
	U8 *ByteStream; /* storage area for 'ContigousRegisters' operations */
	YW_ErrorType_T Error; /* Added for Error handling in I2C */
	unsigned int *DefVal;
	unsigned int MasterClock;
	int ChipId;

	//#ifdef STB6110 added for 6110
	/* Parameters needed for non sub address devices */
	U32 WrStart; /* Id of the first writable register */
	U32 WrSize; /* Number of writable registers */
	U32 RdStart; /* Id of the first readable register */
	U32 RdSize; /* Number of readable registers */
	//#endif

	void *priv;
} TUNER_IOREG_DeviceMap_t;

/* register information */
typedef struct
{
	U16 Addr; /* Address */
	U8 Value; /* Current value */

} STCHIP_Register_t; /*changed to be in sync with LLA :april 09*/

/* register field type */
typedef enum
{
	CHIP_UNSIGNED = 0,
	CHIP_SIGNED = 1
}
STCHIP_FieldType_t;

/* functions --------------------------------------------------------------- */

YW_ErrorType_T TUNER_IOREG_Open(TUNER_IOREG_DeviceMap_t *DeviceMap);
YW_ErrorType_T TUNER_IOREG_Close(TUNER_IOREG_DeviceMap_t *DeviceMap);

YW_ErrorType_T TUNER_IOREG_AddReg(TUNER_IOREG_DeviceMap_t *DeviceMap, S32 RegIndex, U32 Address, U32 ResetValue);
YW_ErrorType_T TUNER_IOREG_AddField(TUNER_IOREG_DeviceMap_t *DeviceMap, S32 RegIndex, S32 FieldIndex, U32 Pos, U32 Bits, U32 Type);

YW_ErrorType_T TUNER_IOREG_Reset(TUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle);
YW_ErrorType_T TUNER_IOREG_SetRegister_SizeU32(TUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, U32 RegIndex, U32 Value);

U32 TUNER_IOREG_GetRegister_SizeU32(TUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, U32 RegIndex);
/* no I/O performed for the following functions */
void TUNER_IOREG_FieldSetVal_SizeU32_1(TUNER_IOREG_DeviceMap_t *DeviceMap, U32 FieldIndex, int Value, U8 *DataArr);
U32 TUNER_IOREG_FieldGetVal_SizeU32_1(TUNER_IOREG_DeviceMap_t *DeviceMap, U32 FieldIndex, U8 *DataArr);

YW_ErrorType_T TUNER_IOREG_SetContigousRegisters_SizeU32_1(TUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, U32 FirstRegAddress, U8 *RegsVal, U32 Number);

YW_ErrorType_T TUNER_IOREG_GetContigousRegisters(TUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, S32 FirstRegIndex, S32 Number);
YW_ErrorType_T TUNER_IOREG_SetField(TUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, S32 FieldIndex, U32 Value);
U32 TUNER_IOREG_GetField(TUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, S32 FieldIndex);

void TUNER_IOREG_FieldSetVal_SizeU32(TUNER_IOREG_DeviceMap_t *DeviceMap, U32 FieldIndex, S32 Value, U8 *DataArr);
YW_ErrorType_T TUNER_IOREG_SetContigousRegisters_SizeU32(TUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, U32 FirstRegAddress, U32 *RegsVal, int Number);

YW_ErrorType_T TUNER_IOREG_GetContigousRegisters_SizeU32(TUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, U32 FirstRegAddress, int Number, U32 *RegsVal);
U32 TUNER_IOREG_FieldGetVal_SizeU32(TUNER_IOREG_DeviceMap_t *DeviceMap, U32 FieldIndex, U8 *DataArr);

int IOREG_GetFieldBits_SizeU32(int mask, int Position);

int IOREG_GetFieldPosition_SizeU32(U32 Mask);

int IOREG_GetFieldSign_SizeU32(U32 FieldInfo);

int TUNER_IOREG_GetField_SizeU32(TUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, U32 FieldMask, U32 FieldInfo);

YW_ErrorType_T TUNER_IOREG_SetField_SizeU32(TUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, U32 FieldMask, U32 FieldInfo, int Value);

YW_ErrorType_T TUNER_IOREG_Reset_SizeU32(TUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, U32 *DefaultVal, U32 *Addressarray);

YW_ErrorType_T TUNER_IOREG_SetRegister(TUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, S32 RegIndex, U32 Value);

#endif /* __TUNER_IOREG_H */

/* End of ioreg.h */
