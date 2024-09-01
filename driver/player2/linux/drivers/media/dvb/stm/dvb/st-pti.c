
/*
 * ST-PTI DVB driver
 *
 * indent: indent -bl -bli0 -cdb -sc -bap -bad -pcs -prs -bls -lp -npsl -bbb st-pti.c
 */
#include <linux/version.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/module.h>
#include <linux/usb.h>
#include <linux/delay.h>
#include <linux/time.h>
#include <linux/errno.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
#include <asm/semaphore.h>
#else
#include <linux/semaphore.h>
#endif

#include <asm/io.h>

#include "dvb_module.h"
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,17)
#include "../../../../../../../pti/pti_hal.h"
#include <linux/bpa2.h>
#else
#include <linux/bigphysarea.h>
#endif

#include "dvb_frontend.h"
#include "dmxdev.h"
#include "dvb_demux.h"
#include "dvb_net.h"

#include "backend.h"

#include <linux/dvb/frontend.h>
#include <linux/dvb/dmx.h>
#include <linux/dvb/ca.h>

#include "st-common.h"
#include "st-merger.h"

//__TDT__: many modifications in this file

#if defined(ADB_BOX)
int TsinMode;
static char *TSIS_mode = "parallel";
module_param(TSIS_mode, charp, 0);
MODULE_PARM_DESC(TSIS_mode, "TSIS_mode type: serial, parallel (default parallel");

int glowica;
static char *NIMS = "single";
module_param(NIMS, charp, 0);
MODULE_PARM_DESC(NIMS, "NIMS type: single,twin (default single");

enum
{
	SERIAL,
	PARALLEL,
};

enum
{
	SINGLE,
	TWIN,
};
#endif

#if defined(UFS922)
extern void cx24116_register_frontend(struct dvb_adapter *dvb_adap);
extern void avl2108_register_frontend(struct dvb_adapter *dvb_adap);
#elif defined(UFC960)
extern void fe_core_register_frontend(struct dvb_adapter *dvb_adap);
#elif defined(FORTIS_HDBOX) \
 || defined(UFS912) \
 || defined(SPARK) \
 || defined(HS7110) \
 || defined(HS7810A) \
 || defined(HS7420) \
 || defined(HS7429) \
 || defined(HS7119) \
 || defined(HS7819) \
 || defined(ATEMIO520) \
 || defined(ATEMIO530) \
 || defined(VITAMIN_HD5000) \
 || defined(SAGEMCOM88) \
 || defined(PACE7241)
extern void stv090x_register_frontend(struct dvb_adapter *dvb_adap);
#elif defined(HL101) \
 || defined(VIP1_V2) \
 || defined(VIP2_V1) \
 || defined(IPBOX9900) \
 || defined(IPBOX99) \
 || defined(IPBOX55) \
 || defined(ADB_BOX)
extern void fe_core_register_frontend(struct dvb_adapter *dvb_adap);
#elif defined(CUBEREVO) \
 || defined(CUBEREVO_MINI2) \
 || defined(CUBEREVO_MINI) \
 || defined(CUBEREVO_250HD) \
 || defined(CUBEREVO_2000HD) \
 || defined(CUBEREVO_9500HD) \
 || defined(CUBEREVO_MINI_FTA) \
 || defined(CUBEREVO_3000HD)
extern void socket_register_adapter(struct dvb_adapter *dvb_adap);
#elif defined(OCTAGON1008)
extern void avl2108_register_frontend(struct dvb_adapter *dvb_adap);
#elif defined(ATEVIO7500)
extern void socket_register_adapter(struct dvb_adapter *dvb_adap);
#elif defined(SPARK7162)
extern void spark7162_register_frontend(struct dvb_adapter *dvb_adap);
#elif defined(UFS913)
extern void socket_register_adapter(struct dvb_adapter *dvb_adap);
#else
extern void cx24116_register_frontend(struct dvb_adapter *dvb_adap);
#endif

extern void demultiplexDvbPackets(struct dvb_demux *demux, const u8 *buf, int count);

extern void pti_hal_init(struct stpti *pti, struct dvb_demux *demux, void (*_demultiplexDvbPackets)(struct dvb_demux *demux, const u8 *buf, int count), int numVideoBuffers);

