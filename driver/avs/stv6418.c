/*
 *   stv6418.c - audio/video switch driver
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <linux/version.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/poll.h>
#include <linux/types.h>

#include <linux/i2c.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,17)
#include <linux/stm/pio.h>
#else
#include <linux/stpio.h>
#endif

#include "avs_core.h"
#include "stv6418.h"
#include "tools.h"

#define STV6418_MAX_REGS 10

static int debug = AVS_DEBUG;

static unsigned char regs[STV6418_MAX_REGS + 1]; /* range 0x01 to 0x10 */

static unsigned char backup_regs[STV6418_MAX_REGS + 1];
/* hold old values for standby */
static unsigned char t_stnby=0;

static struct stpio_pin* eMute;   // mute bin

#define MUTE_CLR() {stpio_set_pin(eMute, 0);}
#define MUTE_SET() {stpio_set_pin(eMute, 1);}

#define STV6418_DATA_SIZE sizeof(regs)

/* register description (as far as known) */

/* reg0 (0x01):
 * Bit 7     = TV Mono (1) /Stereo (0)
 * Bit 6     = Extragain
 * Bit 5 - 1 = Volume -62 - 0 db in -2db steps
 * Bit 0     = Soft Volume Activation (0 active, 1 deactive)
 */
 
/* reg1 (0x02):
 * 
 * Bit 0 - 1 = tv cinch audio output selection
 *           = 00 = mute
 *           = 01 = encoder 
 *           = 10 = vcr
 *           = 11 = tv
 *
 * Bit 2     = ?
 * Bit 3 - 4 = vcr output audio selection
 *           = 00 = mute
 *           = 01 = encoder
 *           = 10 = tv
 *           = 11 = unused ?
 * Bit 5     = vcr mono (1) / stereo (0)
 * Bit 6     = ???
 * Bit 7     = unused ???
 * value     = 00001101
 */   

/* reg2 (0x03)
 *
 * Bit 0 - 2 = tv video output selection
 *           =  000 = y/cvbs & chroma mute
 *           =  001 = y/cvbs encoder, rc encoder
 *           =  010 = y encoder, c encoder
 *           =  011 = y/cvbs vcr, rc vcr
 *           =  100 = cvbs aux & chroma muted (not sure here, taken from stv6412)
 *
 * Bit 3     = chroma defined by d2d1d0 (0), mute (1)
 * Bit 4 - 6 = vcr video output selection
 *           =  000 = y/cvbs & chroma mute
 *           =  001 = y/cvbs encoder, rc encoder
 *           =  010 = y encoder, c encoder
 *           =  011 = ??? 
 *           =  100 = cvbs tv & chroma muted
 * Bit 7     = chroma defined by d6d5d4 (0), mute (1)
 * 
 */

/* reg3 (0x04)
 *
 * Bit 0 - 1 = fast blanking
 *           = 00 = low level 
 *           = 01 = hight level
 *           = 10 = vcr
 *           = 11 = enc
 *
 * Bit 2 - 3 = rgb selection  
 *           = 00 = mute 
 *           = 01 = encoder
 *           = 10 = vcr
 *           = 11 = ???
 *
 * Bit 4 - 5 = db gain
 *           = 00 = 6db 
 *           = 01 = 5db
 *           = 10 = 4db
 *           = 11 = 3db
 * Bit 6     = 0db extra gain (0), weak signal (1)
 * 
 * Bit 7     = 0 = high impedance, 1 = output active
 */

/* reg 4 (0x05)
 * ???
 */
 
/* reg 5 (0x06)
 * 
 * Bit 0     = clamp enabled (0), disabled (1)
 * Bit 1 - 2 = encoder clamp settings (???)
 * Bit 3 - 4 = vcr clamp settings (???)
 * Bit 5 - 6 = encoder input level adjustment
 *           = 00 = 0db
 *           = 01 = 6db
 *           = 10 = 9db
 * Bit 7     = ???
 */

/* reg 6 (0x07)
 *
 * Bit 0     = ???
 * Bit 1     = Interrupt Flag
 * Bit 2 - 5 = slow blanking tv scart
 *           = 0000 = tv/vcr input mode
 *           = 0001 = tv 2v
 *           = 0010 = tv 16:9
 *           = 0011 = tv 4:3
 *           = 0100 = vcr 2v
 *           = 1000 = vcr 16:9
 *           = 1100 = vcr 4:3
 *
 * Bit 6     = 0 = YCVBS - VCR active
 * Bit 7     = 0 = YCVBS - TV active 
 */

