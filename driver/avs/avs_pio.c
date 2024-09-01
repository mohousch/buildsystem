/*
 *   avs_pio.c -
 *
 *   spider-team 2011.
 *
 *   mainly based on avs_core.c from Gillem gillem@berlios.de / Tuxbox-Project
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
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,18)
#include <linux/stpio.h>
#else
#include <linux/stm/pio.h>
#endif

#include "avs_core.h"
#include "avs_pio.h"

static int debug = AVS_DEBUG;

/* hold old values for mute/unmute */
static unsigned char t_mute;

/* hold old values for volume */
static unsigned char t_vol;

/* hold old values for standby */
static unsigned char ft_stnby=0;

static struct stpio_pin*	avs_mode;
static struct stpio_pin*	avs_format;
static struct stpio_pin*	avs_standby;
static struct stpio_pin*	avs_mute;
//static struct stpio_pin*	avs_src;

int avs_pio_src_sel(int src)
{
	return 0;
}

inline int avs_pio_standby(int type)
{
	if ((type<0) || (type>1))
	{
		return -EINVAL;
	}

	if (type==1)
	{
		if (ft_stnby == 0)
		{
#if defined(HS7420) || defined(HS7429) || defined(HS7810A) || defined(HS7819)
			stpio_set_pin(avs_format, 0);
			stpio_set_pin(avs_standby, 1);
			stpio_set_pin(avs_mode, 0);
#else
			stpio_set_pin(avs_standby, !ft_stnby);
#endif
			ft_stnby = 1;
		}
		else
			return -EINVAL;
	}
	else
	{
		if (ft_stnby == 1)
		{
#if defined(HS7420) || defined(HS7429) || defined(HS7810A) || defined(HS7819)
			stpio_set_pin(avs_format, 0);
			stpio_set_pin(avs_standby, 0);
			stpio_set_pin(avs_mode, 0);
#else
			stpio_set_pin(avs_standby, !ft_stnby);
#endif
			ft_stnby = 0;
		}
		else
			return -EINVAL;
	}

	return 0;
}

int avs_pio_set_volume(int vol)
{
	int c=0;

	dprintk("[AVS]: %s (%d)\n", __func__, vol);
	c = vol;

	if (c > 63 || c < 0)
		return -EINVAL;

	c = 63 - c;

	c=c/2;

	t_vol = c;

	return 0;
}

inline int avs_pio_set_mute(int type)
{
	if ((type<0) || (type>1))
	{
		return -EINVAL;
	}

	if (type==AVS_MUTE)
	{
		#if defined(SPARK7162)
		t_mute = 0;
		#else
		t_mute = 1;
		#endif
	}
	else
	{
		#if defined(SPARK7162)
		t_mute = 1;
		#else
		t_mute = 0;
		#endif
	}


#if defined(SPARK) || defined(SPARK7162)
	stpio_set_pin(avs_mute, t_mute);
#endif

	return 0;
}

int avs_pio_get_volume(void)
{
	int c;

	c = t_vol;

	return c;
}

inline int avs_pio_get_mute(void)
{
	return t_mute;
}

int avs_pio_set_mode(int val)
{
	switch(val)
	{
	case SAA_MODE_RGB:
		stpio_set_pin(avs_mode, 1);
		break;
	case SAA_MODE_FBAS:
		stpio_set_pin(avs_mode, 0);
		break;
	}

	return 0;
}

int avs_pio_set_encoder(int vol)
{
	return 0;
}

int avs_pio_set_wss(int val)
{
	dprintk("[AVS]: %s\n", __func__);

	if (val == SAA_WSS_43F)
	{
#if defined(HS7420) || defined(HS7429) || defined(HS7810A) || defined(HS7819)
		stpio_set_pin(avs_standby, 0);
#endif
		stpio_set_pin(avs_format, 0);
	}
	else if (val == SAA_WSS_169F)
	{
#if defined(HS7420) || defined(HS7429) || defined(HS7810A) || defined(HS7819)
		stpio_set_pin(avs_standby, 0);
#endif
		stpio_set_pin(avs_format, 1);
	}
	else if (val == SAA_WSS_OFF)
	{
#if defined(HS7420) || defined(HS7429) || defined(HS7810A) || defined(HS7819)
		stpio_set_pin(avs_format, 1);
#endif
	}
	else
	{
		return  -EINVAL;
	}

	return 0;
}

