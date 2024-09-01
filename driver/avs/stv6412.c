/*
 *   stv6412.c - audio/video switch driver (dbox-II-project)
 *
 *   Homepage: http://www.tuxbox.org
 *
 *   Copyright (C) 2000-2002 Gillem gillem@berlios.de
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
 *   Revision 1.26.
 *
 *   The code has been adapted to TF7700.
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
#include "stv6412.h"

/* ---------------------------------------------------------------------- */

static int debug = AVS_DEBUG;

/*
 * stv6412 data struct
 *
 */
#if 1 
//defined(__LITTLE_ENDIAN__)
typedef struct s_stv6412_data {
 /* Data 0 */
 unsigned char svm			: 1;
 unsigned char t_vol_c			: 5;
 unsigned char t_vol_x			: 1;
 unsigned char t_stereo			: 1;
 /* Data 1 */
 unsigned char tc_asc			: 3;
 unsigned char c_ag			: 1;
 unsigned char v_asc			: 2;
 unsigned char res1			: 1;
 unsigned char v_stereo			: 1;
 /* Data 2 */
 unsigned char t_vsc			: 3;
 unsigned char t_cm			: 1;
 unsigned char v_vsc			: 3;
 unsigned char v_cm			: 1;
 /* Data 3 */
 unsigned char fblk			: 2;
 unsigned char rgb_vsc			: 2;
 unsigned char rgb_gain			: 3;
 unsigned char rgb_tri			: 1;
 /* Data 4 */
 unsigned char t_rcos			: 1;
 unsigned char r_ac			: 1;
 unsigned char r_tfc			: 1;
 unsigned char v_cgc			: 1;
 unsigned char v_coc			: 1;
 unsigned char res2			: 1;
 unsigned char slb			: 1;
 unsigned char it_enable		: 1;
 /* Data 5 */
 unsigned char e_rcsc			: 1;
 unsigned char v_rcsc			: 1;
 unsigned char e_aig			: 2;
 unsigned char t_sb			: 2;
 unsigned char v_sb			: 2;
 /* Data 6 */
 unsigned char e_in			: 1;
 unsigned char v_in			: 1;
 unsigned char t_in			: 1;
 unsigned char a_in			: 1;
 unsigned char v_out			: 1;
 unsigned char c_out			: 1;
 unsigned char t_out			: 1;
 unsigned char r_out			: 1;
} s_stv6412_data;
#else
typedef struct s_stv6412_data {
 /* Data 0 */
 unsigned char t_stereo			: 1;
 unsigned char t_vol_x			: 1;
 unsigned char t_vol_c			: 5;
 unsigned char svm			: 1;
 /* Data 1 */
 unsigned char v_stereo			: 1;
 unsigned char res1			: 1;
 unsigned char v_asc			: 2;
 unsigned char c_ag			: 1;
 unsigned char tc_asc			: 3;
 /* Data 2 */
 unsigned char v_cm			: 1;
 unsigned char v_vsc			: 3;
 unsigned char t_cm			: 1;
 unsigned char t_vsc			: 3;
 /* Data 3 */
 unsigned char rgb_tri			: 1;
 unsigned char rgb_gain			: 3;
 unsigned char rgb_vsc			: 2;
 unsigned char fblk			: 2;
 /* Data 4 */
 unsigned char it_enable		: 1;
 unsigned char slb			: 1;
 unsigned char res2			: 1;
 unsigned char v_coc			: 1;
 unsigned char v_cgc			: 1;
 unsigned char r_tfc			: 1;
 unsigned char r_ac			: 1;
 unsigned char t_rcos			: 1;
 /* Data 5 */
 unsigned char v_sb			: 2;
 unsigned char t_sb			: 2;
 unsigned char e_aig			: 2;
 unsigned char v_rcsc			: 1;
 unsigned char e_rcsc			: 1;
 /* Data 6 */
 unsigned char r_out			: 1;
 unsigned char t_out			: 1;
 unsigned char c_out			: 1;
 unsigned char v_out			: 1;
 unsigned char a_in			: 1;
 unsigned char t_in			: 1;
 unsigned char v_in			: 1;
 unsigned char e_in			: 1;
} s_stv6412_data;
#endif