/* reg 7 (0x08)
 *
 * Bit 3     = 1 = auto tv enable
 * Bit 4     = 1 = auto vcr enable
 * Bit 5     = 1 = auto slb disabele ???
 */

/* reg 8 (0x09)
 *
 * Bit 0     = 1 = enc input disable
 * Bit 1     = 1 = vcr input disable
 * Bit 2     = 1 = tv  input disable
 * Bit 3     = 1 = vcr output off
 * Bit 4     = 1 = tv  output off
 * Bit 5     = 1 = rf  output off
 * Bit 6     = 1 = cvbs tv  enable
 * Bit 7     = 1 = cvbs vcr enable
 */
  
#define cReg0  0x01
#define cReg1  0x02
#define cReg2  0x03
#define cReg3  0x04
#define cReg4  0x05
#define cReg5  0x06
#define cReg6  0x07
#define cReg7  0x08
#define cReg8  0x09
#define cReg9  0x0a

/* hold old values for mute/unmute */
static unsigned char tv_value;
static unsigned char vcr_value;

static int stv6418_s_old_src;

int stv6418_set(struct i2c_client *client)
{
	int i;
	
	regs[0] = 0x00;

	regs[0] = 0x00;
	regs[1] = 0x40; /* 0x40 */
	regs[2] = 0x09; /* 0x09 */
	regs[3] = 0x11;
	regs[4] = 0x84;
	regs[5] = 0x84;
	regs[6] = 0x25;
	regs[7] = 0x08;
	regs[8] = 0x21;
	regs[9] = 0xc0;
	regs[10]= 0x00;

	dprintk("[AVS]: %s > %d\n", __func__, STV6418_DATA_SIZE);

	dprintk("[AVS]: regs = { ");
	for (i = 0; i <= STV6418_MAX_REGS; i++)
		dprintk("0x%02x ", regs[i]);
	dprintk(" }\n");

	if ( STV6418_DATA_SIZE != i2c_master_send(client, regs, STV6418_DATA_SIZE))
	{
		printk("[AVS]: %s: error sending data\n", __func__);
		return -EFAULT;
	}

	dprintk("[AVS]: %s <\n", __func__);

	return 0;
}

int stv6418_set_volume( struct i2c_client *client, int vol )
{
	int c=0;

	dprintk("[AVS]: %s >\n", __func__);
	c = vol;

	if(c==63)
		c=62;
	if(c==0)
		c=1;

	if ((c > 63) || (c < 0))
		return -EINVAL;

	c /= 2;

	set_bits(regs, cReg0, c, 1, 5);

	dprintk("[AVS]: %s <\n", __func__);
	return stv6418_set(client);
}

inline int stv6418_set_mute( struct i2c_client *client, int type )
{
	dprintk("[AVS]: %s >\n", __func__);

	if ((type<0) || (type>1))
	{
		return -EINVAL;
	}

	if (type == AVS_MUTE) 
	{
		if (tv_value == 0xff)
		{
		   tv_value = get_bits(regs, cReg1, 0, 2);
		   vcr_value = get_bits(regs, cReg1, 3, 2);

		   set_bits(regs, cReg1, 0, 0, 2); /* tv cinch mute */
		   set_bits(regs, cReg1, 0, 3, 2); /* vcr mute */
		}
#ifndef ATEVIO7500
#if !defined (IPBOX9900) && !defined (IPBOX99) && !defined(CUBEREVO_3000HD)
		MUTE_SET();
#endif
#endif
	}
	else /* unmute with old values */
	{
		if (tv_value != 0xff)
		{
			set_bits(regs, cReg1, tv_value, 0, 2);
			set_bits(regs, cReg1, vcr_value, 3, 2);

			tv_value = 0xff;
			vcr_value = 0xff;
		}
#ifndef ATEVIO7500
#if !defined (IPBOX9900) && !defined (IPBOX99) && !defined(CUBEREVO_3000HD)
		MUTE_CLR();
#endif
#endif
	}

	dprintk("[AVS]: %s <\n", __func__);
	return stv6418_set(client);
}

