
/*
 * ST-PTI DVB driver
 *
 * Copyright (c) STMicroelectronics 2005
 *
 *   Author:Peter Bennett <peter.bennett@st.com>
 * __TDT__: mod by teamducktales
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License as
 *	published by the Free Software Foundation; either version 2 of
 *	the License, or (at your option) any later version.
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
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
#include <asm/semaphore.h>
#else
#include <linux/semaphore.h>
#endif
#include <linux/platform_device.h>
#include <linux/mutex.h>

#include <asm/io.h>

#include <linux/bpa2.h>

struct DeviceContext_s;
struct StreamContext_s;

#include "dvb_frontend.h"
#include "dmxdev.h"
#include "dvb_demux.h"
#include "dvb_net.h"
#include <linux/dvb/dmx.h>

#include <linux/dvb/frontend.h>
#include <linux/dvb/dmx.h>

#include "pti.h"

#if defined(PLAYER_179) || defined(PLAYER_191)
#if (defined(HL101) || defined(VIP1_V2) || defined(VIP2_V1) || defined(SPARK) || defined(SPARK7162)) || defined(SAGEMCOM88) || defined(PACE7241)
static int waitMS=20;
static int videoMem=4096;
#endif
#elif defined(PLAYER_131)
#else
#endif

int debug ;
#define dprintk(x...) do { if (debug) printk(KERN_WARNING x); } while (0)

#ifdef __TDT__
unsigned int dma_0_buffer_base;
unsigned int dma_0_buffer_top;
unsigned int dma_0_buffer_rp;
#define TAG_COUNT 4
#define AUX_COUNT 20
#else
#define TAG_COUNT 3
#define AUX_COUNT 20
#endif

struct tSlot
{
  int Handle;			/* Our Handle */
  int inUse;
  int tcIndex;
  u16 pid;			/* Current set pid */
};

struct pti_internal {
  struct  dvb_demux* demux[TAG_COUNT];
  #if defined(SPARK7162)
  int		demux_tag[TAG_COUNT];
  #endif
  int	  err_count;

  wait_queue_head_t queue;

  char *back_buffer;
  unsigned int pti_io;

  short int *pidtable;
  short int *num_pids;
  short int *pidsearch;
  volatile unsigned short int *psize;
  volatile unsigned short int *pread;
  volatile unsigned short int *thrown_away;
  volatile unsigned short int *packet_size;
  volatile unsigned short int *header_size;
  volatile unsigned short int *pwrite;
  volatile unsigned short int *discard;

  unsigned int loop_count;
  unsigned int loop_count2;
  unsigned int packet_count;

  struct tSlot **vSlots;
};


static struct stpti *external = NULL;
static struct pti_internal *internal = NULL;
static void (*demultiplex_dvb_packets)(struct dvb_demux* demux, const u8 *buf, int count) = NULL;

#ifdef __TDT__
#define DMA_POLLING_INTERVAL (1)
#else
#define DMA_POLLING_INTERVAL msecs_to_jiffies(2)
#endif
#define HEADER_SIZE (6)
#define PACKET_SIZE (188+HEADER_SIZE)

#ifdef __TDT__
#define PACKET_SIZE_WO_HEADER (188)
#endif

#define PTI_BUFFER_SIZE (PACKET_SIZE * 4096) /*(188*1024)*/

static struct timer_list ptiTimer;

#define SLOT_HANDLE_OFFSET		20000
#define QUEUE_SIZE 400

/* The work queue is a communication means between the process
   polling the DMA and the process injecting the TS data into the
   demultiplexer. It is assumed that the injector process is fast
   enough to process the entries in the work queue. */
static struct
{
  int offset;
  int count;
} workQueue[QUEUE_SIZE];

static struct semaphore workSem;

static int readIndex;
static int writeIndex;

/* PTI write ignores byte enables */
static void PtiWrite(volatile unsigned short int *addr,unsigned short int value)
{
  unsigned int *addr2 = (unsigned int*)(((unsigned int)addr) & ~3);
  unsigned int old_value  = *addr2;

  if (((unsigned)addr & 2)) old_value = (old_value & 0x0000ffff) | value << 16;
  else old_value = (old_value & 0xffff0000) | value;

  *addr2 = old_value;
}

/***********************************************************************************/

#define TCASM_LM_SUPPORT
#include "tc_code.h"

static void loadtc(struct pti_internal *pti)
{
  int n;

  printk("%s: >\n",__FUNCTION__);

  for (n=0;n<=TCASM_LableMap[0].Value;n+=4)
    writel(transport_controller_code[n/4], pti->pti_io + PTI_IRAM_BASE + n );

  for (n=0;n<(sizeof(transport_controller_data) * 2);n+=4)
    writel(transport_controller_data[n/4], pti->pti_io + PTI_DRAM_BASE + n);

  printk("%s: <\n",__FUNCTION__);
}

