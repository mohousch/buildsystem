/*
 *   stv6419.c - audio/video switch driver
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
#include "stv6419.h"
#include "tools.h"

#define STV6419_MAX_REGS 6

static int debug = AVS_DEBUG;

static unsigned char regs[STV6419_MAX_REGS + 1]; /* range 0x01 to 0x05 */

static unsigned char backup_regs[STV6419_MAX_REGS + 1];
/* hold old values for standby */
static unsigned char t_stnby=0;


#define STV6419_DATA_SIZE sizeof(regs)

/* register description (as far as known) */

/* reg0 (0x01):
 * Bit 0 - 1      = Audio switch Control
 *		= 00 = mute
 *		= 01 = vcr
 *		= 10 = encoder
 * 		= 11 = aux
 * 
 * Bit 4 - 2    = Volume -62 - 0 db in -2db steps
 * Bit 6 - 5    = RBG, YPrPb Control
 *		= 00 = mute
 *		= 01 = vcr
 *		= 10 = encoder
 *		= 11 = not allowed
 * Bit 7 Not used 
*/
 
/* reg1 (0x02):
 * 
 * Bit 0  = Slow Blanking Mode
 *           = 0 = normal mode
 *           = 1 = SLB TV is driven by SLB VCR 
 *          
 * Bit 1 - 2 = Slow Blanking TV SCART
 *		= 00 = Input mode only
 *		= 01 = SLB TV output < 2 V
 *		= 10 = SLB TV output 16/9 format
 *		= 11 = SLB TV output 4/3 format
			
 * Bit 3 - 4 = Slow Blanking VCR SCART
 *           = 00 = Input mode only
 *           = 01 = SLB VCR output < 2 V
 *           = 10 = SLB VCR output 16/9 format
 *           = 11 = SLB VCR output 4/3 format

 * Bit 5 - 6 = CVBS Control
 * 		= 00 = muted
 *		= 01 = vcr
 *		= 10 = encoder
 *		= 11 = aux
 * Bit 7     = IT Enable
 *		= 0 = No Interrupt Flag
 *		= 1 = IT enabled
 */   

/* reg2 (0x03)
 *
 * Bit 0 - 1 = Fast blanking control
 *           =  00 = FB forced to low level
 *           =  01 = FB forced to high level
 *           =  10 = FB from VCR
 *           =  11 = Not allowed
 *           
 *
 * Bit 2     = encoder clamp selection
 *		= 0 = bottom level clamp (R,G,B)
 *		= 1 = synchronize clamp (Pr,Y,Pb)
 * Bit 3 - 7 = Not used
 *           
 * 
 */


  
#define cReg0  0x01
#define cReg1  0x02
#define cReg2  0x03
#define cReg3  0x04
#define cReg4  0x05

/* hold old values for mute/unmute */
static unsigned char audio_value; /* audio switch control */

static int stv6419_s_old_src;

int stv6419_set(struct i2c_client *client)
{
	int i;
	
	regs[0] = 0x00;

	regs[0] = 0x00; //bit staly START
	regs[1] = 0x1E; //0x00h > 0x06 
	regs[2] = 0x00; //0x01h > 0x02 
	regs[3] = 0x51; //0x02h > 0x52
	regs[4] = 0x55; //0x03h > 0x03 04
	regs[5] = 0x00; //0x04h > 0x04
	regs[6] = 0x00; //bit staly END

	dprintk("[AVS]: %s > %d\n", __func__, STV6419_DATA_SIZE);

	dprintk("[AVS]: regs = { ");
	for (i = 0; i <= STV6419_MAX_REGS; i++)
		dprintk("0x%02x ", regs[i]);
	dprintk(" }\n");

	
if ( STV6419_DATA_SIZE != i2c_master_send(client, regs, STV6419_DATA_SIZE))
	{
		printk("[AVS]: %s: error sending data\n", __func__);
		return -EFAULT;
	}

	dprintk("[AVS]: %s <\n", __func__);

	return 0;
}