inline int stv6418_set_vsw( struct i2c_client *client, int sw, int type )
{
	printk("[AVS]: %s: SET VSW: %d %d\n",__func__, sw, type);

	if (type<0 || type>4)
	{
		return -EINVAL;
	}

	switch(sw)
	{
		case 0:	// vcr
	        set_bits(regs, cReg2, type, 4, 3);
			break;
		case 1:	// rgb
			if (type<0 || type>2)
			{
				return -EINVAL;
			}

			set_bits(regs, cReg3, type, 2, 2);
			break;
		case 2: // tv
			set_bits(regs, cReg2, type, 0, 3);
			break;
		default:
			return -EINVAL;
	}

	dprintk("[AVS]: %s <\n", __func__);
	return stv6418_set(client);
}

inline int stv6418_set_asw( struct i2c_client *client, int sw, int type )
{
	dprintk("[AVS]: %s >\n", __func__);
	switch(sw)
	{
		case 0:
			if (type<=0 || type>3)
			{
				return -EINVAL;
			}

			/* if muted ? yes: save in temp */
			if ( vcr_value == 0xff )
				set_bits(regs, cReg1, type, 3, 2);
			else
				vcr_value = type;

			break;
		case 1:
		case 2:
			if (type<=0 || type>4)
			{
				return -EINVAL;
			}

			/* if muted ? yes: save in temp */
			if ( tv_value == 0xff )
				set_bits(regs, cReg1, type, 0, 2);
			else
				tv_value = type;

			break;
		default:
			return -EINVAL;
	}

	dprintk("[AVS]: %s <\n", __func__);
	return stv6418_set(client);
}

inline int stv6418_set_t_sb( struct i2c_client *client, int type )
{
/* fixme: on stv6418 we have another range
 * think on this ->see register description below
 */
	dprintk("[AVS]: %s >\n", __func__);

	if (type<0 || type>3)
	{
		return -EINVAL;
	}

	set_bits(regs, cReg6, type, 2, 4);
	
	dprintk("[AVS]: %s <\n", __func__);
	return stv6418_set(client);
}

inline int stv6418_set_wss( struct i2c_client *client, int vol )
{
/* fixme: on stv6418 we hav more possibilites here
 * think on this ->see register description below
 */
	dprintk("[AVS]: %s >\n", __func__);

	if (vol == SAA_WSS_43F)
	{
           set_bits(regs, cReg6, 3, 2, 4);
	}
	else if (vol == SAA_WSS_169F)
	{
           set_bits(regs, cReg6, 2, 2, 4);
	}
	else if (vol == SAA_WSS_OFF)
	{
           set_bits(regs, cReg6, 0, 2, 4);
	}
	else
	{
		return  -EINVAL;
	}

	dprintk("[AVS]: %s <\n", __func__);
	return stv6418_set(client);
}

inline int stv6418_set_fblk( struct i2c_client *client, int type )
{
	dprintk("[AVS]: %s >\n", __func__);
	if (type<0 || type>3)
	{
		return -EINVAL;
	}

	set_bits(regs, cReg3, type, 0, 2);

	dprintk("[AVS]: %s <\n", __func__);

	return stv6418_set(client);
}

int stv6418_get_status(struct i2c_client *client)
{
	unsigned char byte;

	dprintk("[AVS]: %s >\n", __func__);

	byte = 0;

	if (1 != i2c_master_recv(client,&byte,1))
	{
		return -1;
	}

	dprintk("[AVS]: %s <\n", __func__);

	return byte;
}

int stv6418_get_volume(void)
{
	int c;

	dprintk("[AVS]: %s >\n", __func__);
	
	c = get_bits(regs, cReg0, 1, 5);

	if (c)
	{
		c *= 2;
	}

	dprintk("[AVS]: %s <\n", __func__);

	return c;
}

inline int stv6418_get_mute(void)
{
	dprintk("[AVS]: %s <>\n", __func__);
	return !((tv_value == 0xff) && (vcr_value == 0xff));
}

inline int stv6418_get_fblk(void)
{
	dprintk("[AVS]: %s <>\n", __func__);
	return get_bits(regs, cReg3, 0, 2);
}

inline int stv6418_get_t_sb(void)
{
	dprintk("[AVS]: %s <>\n", __func__);
	return get_bits(regs, cReg6, 2, 4);
}

inline int stv6418_get_vsw( int sw )
{
	dprintk("[AVS]: %s >\n", __func__);
	switch(sw)
	{
		case 0:
			return get_bits(regs, cReg2, 4, 3);
			break;
		case 1:
			return get_bits(regs, cReg3, 2, 2);
			break;
		case 2:
			return get_bits(regs, cReg2, 0, 3);
			break;
		default:
			return -EINVAL;
	}

	dprintk("[AVS]: %s <\n", __func__);

	return -EINVAL;
}