static void *getsymbol(struct pti_internal *pti, const char *symbol)
{
  char temp[128];
  void *result = NULL;
  int n;

  temp[120] = 0;
  sprintf(temp,"::_%s",symbol);

  for (n=0;n<TRANSPORT_CONTROLLER_LABLE_MAP_SIZE;n++)
  	if (!strcmp(temp,TCASM_LableMap[n].Name))
  	result = (void*)(pti->pti_io + TCASM_LableMap[n].Value);

  return result;

}

/***************************************************************************************/

static void stpti_setup_dma(struct pti_internal *pti)
{
  /* Setup DMA0 to transfer the data to the buffer */
  writel( virt_to_phys(pti->back_buffer), pti->pti_io + PTI_DMA_0_BASE );
  writel( virt_to_phys(pti->back_buffer), pti->pti_io + PTI_DMA_0_WRITE);
  writel( virt_to_phys(pti->back_buffer), pti->pti_io + PTI_DMA_0_READ );
  writel( virt_to_phys(pti->back_buffer + PTI_BUFFER_SIZE - 1), pti->pti_io + PTI_DMA_0_TOP );
#ifdef __TDT__
	dma_0_buffer_base = virt_to_phys(pti->back_buffer);
	dma_0_buffer_top = virt_to_phys(pti->back_buffer) + PTI_BUFFER_SIZE;
	dma_0_buffer_rp = dma_0_buffer_base;
#endif

  writel( 0x8, pti->pti_io + PTI_DMA_0_SETUP ); /* 8 word burst */
}

static void stpti_start_dma(struct pti_internal *pti)
{
  /* Enable the DMA */
  writel( readl(pti->pti_io + PTI_DMA_ENABLE) | 0x1, pti->pti_io + PTI_DMA_ENABLE);

  /* Tell PTI we have lots of room in buffer */
  PtiWrite(pti->pread, 0);
  PtiWrite(pti->pwrite,0);
  PtiWrite(pti->discard, 0);

  /* Reset all the counts */
  pti->loop_count = 0;
  pti->loop_count2 = 0;
  pti->packet_count=0;
}

static void stpti_stop_dma(struct pti_internal *pti)
{
int loop_count = 0;

  /* Stop DMAing data */
  PtiWrite(pti->pread, 0);

  while (*pti->pwrite != *pti->psize +2)
  {
    PtiWrite(pti->pwrite, *pti->psize + 2);
    PtiWrite(pti->pwrite, *pti->psize + 2);
    dprintk("%s: Pwrite = %u Psize = %u\n",__FUNCTION__,*pti->pwrite,*pti->psize);
  };

  udelay(100);

  /* Ensure DMA is enabled */
  writel( readl(pti->pti_io + PTI_DMA_ENABLE) | 0x1, pti->pti_io + PTI_DMA_ENABLE );

  /* Do DMA Done to ensure all data has been transfered */
  writel( PTI_DMA_DONE , pti->pti_io + PTI_DMA_0_STATUS );

  while( (readl(pti->pti_io + PTI_DMA_0_STATUS) & PTI_DMA_DONE) &&
	 (loop_count<100))
    { udelay(1000); loop_count++; };

  /* Disable the DMA */
  writel(readl(pti->pti_io + PTI_DMA_ENABLE) & ~0x1, pti->pti_io + PTI_DMA_ENABLE);
}

static void stpti_reset_dma(struct pti_internal *pti)
{
  /* Stop the DMA */
  stpti_stop_dma(pti);

  /* Reset dma pointers */
  stpti_setup_dma(pti);
}


/*

0x10   ----------------- <--- PTI_DMA_x_BASE
       |               |   |
       |               |  \|/
       |               | <--- PTI_DMA_x_READ  (Last address sent to ADSC)
       |               |
       |               |
       |               | <--- PTI_DMA_x_WRITE (Last address written by PTI)
       |               |   ^
       |               |   |  (free space)
       |               |   |
0x1000 ----------------- <--- PTI_DMA_x_TOP

0x10   ----------------- <--- PTI_DMA_x_BASE
       |               |
       |               |
       |               | <--- PTI_DMA_x_WRITE (Last address written by PTI)
       |               |   |
       |               |   | (free space)
       |               | <--- PTI_DMA_x_READ  (Last address sent to ADSC)
       |               |
       |               |
       |               |
0x1000 ----------------- <--- PTI_DMA_x_TOP

*/

