
/**********************************文件头部注释***********************************/
//
//
// Copyright (C), 2005-2010, AV Frontier Tech. Co., Ltd.
//
//
// 文件名： ioarch.h
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
/*********************************************************************************/

/* define to prevent recursive inclusion */
#ifndef __TUNER_IOARCH_H
#define __TUNER_IOARCH_H

/* includes --------------------------------------------------------------- */

/* IO operation required */
typedef enum
{
	TUNER_IO_READ, /* Read only */
	TUNER_IO_WRITE, /* Write only */

	TUNER_IO_SA_READ, /* Read with prefix subaddress (for devices that support it) */
	TUNER_IO_SA_WRITE, /* Write with prefix subaddress */

	TUNER_IO_SA_READ_NS, /* Read Sub Add with WriteNoStop Cde */
	TUNER_IO_SA_WRITE_NS /* */
}
TUNER_IOARCH_Operation_t;

/* architecture specifics for I/O mode (common) */
typedef enum TUNER_IORoute_e
{
	TUNER_IO_DIRECT, /* open IO address for this instance of device */
	TUNER_IO_REPEATER, /* as above but send IO via ioaccess function (TUNER_IOARCH_RedirFn_t) of other driver (demod) */
	TUNER_IO_PASSTHRU /* do not open address just send IO using other drivers open I/O handle (demod) */
} TUNER_IORoute_t;

/* select I/O target (common) */
typedef enum TUNER_IODriver_e
{
	TUNER_IODRV_NULL, /* null driver */
	TUNER_IODRV_I2C, /* use I2C drivers */
	TUNER_IODRV_DEBUG, /* screen and keyboard IO*/
	TUNER_IODRV_MEM /* ST20 memory driver */
} TUNER_IODriver_t;

typedef U32 IOARCH_Handle_t; /* handle returned by IO driver open */
typedef U32 IOARCH_ExtHandle_t; /* handle to Instance of function being called */

/* IO repeater/passthru function format */
typedef YW_ErrorType_T(*TUNER_IOARCH_RedirFn_t)(IOARCH_ExtHandle_t hInstance, IOARCH_Handle_t IOHandle, TUNER_IOARCH_Operation_t Operation, unsigned short SubAddr, U8 *Data, U32 TransferSize, U32 Timeout);
typedef YW_ErrorType_T(*TUNER_IOARCH_RepeaterFn_t)(IOARCH_ExtHandle_t hInstance, BOOL REPEATER_STATUS);

/* hInstance is the Handle to the Instance of the driver being CALLED (not the caller) from a call to its Open() function
 IOHandle is the CALLERS handle to an already open I/O connection */

/* */
typedef struct
{
	U32 PlaceHolder;
} TUNER_IOARCH_InitParams_t;

/* */
typedef struct
{
	U32 PlaceHolder;
} TUNER_IOARCH_TermParams_t;

/* */
typedef struct
{
	U8 I2CIndex;
	TUNER_IORoute_t IORoute; /* where IO will go */
	TUNER_IODriver_t IODriver; /* which IO to use */
	TUNER_DeviceName_t DriverName; /* driver name (for Init/Open) */
	U32 Address; /* and final destination */

	IOARCH_ExtHandle_t *hInstance; /* pointer to instance of driver being called */
	TUNER_IOARCH_RedirFn_t TargetFunction; /* if IODriver is TUNER_IO_REPEATER or TUNER_IO_PASSTHRU
 then specify function to call instead of TUNER_IOARCH_ReadWrite */
} TUNER_IOARCH_OpenParams_t;

/* */
typedef struct
{
	U32 PlaceHolder;
} TUNER_IOARCH_CloseParams_t;

typedef struct
{
	BOOL IsFree; /* is handle used */
	TUNER_IODriver_t IODriver; /* IO to use */
	TUNER_IORoute_t IORoute; /* where IO will go */
	U32 Address; /* IO address */
	U32 ExtDeviceHandle; /* external device handle to be stored */

	IOARCH_ExtHandle_t *hInstance; /* if opened as TUNER_IO_REPEATER / TUNER_IO_PASSTHRU */
	TUNER_IOARCH_RedirFn_t TargetFunction;
} IOARCH_HandleData_t;

#define TUNER_IOARCH_MAX_HANDLES 12

extern IOARCH_HandleData_t IOARCH_Handle[TUNER_IOARCH_MAX_HANDLES];

/* functions --------------------------------------------------------------- */
YW_ErrorType_T TUNER_IOARCH_Init(TUNER_IOARCH_InitParams_t *InitParams);
YW_ErrorType_T TUNER_IOARCH_Open(IOARCH_Handle_t *Handle, TUNER_IOARCH_OpenParams_t *OpenParams);
YW_ErrorType_T TUNER_IOARCH_Close(IOARCH_Handle_t Handle, TUNER_IOARCH_CloseParams_t *CloseParams);
YW_ErrorType_T TUNER_IOARCH_Term(const TUNER_IOARCH_TermParams_t *TermParams);
YW_ErrorType_T TUNER_IOARCH_ChangeTarget(IOARCH_Handle_t Handle, TUNER_IOARCH_RedirFn_t TargetFunction, IOARCH_ExtHandle_t *hInstance);
YW_ErrorType_T TUNER_IOARCH_ChangeRoute(IOARCH_Handle_t Handle, TUNER_IORoute_t IORoute);
YW_ErrorType_T TUNER_IOARCH_GetAddr(IOARCH_Handle_t Handle, U32 *Address);
YW_ErrorType_T TUNER_IOARCH_ReadWrite(IOARCH_Handle_t Handle, TUNER_IOARCH_Operation_t Operation, unsigned short SubAddr, U8 *Data, U32 TransferSize, U32 Timeout);
YW_ErrorType_T TUNER_IOARCH_ReadWriteNoRep(IOARCH_Handle_t Handle, TUNER_IOARCH_Operation_t Operation, unsigned short SubAddr, U8 *Data, U32 TransferSize, U32 Timeout);
int I2C_ReadWrite(void *I2CHandle, TUNER_IOARCH_Operation_t Operation, unsigned short SubAddr, U8 *Data, U32 TransferSize, U32 Timeout);//lwj add

/* ------------------------------------------------------------------------- */

#endif /* __TUNER_IOARCH_H */

/* End of ioarch.h */
