/*
 * aotom_i2csoft.c
 *
 * (c) 2010 Spider-Team
 * (c) 2011 cjun
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <linux/version.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/stm/pio.h>
#include <linux/delay.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
#include <asm/semaphore.h>
#else
#include <linux/semaphore.h>
#endif

#include "aotom_ywdefs.h"
#include "aotom_gpio.h"
#include "aotom_i2csoft.h"
#include "aotom_main.h"

#ifdef CONFIG_CPU_SUBTYPE_STX7105
//#define DEBUG_TRACE
#ifdef DEBUG_TRACE
#define YWI2C_INTERFACE(x)  printk x
#define YWI2C_TRACE(x)      printk x
#define YWI2C_DEBUG(x)      printk x
#define YWOSTRACE(x)        printk x
#else
#define YWI2C_INTERFACE(x)
#define YWI2C_TRACE(x)
#define YWI2C_DEBUG(x)
#define YWOSTRACE(x)
#endif


#define YWOS_WAIT_INFINITY		(0xFFFFFFFF)
#define YWI2CSOFT_MAX_DEVICE	4
#define YWI2CSOFT_MAX_HANDLE	16
#define YWI2C_NUM_SOFT_I2C		1
#define MAX_DEVICE_NAME			15

#define  I2C_DELAY              iic_delay(1)
#define  I2C_TRISTATE
#define  I2C_ACTIVE

typedef U32 YWOS_ClockMsec_T;
typedef struct semaphore YWOS_SemaphoreID_T;
typedef char I2CDeviceName_T[MAX_DEVICE_NAME + 1];

YWI2CSoft_Handle_t          g_SoftHandle;

typedef struct YWI2CSoft_OpenParam_s
{
	BOOL        IsOpen;
	U8          SlaveAddr;
} YWI2CSoft_OpenParam_t;

typedef struct YWI2CSoft_Device_s
{
	char                I2CName[16];
	BOOL                IsInit;

	U32                 Speed;
	U8                  SlaveAddr;

	YWGPIO_Handle_T     SDAHandle;
	YWGPIO_Handle_T     SCLHandle;

	YWI2CSoft_OpenParam_t OpenParam[YWI2CSOFT_MAX_HANDLE];

	YWOS_SemaphoreID_T  SoftI2cLock;


} YWI2CSoft_Device_t;


static YWI2CSoft_Device_t  I2CSoftDevice[YWI2CSOFT_MAX_DEVICE] =
{
	{"\0", FALSE, 0, 0, (YWGPIO_Handle_T)NULL, (YWGPIO_Handle_T)NULL},
	{"\0", FALSE, 0, 0, (YWGPIO_Handle_T)NULL, (YWGPIO_Handle_T)NULL},
	{"\0", FALSE, 0, 0, (YWGPIO_Handle_T)NULL, (YWGPIO_Handle_T)NULL},
	{"\0", FALSE, 0, 0, (YWGPIO_Handle_T)NULL, (YWGPIO_Handle_T)NULL}
};


typedef struct YWI2C_SoftParam_s
{
	BOOL    IsMasterMode;
	U32     SDAGpio;
	U32     SCLGpio;

	YWGPIO_Handle_T     SDAHandle;
	YWGPIO_Handle_T     SCLHandle;

	U32     SDARegBase;
	U32     SCLRegBase;

	U32     Speed; //hz
	U8      SlaveAddr; //hz


} YWI2C_SoftParam_t;


static void iic_delay(int micros)
{
	udelay(micros * 4);
}


/*=====================================================================*/
/*                         Public Functions                            */
/*=====================================================================*/

/*-----------------------------------------------------------------------
 * Initialization
 */
static void i2csoft_sendstop(YWGPIO_Handle_T SCL, YWGPIO_Handle_T SDA);

S32 YWLIB_Strcmp(S8 *str1, S8 *str2)
{
	int iValue = 0;

	if (str1 != NULL && str2 != NULL)
	{
		iValue = strcmp((char *)str1, (char *)str2);
	}
	return iValue;
}

S8 *YWLIB_Strcpy(S8 *pDstStr, const S8 *pSrcStr)
{
	S8 *pDest = NULL;
	if (pDstStr != NULL && pSrcStr != NULL)
	{
		pDest = (S8 *)strcpy((char *)pDstStr, (char *)pSrcStr);
	}
	return pDest;
}

void *YWLIB_Memset(void *s, int c, U32 n)
{
	if (s == NULL)
	{
		return s;
	}

	return memset(s, c, n);
}

YW_ErrorType_T YWOS_SemaphoreCreate(S8 *Name, U32 Count,
				    YWOS_SemaphoreID_T *SemaphoreID)
{
	Name = Name;
	Count = Count;
	SemaphoreID = SemaphoreID;
	return YW_NO_ERROR;
}

YW_ErrorType_T YWOS_SemaphoreWait(YWOS_SemaphoreID_T  SemaphoreID , YWOS_ClockMsec_T TimeOut)
{
	SemaphoreID = SemaphoreID;
	TimeOut = TimeOut;
	return YW_NO_ERROR;
}