static int stream_injector(void *user_data)
{
  int offset, count;
  int overflow = 0;

//aktivate STREAMCHECK for debug
//#define STREAMCHECK
#ifdef STREAMCHECK
	u8 vpidhigh=0; //high byte of the video stream to check
	u8 vpidlow=0x65; //low byte of the video stream to check
	u8 apidhigh=0; //high byte of the audio stream to check
	u8 apidlow=0x66; //low byte of the audio stream to check
	int vc=99; //video count
	int ac=99; //audio count
	u8 tc; //temp count
	unsigned long prv=30000; //print video output after xxxx counts
	unsigned long pra=30000; //print audio autput after xxxx counts
#endif

  daemonize ("ts-injector");

  allow_signal(SIGKILL);
  allow_signal(SIGTERM);

#ifdef __TDT__
  //set high thread priority
  set_user_nice(current, -20);
#endif

  while(1)
  {
	/* the polling process increments the semaphore whenever it writes
           an entry into the queue */
	if(down_interruptible(&workSem))
	  break;

	if(writeIndex == readIndex)
	{
		if(!overflow)
		{
			printk("PTI: queue overflow\n");
			overflow = 1;
		}
		/* discard the buffer because the injector is too slow */
		continue;
	}
	overflow = 0;

	/* copy the start offset and the packet count to local variables */
	offset = workQueue[readIndex].offset;
	count = workQueue[readIndex].count;

	//printk(".");

	/* invalidate the cache */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
	dma_cache_inv((void*)&internal->back_buffer[offset],
			count * PACKET_SIZE);
#else
	invalidate_ioremap_region(0, internal->back_buffer,
			offset, count * PACKET_SIZE);
#endif

#ifdef STREAMCHECK
	/* The first bytes of the packet after the header should
	   always be a 0x47 if not we got problems */
	if (internal->back_buffer[offset + HEADER_SIZE] != 0x47)
		printk("\n!0x47\n");
#endif

	/* Now go through each packet, check it's tag and
           place it in the correct demux buffer */
	if(demultiplex_dvb_packets != NULL)
	{
	  u8 *pFrom = &internal->back_buffer[offset];
#ifdef __TDT__
	  static u8 auxbuf[TAG_COUNT][PACKET_SIZE_WO_HEADER * AUX_COUNT];
	  u8 *pTo[TAG_COUNT] = {auxbuf[0],auxbuf[1],auxbuf[2],auxbuf[3]};
	  int count1[TAG_COUNT] = {0, 0, 0, 0};
#else
		static u8 auxbuf[TAG_COUNT][(PACKET_SIZE - HEADER_SIZE) * AUX_COUNT];
		u8 *pTo[TAG_COUNT] = {auxbuf[0],auxbuf[1],auxbuf[2]};
	  int count1[TAG_COUNT] = {0, 0, 0};
#endif
	  int n;

	  /* sort the packets according to the tag,
	     remove the TS merger tags and squeeze the
	     packets to improve the performance */
	  for (n = 0; n < count; n++)
	  {
	    /* Only the tag IDs of TS inputs are taken into account.
	       The lower two bits of the tag ID are as follows:
	       00 - TS0
	       01 - TS1
	       10 - TS2
				 11 - SWTS0 */
	    int tag = (pFrom[0] >> 2) & 0xf;
	    /* only copy if the demux exists */
	#if defined(SPARK7162)
	    if((tag < TAG_COUNT) &&
	       (internal->demux[internal->demux_tag[tag]] != NULL))
	#else
	    if((tag < TAG_COUNT) &&
	       (internal->demux[tag] != NULL))
	#endif
	    {

#ifdef STREAMCHECK
				//check for startbyte of all paket
				if (pFrom[6] != 0x47)
					printk("[STREAMCHECK] first byte of packet not 0x47\n");

				//check count of the choised videostream
				if((pFrom[7]&0x1f)==vpidhigh && pFrom[8]==vpidlow) {
					prv++;
					if(prv>20000) {
						printk("[STREAMCHECK] 20000 video PIDs found 0x%x%x\n",(pFrom[7]&0x1f),pFrom[8]);
  					prv=0;
					}
					tc=pFrom[9];
					tc=(tc<<4);
					tc=(tc>>4);
					if(vc==99)
						vc=tc;
					else {
						vc++;
						if(vc>15) vc=0;
						if(vc!=tc) {
							printk("[STREAMCHECK] invalide video count - count=%d, packetcount=%d, pid=0x%x%x\n",vc,tc,(pFrom[7]&0x1f),pFrom[8]);
							vc=tc;
						}
					}
				}

				//check count of the choised audiostream
				if((pFrom[7]&0x1f)==apidhigh && pFrom[8]==apidlow) {
					pra++;
					if(pra>20000) {
						printk("[STREAMCHECK] 20000 audio PID found 0x%x%x\n",(pFrom[7]&0x1f),pFrom[8]);
						pra=0;
					}
					tc=pFrom[9];
					tc=(tc<<4);
					tc=(tc>>4);
					if(ac==99)
  					ac=tc;
					else {
						ac++;
						if(ac>15) ac=0;
						if(ac!=tc) {
							printk("[STREAMCHECK] invalide audio count - count=%d, packetcount=%d, pid=0x%x%x\n",ac,tc,(pFrom[7]&0x1f),pFrom[8]);
							ac=tc;
						}
					}
				}
#endif
#ifdef __TDT__
	      memmove(pTo[tag], pFrom + HEADER_SIZE, PACKET_SIZE_WO_HEADER);
	      pTo[tag] += PACKET_SIZE_WO_HEADER;
#else
				memmove(pTo[tag], pFrom + HEADER_SIZE,
		      PACKET_SIZE - HEADER_SIZE);
	      pTo[tag] += PACKET_SIZE - HEADER_SIZE;
#endif
	      count1[tag]++;
	      if(count1[tag] >= AUX_COUNT)
	      {
		//printk("%d", tag);
		/* inject the packets */
	#if defined(SPARK7162)
			demultiplex_dvb_packets(internal->demux[internal->demux_tag[tag]],
						auxbuf[tag], count1[tag]);
	#else
			demultiplex_dvb_packets(internal->demux[tag],
						auxbuf[tag], count1[tag]);
	#endif
		pTo[tag] = auxbuf[tag];
		count1[tag] = 0;
	      }
	    }
	    pFrom += PACKET_SIZE;
	  }
	  for(n = 0; n < TAG_COUNT; n++)
	  {
	    /* inject remainders if any */
	#if defined(SPARK7162)
	    if((count1[n] > 0) && (internal->demux[internal->demux_tag[n]] != NULL))
	      demultiplex_dvb_packets(internal->demux[internal->demux_tag[n]], auxbuf[n], count1[n]);
	#else
	    if((count1[n] > 0) && (internal->demux[n] != NULL))
	      demultiplex_dvb_packets(internal->demux[n], auxbuf[n], count1[n]);
	#endif
	  }
	}

	/* increment the readIndex */
	readIndex = (readIndex + 1) % QUEUE_SIZE;
  }

  return 0;
}

