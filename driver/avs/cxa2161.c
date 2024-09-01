/*
 *   cxa2161.c - CXA2161 audio/video switch driver - Homecast 5101
 *
 *   written by corev - 29. Dec 2009
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
#include "cxa2161.h"
 
/* ---------------------------------------------------------------------- */
 
static int debug = AVS_DEBUG;

static struct s_cxa2161_data cxa2161_data;
static struct s_cxa2161_data tmp_cxa2161_data;
static int s_old_src;
 
/* ---------------------------------------------------------------------- */
 
int cxa2161_set( struct i2c_client *client )
{    
	char buffer[CXA2161_DATA_SIZE+1];
 
	buffer[0] = 0;
	memcpy( buffer+1, &cxa2161_data, CXA2161_DATA_SIZE );
 
	if ( CXA2161_DATA_SIZE != i2c_master_send(client, buffer, CXA2161_DATA_SIZE))
	{
		return -EFAULT;
	}
 
	return 0;
}

/* ---------------------------------------------------------------------- */

inline int cxa2161_standby( struct i2c_client *client, int type )
{
	if ((type<0) || (type>1))
	{
		return -EINVAL;
	}
 
	if (type==1) 
	{
		if (cxa2161_data.enable_vout1 == CXA2161_OUTPUT_DISABLED)
		{
			tmp_cxa2161_data = cxa2161_data;
			cxa2161_data.tv_mono_switch  = CXA2161_MONOSWITCH_NORMAL;
			cxa2161_data.vcr_mono_switch = CXA2161_MONOSWITCH_NORMAL;
			cxa2161_data.enable_vout1    = CXA2161_OUTPUT_ENABLED;
			cxa2161_data.enable_vout2    = CXA2161_OUTPUT_ENABLED;
			cxa2161_data.enable_vout3    = CXA2161_OUTPUT_ENABLED;
			cxa2161_data.enable_vout4    = CXA2161_OUTPUT_ENABLED;
			cxa2161_data.enable_vout5    = CXA2161_OUTPUT_ENABLED;
			cxa2161_data.enable_vout6    = CXA2161_OUTPUT_ENABLED;
		}
		else
			return -EINVAL;		
	}
	else
	{
		if (cxa2161_data.enable_vout1 == CXA2161_OUTPUT_ENABLED)
		{
			tmp_cxa2161_data = cxa2161_data;
			cxa2161_data.tv_mono_switch  = CXA2161_MONOSWITCH_DEFAULT;
			cxa2161_data.vcr_mono_switch = CXA2161_MONOSWITCH_DEFAULT;
			cxa2161_data.enable_vout1    = CXA2161_OUTPUT_DISABLED;
			cxa2161_data.enable_vout2    = CXA2161_OUTPUT_DISABLED;
			cxa2161_data.enable_vout3    = CXA2161_OUTPUT_DISABLED;
			cxa2161_data.enable_vout4    = CXA2161_OUTPUT_DISABLED;
			cxa2161_data.enable_vout5    = CXA2161_OUTPUT_DISABLED;
			cxa2161_data.enable_vout6    = CXA2161_OUTPUT_DISABLED;
		}
		else
			return -EINVAL;
	}
 
	return cxa2161_set(client);
}
 
/* ---------------------------------------------------------------------- */
 
int cxa2161_set_volume( struct i2c_client *client, int vol )
{
	int c=0;
 
	c = vol;
 
	if (c > 63 || c < 0)
		return -EINVAL;
 
	cxa2161_data.volume_control = (c >> 1);
 
	return cxa2161_set(client);
}
 
/* ---------------------------------------------------------------------- */

int cxa2161_get_volume(void)
{
	int c;
 
	c = cxa2161_data.volume_control;
	c <<= 1; 

	return c;
}
 
/* ---------------------------------------------------------------------- */
 
inline int cxa2161_set_mute( struct i2c_client *client, int type )
{
 	if ((type<0) || (type>1))
	{
		return -EINVAL;
	}
 
	if (type==AVS_MUTE) 
	{
		cxa2161_data.tv_audio_mute  = CXA2161_AUDIO_MUTED;
		cxa2161_data.tv_audio_mute2 = CXA2161_AUDIO_MUTED;
	}
	else
	{
		cxa2161_data.tv_audio_mute  = CXA2161_AUDIO_ACTIVE;
		cxa2161_data.tv_audio_mute2 = CXA2161_AUDIO_ACTIVE;
	}
 
	return cxa2161_set(client);
}
 
/* ---------------------------------------------------------------------- */
 