YW_ErrorType_T YWOS_SemaphoreSend(YWOS_SemaphoreID_T  SemaphoreID)
{
	SemaphoreID = SemaphoreID;
	return YW_NO_ERROR;
}

YW_ErrorType_T YWOS_SemaphoreDelete(YWOS_SemaphoreID_T  SemaphoreID)
{
	SemaphoreID = SemaphoreID;
	return YW_NO_ERROR;
}

YW_ErrorType_T YWGPIO_Open(YWGPIO_Handle_T *pGpioHandle,
			   YWGPIO_OpenParams_T *GpioOpenParams)
{
	struct stpio_pin *pPio = NULL;

	pPio = stpio_request_pin(GpioOpenParams->GpioIndex / PIO_BITS,
				 GpioOpenParams->GpioIndex % PIO_BITS,
				 "LED", STPIO_OUT);
	//printk("pPio = 0x%x\n", (int)pPio);

	if (pPio)
	{
		(*pGpioHandle) = (YWGPIO_Handle_T)pPio;
	}
	return YW_NO_ERROR;
}

YW_ErrorType_T YWGPIO_Close(YWGPIO_Handle_T GpioHandle)
{
	struct stpio_pin *pPio = (struct stpio_pin *)GpioHandle;
	stpio_free_pin(pPio);
	return YW_NO_ERROR;
}

YW_ErrorType_T YWGPIO_Write(YWGPIO_Handle_T GpioHandle, U8 PioValue)
{
	struct stpio_pin *pPio = (struct stpio_pin *)GpioHandle;

	stpio_set_pin(pPio, PioValue);

	return YW_NO_ERROR;
}

YW_ErrorType_T YWGPIO_Read(YWGPIO_Handle_T GpioHandle, U8 *pPioValue)
{
	struct stpio_pin *pPio = (struct stpio_pin *)GpioHandle;

	(*pPioValue) = stpio_get_pin(pPio);

	return YW_NO_ERROR;
}

YW_ErrorType_T YWGPIO_SetIoMode(YWGPIO_Handle_T GpioHandle,
				YWGPIO_IOMode_T IoMode)
{
	struct stpio_pin *pPio = (struct stpio_pin *)GpioHandle;

	switch (IoMode)
	{
		case YWGPIO_IO_MODE_INPUT:
			stpio_configure_pin(pPio, STPIO_IN);
			break;
		case YWGPIO_IO_MODE_OUTPUT:
			stpio_configure_pin(pPio, STPIO_OUT);
			break;
		case YWGPIO_IO_MODE_BIDIRECTIONAL:
			stpio_configure_pin(pPio, STPIO_BIDIR);
			break;
		default:

			break;
	}
	return YW_NO_ERROR;
}

static BOOL IsI2CAlreadyInitialised(char *Name)
{
	int     i;
	BOOL    IsInited = FALSE;

	for (i = 0; i < YWI2CSOFT_MAX_DEVICE; i++)
	{
		if (YWLIB_Strcmp((S8 *)Name, (S8 *)I2CSoftDevice[i].I2CName) == 0)
		{
			if (I2CSoftDevice[i].IsInit)
			{
				IsInited = TRUE;
				break;
			}
		}
	}
	return IsInited;
}

static BOOL CheckI2cParam(YWI2CSoft_Handle_t Handle, U32 *DeviceIndex, U32 *HandleIndex)
{
	int                     i, j;
	BOOL                    FindHandle = FALSE;

	for (i = 0; i < YWI2CSOFT_MAX_DEVICE; i++)
	{
		if ((I2CSoftDevice[i].IsInit))
		{
			for (j = 0; j < YWI2CSOFT_MAX_HANDLE; j++)
			{
				//printk("Handle=0x%x &I2CSoftDevice[i].OpenParam[j]=0x%x\n",Handle,&I2CSoftDevice[i].OpenParam[j]);
				if (((YWI2CSoft_Handle_t)&I2CSoftDevice[i].OpenParam[j]) == Handle)
				{
					FindHandle = TRUE;
					break;
				}
			}
			if (FindHandle)
			{
				break;
			}
		}
	}

	if (FindHandle)
	{
		*DeviceIndex = i;
		*HandleIndex = j;
	}
	else
	{
		*DeviceIndex = YWI2CSOFT_MAX_DEVICE;
		*HandleIndex = YWI2CSOFT_MAX_HANDLE;
	}

	return FindHandle;

}

static void i2csoft_lock(U32 DeviceIndex)
{
	YWOS_SemaphoreWait(I2CSoftDevice[DeviceIndex].SoftI2cLock, YWOS_WAIT_INFINITY);
}


static void i2csoft_unlock(U32 DeviceIndex)
{
	YWOS_SemaphoreSend(I2CSoftDevice[DeviceIndex].SoftI2cLock);
}



static void i2csoft_reset(YWGPIO_Handle_T SCL, YWGPIO_Handle_T SDA)
{
	int j;

	YWGPIO_Write(SCL, 1); //I2C_SCL(1);
	YWGPIO_Write(SDA, 1); //I2C_SDA(1);

	//I2C_TRISTATE;
	for (j = 0; j < 9; j++)
	{
		YWGPIO_Write(SCL, 0); //I2C_SCL(0);
		I2C_DELAY;
		I2C_DELAY;
		YWGPIO_Write(SCL, 1); //I2C_SCL(1);
		I2C_DELAY;
		I2C_DELAY;
	}
	i2csoft_sendstop(SCL, SDA);
	I2C_TRISTATE;

}




