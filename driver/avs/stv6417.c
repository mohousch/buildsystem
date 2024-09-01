/*
 *   stv6417.c - audio/video switch driver
 *
 *   dagobert: most parts are equal to stv6412 but not all I think.
 *   I've made this different to stv6412 because I find it hard to
 *   know what is ment when setting register like there.
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

#include "avs_core.h"
#include "stv6417.h"
#include "tools.h"

#define STV6417_MAX_REGS 10

static int debug = AVS_DEBUG;

static unsigned char regs[STV6417_MAX_REGS + 1]; /* range 0x01 to 0x10 */

static unsigned char backup_regs[STV6417_MAX_REGS + 1];
/* hold old values for standby */
static unsigned char t_stnby=0;

#define stv6417_DATA_SIZE sizeof(regs)

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
 */   

#define AOS_TV_REG     cReg1
#define AOS_TV_START   0
#define AOS_TV_SIZE    2
#define AOS_TV_MUTE    0
#define AOS_TV_ENCODER 1
#define AOS_TV_VCR     2
#define AOS_TV_TV      3

#define AOS_VCR_REG     cReg1
#define AOS_VCR_START   3
#define AOS_VCR_SIZE    2
#define AOS_VCR_MUTE    0
#define AOS_VCR_ENCODER 1
#define AOS_VCR_TV      2

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
  


/* hold old values for mute/unmute */
static unsigned char tv_value;
static unsigned char vcr_value;

static int stv6417_s_old_src;

/* ---------------------------------------------------------------------- */
int stv6417_set(struct i2c_client *client)
{
        int i;
	
	regs[0] = 0x00;

        dprintk("%s > %d\n", __func__, stv6417_DATA_SIZE);

        dprintk("regs: ");
        for (i = 0; i <= STV6417_MAX_REGS; i++)
           dprintk("0x%02x ", regs[i]);
	dprintk("\n");    

	if ( stv6417_DATA_SIZE != i2c_master_send(client, regs, stv6417_DATA_SIZE))
	{
                printk("%s error sending data\n", __func__);
		return -EFAULT;
	}

        dprintk("%s <\n", __func__);

	return 0;
}
/* ---------------------------------------------------------------------- */

int stv6417_set_volume( struct i2c_client *client, int vol )
{
	int c=0;

        dprintk("%s >\n", __func__);
	c = vol;

	if(c==63)
		c=62;
	if(c==0)
		c=1;

	if ((c > 63) || (c < 0))
		return -EINVAL;

	c /= 2;

	set_bits(regs, cReg0, c, 1, 5);

        dprintk("%s <\n", __func__);
	return stv6417_set(client);
}

/* ---------------------------------------------------------------------- */

inline int stv6417_set_mute( struct i2c_client *client, int type )
{
        dprintk("%s >\n", __func__);
	if ((type<0) || (type>1))
	{
		return -EINVAL;
	}

printk("[AVS] %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n", 
get_bits(regs, cReg0, 0, 8), get_bits(regs, cReg1, 0, 8), get_bits(regs, cReg2, 0, 8), get_bits(regs, cReg3, 0, 8), 
get_bits(regs, cReg4, 0, 8), get_bits(regs, cReg5, 0, 8), get_bits(regs, cReg6, 0, 8), get_bits(regs, cReg7, 0, 8), 
get_bits(regs, cReg8, 0, 8), get_bits(regs, cReg9, 0, 8));

	// I cant find the error at the moment, do we even need mute capability in the avs ?
	return 0;

	if (type == AVS_MUTE) 
	{
		// Check if we are already mute
		unsigned char tmp_tv_value  = get_bits(regs, AOS_TV_REG,  AOS_TV_START,  AOS_TV_SIZE);
		unsigned char tmp_vcr_value = get_bits(regs, AOS_VCR_REG, AOS_VCR_START, AOS_VCR_SIZE);
		
		if (tmp_tv_value != AOS_TV_MUTE && tmp_vcr_value != AOS_VCR_MUTE)
		{
			tv_value  = tmp_tv_value;
			vcr_value = tmp_vcr_value;

			set_bits(regs, AOS_TV_REG,  AOS_TV_MUTE,  AOS_TV_START,  AOS_TV_SIZE); /* tv cinch mute */
			set_bits(regs, AOS_VCR_REG, AOS_VCR_MUTE, AOS_VCR_START, AOS_VCR_SIZE); /* vcr mute */
		}
	}
	else /* unmute with old values */
	{
		set_bits(regs, AOS_TV_REG,  tv_value,  AOS_TV_START,  AOS_TV_SIZE);
		set_bits(regs, AOS_VCR_REG, vcr_value, AOS_VCR_START, AOS_VCR_SIZE);
	}

        dprintk("%s <\n", __func__);
	return stv6417_set(client);
}

