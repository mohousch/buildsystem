/*
 * @brief lnb_pio.c
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

#include <linux/version.h>

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,17)
#include <linux/stm/pio.h>
#else
#include <linux/stpio.h>
#endif

#include "dvb_frontend.h"
#include "equipment.h"

short paramDebug = 0;  // debug print level is zero as default (0=nothing, 1= errors, 10=some detail, 20=more detail, 50=open/close functions, 100=all)
#define TAGDEBUG "[lnb_pio] "

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
	struct stpio_pin *lnb_pin;
	struct stpio_pin *lnb_enable_pin;
	u32              lnb[6];
};

u16 lnb_pio_set_voltage(void *_state, struct dvb_frontend *fe, fe_sec_voltage_t voltage)
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
				if (state->lnb_enable_pin)
				{
					stpio_set_pin(state->lnb_enable_pin, !state->lnb[2]);
				}
			}
#else
			stpio_set_pin (state->lnb_enable_pin, !state->lnb[2]);
#endif
			break;
		}
		case SEC_VOLTAGE_13:  // vertical
		{
			dprintk(10, "Set LNB voltage vertical\n");
			if (state->lnb_enable_pin)
			{
				stpio_set_pin(state->lnb_enable_pin, state->lnb[2]);
			}
			stpio_set_pin(state->lnb_pin, state->lnb[5]);
			dprintk(10, "%s: v %p %d\n", __func__, state->lnb_pin, state->lnb[5]);
			break;
		}
		case SEC_VOLTAGE_18:  // horizontal
		{
			dprintk(10, "Set LNB voltage horizontal\n");
			if (state->lnb_enable_pin)
			{
				stpio_set_pin(state->lnb_enable_pin, state->lnb[2]);
			}
			stpio_set_pin(state->lnb_pin, !state->lnb[5]);
			dprintk(10, "%s: h %p %d\n", __func__, state->lnb_pin, !state->lnb[5]);
			break;
		}
		default:
		{
			return -EINVAL;
		}
	}
	return 0;
}

void *lnb_pio_attach(u32 *lnb, struct equipment_s *equipment)
{
	struct lnb_state *state = kmalloc(sizeof(struct lnb_state), GFP_KERNEL);

	memcpy(state->lnb, lnb, sizeof(state->lnb));
	equipment->lnb_set_voltage = lnb_pio_set_voltage;
	state->lnb_enable_pin = stpio_request_pin(lnb[0], lnb[1], "lnb_enab", STPIO_OUT);
	dprintk(10, "lnb_enable_pin %p\n", state->lnb_enable_pin);
	stpio_set_pin(state->lnb_enable_pin, lnb[2]);
	state->lnb_pin = stpio_request_pin(lnb[3], lnb[4], "lnb_sel", STPIO_OUT);
	return state;
}
EXPORT_SYMBOL(lnb_pio_attach);

/* ******************************* */
/* module functions                */
/* ******************************* */

int __init lnbpio_init(void)
{
	dprintk(100, "%s >\n", __func__);
	return 0;
}

static void lnbpio_cleanup(void)
{
	dprintk(100, "%s >\n", __func__);
}

module_param(paramDebug, short, 0644);
MODULE_PARM_DESC(paramDebug, "Enable logging (default : 0 = no logging)");

MODULE_DESCRIPTION("LNB PIO Handling");

MODULE_AUTHOR("konfetti");
MODULE_LICENSE("GPL");

module_init (lnbpio_init);
module_exit (lnbpio_cleanup);
// vim:ts=4