static void i2csoft_sendstart(YWGPIO_Handle_T SCL, YWGPIO_Handle_T SDA)
{
	YWGPIO_SetIoMode(SDA, YWGPIO_IO_MODE_OUTPUT);
	YWGPIO_Write(SDA, 1); //I2C_SDA(1);
	iic_delay(1);
	YWGPIO_Write(SCL, 1); //I2C_SCL(1);
	iic_delay(5);
	YWGPIO_Write(SDA, 0); //I2C_SDA(0);
	iic_delay(5);
	YWGPIO_Write(SCL, 0); //I2C_SCL(0);
	iic_delay(2);
}

static void i2csoft_sendstop(YWGPIO_Handle_T SCL, YWGPIO_Handle_T SDA)
{
	YWGPIO_Write(SCL, 0); //I2C_SCL(0);
	iic_delay(2);
	YWGPIO_SetIoMode(SDA, YWGPIO_IO_MODE_OUTPUT);
	YWGPIO_Write(SDA, 0); //I2C_SDA(0);
	iic_delay(1);
	YWGPIO_Write(SCL, 1); //I2C_SCL(1);
	iic_delay(5);
	YWGPIO_Write(SDA, 1); //I2C_SDA(1);
	iic_delay(4);
}

static void i2csoft_sendack(YWGPIO_Handle_T SCL, YWGPIO_Handle_T SDA, int ack)
{
	YWGPIO_Write(SCL, 0); //I2C_SCL(0);
	iic_delay(3);
	YWGPIO_SetIoMode(SDA, YWGPIO_IO_MODE_OUTPUT);

	if (ack)
		YWGPIO_Write(SDA, 1); //I2C_SDA(1);
	else
		YWGPIO_Write(SDA, 0); //I2C_SDA(0);

	iic_delay(3);
	YWGPIO_Write(SCL, 1); //I2C_SCL(1);
	iic_delay(5);
	YWGPIO_SetIoMode(SDA, YWGPIO_IO_MODE_BIDIRECTIONAL);
	YWGPIO_Write(SCL, 0); //I2C_SCL(0);
	iic_delay(2);

}
static int i2csoft_writebyte(YWGPIO_Handle_T SCL, YWGPIO_Handle_T SDA, U8 data)
{
	int j;
	U8 nack;
	YWGPIO_SetIoMode(SDA, YWGPIO_IO_MODE_OUTPUT);
	for (j = 0; j < 8; j++)
	{
		YWGPIO_Write(SCL, 0); //I2C_SCL(0);
		iic_delay(1);
		if (data & 0x80)
			YWGPIO_Write(SDA, 1); //I2C_SDA(1);
		else
			YWGPIO_Write(SDA, 0); //I2C_SDA(0);
		iic_delay(1);
		YWGPIO_Write(SCL, 1); //I2C_SCL(1);
		iic_delay(5);
		//I2C_SCL(0);

		data <<= 1;
	}

	/*
	* Look for an <ACK>(negative logic) and return it.
	*/
	YWGPIO_SetIoMode(SDA, YWGPIO_IO_MODE_BIDIRECTIONAL);
	YWGPIO_Write(SCL, 0); //I2C_SCL(0);
	iic_delay(2);
	YWGPIO_Write(SDA, 1); //I2C_SDA(1);
	iic_delay(2);
	YWGPIO_Write(SCL, 1); //I2C_SCL(1);
	YWGPIO_SetIoMode(SDA, YWGPIO_IO_MODE_INPUT);
	iic_delay(3);
	YWGPIO_Read(SDA, &nack);    //nack = I2C_READ();

	YWGPIO_SetIoMode(SDA, YWGPIO_IO_MODE_BIDIRECTIONAL);

	YWGPIO_Write(SCL, 0); //2C_SCL(0);
	iic_delay(2);

	return (nack);	/* not a nack is an ack */
}

static U8 i2csoft_readbyte(YWGPIO_Handle_T SCL, YWGPIO_Handle_T SDA, int ack)
{
	U8  data;
	int  j;
	U8  Value;

	data = 0;
	YWGPIO_SetIoMode(SDA, YWGPIO_IO_MODE_BIDIRECTIONAL);
	YWGPIO_Write(SDA, 1); //I2C_SDA(1);

	YWGPIO_SetIoMode(SDA, YWGPIO_IO_MODE_INPUT);
	//printk("!!!!:");
	for (j = 0; j < 8; j++)
	{
		iic_delay(1);
		YWGPIO_Write(SCL, 0); //I2C_SCL(0);
		iic_delay(5);
		YWGPIO_Write(SCL, 1); //I2C_SCL(1);
		iic_delay(3);
		//Value = I2C_READ();
		YWGPIO_Read(SDA, &Value);
		iic_delay(2);
		data = data << 1;
		data = data | Value;
		//printk("%d ",Value);
		//iic_delay(100000);

		//I2C_SCL(0);
	}
	//printk("\n");
	YWGPIO_SetIoMode(SDA, YWGPIO_IO_MODE_BIDIRECTIONAL);
	YWGPIO_Write(SCL, 0); //I2C_SCL(0);
	iic_delay(2);

	//YWGPIO_SetIoMode(SDA, YWGPIO_IO_MODE_OUTPUT);

	i2csoft_sendack(SCL, SDA, ack);
	return (data);
}