extern int swts;

#if defined(SAGEMCOM88)
extern int hasdvbt;
#endif

#if defined(IPBOX9900) \
 || defined(IPBOX99)
extern int twinhead;
#endif

int stpti_start_feed(struct dvb_demux_feed *dvbdmxfeed,
					 struct DeviceContext_s *DeviceContext)
{
	struct dvb_demux *demux = dvbdmxfeed->demux;
	int vLoop, my_pes_type;
	struct PtiSession *pSession = DeviceContext->pPtiSession;
	BUFFER_TYPE bufType = MISC_BUFFER;
	if (pSession == NULL)
	{
		printk("%s: pSession == NULL\n", __func__);
		return -1;
	}
	/* PTI is only started if the source is one of two frontends or
	 if playback via SWTS is activated. Otherwise playback would
	 unnecessarily waste a buffer (might lead to loss of a second
	 recording). */
#if defined(ADB_BOX) \
 || defined(SAGEMCOM88) \
 || defined(SPARK7162)
	if (!(((pSession->source >= DMX_SOURCE_FRONT0) &&
			(pSession->source <= DMX_SOURCE_FRONT3)) ||
			((pSession->source == DMX_SOURCE_DVR0) && swts)))
		return -1;
#else
	if (!(((pSession->source >= DMX_SOURCE_FRONT0) &&
			(pSession->source <= DMX_SOURCE_FRONT2)) ||
			((pSession->source == DMX_SOURCE_DVR0) && swts)))
		return -1;
#endif
#ifdef VERY_VERBOSE
	printk("start dmx %p, sh %d, pid %d, t %d, pt %d\n", demux,
		   pSession->session, dvbdmxfeed->pid, dvbdmxfeed->type,
		   dvbdmxfeed->pes_type);
#endif
	switch (dvbdmxfeed->type)
	{
		case DMX_TYPE_TS:
			break;
		case DMX_TYPE_SEC:
			bufType = MISC_BUFFER;
			break;
		case DMX_TYPE_PES:
		default:
			printk("%s: feed type = %d (not supported) <\n", __FUNCTION__,
				   dvbdmxfeed->type);
			return -EINVAL;
	}
	if (dvbdmxfeed->type == DMX_TYPE_TS)
	{
		switch (dvbdmxfeed->pes_type)
		{
			case DMX_TS_PES_VIDEO0:
			case DMX_TS_PES_VIDEO1:
				bufType = VID_BUFFER;
				break;
			case DMX_TS_PES_AUDIO0:
			case DMX_TS_PES_AUDIO1:
				bufType = AUD_BUFFER;
				break;
			case DMX_TS_PES_TELETEXT:
			case DMX_TS_PES_PCR:
			case DMX_TS_PES_OTHER:
				break;
			default:
				printk("%s: pes type = %d (not supported) <\n", __FUNCTION__,
					   dvbdmxfeed->pes_type);
				return -EINVAL;
		}
	}
#ifdef VERY_VERBOSE
	else
	{
		dprintk("type = %d \n", dvbdmxfeed->type);
	}
#endif
	if (dvbdmxfeed->type == DMX_TYPE_SEC)
		my_pes_type = 99;
	else
		my_pes_type = dvbdmxfeed->pes_type;
	for (vLoop = 0; vLoop < pSession->num_pids; vLoop++)
	{
		if (((unsigned short) pSession->pidtable[vLoop] ==
				(unsigned short) dvbdmxfeed->pid))
		{
			pSession->references[vLoop]++;
			//ok we have a reference but maybe this one must be rechecked to a new
			//dma (video) and maybe we must attach the descrambler
			//normally we should all these things (referencing etc)
			//in the hal module. later ;-)
			/* link audio/video slot to the descrambler */
			if (dvbdmxfeed->type == DMX_TYPE_TS)
			{
				if ((dvbdmxfeed->pes_type == DMX_TS_PES_VIDEO) || (dvbdmxfeed->pes_type == DMX_TS_PES_AUDIO) ||
						((dvbdmxfeed->pes_type == DMX_TS_PES_OTHER) && (dvbdmxfeed->pid > 50))
				   )
					/* go hack: let's think about this (>50) maybe it is necessary to descramble this tables too ?!?! */
				{
					int err;
					if ((err = pti_hal_descrambler_link(pSession->session, pSession->descramblers[pSession->descramblerindex[vLoop]], pSession->slots[vLoop])) != 0)
						printk("Error linking slot %d to descrambler %d, err = %d\n", pSession->slots[vLoop], pSession->descramblers[pSession->descramblerindex[vLoop]], err);
#ifdef VERY_VERBOSE
					else
						printk("linking slot %d to descrambler %d, session = %d type = %d\n", pSession->slots[vLoop], pSession->descramblers[pSession->descramblerindex[vLoop]], pSession->session, dvbdmxfeed->pes_type);
#endif
				}
			}
#ifdef VERY_VERBOSE
			printk("pid %d already collecting. references %d \n",
				   dvbdmxfeed->pid, pSession->references[vLoop]);
#endif
			return 0;
		}
	}
	pSession->pidtable[pSession->num_pids] = dvbdmxfeed->pid;
	pSession->type[pSession->num_pids] = dvbdmxfeed->type;
	pSession->pes_type[pSession->num_pids] = my_pes_type;
	pSession->references[pSession->num_pids] = 1;
	pSession->slots[pSession->num_pids] = pti_hal_get_new_slot_handle(pSession->session,
																	  dvbdmxfeed->type,
																	  dvbdmxfeed->
																	  pes_type, demux, NULL, NULL);
	pSession->descramblerindex[pSession->num_pids] = pSession->descramblerForPid[dvbdmxfeed->pid];
#ifdef VERY_VERBOSE
	printk("SlotHandle = %d\n", pSession->slots[pSession->num_pids]);
#endif
	if (pti_hal_slot_link_buffer(pSession->session,
								 pSession->slots[pSession->num_pids],
								 bufType) != 0)
	{
		// free slot
		pti_hal_slot_free(pSession->session, pSession->slots[pSession->num_pids]);
		return -1;
	}
	if (dvbdmxfeed->type == DMX_TYPE_TS)
	{
		/* link audio/video slot to the descrambler */
		if ((dvbdmxfeed->pes_type == DMX_TS_PES_VIDEO) || (dvbdmxfeed->pes_type == DMX_TS_PES_AUDIO) ||
				((dvbdmxfeed->pes_type == DMX_TS_PES_OTHER) && (dvbdmxfeed->pid > 50))
				/* go hack: let's think about this (>50) maybe it is necessary to descramble this tables too ?!?! */
		   )
		{
			int err;
			if ((err = pti_hal_descrambler_link(pSession->session,
												pSession->descramblers[pSession->descramblerindex[pSession->num_pids]],
												pSession->slots[pSession->num_pids])) != 0)
				printk("Error linking slot %d to descrambler %d, err = %d\n",
					   pSession->slots[pSession->num_pids],
					   pSession->descramblers[pSession->descramblerindex[pSession->num_pids]], err);
#ifdef VERY_VERBOSE
			else
				printk("linking slot %d to descrambler %d, session = %d type=%d\n", pSession->slots[pSession->num_pids], pSession->descramblers[pSession->descramblerindex[pSession->num_pids]], pSession->session, dvbdmxfeed->pes_type);
#endif
		}
	}
	pti_hal_slot_set_pid(pSession->session, pSession->slots[pSession->num_pids],
						 dvbdmxfeed->pid);
	//pti_hal_buffer_enable ( pSession->session, pSession->buffers[0] );
	//pti_hal_buffer_enable ( pSession->session, pSession->buffers[1] );
	pSession->num_pids++;
	dprintk("%s: pid = %d, num_pids = %d \n", __FUNCTION__, dvbdmxfeed->pid,
			pSession->num_pids);
#if 0
	printk("# pid t pt ref\n");
	for (vLoop = 0; vLoop < (pSession->num_pids); vLoop++)
	{
		printk("%d %4d %d %2d %d\n", vLoop, pSession->pidtable[vLoop], pSession->type[vLoop], pSession->pes_type[vLoop],
			   pSession->references[vLoop]);
	}
#endif
//	dprintk("%s: <\n", __FUNCTION__);
	return 0;
}