int stv6419_set_volume( struct i2c_client *client, int vol )
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

	set_bits(regs, cReg1, c, 1, 5);

	dprintk("[AVS]: %s <\n", __func__);
	return stv6419_set(client);
}

inline int stv6419_set_mute( struct i2c_client *client, int type )
{
	dprintk("[AVS]: %s >\n", __func__);

	if ((type<0) || (type>1))
	{
		return -EINVAL;
	}
	

	if (type == AVS_MUTE) 
	{
		
		   audio_value = get_bits(regs, cReg0, 0, 2); 
		 

		   set_bits(regs, cReg0, 0, 0, 2); /* audio mute */
		 
		
	}
	else /* unmute with old values */
	{
		
			
			set_bits(regs, cReg0, audio_value, 0, 2); /* audio unmute */
			
			
			audio_value = 0xff;
		
	}
	dprintk("[AVS]: %s <\n", __func__);
	return stv6419_set(client);
}

inline int stv6419_set_vsw( struct i2c_client *client, int sw, int type )
{
	printk("[AVS]: %s: SET VSW: %d %d\n",__func__, sw, type);

	if (type<0 || type>4)
	{
		return -EINVAL;
	}

	switch(sw)
	{
		case 0:	// VCR video output selection
	        set_bits(regs, cReg2, type, 7, 1);
			break;
		case 1:	// rgb selection
			if (type<0 || type>2)
			{
				return -EINVAL;
			}

			set_bits(regs, cReg2, type, 5, 2);
			break;
		case 2: // TV video output selection
			set_bits(regs, cReg2, type, 3, 2);
			break;
		default:
			return -EINVAL;
	}

	dprintk("[AVS]: %s <\n", __func__);
	return stv6419_set(client);
}

inline int stv6419_set_asw( struct i2c_client *client, int sw, int type )
{
	dprintk("[AVS]: %s >\n", __func__);
	switch(sw)
	{
		case 0:
		case 1:
		case 2:
		if (type<=0 || type>3)
			{
				return -EINVAL;
			}

			/* if muted ? yes: save in temp */
			if ( audio_value == 0xff )
				set_bits(regs, cReg0, type, 0, 2);
			else
				audio_value = type;


			break;
		
		default:
			return -EINVAL;
	}

	dprintk("[AVS]: %s <\n", __func__);
	return stv6419_set(client);
}



inline int stv6419_set_t_sb( struct i2c_client *client, int type )
{
/* fixme: on stv6419 we have another range
 * think on this ->see register description below
 */
	dprintk("[AVS]: %s >\n", __func__);

	if (type<0 || type>3)
	{
		return -EINVAL;
	}

	set_bits(regs, cReg3, type, 2, 3);
	
	dprintk("[AVS]: %s <\n", __func__);
	return stv6419_set(client);
}

inline int stv6419_set_wss( struct i2c_client *client, int vol )
{
/* fixme: on stv6418 we hav more possibilites here
 * think on this ->see register description below
 */
	dprintk("[AVS]: %s >\n", __func__);

	if (vol == SAA_WSS_43F)
	{
           set_bits(regs, cReg3, 3, 2, 3);
	}
	else if (vol == SAA_WSS_169F)
	{
           set_bits(regs, cReg3, 2, 2, 3);
	}
	else if (vol == SAA_WSS_OFF)
	{
           set_bits(regs, cReg3, 0, 2, 3);
	}
	else
	{
		return  -EINVAL;
	}

	dprintk("[AVS]: %s <\n", __func__);
	return stv6419_set(client);
}

inline int stv6419_set_fblk( struct i2c_client *client, int type )
{
	dprintk("[AVS]: %s >\n", __func__);
	if (type<0 || type>3)
	{
		return -EINVAL;
	}

	set_bits(regs, cReg3, type, 5, 2);

	dprintk("[AVS]: %s <\n", __func__);

	return stv6419_set(client);
}



int stv6419_get_status(struct i2c_client *client)
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