static int  i2c_gpio_read(U8 SlaveAddr, U8 *buffer, int len, int timeout, YWI2CSoft_Device_t *I2CSoftDevice)
{
	//int shift;
	//int ack;
	int i;
#if 0
	while (timeout--)
	{
		i2csoft_sendstart(I2CSoftDevice->SCLHandle, I2CSoftDevice->SDAHandle);
		//write_byte(SoftI2c.SlaveAddr +1);	/* send cycle */

		//if(write_byte(SlaveAddr+1)) //ACK
		if (!i2csoft_writebyte(I2CSoftDevice->SCLHandle, I2CSoftDevice->SDAHandle, SlaveAddr + 1)) //ACK
		{
			break;
		}

		i2csoft_sendstop(I2CSoftDevice->SCLHandle, I2CSoftDevice->SDAHandle);
		iic_delay(10);
	}

	if (timeout == 0)
	{
		printk("read error\n");
		return (-1);
	}
#endif
	i2csoft_sendstart(I2CSoftDevice->SCLHandle, I2CSoftDevice->SDAHandle);
	i2csoft_writebyte(I2CSoftDevice->SCLHandle, I2CSoftDevice->SDAHandle, SlaveAddr + 1);

	//printk("len = %d :",len);
	for (i = 0; i < (len - 1); i++)
	{
		buffer[i] = i2csoft_readbyte(I2CSoftDevice->SCLHandle, I2CSoftDevice->SDAHandle, 0);
		//printk("0x%x ",buffer[i]);
	}
	buffer[len - 1] = i2csoft_readbyte(I2CSoftDevice->SCLHandle, I2CSoftDevice->SDAHandle, 1);
	//printk("0x%x ",buffer[len-1]);
	//printk("\n");

	i2csoft_sendstop(I2CSoftDevice->SCLHandle, I2CSoftDevice->SDAHandle);
	return (0);
}

int i2c_gpio_write(U8 SlaveAddr, U8 *buffer, int len, int timeout, YWI2CSoft_Device_t *I2CSoftDevice)
{
	int failures = 0;

	i2csoft_sendstart(I2CSoftDevice->SCLHandle, I2CSoftDevice->SDAHandle);
	//if(write_byte(SoftI2c.SlaveAddr)) //send slave addr receive ack
	//printk("0x%x ", SlaveAddr);
	if (i2csoft_writebyte(I2CSoftDevice->SCLHandle, I2CSoftDevice->SDAHandle, SlaveAddr)) //send slave addr receive ack
	{
		/* write cycle */
		//send_stop();
		i2csoft_sendstop(I2CSoftDevice->SCLHandle, I2CSoftDevice->SDAHandle);
		YWI2C_DEBUG(("i2c_write, no chip responded \n"));
		return (-1);
	}

	while (len-- > 0)
	{
		//printk("0x%x ", *buffer++);
		if (i2csoft_writebyte(I2CSoftDevice->SCLHandle, I2CSoftDevice->SDAHandle, *buffer++))
		{
			failures++;
		}
	}
	//printk("\n");

	i2csoft_sendstop(I2CSoftDevice->SCLHandle, I2CSoftDevice->SDAHandle);

	return (failures);
}

static int  i2c_gpio_readnostop(U8 SlaveAddr, U8 *buffer, int len, int timeout, YWI2CSoft_Device_t *I2CSoftDevice)
{
	//int shift;
	//int ack;
	int i;
#if 0
	while (timeout--)
	{
		i2csoft_sendstart(I2CSoftDevice->SCLHandle, I2CSoftDevice->SDAHandle);
		//write_byte(SoftI2c.SlaveAddr +1);	/* send cycle */

		//if(write_byte(SlaveAddr+1)) //ACK
		if (!i2csoft_writebyte(I2CSoftDevice->SCLHandle, I2CSoftDevice->SDAHandle, SlaveAddr + 1)) //ACK
		{
			break;
		}

		i2csoft_sendstop(I2CSoftDevice->SCLHandle, I2CSoftDevice->SDAHandle);
		iic_delay(10);
	}

	if (timeout == 0)
	{
		printk("read error\n");
		return (-1);
	}
#endif
	i2csoft_sendstart(I2CSoftDevice->SCLHandle, I2CSoftDevice->SDAHandle);
	i2csoft_writebyte(I2CSoftDevice->SCLHandle, I2CSoftDevice->SDAHandle, SlaveAddr + 1);

	//printk("len = %d :",len);
	for (i = 0; i < (len - 1); i++)
	{
		buffer[i] = i2csoft_readbyte(I2CSoftDevice->SCLHandle, I2CSoftDevice->SDAHandle, 0);
		//printk("0x%x ",buffer[i]);
	}
	buffer[len - 1] = i2csoft_readbyte(I2CSoftDevice->SCLHandle, I2CSoftDevice->SDAHandle, 1);
	//printk("0x%x ",buffer[len-1]);
	//printk("\n");

	//i2csoft_sendstop(I2CSoftDevice->SCLHandle, I2CSoftDevice->SDAHandle);
	return (0);
}