int avs_pio_command(unsigned int cmd, void *arg )
{
	int val=0;

	dprintk("[AVS]: %s(%d)\n", __func__, cmd);

	if (cmd & AVSIOSET)
	{
		if ( copy_from_user(&val,arg,sizeof(val)) )
		{
			return -EFAULT;
		}

		switch (cmd)
		{
			case AVSIOSVOL:
				return avs_pio_set_volume(val);
			case AVSIOSMUTE:
				return avs_pio_set_mute(val);
			case AVSIOSTANDBY:
				return avs_pio_standby(val);
			default:
				return -EINVAL;
		}
	}
	else if (cmd & AVSIOGET)
	{
		switch (cmd)
		{
			case AVSIOGVOL:
				val = avs_pio_get_volume();
				break;
			case AVSIOGMUTE:
				val = avs_pio_get_mute();
				break;
			default:
				return -EINVAL;
		}

		return put_user(val,(int*)arg);
	}
	else
	{
		dprintk("[AVS]: %s: SAA command\n", __func__);

		/* an SAA command */
		if ( copy_from_user(&val,arg,sizeof(val)) )
		{
			return -EFAULT;
		}

		switch(cmd)
		{
		case SAAIOSMODE:
           		 return avs_pio_set_mode(val);
 	        case SAAIOSENC:
        		 return avs_pio_set_encoder(val);
		case SAAIOSWSS:
			return avs_pio_set_wss(val);
		case SAAIOSSRCSEL:
        		return avs_pio_src_sel(val);
		default:
			dprintk("[AVS]: %s: SAA command not supported\n", __func__);
			return -EINVAL;
		}
	}

	return 0;
}

int avs_pio_command_kernel(unsigned int cmd, void *arg)
{
    int val=0;

	if (cmd & AVSIOSET)
	{
		val = (int) arg;

      	dprintk("[AVS]: %s: AVSIOSET command\n", __func__);

		switch (cmd)
		{
			case AVSIOSVOL:
		            return avs_pio_set_volume(val);
		        case AVSIOSMUTE:
		            return avs_pio_set_mute(val);
		        case AVSIOSTANDBY:
		            return avs_pio_standby(val);
			default:
				return -EINVAL;
		}
	}
	else if (cmd & AVSIOGET)
	{
		dprintk("[AVS]: %s: AVSIOGET command\n", __func__);

		switch (cmd)
		{
			case AVSIOGVOL:
			    val = avs_pio_get_volume();
			    break;
			case AVSIOGMUTE:
			    val = avs_pio_get_mute();
			    break;
			default:
				return -EINVAL;
		}

		arg = (void *) val;
	        return 0;
	}
	else
	{
		dprintk("[AVS]: %s: SAA command (%d)\n", __func__, cmd);

		val = (int) arg;

		switch(cmd)
		{
		case SAAIOSMODE:
           		 return avs_pio_set_mode(val);
 	        case SAAIOSENC:
        		 return avs_pio_set_encoder(val);
		case SAAIOSWSS:
			return avs_pio_set_wss(val);
		case SAAIOSSRCSEL:
        		return avs_pio_src_sel(val);
		default:
			dprintk("[AVS]: %s: SAA command not supported\n", __func__);
			return -EINVAL;
		}
	}

	return 0;
}

int avs_pio_init(void)
{
#if defined(SPARK7162)
	avs_mode	= stpio_request_pin (11, 5, "avs_mode", 	STPIO_OUT);
	avs_format	= stpio_request_pin (11, 4, "avs_format", 	STPIO_OUT);
	avs_standby	= stpio_request_pin (11, 3, "avs_standby", 	STPIO_OUT);
	avs_mute	= stpio_request_pin (11, 2, "avs_mute", 	STPIO_OUT);
//	avs_src		= NULL;
#elif defined(SPARK)
	avs_mode	= stpio_request_pin (6, 0, "avs_mode", 		STPIO_OUT);
	avs_format	= stpio_request_pin (6, 2, "avs_format", 	STPIO_OUT);
	avs_standby	= stpio_request_pin (6, 1, "avs_standby", 	STPIO_OUT);
	avs_mute	= stpio_request_pin (2, 4, "avs_mute", 		STPIO_OUT);
//	avs_src		= NULL;
#elif defined(HS7420) || defined(HS7429) || defined(HS7810A) || defined(HS7819)
	avs_format	= stpio_request_pin (6, 5, "avs0", STPIO_OUT);
	avs_standby	= stpio_request_pin (6, 6, "avs1", STPIO_OUT);
	avs_mode	= stpio_request_pin (6, 4, "avs2", STPIO_OUT);
	avs_mute	= NULL;
//	avs_src		= NULL;
#else
	return 0;
#endif

	if ((avs_mode == NULL) || (avs_format == NULL) || (avs_standby == NULL))
	{
		if(avs_mode != NULL)
			stpio_free_pin(avs_mode);
		else
			dprintk("[AVS]: avs_mode error\n");
		if(avs_format != NULL)
			stpio_free_pin (avs_format);
		else
			dprintk("[AVS]: avs_format error\n");

		if(avs_standby != NULL)
			stpio_free_pin(avs_standby);
		else
			dprintk("[AVS]: avs_standby error\n");

		return -1;
	}

#if defined(SPARK) || defined(SPARK7162)
	if (avs_mute == NULL)
	{
		dprintk("[AVS]: avs_mute error\n");
		return -1;
	}
#endif

	dprintk("[AVS-PIO]: init success\n");

  return 0;
}

int avs_pio_exit(void)
{
	if(avs_mode != NULL)
		stpio_free_pin(avs_mode);

	if(avs_format != NULL)
		stpio_free_pin (avs_format);

	if(avs_standby != NULL)
		stpio_free_pin(avs_standby);

	if(avs_mute != NULL)
		stpio_free_pin(avs_mute);

  return 0;
}