inline int cxa2161_get_mute(void)
{
	return (cxa2161_data.tv_audio_mute == CXA2161_AUDIO_MUTED) ? AVS_MUTE : AVS_UNMUTE;
}
 
/* ---------------------------------------------------------------------- */
 
int cxa2161_set_mode( struct i2c_client *client, int vol )
{
	dprintk("[AVS]: SAAIOSMODE command : %d\n", vol);
	if (vol == SAA_MODE_RGB)
	{
		if(cxa2161_data.tv_video_switch == CXA2161_TV_VIDEOSWITCH_VCR_RGB_SVIDEO) // scart selected
		  s_old_src = CXA2161_TV_VIDEOSWITCH_DIGITALENCODER_RGB_CVBS;
		else
		  cxa2161_data.tv_video_switch = CXA2161_TV_VIDEOSWITCH_DIGITALENCODER_RGB_CVBS;
		cxa2161_data.fast_blank = CXA2161_FASTBLANK_ON;
	}
	else if (vol == SAA_MODE_FBAS)
	{
		if(cxa2161_data.tv_video_switch == CXA2161_TV_VIDEOSWITCH_VCR_RGB_SVIDEO) // scart selected
		  s_old_src = CXA2161_TV_VIDEOSWITCH_DIGITALENCODER_RGB_CVBS;
		else
		  cxa2161_data.tv_video_switch = CXA2161_TV_VIDEOSWITCH_DIGITALENCODER_RGB_CVBS;
		cxa2161_data.fast_blank = CXA2161_FASTBLANK_OFF;
	}
	else if (vol == SAA_MODE_SVIDEO)
	{
		if(cxa2161_data.tv_video_switch == CXA2161_TV_VIDEOSWITCH_VCR_RGB_SVIDEO) // scart selected
		  s_old_src = CXA2161_TV_VIDEOSWITCH_DIGITALENCODER_SVIDEO;
		else
		  cxa2161_data.tv_video_switch = CXA2161_TV_VIDEOSWITCH_DIGITALENCODER_SVIDEO;
		cxa2161_data.fast_blank = CXA2161_FASTBLANK_OFF;
	}
	else
	{
		return  -EINVAL;
	}
 
	return cxa2161_set(client);
}
 
/* ---------------------------------------------------------------------- */
//NOT IMPLEMENTED
int cxa2161_set_encoder( struct i2c_client *client, int vol )
{
	return 0;
}
 
/* ---------------------------------------------------------------------- */
 
int cxa2161_set_wss( struct i2c_client *client, int vol )
{
	if (vol == SAA_WSS_43F)
	{
		cxa2161_data.fnc_level = CXA2161_FNCLEVEL_WSS_4_3;
	}
	else if (vol == SAA_WSS_169F)
	{
		cxa2161_data.fnc_level = CXA2161_FNCLEVEL_WSS_16_9;
	}
	else if (vol == SAA_WSS_OFF)
	{
		cxa2161_data.fnc_level = CXA2161_FNCLEVEL_INTERNAL;
	}
	else
	{
		return  -EINVAL;
	}
 
	return cxa2161_set(client);
}

/* ---------------------------------------------------------------------- */
 
int cxa2161_src_sel( struct i2c_client *client, int src )
{
	if (src == SAA_SRC_ENC)
	{
		cxa2161_data.tv_video_switch  = s_old_src;
		cxa2161_data.vcr_video_switch = CXA2161_VCR_VIDEOSWITCH_DIGITALENCODER_SVIDEO_CVBS;
	}
	else if(src == SAA_SRC_SCART)
	{
		s_old_src = cxa2161_data.tv_video_switch;
		cxa2161_data.tv_video_switch  = CXA2161_TV_VIDEOSWITCH_VCR_RGB_SVIDEO;
		cxa2161_data.vcr_video_switch = CXA2161_VCR_VIDEOSWITCH_MUTE;
	}
	else
	{
		return  -EINVAL;
	}
 
	return cxa2161_set(client);
}
 
/* ---------------------------------------------------------------------- */
 
int cxa2161_command(struct i2c_client *client, unsigned int cmd, void *arg)
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
				return cxa2161_set_volume(client,val);
			case AVSIOSMUTE:
				return cxa2161_set_mute(client,val);
			case AVSIOSTANDBY:
				return cxa2161_standby(client,val);
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
				val = cxa2161_get_volume();
			case AVSIOGMUTE:
				val = cxa2161_get_mute();
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
				return cxa2161_set_mode(client,val);
			case SAAIOSENC:
				return cxa2161_set_encoder(client,val);
			case SAAIOSWSS:
				return cxa2161_set_wss(client,val);
			case SAAIOSSRCSEL:
				return cxa2161_src_sel(client,val);
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
 