#define STV6412_DATA_SIZE sizeof(s_stv6412_data)


/* hold old values for mute/unmute */
unsigned char tc_asc;
unsigned char v_asc;

/* hold old values for standby */
unsigned char t_stnby=0;

/* ---------------------------------------------------------------------- */

static struct s_stv6412_data stv6412_data;
static struct s_stv6412_data tmpstv6412_data;
static int stv6412_s_old_src;

/* ---------------------------------------------------------------------- */
#if defined(FORTIS_HDBOX) || defined(HL101)  || defined(IPBOX9900)
//Trick: hack ;)
int stv6412_set(struct i2c_client *client)
{
	char buffer[10];

	printk("[AVS] [STV6418] set!\n");

	buffer[0] = 0x00;
	buffer[1] = 0x40;
	buffer[2] = 0x09;
	buffer[3] = 0x11;
	buffer[4] = 0x84;
	buffer[5] = 0x84;
	buffer[6] = 0x25;
	buffer[7] = 0x08;
	buffer[8] = 0x21;
	buffer[9] = 0xc0;
	buffer[10] = 0x00;
	i2c_master_send(client, buffer, 10);

	return 0;
}
#else
int stv6412_set(struct i2c_client *client)
{
	char buffer[STV6412_DATA_SIZE+1];

	buffer[0] = 0;

	memcpy( buffer+1, &stv6412_data, STV6412_DATA_SIZE );

	if ( (STV6412_DATA_SIZE+1) != i2c_master_send(client, buffer, STV6412_DATA_SIZE+1))
	{
		return -EFAULT;
	}

	return 0;
}
#endif
/* ---------------------------------------------------------------------- */

int stv6412_set_volume( struct i2c_client *client, int vol )
{
	int c=0;

	c = vol;

// not needed
//	// not smart ;-)
//	if ( c == 63 )
//	{
//		c--;
//	}
#if !defined(ADB_BOX)
	if(c==63)
		c=62;
	if(c==0)
		c=1;

	if ((c > 63) || (c < 0))
		return -EINVAL;

	c /= 2;

	stv6412_data.t_vol_c = c;
#endif
	return stv6412_set(client);
}

/* ---------------------------------------------------------------------- */

inline int stv6412_set_mute( struct i2c_client *client, int type )
{
	if ((type<0) || (type>1))
	{
		return -EINVAL;
	}

	if (type==AVS_MUTE) 
	{
		if (tc_asc == 0xff)
		{
			/* save old values */
			tc_asc = stv6412_data.tc_asc;
			v_asc  = stv6412_data.v_asc;

			/* set mute */
			stv6412_data.tc_asc = 0;	// tv & cinch mute
			stv6412_data.v_asc  = 0;	// vcr mute
		}
	}
	else /* unmute with old values */
	{
		if (tc_asc != 0xff)
		{
			stv6412_data.tc_asc = tc_asc;
			stv6412_data.v_asc  = v_asc;

			tc_asc = 0xff;
			v_asc  = 0xff;
		}
	}

	return stv6412_set(client);
}

/* ---------------------------------------------------------------------- */

inline int stv6412_set_vsw( struct i2c_client *client, int sw, int type )
{
	printk("SET VSW: %d %d\n",sw,type);

	if (type<0 || type>4)
	{
		return -EINVAL;
	}

	switch(sw)
	{
		case 0:	// vcr
			stv6412_data.v_vsc = type;
			break;
		case 1:	// rgb
			if (type<0 || type>2)
			{
				return -EINVAL;
			}

			stv6412_data.rgb_vsc = type;
			break;
		case 2: // tv
			stv6412_data.t_vsc = type;
			break;
		default:
			return -EINVAL;
	}

	return stv6412_set(client);
}

/* ---------------------------------------------------------------------- */

