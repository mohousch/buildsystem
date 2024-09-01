/************************************************************************
Copyright (C) 2007 STMicroelectronics. All Rights Reserved.

This file is part of the Player2 Library.

Player2 is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License version 2 as published by the
Free Software Foundation.

Player2 is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with player2; see the file COPYING. If not, write to the Free Software
Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

The Player2 Library may alternatively be licensed under a proprietary
license from ST.

Source file name : dvb_ca.c
Author : Pete

Implementation of linux dvb dvr input device

Date Modification Name
---- ------------ --------
01-Nov-06 Created Pete

************************************************************************/

#include <linux/module.h>
#include <linux/dvb/audio.h>
#include <linux/dvb/ca.h>

#include "dvb_module.h"
#include "backend.h"
#include "dvb_ca.h"

#ifdef __TDT__
#include "../../../../../../../pti/pti_hal.h"
#include "st-common.h"
#endif

static int CaOpen(struct inode *Inode,
				  struct file *File);
static int CaRelease(struct inode *Inode,
					 struct file *File);
static int CaIoctl(struct inode *Inode,
				   struct file *File,
				   unsigned int IoctlCode,
				   void *ParamAddress);

// *INDENT-OFF*
static struct file_operations CaFops =
{
	owner:        THIS_MODULE,
	ioctl:        dvb_generic_ioctl,
	open:         CaOpen,
	release:      CaRelease,
};

static struct dvb_device CaDevice =
{
	priv:         NULL,
	users:        1,
	readers:      1,
	writers:      1,
	fops:         &CaFops,
	kernel_ioctl: CaIoctl,
};
// *INDENT-ON*

#ifdef __TDT__
static int caInitialized = 0;
#if !defined(VIP2_V1) \
 && !defined (SPARK) \
 && !defined (SPARK7162) \
 && !defined (ADB_BOX) \
 && !defined (CUBEREVO_2000HD) \
 && !defined(SAGEMCOM88) \
 && !defined(PACE7241)
extern int init_ci_controller(struct dvb_adapter *dvb_adap);
#endif
#endif

struct dvb_device *CaInit(struct DeviceContext_s *DeviceContext)
{
#ifdef __TDT__
	printk("CaInit()\n");
	if (!caInitialized)
	{
		/* the following call creates ca0 associated with the cimax hardware */
		printk("Initializing CI Controller\n");
#if !defined(VIP2_V1) \
 && !defined (SPARK) \
 && !defined (SPARK7162) \
 && !defined(ADB_BOX) \
 && !defined (CUBEREVO_2000HD) \
 && !defined(SAGEMCOM88) \
 && !defined(PACE7241)
		init_ci_controller(&DeviceContext->DvbContext->DvbAdapter);
#endif
		caInitialized = 1;
	}
#endif
	return &CaDevice;
}

static int CaOpen(struct inode *Inode,
				  struct file *File)
{
	struct dvb_device *DvbDevice = (struct dvb_device *)File->private_data;
	struct DeviceContext_s *Context = (struct DeviceContext_s *)DvbDevice->priv;
	int Error;
	dprintk("CA Open\n");
	Error = dvb_generic_open(Inode, File);
	if (Error < 0)
		return Error;
	Context->EncryptionOn = 0;
	return 0;
}

static int CaRelease(struct inode *Inode,
					 struct file *File)
{
	struct dvb_device *DvbDevice = (struct dvb_device *)File->private_data;
	struct DeviceContext_s *Context = (struct DeviceContext_s *)DvbDevice->priv;
	dprintk("CA Release\n");
	Context->EncryptionOn = 0;
	return dvb_generic_release(Inode, File);
}