/* This function is called periodically to poll the TC DMA structures.
   The original version used interrupts which caused high CPU load.
   In order to have a higher priority than the injector thread this function
   is called by a timer as opposed to execution in a thread.
   The published TC DMA code is not robust enough when the buffer contains
   lots of packets (presumably > 500) even if the buffer is not full.
   Therefore, it is important that the polling process always does its work
   on time. */
#ifdef __TDT__
static void process_pti_dma(unsigned long data)
{
  unsigned int pti_wp, num_packets, pti_status;
  bool buffer_round=0;

	/* Load the write pointers, so we know where we are in the buffers */
	pti_wp = readl(internal->pti_io + PTI_DMA_0_WRITE);

	//align dma write pointer to packet size
	pti_wp = pti_wp - ((pti_wp - dma_0_buffer_base) % PACKET_SIZE);

	/* Read status registers */
	pti_status = readl(internal->pti_io + PTI_IIF_FIFO_COUNT);

	/* Error if we overflow */
	if (pti_status & PTI_IIF_FIFO_FULL)
	{
		internal->err_count++;
		printk(KERN_WARNING "%s: IIF Overflow\n",__FUNCTION__);
	}

	/* If the PTI had to drop packets because we couldn't process in time error */
	if (*internal->discard)
	{
		printk(KERN_WARNING "%s: PTI had to discard %u packets %u %u\n",__func__,
			*internal->discard,*internal->pread,*internal->pwrite);

		internal->err_count++;
		stpti_reset_dma(internal);
		//writeIndex = 0;
		//readIndex = 0;
		//the semaphore must by also resetet to 0, fix later
		stpti_start_dma(internal);
	} else
	{
		/* If we get to the bottom of the buffer wrap the pointer back to the top */
		if ((dma_0_buffer_rp & ~0xf) == (dma_0_buffer_top & ~0xf)) dma_0_buffer_rp = dma_0_buffer_base;

		/* Calculate the amount of bytes used in the buffer */
		if (dma_0_buffer_rp <= pti_wp) num_packets = (pti_wp - dma_0_buffer_rp) / PACKET_SIZE;
		else
		{
			num_packets = ((dma_0_buffer_top - dma_0_buffer_rp) + (pti_wp - dma_0_buffer_base)) / PACKET_SIZE;
			internal->loop_count2++;
			internal->packet_count = 0;
			buffer_round=1;
		}

		/* If we have some packets */
		if (num_packets)
		{
			/* And the PTI has acknowledged the updated the packets */
			if (!*internal->pread)
			{
				/* Increment the loop counter */
				internal->loop_count++;

				/* Increment the packet_count, by the number of packets we have processed */
				internal->packet_count+=num_packets;

				/* Now update the read pointer in the DMA engine */
				writel(pti_wp, internal->pti_io + PTI_DMA_0_READ );

				/* Now tell the firmware how many packets we have read */
				PtiWrite(internal->pread, num_packets);

				/* notify the injector thread */
				if(buffer_round) {
					unsigned int num_packets1 = (dma_0_buffer_top - dma_0_buffer_rp) / PACKET_SIZE;
					workQueue[writeIndex].offset = dma_0_buffer_rp - dma_0_buffer_base;
					workQueue[writeIndex].count = num_packets1;
					writeIndex = (writeIndex + 1) % QUEUE_SIZE;
					up(&workSem);

					workQueue[writeIndex].offset = 0;
					workQueue[writeIndex].count = num_packets-num_packets1;
					writeIndex = (writeIndex + 1) % QUEUE_SIZE;
					up(&workSem);
				}
				else {
					workQueue[writeIndex].offset = dma_0_buffer_rp - dma_0_buffer_base;
					workQueue[writeIndex].count = num_packets;
					writeIndex = (writeIndex + 1) % QUEUE_SIZE;
					up(&workSem);
				}
				dma_0_buffer_rp = pti_wp;
			} // not read
		} // num_packet
	} // discard

	/* reschedule the timer */
	ptiTimer.expires = jiffies + DMA_POLLING_INTERVAL;
	add_timer(&ptiTimer);
}