int  i2c_gpio_writenostop(U8 SlaveAddr, U8 *buffer, int len, int timeout, YWI2CSoft_Device_t *I2CSoftDevice)
{
	int failures = 0;

	i2csoft_sendstart(I2CSoftDevice->SCLHandle, I2CSoftDevice->SDAHandle);
	//if(write_byte(SoftI2c.SlaveAddr)) //send slave addr receive ack
	if (i2csoft_writebyte(I2CSoftDevice->SCLHandle, I2CSoftDevice->SDAHandle, SlaveAddr)) //send slave addr receive ack
	{
		/* write cycle */
		//send_stop();
		i2csoft_sendstop(I2CSoftDevice->SCLHandle, I2CSoftDevice->SDAHandle);
		YWI2C_DEBUG(("i2c_write, no chip responded \n"));
		return (1);
	}

	while (len-- > 0)
	{
		if (i2csoft_writebyte(I2CSoftDevice->SCLHandle, I2CSoftDevice->SDAHandle, *buffer++))
		{
			failures++;
		}
	}

	//i2csoft_sendstop(I2CSoftDevice->SCLHandle,I2CSoftDevice->SDAHandle);

	return (failures);
}

YW_ErrorType_T i2c_soft_init(char *DeviceName, YWI2cSoft_InitParam_t *InitParam)
{
	int                     i;
	YWGPIO_OpenParams_T     GpioOpenParams;
	YW_ErrorType_T          ErrorType = YW_NO_ERROR;

	YWI2C_INTERFACE(("%s line:%d in\n", __FUNCTION__, __LINE__));
	if (DeviceName == NULL || InitParam == NULL)
	{
		return YWHAL_ERROR_BAD_PARAMETER;
	}

	if (IsI2CAlreadyInitialised(DeviceName))
	{
		return YWHAL_ERROR_ALREADY_INITIALIZED;
	}

	for (i = 0; i < YWI2CSOFT_MAX_DEVICE; i++)
	{
		if (I2CSoftDevice[i].IsInit == FALSE)
		{
			break;
		}
	}

	if (i == YWI2CSOFT_MAX_DEVICE)
	{
		return YWHAL_ERROR_NOT_ENOUGH_DEVICE;
	}

	YWLIB_Memset(&(I2CSoftDevice[i]), 0, sizeof(YWI2CSoft_Device_t));
	YWLIB_Strcpy((S8 *)I2CSoftDevice[i].I2CName, (S8 *)DeviceName);

	GpioOpenParams.GpioIndex    = InitParam->SCLPioIndex;
	GpioOpenParams.IoMode       = YWGPIO_IO_MODE_OUTPUT;
	GpioOpenParams.WorkMode     = YWGPIO_WORK_MODE_PRIMARY;
	GpioOpenParams.PrivateData  = NULL;
	ErrorType = YWGPIO_Open(&(I2CSoftDevice[i].SCLHandle), &GpioOpenParams);
	ErrorType |= YWGPIO_Write(I2CSoftDevice[i].SCLHandle, 1);
	if (ErrorType != YW_NO_ERROR)
	{
		YWI2C_DEBUG(("%s,%d\n", __FILE__, __LINE__));
		return ErrorType;
	}

	GpioOpenParams.GpioIndex    = InitParam->SDAPioIndex;
	GpioOpenParams.IoMode       = YWGPIO_IO_MODE_OUTPUT;
	GpioOpenParams.WorkMode     = YWGPIO_WORK_MODE_PRIMARY;
	GpioOpenParams.PrivateData  = NULL;
	ErrorType = YWGPIO_Open(&(I2CSoftDevice[i].SDAHandle), &GpioOpenParams);
	ErrorType |= YWGPIO_Write(I2CSoftDevice[i].SDAHandle, 1);
	if (ErrorType != YW_NO_ERROR)
	{
		YWI2C_DEBUG(("%s,%d\n", __FILE__, __LINE__));

		YWGPIO_Close(I2CSoftDevice[i].SCLHandle);
		return ErrorType;
	}

	ErrorType = YWOS_SemaphoreCreate((S8 *)"SOFTI2C_LOCK", 1, &I2CSoftDevice[i].SoftI2cLock);
	if (ErrorType != YW_NO_ERROR)
	{
		YWGPIO_Close(I2CSoftDevice[i].SDAHandle);
		YWGPIO_Close(I2CSoftDevice[i].SCLHandle);
		return ErrorType;
	}

	I2CSoftDevice[i].Speed      = InitParam->Speed;
	I2CSoftDevice[i].IsInit     = TRUE;


	i2csoft_lock(i);
	i2csoft_reset(I2CSoftDevice[i].SCLHandle, I2CSoftDevice[i].SDAHandle);
	i2csoft_unlock(i);

	YWI2C_INTERFACE(("%s line:%d OUT\n", __FUNCTION__, __LINE__));

	return YW_NO_ERROR;
}