inline int stv6418_get_asw( int sw )
{
	dprintk("[AVS]: %s >\n", __func__);
	switch(sw)
	{
		case 0:
			// muted ? yes: return tmp values
			if ( vcr_value == 0xff )
				return get_bits(regs, cReg1, 3, 2);
			else
				return vcr_value;
		case 1:
		case 2:
			if ( tv_value == 0xff )
				return get_bits(regs, cReg1, 0, 2);
			else
				return tv_value;
			break;
		default:
			return -EINVAL;
	}

	dprintk("[AVS]: %s <\n", __func__);

	return -EINVAL;
}

int stv6418_set_encoder( struct i2c_client *client, int vol )
{
	return 0;
}
 
int stv6418_set_mode( struct i2c_client *client, int val )
{
	dprintk("[AVS]: SAAIOSMODE command : %d\n", val);

	if (val == SAA_MODE_RGB)
	{
		if(get_bits(regs, cReg2, 0, 3) == 4) // scart selected
			stv6418_s_old_src = 1;
		else
			set_bits(regs, cReg2, 1, 0, 3);
                set_bits(regs, cReg3, 1, 0, 2);    /* fast blanking */
	}
	else if (val == SAA_MODE_FBAS)
	{
		if(get_bits(regs, cReg2, 0, 3) == 4) // scart selected
			stv6418_s_old_src = 1;
		else
			set_bits(regs, cReg2, 1, 0, 3);
                set_bits(regs, cReg3, 0, 0, 2);   /* fast blanking */
	}
	else if (val == SAA_MODE_SVIDEO)
	{
		if(get_bits(regs, cReg2, 0, 3) == 4) // scart selected
			stv6418_s_old_src = 2;
		else
			set_bits(regs, cReg2, 2, 0, 3);
                set_bits(regs, cReg3, 0, 0, 2);   /* fast blanking */
	}
	else
	{
		return  -EINVAL;
	}
 
	dprintk("[AVS]: %s <\n", __func__);
	return stv6418_set(client);
}
 
int stv6418_src_sel( struct i2c_client *client, int src )
{
	dprintk("[AVS]: %s >\n", __func__);

	if (src == SAA_SRC_ENC)
	{
	   set_bits(regs, cReg2, stv6418_s_old_src, 0, 3); /* t_vsc */
	   set_bits(regs, cReg2, stv6418_s_old_src, 4, 3); /* v_vsc */
	   set_bits(regs, cReg1, 1, 0, 2); /* tc_asc */
	   set_bits(regs, cReg1, 1, 3, 2); /* v_asc */
	}
	else if(src == SAA_SRC_SCART)
	{
	   stv6418_s_old_src = get_bits(regs, cReg2, 0, 3);
	   set_bits(regs, cReg2, 4, 0, 3);
	   set_bits(regs, cReg2, 0, 4, 3);
	   set_bits(regs, cReg1, 2, 0, 2);
	   set_bits(regs, cReg1, 0, 3, 2);
  	}
  	else
	{
		return  -EINVAL;
	}
 
	dprintk("[AVS]: %s <\n", __func__);
	return stv6418_set(client);
}

inline int stv6418_standby( struct i2c_client *client, int type )
{
 
	dprintk("[AVS]: %s >\n", __func__);

	if ((type<0) || (type>1))
	{
		return -EINVAL;
	}
 
	if (type==1) 
	{
		if (t_stnby == 0)
		{
			memcpy(backup_regs, regs, STV6418_DATA_SIZE);
			
			// Full Stop mode
			set_bits(regs, cReg8, 1, 0, 1); /* enc in */
			set_bits(regs, cReg8, 1, 1, 1); /* vcr in */
			set_bits(regs, cReg8, 1, 2, 1); /* tv  in */
			set_bits(regs, cReg8, 1, 3, 1); /* vcr out */
			set_bits(regs, cReg8, 1, 4, 1); /* tv  out */
			set_bits(regs, cReg8, 1, 5, 1); /* rf  out */
			set_bits(regs, cReg8, 1, 6, 1);
			set_bits(regs, cReg8, 1, 7, 1);

			t_stnby = 1;
		}
		else
			return -EINVAL;		
	}
	else
	{
		if (t_stnby == 1)
		{
			memcpy(regs, backup_regs, STV6418_DATA_SIZE);
			t_stnby = 0;
		}
		else
			return -EINVAL;
	}
 
	dprintk("[AVS]: %s <\n", __func__);
	return stv6418_set(client);
}

