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
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,17)
#include <linux/stm/pio.h>
#else
#include <linux/stpio.h>
#endif

#include <linux/i2c.h>

#include "avs_core.h"
#include "fake_avs.h"

static int debug = AVS_DEBUG;

/* hold old values for mute/unmute */
unsigned char t_mute;

/* hold old values for volume */
unsigned char t_vol;

/* hold old values for standby */
unsigned char ft_stnby=0;

#if !defined(ADB_BOX)
struct stpio_pin*	wss_pin;
struct stpio_pin*	vid_pin;
#endif
/* ---------------------------------------------------------------------- */
 
int fake_avs_src_sel( struct i2c_client *client, int src )
{ 
	// fake avs do not need it
	return 0;
}

/* ---------------------------------------------------------------------- */

inline int fake_avs_standby( struct i2c_client *client, int type )
{

	if ((type<0) || (type>1))
	{
		return -EINVAL;
	}
 
	if (type==1) 
	{
		if (ft_stnby == 0)
		{
#if !defined(ADB_BOX)
			stpio_set_pin (vid_pin, ft_stnby);
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
#if !defined(ADB_BOX)
			stpio_set_pin (vid_pin, ft_stnby);
#endif
			ft_stnby = 0;
		}
		else
			return -EINVAL;
	}

	return 0;
}
 
/* ---------------------------------------------------------------------- */
 
int fake_avs_set_volume( struct i2c_client *client, int vol )
{
	int c=0;
 
	c = vol;
 
	if (c > 63 || c < 0)
		return -EINVAL;
 
	c = 63 - c;
 
	c=c/2;
 
	t_vol = c;
 
	return 0;
}
 
/* ---------------------------------------------------------------------- */
 
inline int fake_avs_set_mute( struct i2c_client *client, int type )
{

	if ((type<0) || (type>1))
	{
		return -EINVAL;
	}
 
	if (type==AVS_MUTE) 
	{
		t_mute = 1;
	}
	else
	{
		t_mute = 0;
	}
 
	return 0;
}
 
/* ---------------------------------------------------------------------- */
 
int fake_avs_get_volume(void)
{
	int c;
 
	c = t_vol;
 
	return c;
}
 
/* ---------------------------------------------------------------------- */
 
inline int fake_avs_get_mute(void)
{
	return t_mute;
}
 
/* ---------------------------------------------------------------------- */
 
int fake_avs_set_mode( struct i2c_client *client, int vol )
{
	dprintk("[AVS]: SAAIOSMODE command : %d\n", vol);
	
	// fake avs do not need it
 
	return 0;
}
 
/* ---------------------------------------------------------------------- */
//NOT IMPLEMENTED
int fake_avs_set_encoder( struct i2c_client *client, int vol )
{
	return 0;
}
 
/* ---------------------------------------------------------------------- */
 
int fake_avs_set_wss( struct i2c_client *client, int vol )
{
	if (vol == SAA_WSS_43F)
	{
#if !defined(ADB_BOX)
		stpio_set_pin (wss_pin, 0);
#endif
	}
	else if (vol == SAA_WSS_169F)
	{
#if !defined(ADB_BOX)
		stpio_set_pin (wss_pin, 0);
#endif
	}
	else if (vol == SAA_WSS_OFF)
	{
#if !defined(ADB_BOX)
		stpio_set_pin (wss_pin, 1);
#endif
	}
	else
	{
		return  -EINVAL;
	}
 
	return 0;
}

/* ---------------------------------------------------------------------- */

int fake_avs_command(struct i2c_client *client, unsigned int cmd, void *arg )
{
	int val=0;

	printk("[AVS]: command\n");
	
	if (cmd & AVSIOSET)
	{
		if ( copy_from_user(&val,arg,sizeof(val)) )
		{
			return -EFAULT;
		}

		switch (cmd)
		{
			case AVSIOSVOL:
				return fake_avs_set_volume(client,val);
			case AVSIOSMUTE:
				return fake_avs_set_mute(client,val);
			case AVSIOSTANDBY:
				return fake_avs_standby(client,val);
			default:
				return -EINVAL;
		}
	} else if (cmd & AVSIOGET)
	{
		switch (cmd)
		{
			case AVSIOGVOL:
				val = fake_avs_get_volume();
				break;
			case AVSIOGMUTE:
				val = fake_avs_get_mute();
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
           		 return fake_avs_set_mode(client,val);
 	        case SAAIOSENC:
        		 return fake_avs_set_encoder(client,val);
		case SAAIOSWSS:
			return fake_avs_set_wss(client,val);
		case SAAIOSSRCSEL:
        		return fake_avs_src_sel(client,val);
		default:
			dprintk("[AVS]: SAA command not supported\n");
			return -EINVAL;
		}
	}

	return 0;
}

/* ---------------------------------------------------------------------- */
 
int fake_avs_command_kernel(struct i2c_client *client, unsigned int cmd, void *arg)
{
   int val=0;

	dprintk("[AVS]: command_kernel(%u)\n", cmd);
	
	if (cmd & AVSIOSET)
	{
		val = (int) arg;

      		dprintk("[AVS]: AVSIOSET command\n");

		switch (cmd)
		{
			case AVSIOSVOL:
		            return fake_avs_set_volume(client,val);
		        case AVSIOSMUTE:
		            return fake_avs_set_mute(client,val);
		        case AVSIOSTANDBY:
		            return fake_avs_standby(client,val);
			default:
				return -EINVAL;
		}
	} else if (cmd & AVSIOGET)
	{
		dprintk("[AVS]: AVSIOGET command\n");

		switch (cmd)
		{
			case AVSIOGVOL:
			    val = fake_avs_get_volume();
			    break;
			case AVSIOGMUTE:
			    val = fake_avs_get_mute();
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
           		 return fake_avs_set_mode(client,val);
 	        case SAAIOSENC:
        		 return fake_avs_set_encoder(client,val);
		case SAAIOSWSS:
			return fake_avs_set_wss(client,val);
		case SAAIOSSRCSEL:
        		return fake_avs_src_sel(client,val);
		default:
			dprintk("[AVS]: SAA command not supported\n");
			return -EINVAL;
		}
	}

	return 0;
}

/* ---------------------------------------------------------------------- */

int fake_avs_init(struct i2c_client *client)
{
#if !defined(ADB_BOX)
  wss_pin = stpio_request_pin (3, 4, "WSS enable", STPIO_OUT);
  vid_pin = stpio_request_pin (3, 3, "Video enab", STPIO_OUT);

  if ((wss_pin == NULL) || (vid_pin == NULL))
  {
	if(wss_pin != NULL)
      		stpio_free_pin (wss_pin);
    	if(vid_pin != NULL)
      		stpio_free_pin (vid_pin);
	return 1;
  }

  stpio_set_pin (wss_pin, 1);
  stpio_set_pin (vid_pin, 1);
#endif

  return 0;
}

/* ---------------------------------------------------------------------- */
