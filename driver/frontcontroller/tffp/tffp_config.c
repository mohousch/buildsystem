#include <linux/i2c.h>
#include <linux/i2c-algo-bit.h>
#include <linux/crc16.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include "tffp_config.h"

#define EEPROM_I2C_BUS 1
#define EEPROM_ADDR (0xa0 >> 1)
#define TFFP_CFG_OFFSET 0x80
#define TFFP_CFG_VERSION 1
/* the chips supports writes for up to 8 bytes */
#define EEPROM_BURST_SIZE 8
#define EEPROM_MAX_RETRIES 10

static tTffpConfig defaultConfig =
{
	.crc              =   0,
	.gmtOffset        =   -1,
	.version          =   1,
	.brightness       =   3,
	.irFilter1        =   1,
	.irFilter2        =   0,
	.irFilter3        =   0,
	.irFilter4        =   1,
	.allCaps          =   1,
	.typematicDelay   =   3,
	.typematicRate    =   1,
	.scrollMode       =   1,
	.scrollPause      = 100,
	.scrollDelay      =  10
};

int readEepromByte(u8 offset, u8 *pData)
{
	struct i2c_msg msg[] =
	{
		{.addr = EEPROM_ADDR, .flags = 0, .buf = &offset, .len = 1},
		{.addr = EEPROM_ADDR, .flags = I2C_M_RD, .buf = pData, .len = 1}
	};
	struct i2c_adapter *i2c_adap = i2c_get_adapter(EEPROM_I2C_BUS);

	if (i2c_transfer(i2c_adap, msg, 2) != 2)
		return -1;

	return 1;
}


int readEepromData(u8 offset, short length, u8 *pData)
{
	struct i2c_msg msg[] =
	{
		{.addr = EEPROM_ADDR, .flags = 0, .buf = &offset, .len = 1},
		{.addr = EEPROM_ADDR, .flags = I2C_M_RD, .buf = pData, .len = length}
	};
	struct i2c_adapter *i2c_adap = i2c_get_adapter(EEPROM_I2C_BUS);

	if (i2c_transfer(i2c_adap, msg, 2) != 2)
		return -1;

	return length;
}

int writeEepromData(u8 offset, short length, u8 *pData)
{
	u8 buf[260];
	struct i2c_msg msg = {.addr = EEPROM_ADDR, .flags = 0};
	struct i2c_adapter *i2c_adap = i2c_get_adapter(EEPROM_I2C_BUS);
	int i;

	if (length > 256)
		length = 256;

	memcpy(buf + 1, pData, length);

	for (i = 0; i < length; i += EEPROM_BURST_SIZE)
	{
		int j = 0;
		msg.len = 1 + (((length - i) > EEPROM_BURST_SIZE) ? EEPROM_BURST_SIZE : (length - i));
		buf[i] = offset + i;
		msg.buf = buf + i;
		while (i2c_transfer(i2c_adap, &msg, 1) != 1)
		{
			j++;
			if (j > EEPROM_MAX_RETRIES)
				return -1;
		}
	}

	return length;
}


int readTffpConfig(tTffpConfig *pCfg)
{
	int rc = 0;

	if (readEepromData(TFFP_CFG_OFFSET, sizeof(*pCfg), (char *)pCfg) < 0)
	{
		printk("failed to read EEPROM\n");
		rc = -1;
	}
	else if (pCfg->version != TFFP_CFG_VERSION)
	{
		printk("invalid TFFP config version\n");
		rc = -1;
	}
	else if (crc16(0, (char *)pCfg + 2, sizeof(*pCfg) - 2) != pCfg->crc)
	{
		printk("invalid TFFP config CRC\n");
		rc = -1;
	}

	if (rc != 0)
		*pCfg = defaultConfig;

	return rc;
}

int writeTffpConfig(tTffpConfig *pCfg)
{
	int rc = 0;

	pCfg->crc = crc16(0, (char *)pCfg + 2, sizeof(*pCfg) - 2);
	pCfg->version = TFFP_CFG_VERSION;

	if (writeEepromData(TFFP_CFG_OFFSET, sizeof(*pCfg), (char *)pCfg) < 0)
	{
		printk("failed to write EEPROM\n");
		rc = -1;
	}

	return rc;
}