YW_ErrorType_T i2c_soft_term(char *DeviceName)
{
	int                     i;
	YW_ErrorType_T          ErrorType = YW_NO_ERROR;
	YWI2C_INTERFACE(("%s line:%d in\n", __FUNCTION__, __LINE__));

	if (DeviceName == NULL)
	{
		return YWHAL_ERROR_BAD_PARAMETER;
	}

	if (IsI2CAlreadyInitialised(DeviceName) == FALSE)
	{
		return YWHAL_ERROR_ALREADY_INITIALIZED;
	}

	for (i = 0; i < YWI2CSOFT_MAX_DEVICE; i++) //find device
	{
		if (I2CSoftDevice[i].IsInit == TRUE)
		{
			break;
		}
	}

	if (i == YWI2CSOFT_MAX_DEVICE)
	{
		return YWHAL_ERROR_NOT_ENOUGH_DEVICE;
	}

	ErrorType = YWGPIO_Close(I2CSoftDevice[i].SDAHandle);
	ErrorType |= YWGPIO_Close(I2CSoftDevice[i].SCLHandle);
	ErrorType |= YWOS_SemaphoreDelete(I2CSoftDevice[i].SoftI2cLock);


	YWLIB_Memset(&(I2CSoftDevice[i]), 0, sizeof(YWI2CSoft_Device_t));
	YWI2C_INTERFACE(("%s line:%d OUT\n", __FUNCTION__, __LINE__));

	return YW_NO_ERROR;
}

YW_ErrorType_T i2c_soft_open(char *DeviceName, YWI2CSoft_Handle_t *Handle, YWI2cSoft_OpenParams_t *OpenParam)
{
	int                     i, j;
//    YW_ErrorType_T          ErrorType = YW_NO_ERROR;
	YWI2C_INTERFACE(("%s line:%d in\n", __FUNCTION__, __LINE__));

	if (DeviceName == NULL || OpenParam == NULL)
	{
		return YWHAL_ERROR_BAD_PARAMETER;
	}

	if (IsI2CAlreadyInitialised(DeviceName) == FALSE)
	{
		return YWHAL_ERROR_ALREADY_INITIALIZED;
	}

	for (i = 0; i < YWI2CSOFT_MAX_DEVICE; i++) //find device
	{
		if ((I2CSoftDevice[i].IsInit) && (YWLIB_Strcmp((S8 *)DeviceName, (S8 *)I2CSoftDevice[i].I2CName) == 0))
		{
			break;
		}
	}

	if (i == YWI2CSOFT_MAX_DEVICE)
	{
		return YWHAL_ERROR_NOT_ENOUGH_DEVICE;
	}


	for (j = 0; j < YWI2CSOFT_MAX_HANDLE; j++)
	{
		if (I2CSoftDevice[i].OpenParam[j].IsOpen == FALSE)
		{
			break;
		}
	}

	if (i == YWI2CSOFT_MAX_HANDLE)
	{
		return YWHAL_ERROR_FEATURE_NOT_SUPPORTED;
	}

	YWOS_SemaphoreWait(I2CSoftDevice[i].SoftI2cLock, YWOS_WAIT_INFINITY);

	I2CSoftDevice[i].OpenParam[j].SlaveAddr = OpenParam->I2cAddress;
	I2CSoftDevice[i].OpenParam[j].IsOpen    = TRUE;
	*Handle                                 = (YWI2CSoft_Handle_t)(&I2CSoftDevice[i].OpenParam[j]);
	YWOS_SemaphoreSend(I2CSoftDevice[i].SoftI2cLock);
	YWI2C_INTERFACE(("%s line:%d OUT\n", __FUNCTION__, __LINE__));

	return YW_NO_ERROR;

}

YW_ErrorType_T i2c_soft_close(YWI2CSoft_Handle_t Handle)
{
	//YW_ErrorType_T          ErrorType = YW_NO_ERROR;
	U32                     DeviceIndex, HandleIndex;
	YWI2CSoft_OpenParam_t   *Param = (YWI2CSoft_OpenParam_t *)Handle;
	YWI2C_INTERFACE(("%s line:%d in\n", __FUNCTION__, __LINE__));


	if (!CheckI2cParam(Handle, &DeviceIndex, &HandleIndex))
	{
		return YWHAL_ERROR_INVALID_HANDLE;
	}

	if (!Param->IsOpen)
	{
		return YWHAL_ERROR_INVALID_HANDLE;
	}

	i2csoft_lock(DeviceIndex);

	I2CSoftDevice[DeviceIndex].OpenParam[HandleIndex].IsOpen = FALSE;

	i2csoft_unlock(DeviceIndex);

	YWI2C_INTERFACE(("%s line:%d OUT\n", __FUNCTION__, __LINE__));

	return YW_NO_ERROR;
}

