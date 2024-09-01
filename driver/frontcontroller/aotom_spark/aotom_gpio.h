#ifndef __AOTOM_GPIO_H__
#define __AOTOM_GPIO_H__

#define PIO_BITS                            8

#define GPIO_SIMULATE_I2C_SCL_PORT          2
#define GPIO_SIMULATE_I2C_SCL_BIT           0

#define GPIO_SIMULATE_I2C_SDA_PORT          2
#define GPIO_SIMULATE_I2C_SDA_BIT           1

#define GPIO_I2C_FIRST_SCL_PORT             2
#define GPIO_I2C_FIRST_SCL_BIT              2

#define GPIO_I2C_FIRST_SDA_PORT             2
#define GPIO_I2C_FIRST_SDA_BIT              3

#define GPIO_I2C_SECOND_SCL_PORT            2
#define GPIO_I2C_SECOND_SCL_BIT             5

#define GPIO_I2C_SECOND_SDA_PORT            2
#define GPIO_I2C_SECOND_SDA_BIT             6

typedef struct YWLIB_ListHead_s
{
	struct YWLIB_ListHead_s *Next, *Prev;
} YWLIB_ListHead_T;

/*Error Define*/
enum
{
	YWGPIO_ERROR_INIT  = YW_MODULE_SET_ID(YWHAL_MODULE_GPIO_ID),
	YWGPIO_ERROR_TERM,
	YWGPIO_ERROR_OPEN,
	YWGPIO_ERROR_CLOSE,
	YWGPIO_ERROR_READ,
	YWGPIO_ERROR_WRITE,
	YWGPIO_ERROR_CONFIG,
	YWGPIO_ERROR_CTROL,
	YWGPIO_ERROR_BUSY
};

typedef enum YWGPIO_IOMode_e
{
	YWGPIO_IO_MODE_NOT_SPECIFIED,
	YWGPIO_IO_MODE_BIDIRECTIONAL,
	YWGPIO_IO_MODE_OUTPUT,
	YWGPIO_IO_MODE_INPUT,
	YWGPIO_IO_MODE_ALTERNATE_OUTPUT,
	YWGPIO_IO_MODE_ALTERNATE_BIDIRECTIONAL,
	YWGPIO_IO_MODE_BIDIRECTIONAL_HIGH,            /*4th bit remains HIGH*/
	YWGPIO_IO_MODE_OUTPUT_HIGH                    /*4th bit remains HIGH*/
} YWGPIO_IOMode_T;

typedef enum YWGPIO_WorkMode_e
{
	YWGPIO_WORK_MODE_PRIMARY,   /* Configure for primary functionality */
	YWGPIO_WORK_MODE_SECONDARY  /* Configure for GPIO functionality */
} YWGPIO_WorkMode_T;

typedef enum YWGPIO_InterruptMode_e
{
	YWGPIO_INT_NONE          = 0,
	YWGPIO_INT_HIGH          = 1,
	YWGPIO_INT_LOW           = 2,
	YWGPIO_INT_RAISING_EDGE  = 4,
	YWGPIO_INT_FALLING_EDGE  = 8,
	YWGPIO_INT_VERGE         = 16
} YWGPIO_InterruptMode_T;

typedef U32 YWGPIO_Handle_T;
typedef U32 YWGPIO_Index_T;

typedef struct YWGPIO_GpioDeviceList_s
{
	YWLIB_ListHead_T     YWGPIO_GpioList;
	U32                  GpioIndex;
	void                 *PrivateData;
} YWGPIO_GpioDeviceList_T;

typedef struct YWGPIO_Feature_s
{
	U32                  GpioNum;
	YWLIB_ListHead_T     *GpioListHead;
} YWGPIO_Feature_T;

typedef struct YWGPIO_OpenParams_s
{
	U32                 GpioIndex;
	YWGPIO_IOMode_T     IoMode;
	YWGPIO_WorkMode_T   WorkMode;
	void                *PrivateData;
} YWGPIO_OpenParams_T;

typedef void (* YWGPIO_ISRFunc_T)(YWGPIO_Handle_T GpioHandle);

YW_ErrorType_T YWGPIO_Init(void);
YW_ErrorType_T YWGPIO_Term(void);
YW_ErrorType_T YWGPIO_GetFeature(YWGPIO_Feature_T *GpioFeature);
YW_ErrorType_T YWGPIO_Open(YWGPIO_Handle_T *pGpioHandle, YWGPIO_OpenParams_T *GpioOpenParams);
YW_ErrorType_T YWGPIO_Close(YWGPIO_Handle_T GpioHandle);
YW_ErrorType_T YWGPIO_SetIoMode(YWGPIO_Handle_T GpioHandle, YWGPIO_IOMode_T IoMode);
YW_ErrorType_T YWGPIO_SetWorkMode(YWGPIO_Handle_T GpioHandle, YWGPIO_WorkMode_T WorkMode);

YW_ErrorType_T YWGPIO_Read(YWGPIO_Handle_T GpioHandle, U8 *PioValue);
YW_ErrorType_T YWGPIO_Write(YWGPIO_Handle_T GpioHandle, U8 PioValue);
YW_ErrorType_T YWGPIO_RegisterISR(YWGPIO_Handle_T GpioHandle, YWGPIO_ISRFunc_T  ISR, U32 Priority);
YW_ErrorType_T YWGPIO_UnRegisterISR(YWGPIO_Handle_T GpioHandle);
YW_ErrorType_T YWGPIO_EnableInterrupt(YWGPIO_Handle_T GpioHandle, YWGPIO_InterruptMode_T INTMode);
YW_ErrorType_T YWGPIO_DisableInterrupt(YWGPIO_Handle_T GpioHandle);
U32 YWGpioO_GetVersion(S8 *pchVer, U32 nSize);

#endif /* __AOTOM_GPIO_H__ */