int stv6419_get_volume(void)
{
	int c;

	dprintk("[AVS]: %s >\n", __func__);
	
	c = get_bits(regs, cReg1, 1, 5);

	if (c)
	{
		c *= 2;
	}

	dprintk("[AVS]: %s <\n", __func__);

	return c;
}

inline int stv6419_get_mute(void)
{
	dprintk("[AVS]: %s <>\n", __func__);
	return !(audio_value == 0xff);
}

inline int stv6419_get_fblk(void)
{
	dprintk("[AVS]: %s <>\n", __func__);
	return get_bits(regs, cReg3, 5, 2);
}

inline int stv6419_get_t_sb(void)
{
	dprintk("[AVS]: %s <>\n", __func__);
	return get_bits(regs, cReg3, 2, 3);
}



inline int stv6419_get_vsw( int sw )
{
	dprintk("[AVS]: %s >\n", __func__);
	switch(sw)
	{
		case 0:
			return get_bits(regs, cReg2, 7, 1);
			break;
		case 1:
			return get_bits(regs, cReg2, 5, 2);
			break;
		case 2:
			return get_bits(regs, cReg2, 3, 2);
			break;
		default:
			return -EINVAL;
	}

	dprintk("[AVS]: %s <\n", __func__);

	return -EINVAL;
}

inline int stv6419_get_asw( int sw )
{
	dprintk("[AVS]: %s >\n", __func__);
	switch(sw)
	{
		case 0:
		case 1:
		case 2:
			// muted ? yes: return tmp values
			if ( audio_value == 0xff )
				return get_bits(regs, cReg0, 0, 2);
			else
				return audio_value;
		
		default:
			return -EINVAL;
	}

	dprintk("[AVS]: %s <\n", __func__);

	return -EINVAL;
}

int stv6419_set_encoder( struct i2c_client *client, int vol )
{
	return 0;
}
 
int stv6419_set_mode( struct i2c_client *client, int val )
{
	dprintk("[AVS]: SAAIOSMODE command : %d\n", val);

	if (val == SAA_MODE_RGB)
	{
		if(get_bits(regs, cReg2, 5, 2) == 2) // scart selected
			stv6419_s_old_src = 2;
		else
			set_bits(regs, cReg2, 2, 3, 2);
                set_bits(regs, cReg3, 1, 5, 2);    /* fast blanking */
	}
	else if (val == SAA_MODE_FBAS)
	{
		if(get_bits(regs, cReg2, 5, 2) == 2) // scart selected
			stv6419_s_old_src = 2;
		else
			set_bits(regs, cReg2, 2, 3, 3);
                set_bits(regs, cReg3, 0, 5, 2);   /* fast blanking */
	}
	else if (val == SAA_MODE_SVIDEO)
	{
		if(get_bits(regs, cReg2, 5, 2) == 2) // scart selected
			stv6419_s_old_src = 2;
		else
			set_bits(regs, cReg2, 2, 5, 2);
                set_bits(regs, cReg3, 0, 5, 2);   /* fast blanking */
	}
	else
	{
		return  -EINVAL;
	}
 
	dprintk("[AVS]: %s <\n", __func__);
	return stv6419_set(client);
}
 
int stv6419_src_sel( struct i2c_client *client, int src )
{
	dprintk("[AVS]: %s >\n", __func__);

	if (src == SAA_SRC_ENC)
	{
	   set_bits(regs, cReg2, stv6419_s_old_src, 5, 2); /* t_vsc */
	   set_bits(regs, cReg2, stv6419_s_old_src, 7, 1); /* v_vsc */
	   set_bits(regs, cReg2, 2, 5, 2); /* rgb enc*/
	   set_bits(regs, cReg2, 0, 7, 1); /* vcr enc  */
	}
	else if(src == SAA_SRC_SCART)
	{
	   stv6419_s_old_src = get_bits(regs, cReg2, 5, 2);
	   set_bits(regs, cReg2, 2, 5, 2);
	   set_bits(regs, cReg2, 0, 7, 1);
	   
  	}
  	else
	{
		return  -EINVAL;
	}
 
	dprintk("[AVS]: %s <\n", __func__);
	return stv6419_set(client);
}