#else

static void process_pti_dma(unsigned long data)
{
    unsigned int pti_rp, pti_wp, pti_base, pti_top, size_first, num_packets,
		new_rp, pti_status, dma_status;
    static unsigned int pti_status_old=0;
    static unsigned int dma_status_old=0;

	  /* Load the read and write pointers, so we know where we are in the buffers */
	  pti_rp   = readl(internal->pti_io + PTI_DMA_0_READ);
	  pti_wp   = readl(internal->pti_io + PTI_DMA_0_WRITE);
	  pti_base = readl(internal->pti_io + PTI_DMA_0_BASE);
	  pti_top  = pti_base + PTI_BUFFER_SIZE;

	  /* Read status registers */
	  pti_status = readl(internal->pti_io + PTI_IIF_FIFO_COUNT);
	  dma_status = readl(internal->pti_io + PTI_DMA_0_STATUS);

	  /* Error if we overflow */
	  if (pti_status & PTI_IIF_FIFO_FULL)
	  {
	     internal->err_count++;
	     printk(KERN_WARNING "%s: IIF Overflow\n",__FUNCTION__);
	  }

	  if (dma_status != dma_status_old)
	  	dprintk("%s: DMA Status %x %x\n",__FUNCTION__,dma_status,internal->loop_count);

	  /* If the PTI had to drop packets because we couldn't process in time error */
	  if (*internal->discard)
	  {
	    printk(KERN_WARNING "%s: PTI had to discard %u packets %u %u\n",__func__,
			*internal->discard,*internal->pread,*internal->pwrite);
	    printk(KERN_WARNING "%s: Reseting DMA\n",__FUNCTION__);

		internal->err_count++;
	        stpti_reset_dma(internal);
	        stpti_start_dma(internal);
	  } else
	  {

	     pti_status_old = pti_status;
	     dma_status_old = dma_status;

	     /* If we get to the bottom of the buffer wrap the pointer back to the top */
	     if ((pti_rp & ~0xf) == (pti_top & ~0xf)) pti_rp = pti_base;

	  /* Calculate the amount of bytes used in the buffer */
	  if (pti_rp <= pti_wp) size_first = pti_wp - pti_rp;
	  else size_first = pti_top - pti_rp;

	  /* Calculate the number of packets in the buffer */
	  num_packets = size_first / PACKET_SIZE;

	  /* If we have some packets */
	  if (num_packets)
	  {
	    /* And the PTI has acknowledged the updated the packets */
	    if (!*internal->pread)
	    {
	      int start_offset = pti_rp - pti_base;

	      /* Increment the loop counter */
	      internal->loop_count++;

	      /* The read pointer should always be a multiple of the packet size */
	      if ((pti_rp - pti_base) % PACKET_SIZE)
		printk(KERN_WARNING "%s: 0x%x not multiple of %d\n",__FUNCTION__,(pti_rp - pti_base),PACKET_SIZE);

		/* Update the read pointer based on the number of packets we have processed */
		new_rp = pti_rp + num_packets * PACKET_SIZE;

		/* Increment the packet_count, by the number of packets we have processed */
		internal->packet_count+=num_packets;
		if (new_rp > pti_top) dprintk("You b@*tard you killed kenny\n");

		/* If we have gone round the buffer */
		if (new_rp >= pti_top)
		{
		  //printk("+");
		  /* Update the read pointer so it now points back to the top */
		  new_rp = pti_base + (new_rp - pti_top); internal->loop_count2++;

		  /* Print out some useful debug information when debugging is on */
		  dprintk("%s: round the buffer %u times %u=pwrite %u=packet_count %u=num_packets %lu=jiffies %u %u\n",__FUNCTION__,
			  internal->loop_count2,*internal->pwrite,internal->packet_count,num_packets,jiffies,pti_rp,pti_wp);

		  /* Update the packet count */
		  internal->packet_count = 0;
		}

		/* Now update the read pointer in the DMA engine */
		writel(new_rp, internal->pti_io + PTI_DMA_0_READ );

		/* Now tell the firmware how many packets we have read */
		PtiWrite(internal->pread,num_packets);

		//printk("*");

		/* notify the injector thread */
		workQueue[writeIndex].offset = start_offset;
		workQueue[writeIndex].count = num_packets;
		writeIndex = (writeIndex + 1) % QUEUE_SIZE;
		up(&workSem);
	    } // not read
  	  } // num_packet
        } // discard

	/* reschedule the timer */
	ptiTimer.expires = jiffies + DMA_POLLING_INTERVAL;
	add_timer(&ptiTimer);
}
#endif

