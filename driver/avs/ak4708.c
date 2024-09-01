/*
 *   ak4708.c - AK4708 audio/video switch driver - Kathrein UFS-910
 *
 *   written by captaintrip - 19.Nov 2007
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
#include "ak4708.h"

/* ---------------------------------------------------------------------- */
 
static int debug = AVS_DEBUG;

/*
 * ak4708 data struct
 *
 */

typedef struct s_ak4708_data {

 /* register 00 - control */
 unsigned char R00STBY	 	: 1;
 unsigned char R00MUTE	 	: 1;
 unsigned char R00DAPD	 	: 1;
 unsigned char R00AUTO		: 1;
 unsigned char R00DIF10 		: 2;
 unsigned char R00DEM10 		: 2;

 /* register 01 - switch */
 unsigned char R01TV10	 	: 2;
 unsigned char R01VOL	 	: 1;
 unsigned char R01MONO	 	: 1;
 unsigned char R01VCR10		: 2;
 unsigned char R01FIXED1		: 1;
 unsigned char R01VMUTE 		: 1;

 /* register 02 - main volume */
 unsigned char R02L50	 	: 6;
 unsigned char R02FIXED1		: 2;

 /* register 03 - zerocross */
 unsigned char R03MDT10	 	: 2;
 unsigned char R03MOD	 	: 1;
 unsigned char R03DVOL10	 	: 2;
 unsigned char R03FIXED1	 	: 1;
 unsigned char R03VMONO	 	: 1;
 unsigned char R03FIXED2	 	: 1;

 /* register 04 - video switch */
 unsigned char R04VTV20	 	: 3;
 unsigned char R04VVCR20	 	: 3;
 unsigned char R04VRF10	 	: 2;

 /* register 05 - output enable */
 unsigned char R05TVV	 	: 1;
 unsigned char R05TVR	 	: 1;
 unsigned char R05TVG	 	: 1;
 unsigned char R05TVB	 	: 1;
 unsigned char R05VCRV	 	: 1;
 unsigned char R05VCRC	 	: 1;
 unsigned char R05VCRB	 	: 1;
 unsigned char R05CIO	 	: 1;

 /* register 06 - video volume */
 unsigned char R06VVOL10	 	: 2;
 unsigned char R06CLAMP20	 	: 3;
 unsigned char R06VCLP10	 	: 2;
 unsigned char R06CLAMPB	 	: 1;

 /* register 07 - s/f blanking */
 unsigned char R07FB10	 	: 2;
 unsigned char R07SBT10	 	: 2;
 unsigned char R07SBV10	 	: 2;
 unsigned char R07SBIO10	 	: 2;

 /* register 08 - monitor - FOR MONITOR PURPOSE ONLY??? */
 unsigned char R08SVCR10	 	: 2;
 unsigned char R08FVCR	 	: 1;
 unsigned char R08VCMON	 	: 1;
 unsigned char R08TVMON	 	: 1;
 unsigned char R08FIXED1	 	: 3;

 /* register 09 - monitor mask - FOR MONITOR PURPOSE ONLY??? */
 unsigned char R09FIXED1	 	: 1;
 unsigned char R09MSVCR	 	: 1;
 unsigned char R09MFVCR	 	: 1;
 unsigned char R09MVC	 	: 1;
 unsigned char R09MTV	 	: 1;
 unsigned char R09FIXED2	 	: 2;
 unsigned char R09MCOMN	 	: 1;

} s_ak4708_data;

#define ak4708_DATA_SIZE sizeof(s_ak4708_data)
 
/* ---------------------------------------------------------------------- */

static struct s_ak4708_data ak4708_data;
static struct s_ak4708_data tmpak4708_data;
static int s_old_src;

/* ---------------------------------------------------------------------- */

int ak4708_set(struct i2c_client *client)
{
	char buffer[ak4708_DATA_SIZE+1];

	dprintk("[AVS]: ak4708_set >>\n");

	buffer[0] = 0;
	memcpy( buffer+1, &ak4708_data, ak4708_DATA_SIZE );

	if ( ak4708_DATA_SIZE != i2c_master_send(client, buffer, ak4708_DATA_SIZE))
	{
		dprintk("[AVS]: ak4708_set error\n");
		return -EFAULT;
	}

	dprintk("[AVS]: ak4708_set <<\n");
	return 0;
}

/* ---------------------------------------------------------------------- */

inline int ak4708_standby( struct i2c_client *client, int type )
{
	if ((type<0) || (type>1))
	{
		return -EINVAL;
	}

	if (type==1) 
	{
		if (ak4708_data.R00STBY == 0)
		{
			tmpak4708_data = ak4708_data;
			ak4708_data.R00STBY = 1;
		}
		else
			return -EINVAL;		
	}
	else
	{
		if (ak4708_data.R00STBY == 1)
			ak4708_data = tmpak4708_data;
		else
			return -EINVAL;
	}

	return ak4708_set(client);
}
 
/* ---------------------------------------------------------------------- */
 