inline int stv6419_standby( struct i2c_client *client, int type )
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
			memcpy(backup_regs, regs, STV6419_DATA_SIZE);
			
			// Full Stop mode
			set_bits(regs, cReg4, 1, 0, 1); /* enc in */
			set_bits(regs, cReg4, 1, 1, 1); /* vcr in */
			set_bits(regs, cReg4, 1, 2, 1); /* tv  in */
			set_bits(regs, cReg4, 1, 3, 1); /* vcr out */
			set_bits(regs, cReg4, 1, 4, 1); /* tv  out */
			set_bits(regs, cReg4, 1, 5, 1); /* rf  out */
			set_bits(regs, cReg4, 1, 6, 1);
			set_bits(regs, cReg4, 1, 7, 1);

			t_stnby = 1;
		}
		else
			return -EINVAL;		
	}
	else
	{
		if (t_stnby == 1)
		{
			memcpy(regs, backup_regs, STV6419_DATA_SIZE);
			t_stnby = 0;
		}
		else
			return -EINVAL;
	}
 
	dprintk("[AVS]: %s <\n", __func__);
	return stv6419_set(client);
}

int stv6419_command(struct i2c_client *client, unsigned int cmd, void *arg )
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
				return stv6419_set_vsw(client,0,val);
			case AVSIOSVSW2:
				return stv6419_set_vsw(client,1,val);
			case AVSIOSVSW3:
				return stv6419_set_vsw(client,2,val);
			/* set audio */
			case AVSIOSASW1:
				return stv6419_set_asw(client,0,val);
			case AVSIOSASW2:
				return stv6419_set_asw(client,1,val);
			case AVSIOSASW3:
				return stv6419_set_asw(client,2,val);
			/* set vol & mute */
			case AVSIOSVOL:
				return stv6419_set_volume(client,val);
			case AVSIOSMUTE:
				return stv6419_set_mute(client,val);
			/* set video fast blanking */
			case AVSIOSFBLK:
				return stv6419_set_fblk(client,val);
/* no direct manipulation allowed, use set_wss instead */
			/* set slow blanking (tv) */
			case AVSIOSSCARTPIN8:
				return stv6419_set_t_sb(client,scartPin8Table[val]);
			case AVSIOSFNC:
				return stv6419_set_t_sb(client,val);
			case AVSIOSTANDBY:
				return stv6419_standby(client,val);
			default:
				return -EINVAL;
		}
	} else if (cmd & AVSIOGET)
	{
		switch (cmd)
		{
			/* get video */
			case AVSIOGVSW1:
				val = stv6419_get_vsw(0);
				break;
			case AVSIOGVSW2:
				val = stv6419_get_vsw(1);
				break;
			case AVSIOGVSW3:
				val = stv6419_get_vsw(2);
				break;
			/* get audio */
			case AVSIOGASW1:
				val = stv6419_get_asw(0);
				break;
			case AVSIOGASW2:
				val = stv6419_get_asw(1);
				break;
			case AVSIOGASW3:
				val = stv6419_get_asw(2);
				break;
			/* get vol & mute */
			case AVSIOGVOL:
				val = stv6419_get_volume();
				break;
			case AVSIOGMUTE:
				val = stv6419_get_mute();
				break;
			/* get video fast blanking */
			case AVSIOGFBLK:
				val = stv6419_get_fblk();
				break;
			case AVSIOGSCARTPIN8:
				val = scartPin8Table_reverse[stv6419_get_t_sb()];
				break;
			/* get slow blanking (tv) */
			case AVSIOGFNC:
				val = stv6419_get_t_sb();

				break;
			/* get status */
			case AVSIOGSTATUS:
				// TODO: error handling
				val = stv6419_get_status(client);
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
           		 return stv6419_set_mode(client,val);
 	        case SAAIOSENC:
        		 return stv6419_set_encoder(client,val);
		case SAAIOSWSS:
			return stv6419_set_wss(client,val);

		case SAAIOSSRCSEL:
        		 return stv6419_src_sel(client,val);

		default:
			dprintk("[AVS]: SAA command not supported\n");
			return -EINVAL;
		}
	}

	dprintk("[AVS: %s <\n", __func__);
	return 0;
}