EXPORT_SYMBOL(stpti_start_feed);

int stpti_stop_feed(struct dvb_demux_feed *dvbdmxfeed,
					struct DeviceContext_s *pContext)
{
	int n, vLoop, my_pes_type;
	int haveFound = 0;
	struct PtiSession *pSession = pContext->pPtiSession;
	if (pSession == NULL)
	{
		printk("%s: pSession == NULL\n", __func__);
		return -1;
	}
	/* PTI was only started if the source is one of two frontends or
	 if playback via SWTS was activated. */
#if defined(ADB_BOX) \
 || defined(SAGEMCOM88) \
 || defined(SPARK7162)
	if (!(((pSession->source >= DMX_SOURCE_FRONT0) &&
			(pSession->source <= DMX_SOURCE_FRONT3)) ||
			((pSession->source == DMX_SOURCE_DVR0) && swts)))
		return -1;
#else
	if (!(((pSession->source >= DMX_SOURCE_FRONT0) &&
			(pSession->source <= DMX_SOURCE_FRONT2)) ||
			((pSession->source == DMX_SOURCE_DVR0) && swts)))
		return -1;
#endif
#ifdef VERY_VERBOSE
	printk("stop sh %d, pid %d, pt %d\n", pSession->session, dvbdmxfeed->pid, dvbdmxfeed->pes_type);
#endif
	//printk ( "%s(): demux = %p, context = %p, sesison = %p, pid = %d, type = %d, pes_type = %d>", __FUNCTION__, dvbdmxfeed->demux, pContext, pSession, dvbdmxfeed->pid, dvbdmxfeed->type, dvbdmxfeed->pes_type );
	if (dvbdmxfeed->type == DMX_TYPE_SEC)
		my_pes_type = 99;
	else
		my_pes_type = dvbdmxfeed->pes_type;
	/*
	 * Now reallocate the pids, and update id information
	 */
	for (vLoop = 0; vLoop < (pSession->num_pids); vLoop++)
	{
		if ((pSession->pidtable[vLoop] == dvbdmxfeed->pid))
		{
			pSession->references[vLoop]--;
#ifdef VERY_VERBOSE
			printk("Reference = %d\n", pSession->references[vLoop]);
#endif
			haveFound = 1;
			if (pSession->references[vLoop] == 0)
			{
				pti_hal_slot_unlink_buffer(pSession->session,
										   pSession->slots[vLoop]);
				//pti_hal_buffer_disable ( pSession->session, pSession->buffers[0] );
				pti_hal_slot_clear_pid(pSession->session, pSession->slots[vLoop]);
				pti_hal_slot_free(pSession->session, pSession->slots[vLoop]);
				//printk ( "found pid to stop: %d (index = %d) %d, %d\n", pSession->pidtable[vLoop],
				// vLoop , pSession->type[vLoop], pSession->pes_type[vLoop]);
				for (n = vLoop; n < (pSession->num_pids - 1); n++)
				{
					//printk ( "n = %d, old pid = %d, %d, %d, new pid = %d\n", n, pSession->pidtable[n], pSession->type[n], pSession->pes_type[n],
					// pSession->pidtable[n + 1] );
					pSession->pidtable[n] = pSession->pidtable[n + 1];
					pSession->slots[n] = pSession->slots[n + 1];
					pSession->type[n] = pSession->type[n + 1];
					pSession->pes_type[n] = pSession->pes_type[n + 1];
					pSession->references[n] = pSession->references[n + 1];
					pSession->descramblerindex[n] = pSession->descramblerindex[n + 1];
				}
				pSession->num_pids--;
#if 0
				if (dvbdmxfeed->pes_type == DMX_TS_PES_VIDEO)
				{
					/* reset the DMA threshold to 1 to allow low rate TS
					 to be signalled on time */
					/* FIXME: quick hack assuming that DMA 0 is always responsible for
					 the video */
					setDmaThreshold(0, 1);
				}
#endif
				break;
			}
		}
	}
	if (!haveFound)
	{
		printk("demux try to stop feed not captured by pti\n");
	}
	return 0;
}