int ak4708_set_volume( struct i2c_client *client, int vol )
{
	int c=0;

	c = vol;

//	if (c > 63 || c < 0)
//		return -EINVAL;
// 	ak4708_data.R02L50 = c;

	return ak4708_set(client);
}

/* ---------------------------------------------------------------------- */

inline int ak4708_set_mute( struct i2c_client *client, int type )
{
	if ((type<0) || (type>1))
	{
		return -EINVAL;
	}

	if (type==AVS_MUTE) 
	{
		ak4708_data.R01VMUTE = 1;
	}
	else
	{
		ak4708_data.R01VMUTE = 0;
	}

	return ak4708_set(client);
}

/* ---------------------------------------------------------------------- */
 
int ak4708_get_volume(void)
{
	int c;

	c = ak4708_data.R02L50;

	return c;
}

/* ---------------------------------------------------------------------- */

inline int ak4708_get_mute(void)
{
	return ak4708_data.R00MUTE;
}

/* ---------------------------------------------------------------------- */

int ak4708_set_mode( struct i2c_client *client, int vol )
{
	dprintk("[AVS]: SAAIOSMODE command : %d\n", vol);
	if (vol == SAA_MODE_RGB)
	{
		if(ak4708_data.R04VTV20 == 4) // scart selected
			s_old_src = 1;
		else
		{
			ak4708_data.R04VTV20 = 1;
			ak4708_data.R07FB10 = 1;
		}
	}
	else if (vol == SAA_MODE_FBAS)
	{
		if(ak4708_data.R04VTV20 == 4) // scart selected
			s_old_src = 1;
		else
		{
			ak4708_data.R04VTV20 = 1;
			ak4708_data.R07FB10 = 0;
		}
	}
	else if (vol == SAA_MODE_SVIDEO)
	{
		if(ak4708_data.R04VTV20 == 4) // scart selected
			s_old_src = 3;
		else
		{
			ak4708_data.R04VTV20 = 3;
			ak4708_data.R07FB10 = 0;
		}
	}
	else
	{
		return  -EINVAL;
	}

	return ak4708_set(client);
}

/* ---------------------------------------------------------------------- */
//NOT IMPLEMENTED
int ak4708_set_encoder( struct i2c_client *client, int vol )
{

	return 0;
}

/* ---------------------------------------------------------------------- */

int ak4708_set_wss( struct i2c_client *client, int vol )
{
	if (vol == SAA_WSS_43F)
	{
		ak4708_data.R07SBT10 = 3;
	}
	else if (vol == SAA_WSS_169F)
	{
		ak4708_data.R07SBT10 = 1;
	}
	else if (vol == SAA_WSS_OFF) //?? wir setzen einfach mal auf 4/3
	{
		ak4708_data.R07SBT10 = 0;
	}
	else
	{
		return  -EINVAL;
	}

	return ak4708_set(client);
}

/* ---------------------------------------------------------------------- */

int ak4708_src_sel( struct i2c_client *client, int src )
{
	if (src == SAA_SRC_ENC)
	{
		ak4708_data.R04VTV20 = s_old_src;
		ak4708_data.R04VVCR20 = s_old_src;
		ak4708_data.R01TV10 = 0;
		ak4708_data.R01VCR10 = 0;
	}
	else if(src == SAA_SRC_SCART)
	{
		s_old_src = ak4708_data.R04VTV20;
		ak4708_data.R04VTV20 = 4;
		ak4708_data.R04VVCR20 = 0;
	 	ak4708_data.R01TV10 = 1;
		ak4708_data.R01VCR10 = 2;
	}
	else
	{
		return  -EINVAL;
	}

	return ak4708_set(client);
}

/* ---------------------------------------------------------------------- */

int ak4708_command(struct i2c_client *client, unsigned int cmd, void *arg)
{
	int val=0;

	dprintk("[AVS]: command(%u)\n", cmd);

	if (cmd&AVSIOSET)
	{
		if ( copy_from_user(&val,arg,sizeof(val)) )
		{
			return -EFAULT;
		}

		dprintk("[AVS]: AVSIOSET command\n");

		switch (cmd)
		{
			case AVSIOSVOL:
				return ak4708_set_volume(client,val);
			case AVSIOSMUTE:
				return ak4708_set_mute(client,val);
			case AVSIOSTANDBY:
				return ak4708_standby(client,val);
			default:
			{
				dprintk("[AVS]: AVSIOSET command not supported\n");
				return -EINVAL;
			}
		}
	} else if (cmd&AVSIOGET)
	{
		dprintk("[AVS]: AVSIOGET command\n");

		switch (cmd)
		{
			case AVSIOGVOL:
				val = ak4708_get_volume();
			case AVSIOGMUTE:
				val = ak4708_get_mute();
				break;
			default:
			{
				dprintk("[AVS]: AVSIOGET command not supported\n");
				return -EINVAL;
			}
		}

		return put_user(val,(int*)arg);
	} else
	{
		dprintk("[AVS]: SAA command\n");

		if ( copy_from_user(&val,arg,sizeof(val)) )
		{
			dprintk("[AVS]: SAA command - fault\n");
			return -EFAULT;
		}

		switch (cmd)
		{
			case SAAIOSMODE:
				return ak4708_set_mode(client,val);
			case SAAIOSENC:
				return ak4708_set_encoder(client,val);
			case SAAIOSWSS:
				return ak4708_set_wss(client,val);
			case SAAIOSSRCSEL:
				return ak4708_src_sel(client,val);
			default:
			{
				dprintk("[AVS]: SAA command not supported\n");
				return -EINVAL;
			}
		}

	}

	return 0;
}