int stv6419_command_kernel(struct i2c_client *client, unsigned int cmd, void *arg)
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
				return stv6419_set_vsw(client,0,val);
			case AVSIOSVSW2:
				return stv6419_set_vsw(client,1,val);
			case AVSIOSVSW3:
				return stv6419_set_vsw(client,2,val);
			/* set audio */
			case AVSIOSASW1:
				return stv6419_set_asw(client,0,val);
			case AVSIOSASW2:
				return stv6419_set_asw(client,1,val);
			case AVSIOSASW3:
				return stv6419_set_asw(client,2,val);
			/* set vol & mute */
			case AVSIOSVOL:
				return stv6419_set_volume(client,val);
			case AVSIOSMUTE:
				return stv6419_set_mute(client,val);
			/* set video fast blanking */
			case AVSIOSFBLK:
				return stv6419_set_fblk(client,val);
/* no direct manipulation allowed, use set_wss instead */
			/* set slow blanking (tv) */
			case AVSIOSSCARTPIN8:
			return stv6419_set_t_sb(client,scartPin8Table[val]);

			case AVSIOSFNC:
				return stv6419_set_t_sb(client,val);

			case AVSIOSTANDBY:
				return stv6419_standby(client,val);
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
                                val = stv6419_get_vsw(0);
                                break;
			case AVSIOGVSW2:
                                val = stv6419_get_vsw(1);
                                break;
			case AVSIOGVSW3:
                                val = stv6419_get_vsw(2);
                                break;
			/* get audio */
			case AVSIOGASW1:
                                val = stv6419_get_asw(0);
                                break;
			case AVSIOGASW2:
                                val = stv6419_get_asw(1);
                                break;
			case AVSIOGASW3:
                                val = stv6419_get_asw(2);
                                break;
			/* get vol & mute */
			case AVSIOGVOL:
                                val = stv6419_get_volume();
                                break;
			case AVSIOGMUTE:
                                val = stv6419_get_mute();
                                break;
			/* get video fast blanking */
			case AVSIOGFBLK:
                                val = stv6419_get_fblk();
                                break;
			case AVSIOGSCARTPIN8:
				val = scartPin8Table_reverse[stv6419_get_t_sb()];

				break;
			/* get slow blanking (tv) */
			case AVSIOGFNC:
				val = stv6419_get_t_sb();

				break;
			/* get status */
			case AVSIOGSTATUS:
				// TODO: error handling
				val = stv6419_get_status(client);
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
           		 return stv6419_set_mode(client,val);
 	        case SAAIOSENC:
        		 return stv6419_set_encoder(client,val);
		case SAAIOSWSS:
			return stv6419_set_wss(client,val);

		case SAAIOSSRCSEL:
        		return stv6419_src_sel(client,val);
		default:
			dprintk("[AVS]: %s: SAA command(%d) not supported\n", __func__, cmd);
			return -EINVAL;
		}
	}

	return 0;
}

int stv6419_init(struct i2c_client *client)
{
	
	regs[0] = 0x00; //bit staly START
	regs[1] = 0x1E; //0x00h > 0x06 
	regs[2] = 0x00; //0x01h > 0x02 
	regs[3] = 0x51; //0x02h > 0x52
	regs[4] = 0x55; //0x03h > 0x03 04
	regs[5] = 0x00; //0x04h > 0x04
	regs[6] = 0x00; //bit staly END

/*
	regs[0] = 0x00; //bit staly START
	regs[1] = 0x06; //0x00h
	regs[2] = 0x02; //0x01h
	regs[3] = 0x52; //0x02h
	regs[4] = 0x04; //0x03h
	regs[5] = 0x00; //0x04h
	regs[6] = 0x00; // bit staly END
*/
    return stv6419_set(client);
}
