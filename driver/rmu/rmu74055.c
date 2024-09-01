/*
 *   rmu74055.c - rf modulator driver
 *
 *
 *   Copyright (C) 2009 Sysifos gillem@berlios.de
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

#include "rmu_core.h"
#include "rmu74055.h"


#define IOCTL_SET_CHANNEL           0
#define IOCTL_SET_TESTMODE          1
#define IOCTL_SET_SOUNDENABLE       2    
#define IOCTL_SET_SOUNDSUBCARRIER   3
#define IOCTL_SET_FINETUNE          4
#define IOCTL_SET_STANDBY           5

/* ---------------------------------------------------------------------- */

/*
 * rmu74055 data struct
 *
 */
/* for LITTLE ENDIAN 
   d0-d3          */
typedef struct s_rmu74055_data {

 /* FM */
 unsigned char nhigh			: 6; // 
 unsigned char tpen			: 1; // 
 unsigned char res10			: 1; //0 

 /*FL */
 unsigned char x0			: 1; //0
 unsigned char x1			: 1; //0
 unsigned char nlow			: 6; //1

 /* C1 */
 unsigned char res5			: 1; //0
 unsigned char x2			: 1; //
 unsigned char res6			: 1; //0
 unsigned char ps			: 1; //
 unsigned char res7			: 1; //0
 unsigned char so			: 1; //
 unsigned char res8			: 1; //0
 unsigned char res9			: 1; //1

 /* C0 */
 unsigned char res1			: 1; //0
 unsigned char res2			: 1; //0
 unsigned char res3			: 1; //0
 unsigned char sfd0			: 1; //
 unsigned char sfd1			: 1; //
 unsigned char att			: 1; //
 unsigned char osc			: 1; //
 unsigned char res4			: 1; //0

 
} s_rmu74055_data;


#define RMU74055_DATA_SIZE sizeof(s_rmu74055_data)

extern int debug ;

/* ---------------------------------------------------------------------- */

static struct s_rmu74055_data rmu74055_data;

/* ---------------------------------------------------------------------- */

int rmu74055_set(struct i2c_client *client)
{
	dprintk("[RMU]: rmu75055_set\n");

	char buffer[RMU74055_DATA_SIZE+2];

        buffer[0]=buffer[1]=0;
        memcpy( buffer+2, &rmu74055_data, RMU74055_DATA_SIZE );
#if 0
        dprintk("RMU74055_DATA_SIZE [%d] ", RMU74055_DATA_SIZE);
        int i;
        for(i = 2; i<RMU74055_DATA_SIZE+2; i++)
    		dprintk(" %02x",buffer[i]);
    	dprintk(" \n");
#endif

	if ( (RMU74055_DATA_SIZE) != i2c_master_send(client, buffer+2, RMU74055_DATA_SIZE))
	{
		return -EFAULT;
	}

	return 0;
}

int rmu74055_set_channel( struct i2c_client *client, int channel )
{
	dprintk("[RMU] SET CHANNEL: %d\n",channel);
	
	int c ;
        int freq;
        c = channel;

        freq = 1885+((c-21)*32);

        rmu74055_data.nlow = freq & 0x3F;
        rmu74055_data.nhigh = (freq >> 6) & 0x3F;

        dprintk ("freq %d\n", freq);        
        dprintk ("nlow %d\n", rmu74055_data.nlow);
        dprintk ("nhigh %d\n", rmu74055_data.nhigh);

	return rmu74055_set(client);
}

int rmu74055_set_finetune( struct i2c_client *client, int finetune )
{
	dprintk("[RMU] SET FINETUNE: %d\n",finetune);
	int c;
        int low = 0x3D;  
        int high = rmu74055_data.nhigh;          
	c = finetune;
        if (c > 2){
        	c = 2;
    		return 0;
        } 
        if (c < -4){
        	c = -4;
        	return 0;
        } 

        rmu74055_data.nlow = (low + c) & 0x3f;

	dprintk ("nlow %d\n", rmu74055_data.nlow);
        dprintk ("nhigh %d\n", rmu74055_data.nhigh);        

	return rmu74055_set(client);
}