YW_ErrorType_T i2c_soft_read(YWI2CSoft_Handle_t Handle,
			     U8              *Buffer_p,
			     U32             MaxLen,
			     U32             Timeout,
			     U32             *ActLen_p)
{
	int                     ret;
	//YW_ErrorType_T          ErrorType = YW_NO_ERROR;
	U32                     DeviceIndex, HandleIndex;
	YWI2CSoft_OpenParam_t   *Param = (YWI2CSoft_OpenParam_t *)Handle;

	YWI2C_INTERFACE(("%s line:%d in\n", __FUNCTION__, __LINE__));
	if (!CheckI2cParam(Handle, &DeviceIndex, &HandleIndex))
	{
		YWI2C_DEBUG(("%s %d\n", __FUNCTION__, __LINE__));
		return YWHAL_ERROR_INVALID_HANDLE;
	}

	if (!Param->IsOpen)
	{
		YWI2C_DEBUG(("%s %d\n", __FUNCTION__, __LINE__));
		return YWHAL_ERROR_INVALID_HANDLE;
	}

	i2csoft_lock(DeviceIndex);

	ret = i2c_gpio_read(Param->SlaveAddr, Buffer_p, MaxLen, Timeout, &(I2CSoftDevice[DeviceIndex]));

	i2csoft_unlock(DeviceIndex);

	*ActLen_p = ret;
	YWI2C_INTERFACE(("%s line:%d OUT\n", __FUNCTION__, __LINE__));


	return YW_NO_ERROR;

}

YW_ErrorType_T i2c_soft_write(YWI2CSoft_Handle_t Handle,
			      U8              *Buffer_p,
			      U32             MaxLen,
			      U32             Timeout,
			      U32             *ActLen_p)
{
	int                     ret;
	//YW_ErrorType_T          ErrorType = YW_NO_ERROR;
	U32                     DeviceIndex, HandleIndex;
	YWI2CSoft_OpenParam_t   *Param = (YWI2CSoft_OpenParam_t *)Handle;

	YWI2C_INTERFACE(("%s line:%d in\n", __FUNCTION__, __LINE__));

	if (!CheckI2cParam(Handle, &DeviceIndex, &HandleIndex))
	{
		YWI2C_DEBUG(("%s %d\n", __FUNCTION__, __LINE__));
		return YWHAL_ERROR_INVALID_HANDLE;
	}

	if (!Param->IsOpen)
	{
		YWI2C_DEBUG(("%s %d\n", __FUNCTION__, __LINE__));
		return YWHAL_ERROR_INVALID_HANDLE;
	}

	i2csoft_lock(DeviceIndex);
#if 0
	{
		int i;
		for (i = 0; i < MaxLen; i++)
		{
			printk("0x%x ", Buffer_p[i]);
		}
		printk("\n");
	}
#endif  /* 0 */

	ret = i2c_gpio_write(Param->SlaveAddr, Buffer_p, MaxLen, Timeout, &(I2CSoftDevice[DeviceIndex]));

	i2csoft_unlock(DeviceIndex);

	if (ret < 0)
	{
		return YWHAL_ERROR_FEATURE_NOT_SUPPORTED;
	}
	*ActLen_p = MaxLen - ret;

	YWI2C_INTERFACE(("%s line:%d OUT\n", __FUNCTION__, __LINE__));
	return YW_NO_ERROR;

}

YW_ErrorType_T i2c_soft_readnostop(YWI2CSoft_Handle_t Handle,
				   U8              *Buffer_p,
				   U32             MaxLen,
				   U32             Timeout,
				   U32             *ActLen_p)
{
	int                     ret;
	//YW_ErrorType_T          ErrorType = YW_NO_ERROR;
	U32                     DeviceIndex, HandleIndex;
	YWI2CSoft_OpenParam_t   *Param = (YWI2CSoft_OpenParam_t *)Handle;

	YWI2C_INTERFACE(("%s line:%d in\n", __FUNCTION__, __LINE__));
	if (!CheckI2cParam(Handle, &DeviceIndex, &HandleIndex))
	{
		YWI2C_DEBUG(("%s %d\n", __FUNCTION__, __LINE__));
		return YWHAL_ERROR_INVALID_HANDLE;
	}

	if (!Param->IsOpen)
	{
		YWI2C_DEBUG(("%s %d\n", __FUNCTION__, __LINE__));
		return YWHAL_ERROR_INVALID_HANDLE;
	}

	i2csoft_lock(DeviceIndex);

	ret = i2c_gpio_readnostop(Param->SlaveAddr, Buffer_p, MaxLen, Timeout, &(I2CSoftDevice[DeviceIndex]));

	i2csoft_unlock(DeviceIndex);

	*ActLen_p = ret;
	YWI2C_INTERFACE(("%s line:%d OUT\n", __FUNCTION__, __LINE__));


	return YW_NO_ERROR;
}