inline int stv6412_set_asw( struct i2c_client *client, int sw, int type )
{
	switch(sw)
	{
		case 0:
			if (type<=0 || type>3)
			{
				return -EINVAL;
			}

			/* if muted ? yes: save in temp */
			if ( v_asc == 0xff )
				stv6412_data.v_asc = type;
			else
				v_asc = type;

			break;
		case 1:
		case 2:
			if (type<=0 || type>4)
			{
				return -EINVAL;
			}

			/* if muted ? yes: save in temp */
			if ( tc_asc == 0xff )
				stv6412_data.tc_asc = type;
			else
				tc_asc = type;

			break;
		default:
			return -EINVAL;
	}

	return stv6412_set(client);
}

/* ---------------------------------------------------------------------- */

inline int stv6412_set_t_sb( struct i2c_client *client, int type )
{
	if (type<0 || type>3)
	{
		return -EINVAL;
	}

	stv6412_data.t_sb = type;

	return stv6412_set(client);
}

/* ---------------------------------------------------------------------- */

inline int stv6412_set_wss( struct i2c_client *client, int vol )
{
	if (vol == SAA_WSS_43F)
	{
		stv6412_data.t_sb = 3;
	}
	else if (vol == SAA_WSS_169F)
	{
		stv6412_data.t_sb = 2;
	}
	else if (vol == SAA_WSS_OFF)
	{
		stv6412_data.t_sb = 0; //1;
	}
	else
	{
		return  -EINVAL;
	}

	return stv6412_set(client);
}

/* ---------------------------------------------------------------------- */

inline int stv6412_set_fblk( struct i2c_client *client, int type )
{
	if (type<0 || type>3)
	{
		return -EINVAL;
	}

	stv6412_data.fblk = type;

	return stv6412_set(client);
}

/* ---------------------------------------------------------------------- */

int stv6412_get_status(struct i2c_client *client)
{
	unsigned char byte;

	byte = 0;

	if (1 != i2c_master_recv(client,&byte,1))
	{
		return -1;
	}

	return byte;
}

/* ---------------------------------------------------------------------- */

int stv6412_get_volume(void)
{
	int c;

	c = stv6412_data.t_vol_c;

	if (c)
	{
		c *= 2;
	}

	return c;
}

/* ---------------------------------------------------------------------- */

inline int stv6412_get_mute(void)
{
	return !((tc_asc == 0xff) && (v_asc == 0xff));
}

/* ---------------------------------------------------------------------- */

inline int stv6412_get_fblk(void)
{
	return stv6412_data.fblk;
}

/* ---------------------------------------------------------------------- */

inline int stv6412_get_t_sb(void)
{
	return stv6412_data.t_sb;
}

/* ---------------------------------------------------------------------- */

inline int stv6412_get_vsw( int sw )
{
	switch(sw)
	{
		case 0:
			return stv6412_data.v_vsc;
			break;
		case 1:
			return stv6412_data.rgb_vsc;
			break;
		case 2:
			return stv6412_data.t_vsc;
			break;
		default:
			return -EINVAL;
	}

	return -EINVAL;
}

/* ---------------------------------------------------------------------- */

inline int stv6412_get_asw( int sw )
{
	switch(sw)
	{
		case 0:
			// muted ? yes: return tmp values
			if ( v_asc == 0xff )
				return stv6412_data.v_asc;
			else
				return v_asc;
		case 1:
		case 2:
			if ( tc_asc == 0xff )
				return stv6412_data.tc_asc;
			else
				return tc_asc;
			break;
		default:
			return -EINVAL;
	}

	return -EINVAL;
}

/* ---------------------------------------------------------------------- */
//NOT IMPLEMENTED
int stv6412_set_encoder( struct i2c_client *client, int vol )
{

	return 0;
}

/* ---------------------------------------------------------------------- */
 