EXPORT_SYMBOL(stpti_stop_feed);

static int convert_source(const dmx_source_t source)
{
	int tag = TS_NOTAGS;
	switch (source)
	{
		case DMX_SOURCE_FRONT0:
#if defined(UFS910) \
 || defined(OCTAGON1008) \
 || defined(UFS912) \
 || defined(ADB_BOX) \
 || defined(SPARK) \
 || defined(SPARK7162) \
 || defined(SAGEMCOM88)
			tag = TSIN2;
#else
			tag = TSIN0;
#endif
			break;
		case DMX_SOURCE_FRONT1:
#if defined(ADB_BOX)
			if (glowica == SINGLE)
			{
				tag = SWTS0;
			}
			else if (glowica == TWIN)
			{
				tag = TSIN0;
			}
#elif defined(UFS913)
			tag = 3;//TSIN2; //TSIN3
#elif defined(SAGEMCOM88)
			tag = TSIN3;
#elif defined(ARIVALINK200) \
 || defined(IPBOX55) \
 || defined(HL101)
			tag = SWTS0;
#elif defined(IPBOX9900) \
 || defined(IPBOX99)
			if (twinhead == 1)
			{
				tag = SWTS0;
			}
			else
			{
				tag = TSIN1;
			}
#else
			tag = TSIN1;
#endif
			break;
#if defined(SPARK7162)
		case DMX_SOURCE_FRONT2:
			tag = TSIN0;
			break;
		case (dmx_source_t)3: /* for ptiInit() which passes 0,1,2,3 instead of DVR0 */
			tag = SWTS0;
			break;
		case DMX_SOURCE_DVR0:
			tag = SWTS1; //fake tsin for DVR (DVBT-USB at swts0)
			//tag = SWTS0;
			break;
#elif defined(SAGEMCOM88)
		case DMX_SOURCE_FRONT2:
			if (hasdvbt == 1) tag = TSIN0; //Internal DVBT on uhd88
			if (hasdvbt == 0) tag = SWTS0; //DVBT-USB on esi88
			break;
		case DMX_SOURCE_FRONT3:
		case DMX_SOURCE_DVR0:
			if (hasdvbt == 1) tag = SWTS0; //DVBT-USB on uhd88
			if (hasdvbt == 0) tag = SWTS1; //fake tsin for DVR (DVBT-USB at swts0)
			break;
#elif defined(ADB_BOX)
		case DMX_SOURCE_FRONT2:
			if (glowica == SINGLE)
			{
				tag = TSIN0;
			}
			else if (glowica == TWIN)
			{
				tag = SWTS0;
			}
			break;
		case DMX_SOURCE_DVR0:
			tag = TSIN1; //fake tsin for DVR (DVBT-USB at swts0)
			break;
#elif defined(ARIVALINK200) \
 || defined(IPBOX55) \
 || defined(HL101)
		case DMX_SOURCE_DVR0:
			tag = TSIN1; //fake tsin for DVR (DVBT-USB at swts0)
			break;
#elif defined(IPBOX9900) \
 || defined(IPBOX99)
		case DMX_SOURCE_FRONT2:
			if (twinhead == 2)
			{
				tag = SWTS0;
			}
			break;
		case DMX_SOURCE_DVR0:
			tag = TSIN2;
			break;
#else
		case DMX_SOURCE_DVR0:
			tag = SWTS0;
			break;
#endif
		default:
			printk("%s(): invalid frontend source (%d)\n", __func__, source);
	}
	return tag;
}