/* ---------------------------------------------------------------------- */

int ak4708_command_kernel(struct i2c_client *client, unsigned int cmd, void *arg)
{
   int val=0;

   dprintk("[AVS]: command_kernel(%u)\n", cmd);

   if (cmd&AVSIOSET)
   {
      val = (int) arg;

      dprintk("[AVS]: AVSIOSET command\n");

      switch (cmd)
      {
         case AVSIOSVOL:
            return ak4708_set_volume(client,val);
         case AVSIOSMUTE:
            return ak4708_set_mute(client,val);
         case AVSIOSTANDBY:
            return ak4708_standby(client,val);
         default:
         {
            dprintk("[AVS]: AVSIOSET command not supported\n");
            return -EINVAL;
         }
      }
   } else if (cmd&AVSIOGET)
   {
      dprintk("[AVS]: AVSIOGET command\n");

      switch (cmd)
      {
         case AVSIOGVOL:
            val = ak4708_get_volume();
            break;
         case AVSIOGMUTE:
            val = ak4708_get_mute();
            break;
         default:
         {
            dprintk("[AVS]: AVSIOGET command not supported\n");
            return -EINVAL;
         }
      }
      *((int *)arg) = val;
      return 0;
   } else
   {
      dprintk("[AVS]: SAA command\n");

      val = (int) arg;

      switch (cmd)
      {
         case SAAIOSMODE:
            return ak4708_set_mode(client,val);
         case SAAIOSENC:
            return ak4708_set_encoder(client,val);
         case SAAIOSWSS:
            return ak4708_set_wss(client,val);
        case SAAIOSSRCSEL:
            return ak4708_src_sel(client,val);
         default:
         {
            dprintk("[AVS]: SAA command not supported\n");
            return -EINVAL;
         }
      }


   }

   return 0;
}

/* ---------------------------------------------------------------------- */

int ak4708_init(struct i2c_client *client)
{
	dprintk("[AVS]: ak4708_init\n");
	memset((void*)&ak4708_data,0,ak4708_DATA_SIZE);

	/* default values for tv fbas - vcr fbas*/

	/* register 00 - 0x70 = 01110000 */
	ak4708_data.R00DEM10 = 1;
	ak4708_data.R00DIF10 = 3;
	ak4708_data.R00AUTO = 0;
	ak4708_data.R00DAPD = 0;
	ak4708_data.R00MUTE = 0;
	ak4708_data.R00STBY = 0;

	/* register 01 - 0x40 = 01000000 */
	ak4708_data.R01VMUTE = 0;
	ak4708_data.R01FIXED1 = 1;
	ak4708_data.R01VCR10 = 0;
	ak4708_data.R01MONO = 0;
	ak4708_data.R01VOL = 0;
	ak4708_data.R01TV10 = 0;

	/* register 02 - 0x1F = 00011111 */
	ak4708_data.R02FIXED1 = 0;
	ak4708_data.R02L50 = 31;

	/* register 03 - 0x04 = 00000100 */
	ak4708_data.R03FIXED2 = 0;
	ak4708_data.R03VMONO = 0;
	ak4708_data.R03FIXED1 = 0;
	ak4708_data.R03DVOL10 = 0;
	ak4708_data.R03MOD = 1;
	ak4708_data.R03MDT10 = 0;

	/* register 04 - 0x09 = 00001001 */
	ak4708_data.R04VRF10 = 0;
	ak4708_data.R04VVCR20 = 1;
	ak4708_data.R04VTV20 = 1;
	s_old_src = 1;

	/* register 05 - 0xFF = 11111111 */
	ak4708_data.R05CIO = 1;
	ak4708_data.R05VCRB = 1;
	ak4708_data.R05VCRC = 1;
	ak4708_data.R05VCRV = 1;
	ak4708_data.R05TVB = 1;
	ak4708_data.R05TVG = 1;
	ak4708_data.R05TVR = 1;
	ak4708_data.R05TVV = 1;

	/* register 06 - 0x00 = 00000000 */
	ak4708_data.R06CLAMPB = 0;
	ak4708_data.R06VCLP10 = 0;
	ak4708_data.R06CLAMP20 = 0;
	ak4708_data.R06VVOL10 = 0;

	/* register 07 - 0x8C = 01110000 */
	ak4708_data.R07SBIO10 = 2;
	ak4708_data.R07SBV10 = 0;
	ak4708_data.R07SBT10 = 3;
	ak4708_data.R07FB10 = 0;

	return ak4708_set(client);
}

/* ---------------------------------------------------------------------- */
