/*
 *
 * (c) 2010 konfetti, schischu
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

#include <linux/input.h>
#include "cec_opcodes_def.h"
#include "cec_debug.h"

static char *button_driver_name = "TDT RC event driver";
static struct input_dev *button_dev;
unsigned int last_keycode = 0xFFFF;

int input_init(void)
{
	int error;
	int vLoop = 0;

	dprintk(0, "allocating and registering button device\n");

	button_dev = input_allocate_device();
	if (!button_dev)
		return -ENOMEM;

	button_dev->name = button_driver_name;
	button_dev->open = NULL;
	button_dev->close = NULL;

	set_bit(EV_KEY, button_dev->evbit);
	set_bit(EV_REP, button_dev->evbit);

	for (vLoop = 0; vLoop < KEY_MAX; vLoop++)
		set_bit(vLoop, button_dev->keybit);


	error = input_register_device(button_dev);
	if (error)
	{
		input_free_device(button_dev);
		return error;
	}

	input_event(button_dev, EV_REP, REP_DELAY, 500);
	input_event(button_dev, EV_REP, REP_PERIOD, 200);

	return 0;
}

int input_cleanup(void)
{
	dprintk(0, "unregistering button device\n");
	input_unregister_device(button_dev);

	return 0;
}

int input_inject(unsigned int key, unsigned int type)
{
	unsigned int code = 0;
	unsigned int value = 0;

	switch (key)
	{
		case USER_CONTROL_CODE_SELECT:
			code = KEY_OK;
			break;
		case USER_CONTROL_CODE_UP:
			code = KEY_UP;
			break;
		case USER_CONTROL_CODE_DOWN:
			code = KEY_DOWN;
			break;
		case USER_CONTROL_CODE_LEFT:
			code = KEY_LEFT;
			break;
		case USER_CONTROL_CODE_RIGHT:
			code = KEY_RIGHT;
			break;
		case USER_CONTROL_CODE_EXIT:
			code = KEY_HOME;
			break;
		case USER_CONTROL_CODE_PREV_CHANNEL:
			code = KEY_MENU;
			break; //not perfect but also used on fortis boxes
		case USER_CONTROL_CODE_EPG:
			code = KEY_INFO;
			break;
		case USER_CONTROL_CODE_NUMBERS_0:
			code = KEY_0;
			break;
		case USER_CONTROL_CODE_NUMBERS_1:
			code = KEY_1;
			break;
		case USER_CONTROL_CODE_NUMBERS_2:
			code = KEY_2;
			break;
		case USER_CONTROL_CODE_NUMBERS_3:
			code = KEY_3;
			break;
		case USER_CONTROL_CODE_NUMBERS_4:
			code = KEY_4;
			break;
		case USER_CONTROL_CODE_NUMBERS_5:
			code = KEY_5;
			break;
		case USER_CONTROL_CODE_NUMBERS_6:
			code = KEY_6;
			break;
		case USER_CONTROL_CODE_NUMBERS_7:
			code = KEY_7;
			break;
		case USER_CONTROL_CODE_NUMBERS_8:
			code = KEY_8;
			break;
		case USER_CONTROL_CODE_NUMBERS_9:
			code = KEY_9;
			break;
		case USER_CONTROL_CODE_F1_BLUE:
			code = KEY_BLUE;
			break;
		case USER_CONTROL_CODE_F2_RED:
			code = KEY_RED;
			break;
		case USER_CONTROL_CODE_F3_GREEN:
			code = KEY_GREEN;
			break;
		case USER_CONTROL_CODE_F4_YELLOW:
			code = KEY_YELLOW;
			break;
		case USER_CONTROL_CODE_PLAY:
			code = KEY_PLAY;
			break;
		case USER_CONTROL_CODE_STOP:
			code = KEY_STOP;
			break;
		case USER_CONTROL_CODE_PAUSE:
			code = KEY_PAUSE;
			break;
		case USER_CONTROL_CODE_RECORD:
			code = KEY_RECORD;
			break;
		case USER_CONTROL_CODE_REWIND:
			code = KEY_REWIND;
			break;
		case USER_CONTROL_CODE_FASTFORWARD:
			code = KEY_FASTFORWARD;
			break;
		case USER_CONTROL_CODE_ROOT_MENU:
			code = KEY_MENU;
			break;
		case USER_CONTROL_CODE_CONTENTS_MENU:
			code = KEY_FAVORITES;
			break;
		case USER_CONTROL_CODE_SETUP_MENU:
			code = KEY_MENU;
			break;
		case USER_CONTROL_CODE_NEXT:
			code = KEY_FASTFORWARD;
			break;
		case USER_CONTROL_CODE_LAST:
			code = KEY_REWIND;
			break;
		case USER_CONTROL_CODE_PAGEUP:
			code = KEY_PAGEUP;
			break;
		case USER_CONTROL_CODE_PAGEDOWN:
			code = KEY_PAGEDOWN;
			break;
		case USER_CONTROL_CODE_INFO:
			code = KEY_INFO;
			break;
		default:
			break;
	}

	switch (type)
	{
		case 1:
			value = 1;
			last_keycode = code;
			break;
		case 0:
			value = 0;
			code = last_keycode;
			break;
		default:
			break;
	}

	if (code != 0)
		input_event(button_dev, 1, code, value);
	return 0;
}