int stv6418_command(struct i2c_client *client, unsigned int cmd, void *arg )
{
	int val=0;

	unsigned char scartPin8Table[3] = { 0, 2, 3 };
	unsigned char scartPin8Table_reverse[4] = { 0, 0, 1, 2 };

	dprintk("[AVS]: %s(%d)\n", __func__, cmd);
	
	if (cmd & AVSIOSET)
	{
		if ( copy_from_user(&val,arg,sizeof(val)) )
		{
			return -EFAULT;
		}

		switch (cmd)
		{
			/* set video */
			case AVSIOSVSW1:
				return stv6418_set_vsw(client,0,val);
			case AVSIOSVSW2:
				return stv6418_set_vsw(client,1,val);
			case AVSIOSVSW3:
				return stv6418_set_vsw(client,2,val);
			/* set audio */
			case AVSIOSASW1:
				return stv6418_set_asw(client,0,val);
			case AVSIOSASW2:
				return stv6418_set_asw(client,1,val);
			case AVSIOSASW3:
				return stv6418_set_asw(client,2,val);
			/* set vol & mute */
			case AVSIOSVOL:
				return stv6418_set_volume(client,val);
			case AVSIOSMUTE:
				return stv6418_set_mute(client,val);
			/* set video fast blanking */
			case AVSIOSFBLK:
				return stv6418_set_fblk(client,val);
/* no direct manipulation allowed, use set_wss instead */
			/* set slow blanking (tv) */
			case AVSIOSSCARTPIN8:
				return stv6418_set_t_sb(client,scartPin8Table[val]);
			case AVSIOSFNC:
				return stv6418_set_t_sb(client,val);
			case AVSIOSTANDBY:
				return stv6418_standby(client,val);
			default:
				return -EINVAL;
		}
	} else if (cmd & AVSIOGET)
	{
		switch (cmd)
		{
			/* get video */
			case AVSIOGVSW1:
				val = stv6418_get_vsw(0);
				break;
			case AVSIOGVSW2:
				val = stv6418_get_vsw(1);
				break;
			case AVSIOGVSW3:
				val = stv6418_get_vsw(2);
				break;
			/* get audio */
			case AVSIOGASW1:
				val = stv6418_get_asw(0);
				break;
			case AVSIOGASW2:
				val = stv6418_get_asw(1);
				break;
			case AVSIOGASW3:
				val = stv6418_get_asw(2);
				break;
			/* get vol & mute */
			case AVSIOGVOL:
				val = stv6418_get_volume();
				break;
			case AVSIOGMUTE:
				val = stv6418_get_mute();
				break;
			/* get video fast blanking */
			case AVSIOGFBLK:
				val = stv6418_get_fblk();
				break;
			case AVSIOGSCARTPIN8:
				val = scartPin8Table_reverse[stv6418_get_t_sb()];
				break;
			/* get slow blanking (tv) */
			case AVSIOGFNC:
				val = stv6418_get_t_sb();
				break;
			/* get status */
			case AVSIOGSTATUS:
				// TODO: error handling
				val = stv6418_get_status(client);
				break;
			default:
				return -EINVAL;
		}

		return put_user(val,(int*)arg);
	}
	else
	{
		/* an SAA command */
		if ( copy_from_user(&val,arg,sizeof(val)) )
		{
			return -EFAULT;
		}

		switch(cmd)
		{
		case SAAIOSMODE:
           		 return stv6418_set_mode(client,val);
 	        case SAAIOSENC:
        		 return stv6418_set_encoder(client,val);
		case SAAIOSWSS:
			return stv6418_set_wss(client,val);
		case SAAIOSSRCSEL:
        		return stv6418_src_sel(client,val);
		default:
			dprintk("[AVS]: SAA command not supported\n");
			return -EINVAL;
		}
	}

	dprintk("[AVS: %s <\n", __func__);
	return 0;
}