int stv6412_set_mode( struct i2c_client *client, int vol )
{
	dprintk("[AVS]: SAAIOSMODE command : %d\n", vol);
	if (vol == SAA_MODE_RGB)
	{
		if(stv6412_data.t_vsc == 4) // scart selected
			stv6412_s_old_src = 1;
		else
			stv6412_data.t_vsc = 1;
		stv6412_data.fblk = 1;
	}
	else if (vol == SAA_MODE_FBAS)
	{
		if(stv6412_data.t_vsc == 4) // scart selected
			stv6412_s_old_src = 1;
		else
			stv6412_data.t_vsc = 1;
		stv6412_data.fblk = 0;
	}
	else if (vol == SAA_MODE_SVIDEO)
	{
		if(stv6412_data.t_vsc == 4) // scart selected
			stv6412_s_old_src = 2;
		else
			stv6412_data.t_vsc = 2;
		stv6412_data.fblk = 0;
	}
	else
	{
		return  -EINVAL;
	}
 
	return stv6412_set(client);
}

/* ---------------------------------------------------------------------- */
 
int stv6412_src_sel( struct i2c_client *client, int src )
{
	if (src == SAA_SRC_ENC)
	{
	   stv6412_data.t_vsc = stv6412_s_old_src;
	   stv6412_data.v_vsc = stv6412_s_old_src;
	   stv6412_data.tc_asc = 1;
	   stv6412_data.v_asc = 1;
	}
	else if(src == SAA_SRC_SCART)
	{
	   stv6412_s_old_src = stv6412_data.t_vsc;
#if !defined(ADB_BOX)
	   stv6412_data.t_vsc = 4;
	   stv6412_data.v_vsc = 0;
	   stv6412_data.tc_asc = 2;
	   stv6412_data.v_asc = 0;
#endif
  	}
  	else
	{
		return  -EINVAL;
	}
 
	return stv6412_set(client);
}

/* ---------------------------------------------------------------------- */

inline int stv6412_standby( struct i2c_client *client, int type )
{
 
	if ((type<0) || (type>1))
	{
		return -EINVAL;
	}
 
	if (type==1) 
	{
		if (t_stnby == 0)
		{
			tmpstv6412_data = stv6412_data;
			// Full Stop mode
			/* Data 6 */
			stv6412_data.e_in = 1;
			stv6412_data.v_in = 1;
			stv6412_data.t_in = 1;
			stv6412_data.a_in = 1;
			stv6412_data.v_out = 1;
			stv6412_data.c_out = 1;
			stv6412_data.t_out = 1;
			stv6412_data.r_out = 1;
			t_stnby = 1;
		}
		else
			return -EINVAL;		
	}
	else
	{
		if (t_stnby == 1)
		{
			stv6412_data = tmpstv6412_data;
			t_stnby = 0;
		}
		else
			return -EINVAL;
	}
 
	return stv6412_set(client);
}

/* ---------------------------------------------------------------------- */

int stv6412_command(struct i2c_client *client, unsigned int cmd, void *arg )
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
				return stv6412_set_vsw(client,0,val);
			case AVSIOSVSW2:
				return stv6412_set_vsw(client,1,val);
			case AVSIOSVSW3:
				return stv6412_set_vsw(client,2,val);
			/* set audio */
			case AVSIOSASW1:
				return stv6412_set_asw(client,0,val);
			case AVSIOSASW2:
				return stv6412_set_asw(client,1,val);
			case AVSIOSASW3:
				return stv6412_set_asw(client,2,val);
			/* set vol & mute */
			case AVSIOSVOL:
				return stv6412_set_volume(client,val);
			case AVSIOSMUTE:
				return stv6412_set_mute(client,val);
			/* set video fast blanking */
			case AVSIOSFBLK:
#if defined(FORTIS_HDBOX) || defined(HL101) || defined(IPBOX9900) || defined(IPBOX99)
				printk("[AVS STV6418] not support AVSIOSFBLK yet!\n");
				return;
#else
				return stv6412_set_fblk(client,val);
#endif

#if 1
/* no direct manipulation allowed, use set_wss instead */
			/* set slow blanking (tv) */
			case AVSIOSSCARTPIN8:
				return stv6412_set_t_sb(client,scartPin8Table[val]);
			case AVSIOSFNC:
				return stv6412_set_t_sb(client,val);
