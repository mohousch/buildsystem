/*
 * @brief lnbh221.c
 *
 * @author konfetti
 *
 * Copyright (C) 2011 duckbox
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "dvb_frontend.h"
#include "equipment.h"

short paramDebug = 0;  // debug print level is zero as default (0=nothing, 1= errors, 10=some detail, 20=more detail, 50=open/close functions, 100=all)
#define TAGDEBUG "[lnbh221] "

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
	u32 lnb[6];
};

/*
 * LNBH221 write function:
 * i2c bus 0 and 1, addr 0x08
 * seen values: 0xd4 (vertical) 0xdc (horizontal), 0xd0 shutdown
 * 0xd8 ??? fixme ->was genau heisst das, nochmal lesen besonders
 * mit den "is controlled by dsqin pin" ...
 *
 * From Documentation (can be downloaded on st.com):

PCL TTX TEN LLC VSEL EN OTF OLF Function
             0    0   1  X    X V OUT=13.25V, VUP=15.25V
             0    1   1  X    X VOUT=18V, VUP=20V
             1    0   1  X    X VOUT=14.25V, VUP=16.25V
             1    1   1  X    X VOUT=19.5V, VUP=21.5V
          0           1  X    X 22KHz tone is controlled by DSQIN pin
          1           1  X    X 22KHz tone is ON, DSQIN pin disabled
       0              1  X    X VORX output is ON, output voltage controlled by VSEL and LLC
       1              1  X    X VOTX output is ON, 22KHz controlled by DSQIN or TEN,
                                output voltage level controlled by VSEL and LLC
  0                   1  X    X Pulsed (dynamic) current limiting is selected
  1                   1  X    X Static current limiting is selected
  X    X   X X    X   0  X    X Power blocks disabled

 *
 */
static int writereg_lnb_supply(struct lnb_state *state, char data)
{
	int ret = -EREMOTEIO;
	struct i2c_msg msg;
	u8 buf;

	buf = data;

	msg.addr = state->lnb[1];
	msg.flags = 0;
	msg.buf = &buf;
	msg.len = 1;

	dprintk(100, "%s:  write 0x%02x to 0x%02x\n", __func__, data, msg.addr);

	if ((ret = i2c_transfer(state->i2c, &msg, 1)) != 1)
	{
		//wohl nicht LNBH23, mal mit LNBH221 versuchen
		msg.addr = state->lnb[2];
		msg.flags = 0;
		msg.buf = &buf;
		msg.len = 1;
		if ((ret = i2c_transfer(state->i2c, &msg, 1)) != 1)
		{
			printk("%s: writereg error(err == %i)\n", __func__, ret);
			ret = -EREMOTEIO;
		}
	}
	return ret;
}

u16 lnbh221_set_voltage(void *_state, struct dvb_frontend *fe, fe_sec_voltage_t voltage)
{
	struct lnb_state *state = (struct lnb_state *) _state;

	dprintk(10, "%s(%p, %d)\n", __func__, fe, voltage);

	switch (voltage)
	{
		case SEC_VOLTAGE_OFF:
		{
			dprintk(10, "Set LNB voltage off\n");
#if defined(IPBOX9900)
			if (_12v_isON == 0)
			{
				writereg_lnb_supply(state, state->lnb[3]);
			}
#else
			writereg_lnb_supply(state, state->lnb[3]);
#endif
			break;
		}
		case SEC_VOLTAGE_13:  // vertical
		{
			dprintk(10, "Set LNB voltage vertical\n");
			writereg_lnb_supply(state, state->lnb[4]);
			break;
		}
		case SEC_VOLTAGE_18:  // horizontal
		{
			dprintk(10, "Set LNB voltage horizontal\n");
			writereg_lnb_supply(state, state->lnb[5]);
			break;
		}
		default:
		{
			return -EINVAL;
		}
	}
	return 0;
}

void *lnbh221_attach(u32 *lnb, struct equipment_s *equipment)
{
	struct lnb_state *state = kmalloc(sizeof(struct lnb_state), GFP_KERNEL);

	memcpy(state->lnb, lnb, sizeof(state->lnb));
	equipment->lnb_set_voltage = lnbh221_set_voltage;
	state->i2c = i2c_get_adapter(lnb[0]);
	dprintk(10, "i2c adapter = %p\n", state->i2c);
	return state;
}
EXPORT_SYMBOL(lnbh221_attach);

/* ******************************* */
/* module functions                */
/* ******************************* */

int __init lnbh221_init(void)
{
	dprintk(100, "%s >\n", __func__);
	return 0;
}

static void lnbh221_cleanup(void)
{
	dprintk(100, "%s >\n", __func__);
}

module_param(paramDebug, short, 0644);
MODULE_PARM_DESC(paramDebug, "Enable logging (default : 0 = no logging)");

MODULE_DESCRIPTION("LNBH221 Handling");

MODULE_AUTHOR("konfetti");
MODULE_LICENSE("GPL");

module_init(lnbh221_init);
module_exit(lnbh221_cleanup);
// vim:ts=4