/* ---------------------------------------------------------------------- */

inline int stv6417_set_vsw( struct i2c_client *client, int sw, int type )
{
	printk("SET VSW: %d %d\n",sw,type);

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

        dprintk("%s <\n", __func__);
	return stv6417_set(client);
}

/* ---------------------------------------------------------------------- */

inline int stv6417_set_asw( struct i2c_client *client, int sw, int type )
{
	unsigned char tmp_tv_value;
	unsigned char tmp_vcr_value;

        dprintk("%s >\n", __func__);
	// I don't get what this does, seems to be not used
	return 0;
	
	switch(sw)
	{
		case 0:
			if (type<=0 || type>3)
			{
				return -EINVAL;
			}

			/* if muted ? yes: save in temp */
			tmp_tv_value  = get_bits(regs, AOS_TV_REG,  AOS_TV_START,  AOS_TV_SIZE);
			tmp_vcr_value = get_bits(regs, AOS_VCR_REG, AOS_VCR_START, AOS_VCR_SIZE);
			if ( vcr_value == 0xff )
			{
				set_bits(regs, cReg1, type, 3, 2);
			}
			else
			{
				vcr_value = type;
			}
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

        dprintk("%s <\n", __func__);
	return stv6417_set(client);
}

/* ---------------------------------------------------------------------- */

inline int stv6417_set_t_sb( struct i2c_client *client, int type )
{
/* fixme: on stv6417 we have another range
 * think on this ->see register description below
 */
        dprintk("%s >\n", __func__);
	if (type<0 || type>3)
	{
		return -EINVAL;
	}

	set_bits(regs, cReg6, type, 2, 4);
	
        dprintk("%s <\n", __func__);
	return stv6417_set(client);
}

/* ---------------------------------------------------------------------- */

inline int stv6417_set_wss( struct i2c_client *client, int vol )
{
/* fixme: on stv6417 we hav more possibilites here
 * think on this ->see register description below
 */
        dprintk("%s >\n", __func__);
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

        dprintk("%s <\n", __func__);
	return stv6417_set(client);
}

/* ---------------------------------------------------------------------- */

inline int stv6417_set_fblk( struct i2c_client *client, int type )
{
        dprintk("%s >\n", __func__);
	if (type<0 || type>3)
	{
		return -EINVAL;
	}

        set_bits(regs, cReg3, type, 0, 2);

        dprintk("%s <\n", __func__);

	return stv6417_set(client);
}

/* ---------------------------------------------------------------------- */

int stv6417_get_status(struct i2c_client *client)
{
	unsigned char byte;

        dprintk("%s >\n", __func__);

	byte = 0;

	if (1 != i2c_master_recv(client,&byte,1))
	{
		return -1;
	}

        dprintk("%s <\n", __func__);

	return byte;
}

/* ---------------------------------------------------------------------- */

int stv6417_get_volume(void)
{
	int c;

        dprintk("%s >\n", __func__);
	
	c = get_bits(regs, cReg0, 1, 5);

	if (c)
	{
		c *= 2;
	}

        dprintk("%s <\n", __func__);

	return c;
}

/* ---------------------------------------------------------------------- */

inline int stv6417_get_mute(void)
{
        dprintk("%s <>\n", __func__);
	return 0;
	return !((tv_value == 0xff) && (vcr_value == 0xff));
}

/* ---------------------------------------------------------------------- */

inline int stv6417_get_fblk(void)
{
        dprintk("%s <>\n", __func__);
        return get_bits(regs, cReg3, 0, 2);
}

/* ---------------------------------------------------------------------- */

inline int stv6417_get_t_sb(void)
{
        dprintk("%s <>\n", __func__);
	return get_bits(regs, cReg6, 2, 4);
}

/* ---------------------------------------------------------------------- */

inline int stv6417_get_vsw( int sw )
{
        dprintk("%s >\n", __func__);
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

        dprintk("%s <\n", __func__);

	return -EINVAL;
}

/* ---------------------------------------------------------------------- */

inline int stv6417_get_asw( int sw )
{
        dprintk("%s >\n", __func__);
	return 0;
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

        dprintk("%s <\n", __func__);

	return -EINVAL;
}

/* ---------------------------------------------------------------------- */
//NOT IMPLEMENTED
int stv6417_set_encoder( struct i2c_client *client, int vol )
{

	return 0;
}

/* ---------------------------------------------------------------------- */
 
int stv6417_set_mode( struct i2c_client *client, int vol )
{
	dprintk("[AVS]: SAAIOSMODE command : %d\n", vol);
	if (vol == SAA_MODE_RGB)
	{
		if(get_bits(regs, cReg2, 0, 3) == 4) // scart selected
			stv6417_s_old_src = 1;
		else
			set_bits(regs, cReg2, 1, 0, 3);
                set_bits(regs, cReg3, 1, 0, 2);    /* fast blanking */
	}
	else if (vol == SAA_MODE_FBAS)
	{
		if(get_bits(regs, cReg2, 0, 3) == 4) // scart selected
			stv6417_s_old_src = 1;
		else
			set_bits(regs, cReg2, 1, 0, 3);
                set_bits(regs, cReg3, 0, 0, 2);   /* fast blanking */
	}
	else if (vol == SAA_MODE_SVIDEO)
	{
		if(get_bits(regs, cReg2, 0, 3) == 4) // scart selected
			stv6417_s_old_src = 2;
		else
			set_bits(regs, cReg2, 2, 0, 3);
                set_bits(regs, cReg3, 0, 0, 2);   /* fast blanking */
	}
	else
	{
		return  -EINVAL;
	}
 
        dprintk("%s <\n", __func__);
	return stv6417_set(client);
}

/* ---------------------------------------------------------------------- */
 
int stv6417_src_sel( struct i2c_client *client, int src )
{
        dprintk("%s >\n", __func__);
	if (src == SAA_SRC_ENC)
	{
	   set_bits(regs, cReg2, stv6417_s_old_src, 0, 3); /* t_vsc */
	   set_bits(regs, cReg2, stv6417_s_old_src, 4, 3); /* v_vsc */
	   set_bits(regs, cReg1, 1, 0, 2); /* tc_asc */
	   set_bits(regs, cReg1, 1, 3, 2); /* v_asc */
	}
	else if(src == SAA_SRC_SCART)
	{
	   stv6417_s_old_src = get_bits(regs, cReg2, 0, 3);
	   set_bits(regs, cReg2, 4, 0, 3);
	   set_bits(regs, cReg2, 0, 4, 3);
	   set_bits(regs, cReg1, 2, 0, 2);
	   set_bits(regs, cReg1, 0, 3, 2);
  	}
  	else
	{
		return  -EINVAL;
	}
 
        dprintk("%s <\n", __func__);
	return stv6417_set(client);
}

/* ---------------------------------------------------------------------- */

inline int stv6417_standby( struct i2c_client *client, int type )
{
 
        dprintk("%s >\n", __func__);
	if ((type<0) || (type>1))
	{
		return -EINVAL;
	}
 
	if (type==1) 
	{
		if (t_stnby == 0)
		{
			memcpy(backup_regs, regs, stv6417_DATA_SIZE);
			
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
			memcpy(regs, backup_regs, stv6417_DATA_SIZE);
			t_stnby = 0;
		}
		else
			return -EINVAL;
	}
 
        dprintk("%s <\n", __func__);
	return stv6417_set(client);
}

/* ---------------------------------------------------------------------- */

int stv6417_command(struct i2c_client *client, unsigned int cmd, void *arg )
{
	int val=0;

	unsigned char scartPin8Table[3] = { 0, 2, 3 };
	unsigned char scartPin8Table_reverse[4] = { 0, 0, 1, 2 };

	printk("[AVS]: command\n");
	
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
				return stv6417_set_vsw(client,0,val);
			case AVSIOSVSW2:
				return stv6417_set_vsw(client,1,val);
			case AVSIOSVSW3:
				return stv6417_set_vsw(client,2,val);
			/* set audio */
			case AVSIOSASW1:
				return stv6417_set_asw(client,0,val);
			case AVSIOSASW2:
				return stv6417_set_asw(client,1,val);
			case AVSIOSASW3:
				return stv6417_set_asw(client,2,val);
			/* set vol & mute */
			case AVSIOSVOL:
				return stv6417_set_volume(client,val);
			case AVSIOSMUTE:
				return stv6417_set_mute(client,val);
			/* set video fast blanking */
			case AVSIOSFBLK:
				return stv6417_set_fblk(client,val);
/* no direct manipulation allowed, use set_wss instead */
			/* set slow blanking (tv) */
			case AVSIOSSCARTPIN8:
				return stv6417_set_t_sb(client,scartPin8Table[val]);
			case AVSIOSFNC:
				return stv6417_set_t_sb(client,val);
			case AVSIOSTANDBY:
				return stv6417_standby(client,val);
			default:
				return -EINVAL;
		}
	} else if (cmd & AVSIOGET)
	{
		switch (cmd)
		{
			/* get video */
			case AVSIOGVSW1:
                                val = stv6417_get_vsw(0);
                                break;
			case AVSIOGVSW2:
                                val = stv6417_get_vsw(1);
                                break;
			case AVSIOGVSW3:
                                val = stv6417_get_vsw(2);
                                break;
			/* get audio */
			case AVSIOGASW1:
                                val = stv6417_get_asw(0);
                                break;
			case AVSIOGASW2:
                                val = stv6417_get_asw(1);
                                break;
			case AVSIOGASW3:
                                val = stv6417_get_asw(2);
                                break;
			/* get vol & mute */
			case AVSIOGVOL:
                                val = stv6417_get_volume();
                                break;
			case AVSIOGMUTE:
                                val = stv6417_get_mute();
                                break;
			/* get video fast blanking */
			case AVSIOGFBLK:
                                val = stv6417_get_fblk();
                                break;
			case AVSIOGSCARTPIN8:
				val = scartPin8Table_reverse[stv6417_get_t_sb()];
				break;
			/* get slow blanking (tv) */
			case AVSIOGFNC:
				val = stv6417_get_t_sb();
				break;
			/* get status */
			case AVSIOGSTATUS:
                                // TODO: error handling
                                val = stv6417_get_status(client);
                                break;
			default:
				return -EINVAL;
		}

		return put_user(val,(int*)arg);
	}
	else
	{
		printk("[AVS]: SAA command\n");

		/* an SAA command */
		if ( copy_from_user(&val,arg,sizeof(val)) )
		{
			return -EFAULT;
		}

		switch(cmd)
		{
		case SAAIOSMODE:
           		 return stv6417_set_mode(client,val);
 	        case SAAIOSENC:
        		 return stv6417_set_encoder(client,val);
		case SAAIOSWSS:
			return stv6417_set_wss(client,val);
		case SAAIOSSRCSEL:
        		return stv6417_src_sel(client,val);
		default:
			dprintk("[AVS]: SAA command not supported\n");
			return -EINVAL;
		}
	}

        dprintk("%s <\n", __func__);
	return 0;
}