#endif
			case AVSIOSTANDBY:
				return stv6412_standby(client,val);
			default:
				return -EINVAL;
		}
	} else if (cmd & AVSIOGET)
	{
		switch (cmd)
		{
			/* get video */
			case AVSIOGVSW1:
                                val = stv6412_get_vsw(0);
                                break;
			case AVSIOGVSW2:
                                val = stv6412_get_vsw(1);
                                break;
			case AVSIOGVSW3:
                                val = stv6412_get_vsw(2);
                                break;
			/* get audio */
			case AVSIOGASW1:
                                val = stv6412_get_asw(0);
                                break;
			case AVSIOGASW2:
                                val = stv6412_get_asw(1);
                                break;
			case AVSIOGASW3:
                                val = stv6412_get_asw(2);
                                break;
			/* get vol & mute */
			case AVSIOGVOL:
                                val = stv6412_get_volume();
                                break;
			case AVSIOGMUTE:
                                val = stv6412_get_mute();
                                break;
			/* get video fast blanking */
			case AVSIOGFBLK:
#if defined(FORTIS_HDBOX) || defined(HL101) || defined(IPBOX9900) || defined(IPBOX99)
				printk("[AVS STV6418] not support AVSIOSFBLK yet!\n");
				break;
#else
                                val = stv6412_get_fblk();
                                break;
#endif
			case AVSIOGSCARTPIN8:
				val = scartPin8Table_reverse[stv6412_get_t_sb()];
				break;
			/* get slow blanking (tv) */
			case AVSIOGFNC:
				val = stv6412_get_t_sb();
				break;
			/* get status */
			case AVSIOGSTATUS:
                                // TODO: error handling
                                val = stv6412_get_status(client);
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
           		 return stv6412_set_mode(client,val);
 	        case SAAIOSENC:
        		 return stv6412_set_encoder(client,val);
		case SAAIOSWSS:
			return stv6412_set_wss(client,val);
		case SAAIOSSRCSEL:
        		return stv6412_src_sel(client,val);
		default:
			dprintk("[AVS]: SAA command not supported\n");
			return -EINVAL;
		}
	}

	return 0;
}

/* ---------------------------------------------------------------------- */
 