static int sessionCounter = 0;
static int ptiInitialized = 0;

static struct stpti pti;

/********************************************************/

void ptiInit(struct DeviceContext_s *pContext)
{
#if defined(SAGEMCOM88) \
 || defined(UFS912) \
 || defined(UFS913) \
 || defined(SPARK) \
 || defined(SPARK7162) \
 || defined(ATEVIO7500) \
 || defined(HS7110) \
 || defined(HS7810A) \
 || defined(HS7420) \
 || defined(HS7429) \
 || defined(HS7119) \
 || defined(HS7819) \
 || defined(ATEMIO520) \
 || defined(ATEMIO530) \
 || defined(VITAMIN_HD5000) \
 || defined(PACE7241)
	unsigned long start = 0xfe230000;
#else
	unsigned long start = 0x19230000;
#endif
	struct PtiSession *pSession;
	int tag;
	int i;
#if defined(ADB_BOX)
	if ((TSIS_mode[0] == 0) || (strcmp("serial", TSIS_mode) == 0))
	{
		printk("TsinMode = SERIAL\n");
		TsinMode = SERIAL;
	}
	else if (strcmp("parallel", TSIS_mode) == 0)
	{
		printk("TsinMode = PARALLEL\n");
		TsinMode = PARALLEL;
	}
	if ((TSIS_mode[0] == 0) || (strcmp("single", NIMS) == 0))
	{
		printk("NIMS = SINGLE\n");
		glowica = SINGLE;
	}
	else if (strcmp("twin", NIMS) == 0)
	{
		printk("NIMS = TWIN\n");
		glowica = TWIN;
	}
#endif
	printk("%s context = %p, demux = %p\n", __FUNCTION__,
		   pContext, &pContext->DvbDemux);
	if (pContext->pPtiSession != NULL)
	{
		printk("PTI ERROR: attempted to initialize a device context with an existing session\n");
		return;
	}
	if (!ptiInitialized)
	{
		// the function is called for the first time
		// perform common PTI initialization
		/*
		 * ioremap the pti address space
		 */
		pti.pti_io = (unsigned int)ioremap(start, 0xFFFF);
		/*
		 * Setup the transport stream merger based on the configuration
		 */
		stm_tsm_init(/*config */ 1);
#if defined(ARIVALINK200) \
 || defined(TF7700) \
 || defined(UFS922) \
 || defined(UFC960) \
 || defined(FORTIS_HDBOX) \
 || defined(HL101) \
 || defined(VIP1_V2) \
 || defined(VIP2_V1) \
 || defined(CUBEREVO) \
 || defined(CUBEREVO_MINI2) \
 || defined(CUBEREVO_MINI) \
 || defined(CUBEREVO_250HD) \
 || defined(CUBEREVO_2000HD) \
 || defined(CUBEREVO_9500HD) \
 || defined(CUBEREVO_MINI_FTA) \
 || defined(CUBEREVO_3000HD) \
 || defined(ATEVIO7500) \
 || defined(IPBOX9900) \
 || defined(IPBOX99) \
 || defined(IPBOX55) \
 || defined(ADB_BOX) \
 || defined(UFS913) \
 || defined(SAGEMCOM88)
		pti_hal_init(&pti, &pContext->DvbDemux, demultiplexDvbPackets, 2);
#elif defined(SPARK7162)
		pti_hal_init(&pti, &pContext->DvbDemux, demultiplexDvbPackets, 3);
#else
		pti_hal_init(&pti, &pContext->DvbDemux, demultiplexDvbPackets, 1);
#endif
#if defined(FORTIS_HDBOX) \
 || defined(UFS912) \
 || defined(SPARK) \
 || defined(HS7110) \
 || defined(HS7420) \
 || defined(HS7429) \
 || defined(HS7810A) \
 || defined(HS7119) \
 || defined(HS7819) \
 || defined(ATEMIO520) \
 || defined(ATEMIO530) \
 || defined(VITAMIN_HD5000) \
 || defined(SAGEMCOM88) \
 || defined(PACE7241)
		stv090x_register_frontend(&pContext->DvbContext->DvbAdapter);
#elif defined(HL101) \
 || defined(VIP1_V2) \
 || defined(VIP2_V1) \
 || defined(IPBOX9900) \
 || defined(IPBOX99) \
 || defined(IPBOX55) \
 || defined(ADB_BOX)
		fe_core_register_frontend(&pContext->DvbContext->DvbAdapter);
#elif defined(CUBEREVO) \
 || defined(CUBEREVO_MINI2) \
 || defined(CUBEREVO_MINI) \
 || defined(CUBEREVO_250HD) \
 || defined(CUBEREVO_2000HD) \
 || defined(CUBEREVO_9500HD) \
 || defined(CUBEREVO_MINI_FTA) \
 || defined(CUBEREVO_3000HD)
		socket_register_adapter(&pContext->DvbContext->DvbAdapter);
#elif defined(OCTAGON1008)
		avl2108_register_frontend(&pContext->DvbContext->DvbAdapter);
#elif defined(ATEVIO7500)
		socket_register_adapter(&pContext->DvbContext->DvbAdapter);
#elif defined(SPARK7162)
		spark7162_register_frontend(&pContext->DvbContext->DvbAdapter);
#elif defined(UFC960)
		fe_core_register_frontend(&pContext->DvbContext->DvbAdapter);
#elif defined(UFS922)
		cx24116_register_frontend(&pContext->DvbContext->DvbAdapter);
		avl2108_register_frontend(&pContext->DvbContext->DvbAdapter);
#elif defined(UFS913)
		socket_register_adapter(&pContext->DvbContext->DvbAdapter);
#else
		cx24116_register_frontend(&pContext->DvbContext->DvbAdapter);
#endif
		ptiInitialized = 1;
	}
	/*
	 * Allocate the session structure
	 */
	pSession = (struct PtiSession *) kmalloc(sizeof(struct PtiSession), GFP_KERNEL);
	if (!pSession)
		return;
	memset((void *) pSession, 0, sizeof(*pSession));
	pSession->num_pids = 0;
	// get new session handle
	tag = convert_source(sessionCounter + DMX_SOURCE_FRONT0);
	pSession->session = pti_hal_get_new_session_handle(tag, &pContext->DvbDemux);
	// get new descrambler handle
	pSession->descrambler = pti_hal_get_new_descrambler(pSession->session);
	pSession->descramblers[0] = pSession->descrambler;
	for (i = 1; i < NUMBER_OF_DESCRAMBLERS - 1; i++)
		pSession->descramblers[i] = pti_hal_get_new_descrambler(pSession->session);
	printk("Descrambler Handler = %d\n", pSession->descrambler);
	for (i = 0; i < 8192; i++)
		pSession->descramblerForPid[i] = 0;
	pContext->pPtiSession = pSession;
	sessionCounter++;
	return;
}

