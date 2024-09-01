/*
 * e2_proc_bus
 */

#include <linux/proc_fs.h> /* proc fs */
#include <asm/uaccess.h> /* copy_from_user */

#include <linux/dvb/video.h> /* Video Format etc */
#include <linux/dvb/audio.h>
#include <linux/dvb/version.h>
#include <linux/smp_lock.h>
#include <linux/string.h>

#include "../backend.h"
#include "../dvb_module.h"
#include "linux/dvb/stm_ioctls.h"
#include "dvbdev.h"
#include "dvb_frontend.h"

extern struct DeviceContext_s *ProcDeviceContext;

#define MAX_NIM_LENGTH 100
#define nums2minor(num,type,id) ((num << 6) | (id << 4) | type)

static struct dvb_device *dvbdev_find_device(int minor)
{
	struct dvb_adapter *adap = &ProcDeviceContext->DvbContext->DvbAdapter;
	struct list_head *entry;
	int number = minor >> 6;
	struct dvb_device *dev;
	if (adap->num != 0)
	{
		printk("%s(): configuration failure - the STM DVB adapter is not in slot 0\n", __func__);
		return NULL;
	}
	/* It is assumed that the STM adapter is registered first (adapter0).
	 That is, the actual list head is pointed to by adap->list_head.prev
	 because register_dvb_adapter are added to tail and not to head.
	 Don't start with &adap->list_head because the actual list_head
	 is not part of an adapter structure and will produce a crash. */
	list_for_each(entry, adap->list_head.prev)
	{
		struct dvb_adapter *adap1;
		adap1 = list_entry(entry, struct dvb_adapter, list_head);
#if 0
		printk("adap %d, list_head %p, prev %p, next %p\n",
			   adap1->num, &adap1->list_head, adap1->list_head.prev,
			   adap1->list_head.next);
#endif
		if (adap1->num == number)
		{
			struct list_head *entry1;
			list_for_each(entry1, &adap1->device_list)
			{
				dev = list_entry(entry1, struct dvb_device, list_head);
				if (nums2minor(adap1->num, dev->type, dev->id) == minor)
					return dev;
			}
		}
	}
	return NULL;
}

int proc_bus_nim_sockets_read(char *page, char **start, off_t off, int count,
							  int *eof, void *data_unused)
{
	int len = 0;
	int i, j, k;
	int feIndex = 0;
	struct dvb_device *dev;
#if (DVB_API_VERSION < 5)
	struct dvbfe_info fe_info; // old api 3 (insertion)
#else
	struct dtv_property p; // new api 5 (standard)
#endif
	struct dvb_frontend *fe;
	char *pType = "unknown";
	char *pMode = NULL;
	struct
	{
		int type;
		char *pType;
	} typeMap[] =
	{
#if (DVB_API_VERSION < 5)
		{ DVBFE_DELSYS_DVBS2, "DVB-S2"},
		{ DVBFE_DELSYS_DVBS, "DVB-S" },
		{ DVBFE_DELSYS_DVBT, "DVB-T" },
		{ DVBFE_DELSYS_DVBC, "DVB-C" }
#else
		{ SYS_DVBS2, "DVB-S2"},
		{ SYS_DVBS, "DVB-S" },
		{ SYS_DVBT2, "DVB-T2" },
		{ SYS_DVBT, "DVB-T" },
		{ SYS_DSS, "DVB-DSS"},
		{ SYS_DVBC_ANNEX_AC, "DVB-C"},
		{ SYS_DVBC_ANNEX_B, "DVB-C" }
#endif
	};
	/* loop over all adapters */
	for (j = 0; j < 4; j++)
	{
		/* loop over all frontends */
		for (i = 0; i < 4; i++)
		{
			if ((len + MAX_NIM_LENGTH) > count)
			{
				printk("%s(): not enough space\n", __func__);
				break;
			}
			if ((dev = dvbdev_find_device(nums2minor(j, DVB_DEVICE_FRONTEND, i))) == NULL)
			{
				/* there are no further frontends registered with this adapter */
				break;
			}
			fe = dev->priv;
			/* loop over all types */
			for (k = 0; k < sizeof(typeMap) / sizeof(typeMap[0]); k++)
			{
#if (DVB_API_VERSION < 5)
				fe_info.delivery = typeMap[k].type;
				if (fe->ops.get_info(fe, &fe_info) == 0)
				{
					pType = typeMap[k].pType;
					break;
				}
#else
				p.cmd = DTV_DELIVERY_SYSTEM;
				p.u.data = typeMap[k].type;
				if (fe->ops.get_property(fe, &p) == 0)
				{
					pType = typeMap[k].pType;
					if (typeMap[k].type == SYS_DVBT2 || typeMap[k].type == SYS_DVBT)
					{
						p.cmd = DTV_DELIVERY_SYSTEM;
						p.u.data = SYS_DVBC_ANNEX_AC;
						if (fe->ops.get_property(fe, &p) == 0)
							pMode = "DVB-C";
					}
					break;
				}
#endif
			}
			len += sprintf(page + len, "NIM Socket %d:\n"
						   "Type: %s\n"
						   "Name: %s\n"
						   "Frontend_Device: %d\n",
						   feIndex, pType, fe->ops.info.name, feIndex);
#if (DVB_API_VERSION >= 5)
			if (fe->ops.info.caps & FE_CAN_MULTISTREAM)
				len += sprintf(page + len, "Multistream: yes\n");
#endif
			if (pMode)
			{
				len += sprintf(page + len, "Mode 0: %s\n"
							   "Mode 1: %s\n", pType, pMode);
			}
			feIndex++;
		}
	}
	return len;
}