int stv6412_command_kernel(struct i2c_client *client, unsigned int cmd, void *arg)
{
   int val=0;

	unsigned char scartPin8Table[3] = { 0, 2, 3 };
	unsigned char scartPin8Table_reverse[4] = { 0, 0, 1, 2 };

	dprintk("[AVS]: command_kernel(%u)\n", cmd);
	
	if (cmd & AVSIOSET)
	{
		val = (int) arg;

      		dprintk("[AVS]: AVSIOSET command\n");

		switch (cmd)
		{
			/* set video */
			case AVSIOSVSW1:
				return stv6412_set_vsw(client,0,val);
			case AVSIOSVSW2:
				return stv6412_set_vsw(client,1,val);
			case AVSIOSVSW3:
				return stv6412_set_vsw(client,2,val);
			/* set audio */
			case AVSIOSASW1:
				return stv6412_set_asw(client,0,val);
			case AVSIOSASW2:
				return stv6412_set_asw(client,1,val);
			case AVSIOSASW3:
				return stv6412_set_asw(client,2,val);
			/* set vol & mute */
			case AVSIOSVOL:
				return stv6412_set_volume(client,val);
			case AVSIOSMUTE:
				return stv6412_set_mute(client,val);
			/* set video fast blanking */
			case AVSIOSFBLK:
				return stv6412_set_fblk(client,val);
#if 1
/* no direct manipulation allowed, use set_wss instead */
			/* set slow blanking (tv) */
			case AVSIOSSCARTPIN8:
				return stv6412_set_t_sb(client,scartPin8Table[val]);
			case AVSIOSFNC:
				return stv6412_set_t_sb(client,val);
#endif
			case AVSIOSTANDBY:
				return stv6412_standby(client,val);
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
                                val = stv6412_get_vsw(0);
                                break;
			case AVSIOGVSW2:
                                val = stv6412_get_vsw(1);
                                break;
			case AVSIOGVSW3:
                                val = stv6412_get_vsw(2);
                                break;
			/* get audio */
			case AVSIOGASW1:
                                val = stv6412_get_asw(0);
                                break;
			case AVSIOGASW2:
                                val = stv6412_get_asw(1);
                                break;
			case AVSIOGASW3:
                                val = stv6412_get_asw(2);
                                break;
			/* get vol & mute */
			case AVSIOGVOL:
                                val = stv6412_get_volume();
                                break;
			case AVSIOGMUTE:
                                val = stv6412_get_mute();
                                break;
			/* get video fast blanking */
			case AVSIOGFBLK:
                                val = stv6412_get_fblk();
                                break;
			case AVSIOGSCARTPIN8:
				val = scartPin8Table_reverse[stv6412_get_t_sb()];
				break;
			/* get slow blanking (tv) */
			case AVSIOGFNC:
				val = stv6412_get_t_sb();
				break;
			/* get status */
			case AVSIOGSTATUS:
                                // TODO: error handling
                                val = stv6412_get_status(client);
                                break;
			default:
				return -EINVAL;
		}

		*((int*) arg) = (int) val;
	        return 0;
	}
	else
	{
		printk("[AVS]: SAA command\n");

		val = (int) arg;

		switch(cmd)
		{
		case SAAIOSMODE:
           		 return stv6412_set_mode(client,val);
 	        case SAAIOSENC:
        		 return stv6412_set_encoder(client,val);
		case SAAIOSWSS:
			return stv6412_set_wss(client,val);
		case SAAIOSSRCSEL:
        		return stv6412_src_sel(client,val);
		default:
			dprintk("[AVS]: SAA command not supported\n");
			return -EINVAL;
		}
	}

	return 0;
}

/* ---------------------------------------------------------------------- */

int stv6412_init(struct i2c_client *client)
{
	memset((void*)&stv6412_data,0,STV6412_DATA_SIZE);

#if 0
	stv6412_data.svm = 0;
	stv6412_data.t_vol_c = 0;
	stv6412_data.t_vol_x = 0;
	stv6412_data.t_stereo = 0;

	stv6412_data.c_ag = 0;
	stv6412_data.tc_asc = 1;
	stv6412_data.v_asc  = 1;
	stv6412_data.v_stereo = 0;

	stv6412_data.t_vsc = 1;
	stv6412_data.rgb_vsc = 1;
	stv6412_data.t_cm = 0;
	stv6412_data.rgb_gain = 0;
	stv6412_data.rgb_tri = 1;

	stv6412_data.v_cm = 0;
	stv6412_data.v_vsc = 1;
	stv6412_data.rgb_gain = 0;
	stv6412_data.rgb_tri = 1;
	stv6412_data.fblk = 0;

	stv6412_data.slb = 0;
	stv6412_data.t_sb = 3;

	stv6412_data.a_in  = 1;
	stv6412_data.v_sb = 0;
#else
	/* Data 0 */
	stv6412_data.t_vol_c = 0;
	 /* Data 1 */
	stv6412_data.v_asc  = 1;
	stv6412_data.tc_asc = 1;
	/* Data 2 */
	stv6412_data.v_vsc  = 1;
	stv6412_data.t_vsc  = 1;
	/* Data 3 */
	stv6412_data.rgb_tri  = 1;
	stv6412_data.rgb_vsc  = 1;
	stv6412_data.fblk     = 2;
	/* Data 4 */
	stv6412_data.t_rcos  = 1;
	stv6412_data.res2  = 1;
	/* Data 5 */
	stv6412_data.t_sb  = 3;
	stv6412_data.v_sb  = 0;
	/* Data 6 */
	stv6412_data.a_in  = 0;
#endif

	stv6412_s_old_src = 1;

	/* save mute/unmute values */
	tc_asc = 0xff;
	v_asc  = 0xff;

	return stv6412_set(client);
}

/* ---------------------------------------------------------------------- */