/********************************************************/

void pti_hal_init ( struct stpti *pti , struct dvb_demux* demux, void (*_demultiplex_dvb_packets)(struct dvb_demux* demux, const u8 *buf, int count))
{
  unsigned long start;
  int vLoopSlots;

  if(_demultiplex_dvb_packets == NULL)
  {
    printk("%s: no demultiplex function provided\n", __func__);
    return;
  }

  demultiplex_dvb_packets = _demultiplex_dvb_packets;
  external = pti;

  internal = kmalloc(sizeof(struct pti_internal), GFP_KERNEL);

  memset(internal, 0, sizeof(struct pti_internal));

	#if defined(SPARK7162)
	{
		int i;
		for (i = 0; i < TAG_COUNT; i++)
		{
			internal->demux_tag[i] = i;
		}
	}
	#endif

  /* Allocate the back buffer */
  internal->back_buffer = (char*)bigphysarea_alloc_pages((PTI_BUFFER_SIZE+PAGE_SIZE-1) / PAGE_SIZE,0,GFP_KERNEL);

  if (internal->back_buffer == NULL)
     printk("error allocating back buffer !!!!!!!!!!!!!!!!!!!!!!!!\n");

  /* ioremap the pti address space */
#if defined(UFS912) \
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
 || defined(SAGEMCOM88)
  start = 0xfe230000;
#else
  start = 0x19230000;
#endif

  internal->pti_io = (unsigned long)ioremap((unsigned long) start, 0xFFFF);

  printk("pti ioremap 0x%.8lx -> 0x%.8x\n", start, internal->pti_io);

  /* Resolve pointers we need for later */
  internal->pidtable    = getsymbol(internal, "pidtable");
  internal->num_pids    = getsymbol(internal, "num_pids");
  internal->pidsearch   = getsymbol(internal, "pidsearch");

  internal->psize       = getsymbol(internal, "psize");
  internal->pread       = getsymbol(internal, "pread");
  internal->pwrite      = getsymbol(internal, "pwrite");
  internal->packet_size = getsymbol(internal, "packet_size");

  internal->discard     = getsymbol(internal, "discard");
  internal->header_size = getsymbol(internal, "header_size");
  internal->thrown_away = getsymbol(internal, "thrown_away");

  /* Setup PTI */
  writel(0x1, internal->pti_io + PTI_DMA_PTI3_PROG);

  printk("Load TC ...\n");

  /* Write instructions and data */
  loadtc(internal);

  printk("Load TC done\n");

  PtiWrite(internal->packet_size,PACKET_SIZE);
  PtiWrite(internal->header_size,HEADER_SIZE);

  /* Enable IIF */
  writel(0x0,         internal->pti_io + PTI_IIF_SYNC_LOCK);
  writel(0x0,         internal->pti_io + PTI_IIF_SYNC_DROP);
  writel(PACKET_SIZE, internal->pti_io + PTI_IIF_SYNC_PERIOD);
  writel(0x0,         internal->pti_io + PTI_IIF_SYNC_CONFIG);
  writel(0x1,         internal->pti_io + PTI_IIF_FIFO_ENABLE);

  /* Start the TC */
  writel(PTI_MODE_ENABLE, internal->pti_io + PTI_MODE );

  /* Setup packet count */
  PtiWrite(internal->psize, (PTI_BUFFER_SIZE / (PACKET_SIZE)) - 2);
  PtiWrite(internal->num_pids,0);

  /* Reset the DMA */
  stpti_setup_dma(internal);
  stpti_reset_dma(internal);

  /* Enable the DMA of data */
#ifdef __TDT__
	stpti_start_dma(internal);
#else
  writel( readl(internal->pti_io + PTI_DMA_ENABLE) | 0x1, internal->pti_io + PTI_DMA_ENABLE );
#endif

  internal->vSlots =
      kmalloc ( sizeof ( struct tSlot * ) * 32,	GFP_KERNEL );

  for ( vLoopSlots = 0; vLoopSlots < 32; vLoopSlots++ )
  {
      internal->vSlots[vLoopSlots] =
	kmalloc ( sizeof ( struct tSlot ), GFP_KERNEL );

      memset ( internal->vSlots[vLoopSlots], 0, sizeof ( struct tSlot ) );

      internal->vSlots[vLoopSlots]->Handle = SLOT_HANDLE_OFFSET + vLoopSlots;
      internal->vSlots[vLoopSlots]->tcIndex = -1;
      internal->vSlots[vLoopSlots]->pid = 0xffff;
  }

  /* set up the processing thread */
  sema_init(&workSem, 0);
  kernel_thread ( stream_injector, internal, 0 );

  /* schedule the polling process */
  init_timer(&ptiTimer);
  ptiTimer.expires = jiffies + DMA_POLLING_INTERVAL;
  ptiTimer.function = process_pti_dma;
  ptiTimer.data = 0;
  add_timer(&ptiTimer);

  return;
}