YW_ErrorType_T i2c_soft_writenostop(YWI2CSoft_Handle_t Handle,
				    U8              *Buffer_p,
				    U32             MaxLen,
				    U32             Timeout,
				    U32             *ActLen_p)
{
	int                     ret;
	//YW_ErrorType_T          ErrorType = YW_NO_ERROR;
	U32                     DeviceIndex, HandleIndex;
	YWI2CSoft_OpenParam_t   *Param = (YWI2CSoft_OpenParam_t *)Handle;

	YWI2C_INTERFACE(("%s line:%d in\n", __FUNCTION__, __LINE__));

	if (!CheckI2cParam(Handle, &DeviceIndex, &HandleIndex))
	{
		YWI2C_DEBUG(("%s %d\n", __FUNCTION__, __LINE__));
		return YWHAL_ERROR_INVALID_HANDLE;
	}

	if (!Param->IsOpen)
	{
		YWI2C_DEBUG(("%s %d\n", __FUNCTION__, __LINE__));
		return YWHAL_ERROR_INVALID_HANDLE;
	}

	i2csoft_lock(DeviceIndex);
	//printk("Param->SlaveAddr=0x%x DeviceIndex=0x%x\n",Param->SlaveAddr,DeviceIndex);
	ret = i2c_gpio_writenostop(Param->SlaveAddr, Buffer_p, MaxLen, Timeout, &(I2CSoftDevice[DeviceIndex]));

	i2csoft_unlock(DeviceIndex);

	*ActLen_p = ret;

	YWI2C_INTERFACE(("%s line:%d OUT\n", __FUNCTION__, __LINE__));
	return YW_NO_ERROR;
	return YW_NO_ERROR;
}

I2CDeviceName_T         SoftI2c_DeviceName[YWI2C_NUM_SOFT_I2C] = {"SOFT_I2C0"};

int softi2c_init(void)
{
	int ret = 0;
	YW_ErrorType_T              YW_ErrorCode = YW_NO_ERROR;

	//printk("%s >\n", __func__);
	{
		YWI2cSoft_InitParam_t InitParam;

		InitParam.IsSlaveDevice = FALSE;
		InitParam.SCLPioIndex   = GPIO_SIMULATE_I2C_SCL_PORT * PIO_BITS +
					  GPIO_SIMULATE_I2C_SCL_BIT;
		InitParam.SDAPioIndex   = GPIO_SIMULATE_I2C_SDA_PORT * PIO_BITS +
					  GPIO_SIMULATE_I2C_SDA_BIT;
		InitParam.Speed         = 100000;

		YWI2C_INTERFACE(("InitParam.SCLPioIndex = %d\n", InitParam.SCLPioIndex));
		YWI2C_INTERFACE(("InitParam.SDAPioIndex = %d\n", InitParam.SDAPioIndex));

		YW_ErrorCode = i2c_soft_init(SoftI2c_DeviceName[0], &InitParam);
		if (YW_ErrorCode != YW_NO_ERROR)
		{
			YWOSTRACE(("[ERROR][YWI2C_Init]init soft failed\n"));
			return YW_ErrorCode;
		}
	}

	{
		YWI2CSoft_Handle_t          SoftHandle;
		YWI2cSoft_OpenParams_t      OpenParam;

		OpenParam.I2cAddress        = 0x50;
		YW_ErrorCode = i2c_soft_open(SoftI2c_DeviceName[0], &SoftHandle, &OpenParam);
		//printk("SoftHandle = 0x%x\n", (int)SoftHandle);
		if (YW_ErrorCode == YW_NO_ERROR)
		{
			g_SoftHandle = SoftHandle;
		}
		else
		{
			YWOSTRACE(("[ERROR][YWI2C_Open]I2C Open[%d] failed ! Error %d\n",
				   0, YW_ErrorCode));
			return (YW_ErrorCode);
		}

	}

#if 0
	{
		U32 ActLen = 0;
		U8 aBuffer[2] = { 0xff, 0xff};

		i2c_soft_write(g_SoftHandle, aBuffer, 2, 100, &ActLen);
		printk("ActLen = %d\n", ActLen);
	}
#endif  /* 0 */

	//printk("%s < %d\n", __func__, ret);
	return ret;
}

int isofti2c_write(U8 *Buffer_p, U32 MaxLen)
{
	U32 ActLen = 0;
	YW_ErrorType_T	errType = YW_NO_ERROR;

	i2c_soft_write(g_SoftHandle, Buffer_p, MaxLen, 100, &ActLen);
	//printk("ActLen = %d\n", ActLen);
	if (errType != YW_NO_ERROR)
	{
		return FALSE;
	}
	if (ActLen != MaxLen)
	{
		return FALSE;
	}
	return TRUE;
}

int isofti2c_read(U8 *Buffer_p, U32 MaxLen)
{
	U32 ActLen = 0;

	i2c_soft_read(g_SoftHandle, Buffer_p, MaxLen, 100, &ActLen);

	return TRUE;
}

int softi2c_online(void)
{
	int bRet = FALSE;
	U8 aBuffer[2] = { 0xff, 0xff};

	bRet = isofti2c_write(aBuffer, 2);

	return bRet;
}

void softi2c_cleanup(void)
{
	YW_ErrorType_T              YW_ErrorCode = YW_NO_ERROR;

	//printk("%s >\n", __func__);
	{
		YW_ErrorCode = i2c_soft_close(g_SoftHandle);
	}
	{
		YW_ErrorCode = i2c_soft_term(SoftI2c_DeviceName[0]);
		if (YW_ErrorCode != YW_NO_ERROR)
		{
			YWOSTRACE(("[ERROR][YWI2C_Term] YWI2C_Term failed ! Error\n"));
		}
	}
	//printk("%s <\n", __func__);
}

#endif /*CONFIG_CPU_SUBTYPE_STX7105*/