EXPORT_SYMBOL(ptiInit);

int SetSource(struct dmx_demux *demux, const dmx_source_t *src)
{
	struct dvb_demux *pDvbDemux = (struct dvb_demux *)demux->priv;
	struct DeviceContext_s *pContext = (struct DeviceContext_s *)pDvbDemux->priv;
	if ((pContext == NULL) || (pContext->pPtiSession == NULL) || (src == NULL))
	{
		printk("%s(): invalid pointer (%p, %p, %p)\n",
			   __func__, pContext, pContext->pPtiSession, src);
		return -EINVAL;
	}
#ifdef VERY_VERBOSE
	printk("SetSource(%p, %d)\n", pDvbDemux, *src);
#if defined(SAGEMCOM88) \
 || defined(ADB_BOX) \
 || defined(ARIVALINK200) \
 || defined(SPARK7162) \
 || defined(IPBOX9900) \
 || defined(IPBOX99) \
 || defined(IPBOX55) \
 || defined(HL101)
	if (*src == DMX_SOURCE_FRONT0)
	{
		printk("DMX_SOURCE_FRONT0\n");
	}
	else if (*src == DMX_SOURCE_FRONT1)
	{
		printk("DMX_SOURCE_FRONT1\n");
	}
	else if (*src == DMX_SOURCE_FRONT2)
	{
		printk("DMX_SOURCE_FRONT2\n");
	}
	else if (*src == DMX_SOURCE_FRONT3)
	{
		printk("DMX_SOURCE_FRONT3\n");
	}
	else if (*src == DMX_SOURCE_DVR0)
	{
		printk("DMX_SOURCE_DVR0\n");
	}
#endif
#endif
	pContext->pPtiSession->source = *src;
	if (((*src >= DMX_SOURCE_FRONT0) && (*src <= DMX_SOURCE_FRONT3)) || (*src == DMX_SOURCE_DVR0))
	{
		pti_hal_set_source(pContext->pPtiSession->session, convert_source(*src));
	}
	return 0;
}