int cxa2161_command_kernel(struct i2c_client *client, unsigned int cmd, void *arg)
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
            return cxa2161_set_volume(client,val);
         case AVSIOSMUTE:
            return cxa2161_set_mute(client,val);
         case AVSIOSTANDBY:
            return cxa2161_standby(client,val);
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
            val = cxa2161_get_volume();
            break;
         case AVSIOGMUTE:
            val = cxa2161_get_mute();
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
            return cxa2161_set_mode(client,val);
         case SAAIOSENC:
            return cxa2161_set_encoder(client,val);
         case SAAIOSWSS:
            return cxa2161_set_wss(client,val);
        case SAAIOSSRCSEL:
            return cxa2161_src_sel(client,val);
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

int cxa2161_init(struct i2c_client *client)
{
	memset((void*)&cxa2161_data,0,CXA2161_DATA_SIZE);
 
	/* default values for tv fbas - vcr fbas*/
	s_old_src = CXA2161_TV_VIDEOSWITCH_DIGITALENCODER_RGB_CVBS;

 	cxa2161_data.tv_video_switch    = CXA2161_TV_VIDEOSWITCH_DIGITALENCODER_RGB_CVBS;
 	cxa2161_data.vcr_video_switch   = CXA2161_VCR_VIDEOSWITCH_DIGITALENCODER_SVIDEO_CVBS;
	cxa2161_data.fast_blank         = CXA2161_FASTBLANK_OFF;

	cxa2161_data.rgb_gain           = CXA2161_RGBGAIN_0;
	cxa2161_data.mixer_control      = CXA2161_MIXERCONTROL_DEFAULT;
	cxa2161_data.vin5_clamp         = CXA2161_CLAMP_DEFAULT;
	cxa2161_data.vin7_clamp         = CXA2161_CLAMP_DEFAULT;
	cxa2161_data.vin3_clamp         = CXA2161_CLAMP_DEFAULT;
	cxa2161_data.sync_select        = CXA2161_SYNCSELECT_VIN8;
        cxa2161_data.enable_vout1       = CXA2161_OUTPUT_ENABLED;
        cxa2161_data.enable_vout2       = CXA2161_OUTPUT_ENABLED;
        cxa2161_data.enable_vout3       = CXA2161_OUTPUT_ENABLED;
        cxa2161_data.enable_vout4       = CXA2161_OUTPUT_ENABLED;
        cxa2161_data.enable_vout5       = CXA2161_OUTPUT_ENABLED;
        cxa2161_data.enable_vout6       = CXA2161_OUTPUT_ENABLED;
	cxa2161_data.bidir_line_control = CXA2161_BIDIRLINE_OFF;
	cxa2161_data.tv_audio_select    = CXA2161_AUDIOSELECT_LRIN4;
	cxa2161_data.vcr_audio_select   = CXA2161_AUDIOSELECT_LRIN4;
	cxa2161_data.tv_mono_switch     = CXA2161_MONOSWITCH_NORMAL;
	cxa2161_data.vcr_mono_switch    = CXA2161_MONOSWITCH_NORMAL;
	cxa2161_data.phono_bypass       = CXA2161_VOLUMEBYPASS_OFF;
	cxa2161_data.tv_volume_bypass   = CXA2161_VOLUMEBYPASS_OFF;
	cxa2161_data.mono_output        = CXA2161_MONOOUTPUT_TV;
	cxa2161_data.volume_control     = CXA2161_VOLUMECONTROL_DEFAULT;
	cxa2161_data.audio_gain         = CXA2161_AUDIOGAIN_DEFAULT;
	cxa2161_data.overlay            = CXA2161_OVERLAY_OFF;
	cxa2161_data.zcd                = CXA2161_ZEROCROSS_ON;
	cxa2161_data.tv_input_mute      = CXA2161_INPUT_ACTIVE;
	cxa2161_data.vcr_input_mute     = CXA2161_INPUT_ACTIVE;
	cxa2161_data.output_limit       = CXA2161_OUTPUTLIMIT_OFF;
        cxa2161_data.fnc_follow         = CXA2161_FNCFOLLOW_ON;
        cxa2161_data.fnc_dir            = CXA2161_FNCDIR_OUT;
	cxa2161_data.fnc_level          = CXA2161_FNCLEVEL_INTERNAL;
	cxa2161_data.logic_level        = CXA2161_LOGICLEVEL_CURRENTSINK;

	return cxa2161_set(client);
}
 
/* ---------------------------------------------------------------------- */