int pti_hal_descrambler_set_aes ( int session_handle, int descrambler_handle,
			      u8 * Data, int parity, int data_type )
{
	return 0;
}

int pti_hal_descrambler_set ( int session_handle, int descrambler_handle,
			      u8 * Data, int parity )
{
        return 0;
}

int pti_hal_descrambler_unlink ( int session_handle, int descrambler_handle )
{
        return 0;
}

int pti_hal_descrambler_link ( int session_handle, int descrambler_handle,
			       int slot_handle )
{
        return 0;
}

int pti_hal_get_new_descrambler ( int session_handle )
{
	return 1;
}

int pti_hal_slot_clear_pid ( int session_handle, int slot_handle )
{
  int i;

  slot_handle -= SLOT_HANDLE_OFFSET;

  printk("%s slot = %d, tc = %d, num = %d\n", __FUNCTION__, slot_handle,
		internal->vSlots[slot_handle]->tcIndex,
		*internal->num_pids);

  if ((internal->vSlots[slot_handle]->tcIndex < 0) ||
	(internal->vSlots[slot_handle]->pid == 0xffff))
  {
       printk("error pid not set, can't clear\n");
       return -1;
  }

  /* Now reallocate the PIDs in the DMA table.
     Move the last PID to the TC index to be cleared (shift operation
     in the original code disturbs other PIDs). The sleep operation seems
     to be necessary because without it the transport stream is disturbed. */
  PtiWrite(&internal->pidtable[internal->vSlots[slot_handle]->tcIndex],
		internal->pidtable[*internal->num_pids - 1]);
  msleep(10);
  PtiWrite(internal->num_pids , *internal->num_pids - 1);

  /* ... and update id information */
  for(i = 0; i < 32; i++)
  {
    /* find the slot using the moved TC index */
    if(*internal->num_pids == internal->vSlots[i]->tcIndex)
    {
      internal->vSlots[i]->tcIndex = internal->vSlots[slot_handle]->tcIndex;
      break;
    }
  }

  internal->vSlots[slot_handle]->tcIndex = -1;

  internal->err_count = 0;

  return 0;
}

int pti_hal_slot_free ( int session_handle, int slot_handle )
{
  printk("%s %d\n", __FUNCTION__, slot_handle);

  if(internal->vSlots[slot_handle - SLOT_HANDLE_OFFSET]->tcIndex != -1)
    pti_hal_slot_clear_pid (session_handle, slot_handle );

  slot_handle -= SLOT_HANDLE_OFFSET;

  internal->vSlots[slot_handle]->inUse = 0;
  internal->vSlots[slot_handle]->pid = 0xffff;
  internal->vSlots[slot_handle]->tcIndex = -1;

  internal->err_count = 0;

  return 0;
}

int pti_hal_slot_unlink_buffer ( int session_handle, int slot_handle)
{
        return 0;
}

int pti_hal_slot_link_buffer ( int session_handle, int slot_handle,
			       int bufType)
{
        return 0;
}


int pti_hal_slot_set_pid ( int session_handle, int slot_handle, u16 pid )
{
  int vLoop;

  printk("%s: %d %d %d\n", __FUNCTION__, session_handle, slot_handle, pid);

  for (vLoop = 0; vLoop < (*internal->num_pids); vLoop++)
  {
	  if ((unsigned short) internal->pidtable[vLoop] == (unsigned short) pid)
	  {
		 printk("pid %d already collecting. ignoring ... \n", pid);
		 return -1;
	  }
  }

  for (vLoop = 0; vLoop < 32; vLoop++)
  {
     if (internal->vSlots[vLoop]->Handle == slot_handle)
     {
	if ( internal->vSlots[vLoop]->inUse == 1 )
	{
	   internal->vSlots[vLoop]->pid = pid;
	   internal->vSlots[vLoop]->tcIndex = *internal->num_pids;

	   printk("%s ok (pid %d, tc = %d)\n", __FUNCTION__, pid, *internal->num_pids);

	   PtiWrite(&internal->pidtable[*internal->num_pids], pid);
	   msleep(10);
	   PtiWrite(internal->num_pids,*internal->num_pids + 1);

           return 0;
	} else
	{
	   printk("%s failed1\n", __FUNCTION__);
	   return -1;
        }
     }
  }

  printk("%s failed2\n", __FUNCTION__);
  return -1;

}

