#ifndef __AOTOM_I2CSOFT_H__
#define __AOTOM_I2CSOFT_H__

typedef unsigned int YWI2CSoft_Handle_t;

typedef struct  YWI2cSoft_InitParam_s
{
	int IsSlaveDevice;
	unsigned int SCLPioIndex;
	unsigned int SDAPioIndex;
	unsigned int Speed;
} YWI2cSoft_InitParam_t;

typedef struct YWI2cSoft_OpenParams_s
{
	unsigned short I2cAddress;
} YWI2cSoft_OpenParams_t;

int  softi2c_init(void);
void softi2c_cleanup(void);
int softi2c_online(void);
int isofti2c_write(unsigned char *Buffer_p, u32 MaxLen);
int isofti2c_read(unsigned char *Buffer_p, u32 MaxLen);

#endif /* __AOTOM_I2CSOFT_H__ */