/* ---------------------------------------------------------------------- */
 
int stv6417_command_kernel(struct i2c_client *client, unsigned int cmd, void *arg)
{
   int val=0;

	unsigned char scartPin8Table[3] = { 0, 2, 3 };
	unsigned char scartPin8Table_reverse[4] = { 0, 0, 1, 2 };

	printk("[AVS]: command_kernel(%u)\n", cmd);
	
	if (cmd & AVSIOSET)
	{
		val = (int) arg;

      		dprintk("[AVS]: AVSIOSET command\n");

		switch (cmd)
		{
			/* set video */
			case AVSIOSVSW1:
				return stv6417_set_vsw(client,0,val);
			case AVSIOSVSW2:
				return stv6417_set_vsw(client,1,val);
			case AVSIOSVSW3:
				return stv6417_set_vsw(client,2,val);
			/* set audio */
			case AVSIOSASW1:
				return stv6417_set_asw(client,0,val);
			case AVSIOSASW2:
				return stv6417_set_asw(client,1,val);
			case AVSIOSASW3:
				return stv6417_set_asw(client,2,val);
			/* set vol & mute */
			case AVSIOSVOL:
				return stv6417_set_volume(client,val);
			case AVSIOSMUTE:
				return stv6417_set_mute(client,val);
			/* set video fast blanking */
			case AVSIOSFBLK:
				return stv6417_set_fblk(client,val);
/* no direct manipulation allowed, use set_wss instead */
			/* set slow blanking (tv) */
			case AVSIOSSCARTPIN8:
				return stv6417_set_t_sb(client,scartPin8Table[val]);
			case AVSIOSFNC:
				return stv6417_set_t_sb(client,val);
			case AVSIOSTANDBY:
				return stv6417_standby(client,val);
			default:
				return -EINVAL;
		}
	} else if (cmd & AVSIOGET)
	{
		dprintk("[AVS]: AVSIOGET command\n");

		switch (cmd)
		{
			/* get video */
			case AVSIOGVSW1:
                                val = stv6417_get_vsw(0);
                                break;
			case AVSIOGVSW2:
                                val = stv6417_get_vsw(1);
                                break;
			case AVSIOGVSW3:
                                val = stv6417_get_vsw(2);
                                break;
			/* get audio */
			case AVSIOGASW1:
                                val = stv6417_get_asw(0);
                                break;
			case AVSIOGASW2:
                                val = stv6417_get_asw(1);
                                break;
			case AVSIOGASW3:
                                val = stv6417_get_asw(2);
                                break;
			/* get vol & mute */
			case AVSIOGVOL:
                                val = stv6417_get_volume();
                                break;
			case AVSIOGMUTE:
                                val = stv6417_get_mute();
                                break;
			/* get video fast blanking */
			case AVSIOGFBLK:
                                val = stv6417_get_fblk();
                                break;
			case AVSIOGSCARTPIN8:
				val = scartPin8Table_reverse[stv6417_get_t_sb()];
				break;
			/* get slow blanking (tv) */
			case AVSIOGFNC:
				val = stv6417_get_t_sb();
				break;
			/* get status */
			case AVSIOGSTATUS:
                                // TODO: error handling
                                val = stv6417_get_status(client);
                                break;
			default:
				return -EINVAL;
		}

		arg = (void*) val;
	        return 0;
	}
	else
	{
		printk("[AVS]: SAA command\n");

		val = (int) arg;

		switch(cmd)
		{
		case SAAIOSMODE:
           		 return stv6417_set_mode(client,val);
 	        case SAAIOSENC:
        		 return stv6417_set_encoder(client,val);
		case SAAIOSWSS:
			return stv6417_set_wss(client,val);
		case SAAIOSSRCSEL:
        		return stv6417_src_sel(client,val);
		default:
			dprintk("[AVS]: SAA command not supported\n");
			return -EINVAL;
		}
	}

        dprintk("%s <\n", __func__);
	return 0;
}

/* ---------------------------------------------------------------------- */

int stv6417_init(struct i2c_client *client)
{
    dprintk("%s >\n", __func__);

    regs[0] = 0x00;
    //0 0 10000 1
    regs[cReg0] = 0x01; // 0x40 Means that scart gets the maximum volume output, note we will never trigger this as volume is controled inside the mixer
    regs[cReg1] = 0x09;
    regs[cReg2] = 0x11;
    regs[cReg3] = 0x84;
    regs[cReg4] = 0x84;
    regs[cReg5] = 0x25;
    regs[cReg6] = 0x08;
    regs[cReg7] = 0x21;
    regs[cReg8] = 0xc0;
    regs[cReg9]= 0x00;

    dprintk("%s <\n", __func__);
    return stv6417_set(client);
}

/* ---------------------------------------------------------------------- */