int stv6418_command_kernel(struct i2c_client *client, unsigned int cmd, void *arg)
{
   int val=0;

	unsigned char scartPin8Table[3] = { 0, 2, 3 };
	unsigned char scartPin8Table_reverse[4] = { 0, 0, 1, 2 };

	
	if (cmd & AVSIOSET)
	{
		val = (int) arg;

      	dprintk("[AVS]: %s: AVSIOSET command(%d)\n", __func__, cmd);

		switch (cmd)
		{
			/* set video */
			case AVSIOSVSW1:
				return stv6418_set_vsw(client,0,val);
			case AVSIOSVSW2:
				return stv6418_set_vsw(client,1,val);
			case AVSIOSVSW3:
				return stv6418_set_vsw(client,2,val);
			/* set audio */
			case AVSIOSASW1:
				return stv6418_set_asw(client,0,val);
			case AVSIOSASW2:
				return stv6418_set_asw(client,1,val);
			case AVSIOSASW3:
				return stv6418_set_asw(client,2,val);
			/* set vol & mute */
			case AVSIOSVOL:
				return stv6418_set_volume(client,val);
			case AVSIOSMUTE:
				return stv6418_set_mute(client,val);
			/* set video fast blanking */
			case AVSIOSFBLK:
				return stv6418_set_fblk(client,val);
/* no direct manipulation allowed, use set_wss instead */
			/* set slow blanking (tv) */
			case AVSIOSSCARTPIN8:
				return stv6418_set_t_sb(client,scartPin8Table[val]);
			case AVSIOSFNC:
				return stv6418_set_t_sb(client,val);
			case AVSIOSTANDBY:
				return stv6418_standby(client,val);
			default:
				return -EINVAL;
		}
	} else if (cmd & AVSIOGET)
	{
      	dprintk("[AVS]: %s: AVSIOSET command(%d)\n", __func__, cmd);

		switch (cmd)
		{
			/* get video */
			case AVSIOGVSW1:
                                val = stv6418_get_vsw(0);
                                break;
			case AVSIOGVSW2:
                                val = stv6418_get_vsw(1);
                                break;
			case AVSIOGVSW3:
                                val = stv6418_get_vsw(2);
                                break;
			/* get audio */
			case AVSIOGASW1:
                                val = stv6418_get_asw(0);
                                break;
			case AVSIOGASW2:
                                val = stv6418_get_asw(1);
                                break;
			case AVSIOGASW3:
                                val = stv6418_get_asw(2);
                                break;
			/* get vol & mute */
			case AVSIOGVOL:
                                val = stv6418_get_volume();
                                break;
			case AVSIOGMUTE:
                                val = stv6418_get_mute();
                                break;
			/* get video fast blanking */
			case AVSIOGFBLK:
                                val = stv6418_get_fblk();
                                break;
			case AVSIOGSCARTPIN8:
				val = scartPin8Table_reverse[stv6418_get_t_sb()];
				break;
			/* get slow blanking (tv) */
			case AVSIOGFNC:
				val = stv6418_get_t_sb();
				break;
			/* get status */
			case AVSIOGSTATUS:
				// TODO: error handling
				val = stv6418_get_status(client);
				break;
			default:
				return -EINVAL;
		}

		arg = (void*) val;
	        return 0;
	}
	else
	{
		dprintk("[AVS]: %s: SAA command (%d)\n", __func__, cmd);

		val = (int) arg;

		switch(cmd)
		{
		case SAAIOSMODE:
           		 return stv6418_set_mode(client,val);
 	        case SAAIOSENC:
        		 return stv6418_set_encoder(client,val);
		case SAAIOSWSS:
			return stv6418_set_wss(client,val);
		case SAAIOSSRCSEL:
        		return stv6418_src_sel(client,val);
		default:
			dprintk("[AVS]: %s: SAA command(%d) not supported\n", __func__, cmd);
			return -EINVAL;
		}
	}

	return 0;
}

int stv6418_init(struct i2c_client *client)
{
	regs[0] = 0x00;
	regs[1] = 0x40; /* 0x40 */
	regs[2] = 0x09; /* 0x09 */
	regs[3] = 0x11;
	regs[4] = 0x84;
	regs[5] = 0x84;
	regs[6] = 0x25;
	regs[7] = 0x08;
	regs[8] = 0x21;
	regs[9] = 0xc0;
	regs[10]= 0x00;

#if !defined(IPBOX9900) && !defined(IPBOX99) && !defined(ATEVIO7500) && !defined(CUBEREVO_3000HD)
	eMute= stpio_request_pin (2, 2, "AVS_MUTE", STPIO_OUT);
	if (eMute == NULL)
	{
		dprintk("[AVS]: request mute bin failed!\n");
		return -EIO;
	}
#endif    
    return stv6418_set(client);
}
