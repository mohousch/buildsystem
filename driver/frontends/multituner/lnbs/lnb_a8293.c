/*
 * a8293.c - Allegro A8293 LNB controller driver
 *
 */

#include <asm/io.h>

#include "dvb_frontend.h"
#include "equipment.h"

short paramDebug = 0;  // debug print level is zero as default (0=nothing, 1= errors, 10=some detail, 20=more detail, 50=open/close functions, 100=all)
#define TAGDEBUG "[a8293] "

#define dprintk(level, x...) do \
{ \
	if ((paramDebug) && (paramDebug >= level) || level == 0) \
	{ \
		printk(TAGDEBUG x); \
	} \
} while (0)

#if defined(IPBOX9900)
extern int _12v_isON;  // defined in e2_proc ->I will implement a better mechanism later
#endif

struct lnb_state
{
	struct i2c_adapter *i2c;
	u32                lnb[6];
};

unsigned char a8293_read(struct lnb_state *state)
{
	u8 buf[2];
	struct i2c_msg msg;

	msg.addr  = state->lnb[1];
	msg.flags = I2C_M_RD;
	msg.buf   = buf;
	msg.len   = 1;

	if (i2c_transfer(state->i2c, &msg, 1) != 1)
	{
		printk("%s: a8293_read error\n", __FUNCTION__);
		buf[0] = 0x00;
	}
	return buf[0];
}

static int a8293_write(struct lnb_state *state, unsigned char data)
{
	int ret = -EREMOTEIO;
	struct i2c_msg msg;
	u8 buf;

	buf = data;

	msg.addr  = state->lnb[1];
	msg.flags = 0;
	msg.buf   = &buf;
	msg.len   = 1;

	dprintk(100, "%s: write 0x%02x to 0x%02x\n", __FUNCTION__, data, msg.addr);

	if ((ret = i2c_transfer(state->i2c, &msg, 1)) != 1)
	{
		printk("%s: writereg error(err == %i)\n", __FUNCTION__, ret);
		ret = -EREMOTEIO;
	}
	return ret;
}

u16 a8293_set_voltage(void *_state, struct dvb_frontend *fe, fe_sec_voltage_t voltage)
{
	struct lnb_state *state = (struct lnb_state *) _state;
	unsigned char reg = state->lnb[5];

	dprintk(10, "%s (%x)\n", __func__, voltage);

	if (voltage != SEC_VOLTAGE_OFF)
	{
		reg |= (1 << 5);  /* lnb enable/disable */
	}
	switch (voltage)
	{
		case SEC_VOLTAGE_OFF:
		{
			dprintk(10, "Set LNB voltage off\n");
#if defined(IPBOX9900)
			if (_12v_isON == 0)
			{
				a8293_write(state, reg);
			}
#else
			a8293_write(state, reg);
#endif
			break;
		}
		case SEC_VOLTAGE_13:
		{
			dprintk(10, "Set LNB voltage vertical\n");
			reg |= state->lnb[3];
			a8293_write(state, reg);
			break;
		}
		case SEC_VOLTAGE_18:
		{
			dprintk(10, "Set LNB voltage horizontal\n");
			reg |= state->lnb[4];
			a8293_write(state, reg);
			break;
		}
		default:
		{
			return -EINVAL;
		}
	}
	return 0;
}

void *lnb_a8293_attach(u32 *lnb, struct equipment_s *equipment)
{
	unsigned char reg;
	int res;

	struct lnb_state *state = kmalloc(sizeof(struct lnb_state), GFP_KERNEL);
	memcpy(state->lnb, lnb, sizeof(state->lnb));
	equipment->lnb_set_voltage = a8293_set_voltage;
	state->i2c = i2c_get_adapter(lnb[0]);
	dprintk(10, "i2c adapter = %p\n", state->i2c);

//TODO: make it configurable

//#ifndef UFS913
	/* this read is necessarily needed, otherwise
	 * lnb power is not supplied!
	 */
	reg = a8293_read(state);
//#endif
	dprintk(1, "%s -> 0x%02X\n", __func__, reg);
	res = a8293_write(state, 0x83);
#ifndef UFS913
	if (res == 0)
	{
		/* setup pio6 */
		u32 reg = ctrl_inl(0xfd026030);

		dprintk(10, "%s: reg = 0x%08x\n", __func__, reg);
		reg |= 0x00000001;
		dprintk(10, "%s: reg = 0x%08x\n", __func__, reg);
		ctrl_outl(reg, 0xfd026030);
		reg = ctrl_inl(0xfd026000);
		dprintk(10, "%s: reg = 0x%08x\n", __func__, reg);
		reg &= ~(0x0000001);
		dprintk(10, "%s: reg = 0x%08x\n", __func__, reg);
		ctrl_outl(reg, 0xfd026000);
	}
#endif
	return state;
}
EXPORT_SYMBOL(lnb_a8293_attach);

/* ******************************* */
/* module functions                */
/* ******************************* */

int __init lnb_a8293_init(void)
{
	dprintk(100, "%s >\n", __func__);
	return 0;
}

static void lnb_a8293_cleanup(void)
{
	dprintk(100, "%s >\n", __func__);
}

module_param(paramDebug, short, 0644);
MODULE_PARM_DESC(paramDebug, "Enable logging (default : 0 = no logging)");

MODULE_DESCRIPTION("A8293 Handling");

MODULE_AUTHOR("");
MODULE_LICENSE("GPL");

module_init(lnb_a8293_init);
module_exit(lnb_a8293_cleanup);
// vim:ts=4