int rmu74055_set_subcarrier( struct i2c_client *client, int sub )
{
	dprintk("[RMU] SET SUBCARRIER: %d\n",sub);
	int c=0;

	c = sub;
	if (c > 4 || c < 0)
		return -EINVAL;
        switch (c){
    	   case 0:
		rmu74055_data.sfd0 = 0;
		rmu74055_data.sfd1 = 0;	
	   break;	
    	   case 1:
		rmu74055_data.sfd0 = 1;
		rmu74055_data.sfd1 = 0;	
	   break;	
    	   case 2:
		rmu74055_data.sfd0 = 0;
		rmu74055_data.sfd1 = 1;	
	   break;	
    	   case 3:
		rmu74055_data.sfd0 = 1;
		rmu74055_data.sfd1 = 1;	
	   break;	
	   default:
		rmu74055_data.sfd0 = 1;
		rmu74055_data.sfd1 = 1;
	  break;
        } 

	return rmu74055_set(client);
}

inline int rmu74055_set_sw( struct i2c_client *client, int sw, int type )
{
	dprintk("[RMU] SET VSW\n");
        int rtype = 0;
	switch(sw)
	{
		case 1:	// 
                        dprintk("switch testmode\n"); 
			rmu74055_data.tpen = type;
			break;
		case 2: // tv
                        dprintk("switch sound enable\n"); 		
			rmu74055_data.so = type;
			break;
		case 5:
		        dprintk("switch standby\n"); 		                
                        if (type == 0) 
                    	    rtype = 1;
                    	else 
                    	    rtype = 0;
                    	rmu74055_data.osc = rtype;		        
            		break; 
                                
		default:
			return -EINVAL;
	}

	return rmu74055_set(client);
}

/* ---------------------------------------------------------------------- */

int rmu74055_command(struct i2c_client *client, unsigned int cmd, void *arg )
{
        int val = 0;

	copy_from_user(&val,arg,sizeof(val));
	dprintk("[RMU]: command %d  %d\n", cmd, val);	
	
        switch(cmd){
    	        case IOCTL_SET_CHANNEL:
    	    	    return rmu74055_set_channel(client,val);
	        break;
    	        case IOCTL_SET_TESTMODE:
    	    	    return rmu74055_set_sw(client,1,val);    	        
	        break;
    	        case IOCTL_SET_SOUNDENABLE:
    	    	    return rmu74055_set_sw(client,2,val);
	        break;
    	        case IOCTL_SET_SOUNDSUBCARRIER:
    	    	    return rmu74055_set_subcarrier(client,val);
	        break;
    	        case IOCTL_SET_FINETUNE:
    	    	    return rmu74055_set_finetune(client,val);
	        break;
    	        case IOCTL_SET_STANDBY:
    	    	    return rmu74055_set_sw(client,5,val);
	        break;
                default: 
                 	return 0;
	        break;
        }   

}


/* ---------------------------------------------------------------------- */

int rmu74055_init(struct i2c_client *client)
{
	dprintk("[RMU]: rmu75055_init\n");
	memset((void*)&rmu74055_data,0,RMU74055_DATA_SIZE);
        /* C0 */
	rmu74055_data.res1 = 0;
	rmu74055_data.res2 = 0;	
	rmu74055_data.res3 = 0;
	rmu74055_data.sfd0 = 1;
	rmu74055_data.sfd1 = 1;
	rmu74055_data.att =  0;
	rmu74055_data.osc =  1;
	rmu74055_data.res4 = 0;

	 /* C1 */
	rmu74055_data.res5 = 0;
	rmu74055_data.x2   = 0;
	rmu74055_data.res6 = 0;
	rmu74055_data.ps   = 1;
	rmu74055_data.res7 = 0;
	rmu74055_data.so   = 0;
	rmu74055_data.res8 = 0;
	rmu74055_data.res9 = 1;
	 /*FL */
	rmu74055_data.x0 = 0;
	rmu74055_data.x1 = 0;
	rmu74055_data.nlow = 0x3d;

	 /* FM */
	rmu74055_data.nhigh = 0x24; 
	rmu74055_data.tpen = 0; 
	rmu74055_data.res10 = 0; 

	return rmu74055_set(client);
}

/* ---------------------------------------------------------------------- */