int pti_hal_get_new_slot_handle ( int session_handle, int dvb_type,
				  int dvb_pes_type, struct dvb_demux *demux,
				  struct StreamContext_s *DemuxStream,
				  struct DeviceContext_s *DeviceContext )
{
  int vLoopSlots;

  for ( vLoopSlots = 0; vLoopSlots < 32; vLoopSlots++ )
  {
	if ( internal->vSlots[vLoopSlots]->inUse == 0 )
	{
          internal->vSlots[vLoopSlots]->inUse = 1;
          internal->vSlots[vLoopSlots]->tcIndex = -1;

	  printk("ret slothandle = %d\n", internal->vSlots[vLoopSlots]->Handle);

          return internal->vSlots[vLoopSlots]->Handle;
	}
  }

  return -1;
}
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,17)
int pti_hal_set_source(int session_handle, const int source)
//FIXME int pti_hal_set_source ( int session_handle, const dmx_source_t source )
#else
int pti_hal_set_source ( int session_handle, const dmx_source_t source )
#endif
{
		printk("source %d, session_handle %d\n", source, session_handle);
		#if defined(SPARK7162)
		{
			int i;
			int old_session;
			int old_source = -1;
			old_session = internal->demux_tag[source];
			printk("before\n");
			for (i = 0; i < TAG_COUNT; i++)
			{
				printk("internal->demux[%d] = 0x%x\n",
						i, (int)internal->demux[i]);
				printk("internal->demux_tag[%d] = %d\n",
						i, internal->demux_tag[i]);
			}
			if (old_session == session_handle)
			{
        		return 1;
			}
			for (i = 0; i < TAG_COUNT; i++)
			{
			    if (internal->demux[i])
			    {
			        if (internal->demux_tag[i] == session_handle)
			        {
			            old_source = i;
			        }
			    }
			}
			internal->demux_tag[source] = session_handle;
			if (old_source > -1)
			{
				internal->demux_tag[old_source] = old_session;
			}

			printk("after\n");
			for (i = 0; i < TAG_COUNT; i++)
			{
				printk("internal->demux[%d] = 0x%x\n",
						i, (int)internal->demux[i]);
				printk("internal->demux_tag[%d] = %d\n",
						i, internal->demux_tag[i]);
			}
		}
		#endif
        return 1;
}

int pti_hal_get_session_handle ( int tc_session_number )
{
	return 1;
}

int pti_hal_get_new_session_handle ( int source, struct dvb_demux * demux )
{
	if((source >= 0) && (source < TAG_COUNT))
	{
		printk("session %d, demux %p\n", source, demux);
		internal->demux[source] = demux;
		#if defined(SPARK7162)
		internal->demux_tag[source] = source;
		#endif
	}

	return source;
}

void paceSwtsByPti(void)
{
}

EXPORT_SYMBOL(paceSwtsByPti);

EXPORT_SYMBOL(pti_hal_descrambler_set);
EXPORT_SYMBOL(pti_hal_descrambler_set_aes);
EXPORT_SYMBOL(pti_hal_descrambler_unlink);
EXPORT_SYMBOL(pti_hal_descrambler_link);
EXPORT_SYMBOL(pti_hal_get_new_descrambler);
EXPORT_SYMBOL(pti_hal_slot_free);
EXPORT_SYMBOL(pti_hal_slot_unlink_buffer);
EXPORT_SYMBOL(pti_hal_slot_link_buffer);
EXPORT_SYMBOL(pti_hal_slot_clear_pid);
EXPORT_SYMBOL(pti_hal_slot_set_pid);
EXPORT_SYMBOL(pti_hal_get_new_slot_handle);
EXPORT_SYMBOL(pti_hal_set_source);
EXPORT_SYMBOL(pti_hal_get_session_handle);
EXPORT_SYMBOL(pti_hal_get_new_session_handle);
EXPORT_SYMBOL(pti_hal_init);

int __init pti_init(void)
{
   printk("pti loaded\n");
   return 0;
}

static void __exit pti_exit(void)
{
   printk("pti unloaded\n");
}

module_init             (pti_init);
module_exit             (pti_exit);

#if defined(PLAYER_179) || defined(PLAYER_191)
#if (defined(HL101) || defined(VIP1_V2) || defined(VIP2_V1) || defined(SPARK) || defined(SPARK7162)) || defined(SAGEMCOM88)
module_param(waitMS, int, 0444);
MODULE_PARM_DESC(waitMS, "waitMS");

module_param(videoMem, int, 0444);
MODULE_PARM_DESC(videoMem, "videoMem\n");
#endif
#elif defined(PLAYER_131)
#else
#endif

MODULE_AUTHOR("Peter Bennett <peter.bennett@st.com>; adapted by TDT");
MODULE_DESCRIPTION("STPTI DVB Driver");
MODULE_LICENSE("GPL");