static int CaIoctl(struct inode *Inode,
				   struct file *File,
				   unsigned int IoctlCode,
				   void *Parameter)
{
	struct dvb_device *DvbDevice = (struct dvb_device *)File->private_data;
	struct DeviceContext_s *Context = (struct DeviceContext_s *)DvbDevice->priv;
#ifdef __TDT__
	struct PtiSession *pSession = Context->pPtiSession;
	if (pSession == NULL)
	{
		printk("CA is not associated with a session\n");
		return -EINVAL;
	}
	dprintk("TEST %s : Ioctl %08x\n", __func__, IoctlCode);
	switch (IoctlCode)
	{
		case CA_SET_PID: // currently this is useless but prevents from softcams errors
		{
			ca_pid_t *service_pid = (ca_pid_t *) Parameter;
			int vLoop;
			unsigned short pid = service_pid->pid;
			int descramble_index = service_pid->index;
			printk("CA_SET_PID index = %d pid %d\n", descramble_index, pid);
			if (descramble_index >= 0)
			{
				if (descramble_index >= NUMBER_OF_DESCRAMBLERS)
				{
					printk("Error only descramblers 0 - %d supportet\n", NUMBER_OF_DESCRAMBLERS - 1);
					return -1;
				}
				pSession->descramblerForPid[pid] = descramble_index;
				for (vLoop = 0; vLoop < pSession->num_pids; vLoop++)
				{
					if (((unsigned short) pSession->pidtable[vLoop] == (unsigned short) pid))
					{
						if (pSession->type[vLoop] == DMX_TYPE_TS)
						{
							/* link audio/video slot to the descrambler */
							if ((pSession->pes_type[vLoop] == DMX_TS_PES_VIDEO) ||
									(pSession->pes_type[vLoop] == DMX_TS_PES_AUDIO) ||
									(pid > 50)) /*dirty hack because for some reason the pes_type is changed to DMX_TS_PES_OTHER*/
							{
								int err;
								if (pSession->descramblerindex[vLoop] != descramble_index)
								{
									pSession->descramblerindex[vLoop] = descramble_index;
									if ((err = pti_hal_descrambler_link(pSession->session,
																		pSession->descramblers[pSession->descramblerindex[vLoop]],
																		pSession->slots[vLoop])) != 0)
										printk("Error linking slot %d to descrambler %d, err = %d\n",
											   pSession->slots[vLoop],
											   pSession->descramblers[pSession->descramblerindex[vLoop]],
											   err);
									else
										dprintk("linking pid %d slot %d to descrambler %d, session = %d pSession %p\n",
												pid, pSession->slots[vLoop],
												pSession->descramblers[pSession->descramblerindex[vLoop]],
												pSession->session, pSession);
									return 0;
								}
								else
								{
									printk("pid %x is already linked to descrambler %d\n", pid, descramble_index);
									return 0;
								}
							}
							else
							{
								printk("pid %x no audio or video pid! type=%d slot=%d not linking to descrambler\n", pid, pSession->pes_type[vLoop], pSession->slots[vLoop]);
								return -1;
							}
						}
						else
						{
							printk("pid %x type is not DMX_TYPE_TS! not linking to descrambler\n", pid);
							return -1;
						}
					}
				}
				printk("pid %x not found in pidtable, it might be inactive\n", pid);
			}
			return 0;
			break;
		}
		case CA_SET_DESCR:
		{
			ca_descr_t *descr = (ca_descr_t *) Parameter;
			dprintk("CA_SET_DESCR\n");
			if (descr->index >= 16)
				return -EINVAL;
			if (descr->parity > 1)
				return -EINVAL;
#if 0
			dprintk("index = %d\n", descr->index);
			dprintk("parity = %d\n", descr->parity);
			dprintk("cw[0] = %d\n", descr->cw[0]);
			dprintk("cw[1] = %d\n", descr->cw[1]);
			dprintk("cw[2] = %d\n", descr->cw[2]);
			dprintk("cw[3] = %d\n", descr->cw[3]);
			dprintk("cw[4] = %d\n", descr->cw[4]);
			dprintk("cw[5] = %d\n", descr->cw[5]);
			dprintk("cw[6] = %d\n", descr->cw[6]);
			dprintk("cw[7] = %d\n", descr->cw[7]);
#endif
			dprintk("Descrambler Index: %d, cw(%d) = %02x %02x %02x %02x %02x %02x %02x %02x\n", descr->index, descr->parity,
					descr->cw[0], descr->cw[1], descr->cw[2], descr->cw[3], descr->cw[4], descr->cw[5], descr->cw[6], descr->cw[7]);
			if (descr->index < 0 || descr->index >= NUMBER_OF_DESCRAMBLERS)
			{
				printk("Error descrambler %d not supported! needs to be in range 0 - %d\n", descr->index, NUMBER_OF_DESCRAMBLERS - 1);
				return -1;
			}
			if (&Context->DvbContext->Lock != NULL)
				mutex_lock(&Context->DvbContext->Lock);
			if (pti_hal_descrambler_set(pSession->session, pSession->descramblers[descr->index], descr->cw, descr->parity) != 0)
				printk("Error while setting descrambler keys\n");
			if (&Context->DvbContext->Lock != NULL)
				mutex_unlock(&Context->DvbContext->Lock);
			return 0;
			break;
		}
		case CA_SET_DESCR_DATA:
		{
			int altDescr = 40000, sess = 10000;
#if 0
			int i;
#endif
			bool useAlt = false;
			ca_descr_data_t *descr = (ca_descr_data_t *) Parameter;
			dprintk("CA_SET_DESCR_DATA\n");
			if (descr->index & 0x100)
			{
				descr->index &= 0xFF;
				sess += descr->index;
				altDescr += 1000 * descr->index;
				useAlt = true;
			}
			if (descr->index >= 16)
				return -EINVAL;
			if (descr->parity > 1)
				return -EINVAL;
#if 0
			if (debug)
			{
				printk("Descrambler Index: %d Parity: %d Type: %d\n", descr->index, descr->parity, descr->data_type);
				for (i = 0; i < descr->length; i++)
					printk("%02x ", descr->data[i]);
				printk("\n");
			}
#endif
			if (descr->index < 0 || descr->index >= NUMBER_OF_DESCRAMBLERS)
			{
				printk("Error descrambler %d not supported! needs to be in range 0 - %d\n", descr->index, NUMBER_OF_DESCRAMBLERS - 1);
				return -1;
			}
			if (&Context->DvbContext->Lock != NULL)
				mutex_lock(&Context->DvbContext->Lock);
			if (useAlt)
			{
				if (pti_hal_descrambler_set_aes(sess, altDescr, descr->data, descr->parity, descr->data_type) != 0)
					printk("Error while setting descrambler keys\n");
			}
			else
			{
				if (pti_hal_descrambler_set_aes(pSession->session, pSession->descramblers[descr->index], descr->data, descr->parity, descr->data_type) != 0)
					printk("Error while setting descrambler keys\n");
			}
			if (&Context->DvbContext->Lock != NULL)
				mutex_unlock(&Context->DvbContext->Lock);
			return 0;
			break;
		}
		default:
			printk("%s: Error - invalid ioctl %08x\n", __FUNCTION__, IoctlCode);
	}
	return -ENOIOCTLCMD;
#else
	//print("CaIoctl : Ioctl %08x\n", IoctlCode);
	switch (IoctlCode)
	{
		case CA_SEND_MSG:
		{
			struct ca_msg *msg;
			msg = (struct ca_msg *)Parameter;
//			if (msg->type==1)
//				tkdma_set_iv(msg->msg);
//			else if (msg->type==2)
//				tkdma_set_key(msg->msg,0);
			if (msg->type == 3)
				Context->EncryptionOn = 1;
			else if (msg->type == 4)
				Context->EncryptionOn = 0;
			else if (msg->type == 7)
				memcpy(&Context->StartOffset, msg->msg, sizeof(Context->StartOffset));
			else if (msg->type == 8)
				memcpy(&Context->EndOffset, msg->msg, sizeof(Context->EndOffset));
			return -ENOIOCTLCMD;
			return 0;
		}
		default:
			printk("%s: Error - invalid ioctl %08x\n", __FUNCTION__, IoctlCode);
	}
	return -ENOIOCTLCMD;
#endif
}
