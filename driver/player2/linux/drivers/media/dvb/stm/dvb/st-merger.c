/*
* STM TSMerger driver
*
* Copyright (c) STMicroelectronics 2006
*
* Author:Peter Bennett <peter.bennett@st.com>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; either version 2 of
* the License, or (at your option) any later version.
*/

#include <linux/version.h>
#include <linux/slab.h>
#include <linux/pagemap.h>
#include <linux/dma-mapping.h>
#include <linux/stm/stm-dma.h>
#include <asm/io.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,23)
#include <linux/stpio.h>
#else
#include <linux/stm/pio.h>
#endif
#include <linux/stm/stm-frontend.h>
#include "tsmerger.h"
#include "st-common.h"
#include "dvb_module.h"

#include <linux/delay.h>

//__TDT__: many modifications in this file
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,17)
#include "../../../../../../../pti/pti_hal.h"
#endif

#ifndef TSM_NUM_PTI_ALT_OUT
unsigned long TSM_NUM_PTI_ALT_OUT;
unsigned long TSM_NUM_1394_ALT_OUT;
#define LOAD_TSM_DATA
#endif

// CamRouting ufs910
static short camRouting = 0;
module_param(camRouting, short, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(camRouting, "Enable camRouting 0=disabled 1=enabled");

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
 || defined(SAGEMCOM88) \
 || defined(PACE7241)
#define TSMergerBaseAddress 0xFE242000
#define SWTS_BASE_ADDRESS 0xFE900000 //STi7105 STi7111
#else
#define TSMergerBaseAddress 0x19242000
#define SWTS_BASE_ADDRESS 0x1A300000
#endif

#define TSM_STREAM0_CFG 0x0000
#define TSM_STREAM1_CFG 0x0020
#define TSM_STREAM2_CFG 0x0040
#define TSM_STREAM3_CFG 0x0060
#define TSM_STREAM4_CFG 0x0080

/* for all 7109er */
#define TSM_STREAM5_CFG 0x00a0
#define TSM_STREAM6_CFG 0x00c0

/* see proc entry from ufs912 */
#define TSM_STREAM7_CFG 0x00e0
#define TSM_STREAM7_SYNC 0x00e8
#define TSM_STREAM7_STAT 0x00f0

/* 7111er */
#define TSM_STREAM0_CFG2 0x0018
#define TSM_STREAM1_CFG2 0x0038
#define TSM_STREAM2_CFG2 0x0058
#define TSM_STREAM3_CFG2 0x0078
#define TSM_STREAM4_CFG2 0x0098
#define TSM_STREAM5_CFG2 0x00b8
#define TSM_STREAM6_CFG2 0x00d8
#define TSM_STREAM7_CFG2 0x00f8

#define TSM_STREAM0_SYNC 0x0008
#define TSM_STREAM1_SYNC 0x0028
#define TSM_STREAM2_SYNC 0x0048
#define TSM_STREAM3_SYNC 0x0068
#define TSM_STREAM4_SYNC 0x0088

/* for all 7109er */
#define TSM_STREAM5_SYNC 0x00a8
#define TSM_STREAM6_SYNC 0x00c8

#define TSM_STREAM0_STA 0x0010
#define TSM_STREAM1_STA 0x0030
#define TSM_STREAM2_STA 0x0050
#define TSM_STREAM3_STA 0x0070
#define TSM_STREAM4_STA 0x0090

#define TSM_PTI_DEST 0x0200
#define TSM_PTI_SEL TSM_PTI_DEST

#define TSM_PTI1_DEST 0x0210
#define TSM_PTI1_SEL TSM_PTI1_DEST

#define TSM_1394_DEST 0x0210
#define TSM_1394_SEL TSM_1394_DEST

#define TSM_PROG_CNT0 0x0400
#define TSM_PROG_CNT1 0x0410

#define SWTS_CFG(x) (0x0600 + (x*0x10))

#define PTI_ALT_OUT_CFG 0x0800
#define TS_1394_CFG 0x0810
#define TSM_SYS_CFG 0x0820
#define TSM_SW_RST 0x0830

#define TSM_SWTS 0x010BE000

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
 || defined(SAGEMCOM88) \
 || defined(PACE7241)
#define SysConfigBaseAddress 0xFE001000
#else
#define SysConfigBaseAddress 0x19001000
#endif

#define SYS_CFG0 0x100
#define SYS_CFG1 0x104
#define SYS_CFG5 0x114

#define SYS_CFG7 0x11C

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)
unsigned long tsm_io = 0;
#else
void *tsm_io = NULL;
#endif

#if defined(ADB_BOX)
extern int TsinMode;
enum
{
	SERIAL,
	PARALLEL,
};
#endif

extern int highSR;

extern void paceSwtsByPti(void);

/* ****************************************
 * Dagobert:
 * Taken from new player - frontend
 * ****************************************
 */

#define SWTS_FDMA_ALIGNMENT 127

/* Maximum number of pages we send in 1 FDMA transfer */
#define MAX_SWTS_PAGES 260

struct stm_tsm_handle
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)
	unsigned long tsm_io;
#else
	void *tsm_io;
#endif
	unsigned long tsm_swts;

	int swts_channel;

	/* FDMA for SWTS */
	int fdma_channel;
	struct stm_dma_params fdma_params;
	unsigned long fdma_reqline;
	struct stm_dma_req *fdma_req;

	struct page *swts_pages[MAX_SWTS_PAGES];
	struct stm_dma_params swts_params[MAX_SWTS_PAGES];
	struct scatterlist swts_sg[MAX_SWTS_PAGES];
};

static struct stm_tsm_handle tsm_handle;

struct stm_dma_req_config fdma_req_config =
{
	.rw = REQ_CONFIG_WRITE,
	.opcode = REQ_CONFIG_OPCODE_32,
	.count = 4,
	.increment = 0,
	.hold_off = 0,
	.initiator = 0,
};

static const char *fdmac_id[] = { STM_DMAC_ID, NULL };
static const char *fdma_cap_hb[] = { STM_DMA_CAP_HIGH_BW, NULL };

/* >>> DVBT-USB
 Step2-Driver injects data to SWTS here
 When properly registered, driver should be visible in list of frontends,
 if you get error just after start of manual scan, you have neighter correct frontendX defined nor incorrect tsm definitions in this file */
#if defined(ADB_BOX) \
 || defined(SAGEMCOM88) \
 || defined(ARIVALINK200) \
 || defined(SPARK7162) \
 || defined(IPBOX9900) \
 || defined(IPBOX99) \
 || defined(IPBOX55) \
 || defined(HL101)
//injecting stream from DVB-T USB driver to SWTS
void extern_inject_data(u32 *data, off_t size)
{
	int blocks = (size + 127) / 128;
	int count = size;
	int words;
	u32 *p = data;
	int n;
	int m;
	u32 *addr = (u32 *)tsm_handle.tsm_swts;
	//paceSwtsByPti();
	//dprintk("%s > size = %d, block %d\n", __FUNCTION__, (int) size, blocks);
	for (n = 0; n < blocks; n++)
	{
		while (!(readl(tsm_handle.tsm_io + SWTS_CFG(0)) & TSM_SWTS_REQ))
		{
			udelay(0);
		}
		if (count > 128)
			words = 128 / 4;
		else
			words = count / 4;
		count -= words * 4;
		for (m = 0; m < words; m++)
			*addr = *p++;
	}
}
EXPORT_SYMBOL(extern_inject_data);

#endif
/* <<< DVBT-USB */

void stm_tsm_inject_data(struct stm_tsm_handle *handle, u32 *data, off_t size)
{
	int blocks = (size + 127) / 128;
	int count = size;
	int words;
	u32 *p = data;
	int n;
	int m;
	u32 *addr = (u32 *)handle->tsm_swts;
	dprintk("%s > size = %d, block %d\n", __FUNCTION__, (int) size, blocks);
	for (n = 0; n < blocks; n++)
	{
		while (!(readl(handle->tsm_io + SWTS_CFG(0)) & TSM_SWTS_REQ))
		{
			printk("%s: Waiting for FIFO %x\n", __FUNCTION__, readl(handle->tsm_io));
			msleep(10);
		}
		if (count > 128)
			words = 128 / 4;
		else
			words = count / 4;
		count -= words * 4;
		for (m = 0; m < words; m++)
			*addr = *p++;
	}
	dprintk("%s < \n", __FUNCTION__);
}

int stm_tsm_inject_user_data(const char __user *data, off_t size)
{
	int nr_pages;
	int ret = 0;
	int page_offset;
	int remaining;
	int n;
	struct stm_dma_params *in_param;
	struct scatterlist *curr_sg;
	int sg_count;
	unsigned long start = (unsigned long)data;
	unsigned long len = size;
	unsigned long taddr, tlen;
	int extra = 0;
	struct stm_tsm_handle *handle = &tsm_handle;
	dprintk("%s > size = %d\n", __FUNCTION__, (int) size);
	dprintk("status: 0x%08x", readl(tsm_io + TSM_STREAM3_STA));
	paceSwtsByPti();
	if (start & SWTS_FDMA_ALIGNMENT)
	{
		int hand = (SWTS_FDMA_ALIGNMENT + 1) - (start & SWTS_FDMA_ALIGNMENT);
		// we need a copy to user...
		dprintk("inject 1\n");
		stm_tsm_inject_data(handle, (u32 *)data, hand);
		start += hand;
		len -= hand;
	}
	if (len & SWTS_FDMA_ALIGNMENT)
	{
		extra = len & SWTS_FDMA_ALIGNMENT;
		len = len & ~SWTS_FDMA_ALIGNMENT;
	}
	nr_pages = (PAGE_ALIGN(start + len) - (start & PAGE_MASK)) >> PAGE_SHIFT;
	down_read(&current->mm->mmap_sem);
	ret = get_user_pages(current, current->mm, start, nr_pages, READ, 0, handle->swts_pages, NULL);
	up_read(&current->mm->mmap_sem);
	if (ret < nr_pages)
	{
		nr_pages = ret;
		ret = -EINVAL;
		dprintk("ret = %d < nr_pages %d\n", ret, nr_pages);
		goto out_unmap;
	}
	page_offset = start & (PAGE_SIZE - 1);
	remaining = len;
	for (n = 0; n < nr_pages; n++)
	{
		int copy = min_t(int, PAGE_SIZE - page_offset, remaining);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
		handle->swts_sg[n].page = handle->swts_pages[n];
#else
		handle->swts_sg[n].page_link = (unsigned long) handle->swts_pages[n];
#endif
		handle->swts_sg[n].offset = page_offset;
		handle->swts_sg[n].length = copy;
		page_offset = 0;
		remaining -= copy;
//dprintk("nr_pages %d, length %d, remaining %d\n", nr_pages, copy, remaining);
	}
	sg_count = dma_map_sg(NULL, &handle->swts_sg[0], nr_pages, DMA_TO_DEVICE);
	in_param = &handle->swts_params[0];
	curr_sg = &handle->swts_sg[0];
	/* Go through the list and unscatter it */
	taddr = sg_dma_address(curr_sg);
	tlen = sg_dma_len(curr_sg);
	for (n = 0; n < sg_count; n++)
	{
		unsigned long naddr, nlen;
		dma_params_addrs(in_param, taddr, 0x1a300000, tlen);
		dma_params_link(in_param, in_param + 1);
		curr_sg++;
		naddr = sg_dma_address(curr_sg);
		nlen = sg_dma_len(curr_sg);
		if (taddr + tlen == naddr && ((n + 1) != sg_count))
		{
			tlen += nlen;
		}
		else
		{
			taddr = naddr;
			tlen = nlen;
			in_param++;
		}
	}
	in_param--;
	dma_params_link(in_param, NULL);
	dma_compile_list(handle->fdma_channel, &handle->swts_params[0], GFP_ATOMIC);
	ret = dma_xfer_list(handle->fdma_channel, &handle->swts_params[0]);
	if (ret)
	{
		printk("xfer ret = %d\n", ret);
		goto out_unmap;
	}
	dma_wait_for_completion(handle->fdma_channel);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)
	dma_unmap_sg(NULL, sg, nr_pages, DMA_TO_DEVICE);
#else
	/* #warning FIXME, STM24, skipping dma_unmap_sg */
	/* dma_unmap_sg(NULL, sg, nr_pages, DMA_TO_DEVICE); */
#endif
	if (extra)
	{
		dprintk("inject 2\n");
		stm_tsm_inject_data(handle, (u32 *)(data + size - extra), extra);
	}
out_unmap:
	for (n = 0; n < nr_pages; n++)
		page_cache_release(handle->swts_pages[n]);
	dprintk("%s < ret = %d\n", __FUNCTION__, ret);
	return ret;
}

EXPORT_SYMBOL(stm_tsm_inject_user_data);

/* ****************************************
 * END: Taken from new player - frontend
 * ****************************************
 */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)
static unsigned long reg_sys_config = 0;
#else
static void *reg_sys_config = NULL;
#endif

#if defined(SPARK)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)

void spark_stm_tsm_init(void)
{
	unsigned int ret;
	unsigned int stream_sync = 0xbc4722;
	int n;
	/* ugly hack: the TSM sometimes seems to stop working, a
	* reset of the config registers fixes it
	* but the DMA stuff must not be touched or everything
	* blows up badly */
	int reinit = 0;
	if (tsm_io)
		reinit = 1;
	if (reinit)
	{
		printk("[Spark] reinit stream routing...\n");
	}
	else
	{
		printk("[Spark] Init Stream Routing...\n");
		/* first configure sysconfig */
		reg_sys_config = ioremap(SysConfigBaseAddress, 0x1000);
	}
	/*
	 ->TSIN0 routed to TSIN2
	 ->TSMerger TSIN2 receives TSIN0 (based on config before)
	 ->TS interface is as indicated by TSMerger configuration bits
	 */
	ctrl_outl(0x3, reg_sys_config + SYS_CFG0);
	ctrl_outl(0x0, reg_sys_config + SYS_CFG1);
	if (reinit)
	{
		/* this seems to shorten the window for the race condition,
		 * however I don't believe it is enough to be really safe.
		 * but it works for now. */
		dma_wait_for_completion(tsm_handle.fdma_channel);
	}
	else
	{
		/* set up tsmerger */
		tsm_handle.tsm_io = tsm_io = ioremap(TSMergerBaseAddress, 0x0900);
	}
	/* 1. Reset */
	ctrl_outl(0x0, tsm_io + TSM_SW_RST);
	ctrl_outl(0x6, tsm_io + TSM_SW_RST);
	/* set all registers to a defined state */
	ctrl_outl(0x0, tsm_io + TSM_STREAM0_CFG);
	ctrl_outl(0x0, tsm_io + TSM_STREAM0_SYNC);
	ctrl_outl(0x0, tsm_io + TSM_STREAM0_STA);
	ctrl_outl(0x0, tsm_io + 0x18 /* reserved ??? */);
	ctrl_outl(0x0, tsm_io + TSM_STREAM1_CFG);
	ctrl_outl(0x0, tsm_io + TSM_STREAM1_SYNC);
	ctrl_outl(0x0, tsm_io + TSM_STREAM1_STA);
	ctrl_outl(0x0, tsm_io + 0x38 /* reserved ??? */);
	ctrl_outl(0x0, tsm_io + TSM_STREAM2_CFG);
	ctrl_outl(0x0, tsm_io + TSM_STREAM2_SYNC);
	ctrl_outl(0x0, tsm_io + TSM_STREAM2_STA);
	ctrl_outl(0x0, tsm_io + 0x58 /* reserved ??? */);
	ctrl_outl(0x0, tsm_io + TSM_STREAM3_CFG);
	ctrl_outl(0x0, tsm_io + TSM_STREAM3_SYNC);
	ctrl_outl(0x0, tsm_io + TSM_STREAM3_STA);
	ctrl_outl(0x0, tsm_io + 0x78 /* reserved ??? */);
	ctrl_outl(0x0, tsm_io + TSM_STREAM4_CFG);
	ctrl_outl(0x0, tsm_io + TSM_STREAM4_SYNC);
	ctrl_outl(0x0, tsm_io + TSM_STREAM4_STA);
	ctrl_outl(0x0, tsm_io + 0x98 /* reserved ??? */);
	ctrl_outl(0x0, tsm_io + TSM_PROG_CNT0);
	ctrl_outl(0x0, tsm_io + TSM_PROG_CNT1);
	ctrl_outl(0x0, tsm_io + TSM_PTI_SEL);
	ctrl_outl(0x0, tsm_io + TSM_1394_SEL);
	ctrl_outl(0x0, tsm_io + PTI_ALT_OUT_CFG);
	ctrl_outl(0x0, tsm_io + TS_1394_CFG);
	ctrl_outl(0x0, tsm_io + SWTS_CFG(0));
	ctrl_outl(0x0, tsm_io + TSM_SYS_CFG);
	ctrl_outl(0x0, tsm_io + TSM_SYS_CFG); /* 2 times ? */
	/* RAM partitioning of streams */
	/* we use a little more ram for the tsm stream because
	 * of overflow of tsm in case of hight cpu load on
	 * sky sport hd live events. this leads to loss of
	 * some ts packets!
	 */
	/* RAM partitioning of streams */
	ctrl_outl(0x0, tsm_io + TSM_STREAM0_CFG); //448kb (8*64)
	ctrl_outl(0x500, tsm_io + TSM_STREAM1_CFG); //448kb (6*64)
	ctrl_outl(0xe00, tsm_io + TSM_STREAM2_CFG); //384kb (8*64)
	ctrl_outl(0x1600, tsm_io + TSM_STREAM3_CFG); //384kb (6*64)
	ctrl_outl(0x1a00, tsm_io + TSM_STREAM4_CFG); //320kb (5*64)
	ctrl_outl(0x1d00, tsm_io + TSM_STREAM5_CFG);
	ctrl_outl(0x1e00, tsm_io + TSM_STREAM6_CFG);
	/* I think this is a fault value !!! 0x1f00 is maximum but
	 * this is the lower address. nevertheless, stream7 not needed
	 */
	ctrl_outl(0x1f00, tsm_io + TSM_STREAM7_CFG);
	ctrl_outl(0x0, tsm_io + TSM_STREAM0_CFG2);
	ctrl_outl(0x0, tsm_io + TSM_STREAM1_CFG2);
	ctrl_outl(0x0, tsm_io + TSM_STREAM2_CFG2);
	ctrl_outl(0x0, tsm_io + TSM_STREAM3_CFG2);
	ctrl_outl(0x0, tsm_io + TSM_STREAM4_CFG2);
	ctrl_outl(0x0, tsm_io + TSM_STREAM5_CFG2);
	ctrl_outl(0x0, tsm_io + TSM_STREAM6_CFG2);
	ctrl_outl(0x0, tsm_io + TSM_STREAM7_CFG2);
	/* configure streams: */
	/* add tag bytes to stream + stream priority */
	ret = ctrl_inl(tsm_io + TSM_STREAM0_CFG);
	ctrl_outl(ret | (0x20020), tsm_io + TSM_STREAM0_CFG);
	ctrl_outl(stream_sync, tsm_io + TSM_STREAM0_SYNC);
	ctrl_outl(0x0, tsm_io + 0x18 /* reserved ??? */);
	/* add tag bytes to stream + stream priority */
	ret = ctrl_inl(tsm_io + TSM_STREAM1_CFG);
	ctrl_outl(ret | (0x20020), tsm_io + TSM_STREAM1_CFG);
	ctrl_outl(stream_sync, tsm_io + TSM_STREAM1_SYNC);
	ctrl_outl(0x0, tsm_io + 0x38 /* reserved ??? */);
	/* add tag bytes to stream + stream priority */
	ret = ctrl_inl(tsm_io + TSM_STREAM2_CFG);
	ctrl_outl(ret | (0x20020), tsm_io + TSM_STREAM2_CFG);
	ctrl_outl(stream_sync, tsm_io + TSM_STREAM2_SYNC);
	ctrl_outl(0x0, tsm_io + 0x58 /* reserved ??? */);
	/* add tag bytes to stream + stream priority */
	ret = ctrl_inl(tsm_io + TSM_STREAM3_CFG);
	ctrl_outl(ret | (0x20020), tsm_io + TSM_STREAM3_CFG);
	ret = ctrl_inl(tsm_io + TSM_STREAM4_CFG);
	ctrl_outl(ret | (0x20020), tsm_io + TSM_STREAM4_CFG);
	ctrl_outl(0x00, tsm_io + TSM_STREAM5_SYNC);
	ctrl_outl(0x00, tsm_io + TSM_STREAM6_SYNC);
	ctrl_outl(stream_sync, tsm_io + TSM_STREAM3_SYNC);
	ctrl_outl(0x0, tsm_io + 0x78 /* reserved ??? */);
	ctrl_outl(0x8f0000e, tsm_io + SWTS_CFG(0));
	ctrl_outl(0x8000000, tsm_io + SWTS_CFG(1));
	ctrl_outl(0x8000000, tsm_io + SWTS_CFG(2));
	/* auto count */
	ctrl_outl(0x0, tsm_io + TSM_PROG_CNT0);
	ctrl_outl(0x15, tsm_io + TSM_PTI_SEL);
	/* set stream 2 on */
	ret = ctrl_inl(tsm_io + TSM_STREAM2_CFG);
	ctrl_outl(ret | 0x80, tsm_io + TSM_STREAM2_CFG);
	/* set stream 4 on */
	ret = ctrl_inl(tsm_io + TSM_STREAM4_CFG);
	ctrl_outl(ret | 0x80, tsm_io + TSM_STREAM4_CFG);
	/* set stream on */
	ret = ctrl_inl(tsm_io + TSM_STREAM0_CFG);
	ctrl_outl(ret | 0x80, tsm_io + TSM_STREAM0_CFG);
	ctrl_outl(TSM_SWTS_REQ_TRIG(128 / 16) | 0x10, tsm_io + TSM_SWTS_CFG(0));
	/* SWTS0 to PTI */
	ret = ctrl_inl(tsm_io + TSM_PTI_SEL);
	ctrl_outl(ret | 0x8, tsm_io + TSM_PTI_SEL);
	ret = ctrl_inl(tsm_io + TSM_STREAM3_CFG);
	ctrl_outl((ret & TSM_RAM_ALLOC_START(0xff)) |
			  TSM_PRIORITY(0x7) | TSM_STREAM_ON | TSM_ADD_TAG_BYTES | TSM_SYNC_NOT_ASYNC | TSM_ASYNC_SOP_TOKEN, tsm_io + TSM_STREAM3_CFG);
	/* don't touch the DMA engine -- seems unnecessary on reinit */
	if (reinit)
		return;
	tsm_handle.swts_channel = 3;
	tsm_handle.tsm_swts = (unsigned long)ioremap(0x1A300000, 0x1000);
	/* Now lets get the SWTS info and setup an FDMA channel */
	tsm_handle.fdma_reqline = 31;
	tsm_handle.fdma_channel = request_dma_bycap(fdmac_id, fdma_cap_hb, "swts0");
	tsm_handle.fdma_req = dma_req_config(tsm_handle.fdma_channel, tsm_handle.fdma_reqline, &fdma_req_config);
	/* Initilise the parameters for the FDMA SWTS data injection */
	for (n = 0; n < MAX_SWTS_PAGES; n++)
	{
		dma_params_init(&tsm_handle.swts_params[n], MODE_PACED, STM_DMA_LIST_OPEN);
		dma_params_DIM_1_x_0(&tsm_handle.swts_params[n]);
		dma_params_req(&tsm_handle.swts_params[n], tsm_handle.fdma_req);
	}
}
#endif
#endif

void stm_tsm_init(int use_cimax)
{
	unsigned int ret;
	int n;
	int reinit = 0;
#if !defined(FORTIS_HDBOX) \
 && !defined(UFS912) \
 && !defined(UFS913) \
 && !defined(SPARK) \
 && !defined(OCTAGON1008) \
 && !defined(ATEVIO7500) \
 && !defined(HS7110) \
 && !defined(HS7810A) \
 && !defined(HS7420) \
 && !defined(HS7429) \
 && !defined(HS7119) \
 && !defined(HS7819) \
 && !defined(ATEMIO520) \
 && !defined(ATEMIO530) \
 && !defined(CUBEREVO) \
 && !defined(CUBEREVO_MINI2) \
 && !defined(CUBEREVO_MINI) \
 && !defined(CUBEREVO_250HD) \
 && !defined(CUBEREVO_2000HD) \
 && !defined(CUBEREVO_9500HD) \
 && !defined(CUBEREVO_MINI_FTA) \
 && !defined(CUBEREVO_3000HD) \
 && !defined(VITAMIN_HD5000)
	unsigned int stream_sync = 0xbc4733;
#else
	unsigned int stream_sync = 0xbc4722;
#endif
#if defined(SPARK)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
	spark_stm_tsm_init();
	return;
#endif
#endif
#if defined(VIP2_V1) \
 || defined(SPARK) \
 || defined(SPARK7162) \
 || defined(IPBOX99) \
 || defined(IPBOX55) \
 || defined(ADB_BOX) \
 || defined(CUBEREVO_2000HD) \
 || defined(SAGEMCOM88) \
 || defined(PACE7241) // none ci targets
	use_cimax = 0;
#endif
	/* first configure sysconfig */
	/* ugly hack: the TSM sometimes seems to stop working, a
	 * reset of the config registers fixes it
	 * but the DMA stuff must not be touched or everything
	 * blows up badly
	 */
	if (tsm_io)
		reinit = 1;
	if (reinit)
	{
		printk("[TSM] reinit stream routing...\n");
	}
	else
	{
		printk("[TSM] init stream routing...\n");
		reg_sys_config = ioremap(SysConfigBaseAddress, 0x0900);
	}
	if (use_cimax != 0)
	{
		/* route tsmerger to cimax and then to pti */
#if defined(UFS912) \
 || defined(HS7420) \
 || defined(HS7429) \
 || defined(HS7810A) \
 || defined(HS7819)
		if (!reinit)
		{
			struct stpio *stream1_pin = stpio_request_pin(5, 0, "TSinterface1", STPIO_IN);
			struct stpio *stream2_pin = stpio_request_pin(5, 3, "TSinterface2", STPIO_IN);
		}
		else
			printk("[TSM] skip stpio stuff in reinit\n");
#elif defined(ATEVIO7500)
		if (!reinit)
		{
			struct stpio_pin *pin;
			/* set the muxer pin otherwise startci output will
			 * not properly passed to tsmerger.
			 */
			pin = stpio_request_pin(5, 3, "tuner2_mux", STPIO_OUT);
			stpio_set_pin(pin, 0);
			pin = stpio_request_pin(5, 4, "tuner2_mux", STPIO_OUT);
			stpio_set_pin(pin, 0);
		}
		else
			printk("[TSM] skip stpio stuff in reinit\n");
#elif defined(OCTAGON1008)
		if (!reinit)
		{
			struct stpio *stream2_pin = stpio_request_pin(1, 3, "STREAM2", STPIO_OUT);
			/* disbaled on 1 */
			stpio_set_pin(stream2_pin, 0);
		}
		else
			printk("[TSM] skip stpio stuff in reinit\n");
#elif defined(UFS913)
		if (!reinit)
		{
			struct stpio *pin = stpio_request_pin(12, 0, "ts", STPIO_ALT_OUT);
			pin = stpio_request_pin(12, 1, "ts", STPIO_ALT_OUT);
			pin = stpio_request_pin(12, 2, "ts", STPIO_ALT_OUT);
			pin = stpio_request_pin(12, 3, "ts", STPIO_ALT_OUT);
			pin = stpio_request_pin(12, 4, "ts", STPIO_ALT_OUT);
			pin = stpio_request_pin(12, 5, "ts", STPIO_ALT_OUT);
			pin = stpio_request_pin(12, 6, "ts", STPIO_ALT_OUT);
			pin = stpio_request_pin(12, 7, "ts", STPIO_ALT_OUT);
			pin = stpio_request_pin(13, 0, "ts", STPIO_ALT_OUT);
			pin = stpio_request_pin(13, 1, "ts", STPIO_ALT_OUT);
			pin = stpio_request_pin(13, 2, "ts", STPIO_ALT_OUT);
			pin = stpio_request_pin(13, 3, "ts", STPIO_ALT_OUT);
			/* ??? */
			pin = stpio_request_pin(1, 1, "ts", STPIO_OUT);
			pin = stpio_request_pin(1, 2, "ts", STPIO_OUT);
			pin = stpio_request_pin(1, 3, "ts", STPIO_OUT);
			ctrl_outl(0xff, reg_sys_config + 0x1c0); /* sys_cfg48 */
			ctrl_outl(0xf, reg_sys_config + 0x1c4); /* sys_cfg49 */
		}
#endif
		/*
		 * 0xbc4733
		 * sync = 3
		 * drop = 3
		 * soap toker = 0x47
		 * packet len = 188
		 */
		/* Streamconfig + jeweils ram start s.u.
		 * 0x20020 =
		 * add_tag_bytes = 1
		 * ram = 0
		 * pri = 2 (binaer 10)

		 * 0x30020 =
		 * add_tag_bytes = 1
		 * ram = 0
		 * pri = 3 (binaer 11)
		 */
		printk("Routing streams through cimax\n");
#if defined(OCTAGON1008)
		/* smartcard settings */
		ret = ctrl_inl(reg_sys_config + SYS_CFG7);
		ctrl_outl(ret | 0x1b0, reg_sys_config + SYS_CFG7);
#endif
		/*
		 * ->TSIN0 routed to TSIN2
		 * ->TSMerger TSIN2 receives TSIN0 (based on config before)
		 * ->TS interface is as indicated by TSMerger configuration bits
		 */
#if defined(UFS910)
		/*
		 * The UFS910 hardware requires the following connections:
		 * ->TSIN1 routed to TSIN2
		 * ->TSMerger TSIN2 receives TSIN1 (based on config before)
		 *->TS interface is as indicated by TSMerger configuration bits
		 */
		/* from fw 202 rc ->see also pti */
		if (camRouting == 1) // CamRouting ufs910
			ctrl_outl(0x6, reg_sys_config + SYS_CFG0);
		else
			ctrl_outl(0x2, reg_sys_config + SYS_CFG0);
#elif defined(FORTIS_HDBOX)
		/* ->TSIN0 routes to TSIN2 */
		ctrl_outl(0x2, reg_sys_config + SYS_CFG0);
#elif defined(UFS912) \
 || defined(SPARK) \
 || defined(VITAMIN_HD5000)
		ctrl_outl(0x3, reg_sys_config + SYS_CFG0);
#elif defined(UFS913)
		ctrl_outl(0x10, reg_sys_config + SYS_CFG0);
#elif defined(ATEVIO7500)
		/* pio12 */
		ctrl_outl(0x0, 0xfe015020);
		ctrl_outl(0x0, 0xfe015030);
		ctrl_outl(0x0, 0xfe015040);
		/* pio13 */
		ctrl_outl(0x0, 0xfe016020);
		ctrl_outl(0x0, 0xfe016030);
		ctrl_outl(0x0, 0xfe016040);
		ctrl_outl(0x0, reg_sys_config + 0x1c0); /* sys_cfg48 */
		ctrl_outl(0x0, reg_sys_config + 0x1c4); /* sys_cfg49 */
		ctrl_outl(0x4, reg_sys_config + SYS_CFG0); /* tsin2 ->tsin0 mux */
#elif defined(OCTAGON1008)
		ctrl_outl(0x6, reg_sys_config + SYS_CFG0);
#else
		/*
		 * The TF7700 hardware requires the following connections:
		 * StarCI2Win TSO1 => TSIN0 => IN0 => OUT (to PTI)
		 * StarCI2Win TSO2 => TSIN1 => IN1 => OUT (to PTI)
		 * TS1394 (TSIN2) => StarCI2Win TSI3
		 */
		ctrl_outl(0x0, reg_sys_config + SYS_CFG0);
#endif
#if !defined(ATEVIO7500) \
 && !defined(UFS912) \
 && !defined(UFS913) \
 && !defined(HS7110) \
 && !defined(HS7810A) \
 && !defined(HS7420) \
 && !defined(HS7429) \
 && !defined(HS7119) \
 && !defined(HS7819) \
 && !defined(ATEMIO520) \
 && !defined(ATEMIO530) \
 && !defined(VITAMIN_HD5000)
		ctrl_outl(0x0, reg_sys_config + SYS_CFG1);
#endif
		if (reinit)
		{
			/* this seems to shorten the window for the race condition,
			 * however I don't believe it is enough to be really safe.
			 * but it works for now. */
			dma_wait_for_completion(tsm_handle.fdma_channel);
		}
		else
		{
			/* set up tsmerger */
			tsm_handle.tsm_io = tsm_io = ioremap(TSMergerBaseAddress, 0x0900);
		}
		/* 1. Reset */
		ctrl_outl(0x0, tsm_io + TSM_SW_RST);
		ctrl_outl(0x6, tsm_io + TSM_SW_RST);
		/* set all registers to a defined state */
		ctrl_outl(0x0, tsm_io + TSM_STREAM0_CFG);
		ctrl_outl(0x0, tsm_io + TSM_STREAM0_SYNC);
		ctrl_outl(0x0, tsm_io + TSM_STREAM0_STA);
		ctrl_outl(0x0, tsm_io + 0x18 /* reserved ??? */);
		ctrl_outl(0x0, tsm_io + TSM_STREAM1_CFG);
		ctrl_outl(0x0, tsm_io + TSM_STREAM1_SYNC);
		ctrl_outl(0x0, tsm_io + TSM_STREAM1_STA);
		ctrl_outl(0x0, tsm_io + 0x38 /* reserved ??? */);
		ctrl_outl(0x0, tsm_io + TSM_STREAM2_CFG);
		ctrl_outl(0x0, tsm_io + TSM_STREAM2_SYNC);
		ctrl_outl(0x0, tsm_io + TSM_STREAM2_STA);
		ctrl_outl(0x0, tsm_io + 0x58 /* reserved ??? */);
		ctrl_outl(0x0, tsm_io + TSM_STREAM3_CFG);
		ctrl_outl(0x0, tsm_io + TSM_STREAM3_SYNC);
		ctrl_outl(0x0, tsm_io + TSM_STREAM3_STA);
		ctrl_outl(0x0, tsm_io + 0x78 /* reserved ??? */);
		ctrl_outl(0x0, tsm_io + TSM_STREAM4_CFG);
		ctrl_outl(0x0, tsm_io + TSM_STREAM4_SYNC);
		ctrl_outl(0x0, tsm_io + TSM_STREAM4_STA);
		ctrl_outl(0x0, tsm_io + 0x98 /* reserved ??? */);
		ctrl_outl(0x0, tsm_io + TSM_PROG_CNT0);
		ctrl_outl(0x0, tsm_io + TSM_PROG_CNT1);
		ctrl_outl(0x0, tsm_io + TSM_PTI_SEL);
		ctrl_outl(0x0, tsm_io + TSM_1394_SEL);
		ctrl_outl(0x0, tsm_io + PTI_ALT_OUT_CFG);
		ctrl_outl(0x0, tsm_io + TS_1394_CFG);
		ctrl_outl(0x0, tsm_io + SWTS_CFG(0));
#if defined(FORTIS_HDBOX) \
 || defined(UFS922) \
 || defined(UFC960) \
 || defined(TF7700) \
 || defined(HL101) \
 || defined(VIP1_V2) \
 || defined(UFS912) \
 || defined(UFS913) \
 || defined(SPARK) \
 || defined(CUBEREVO) \
 || defined(CUBEREVO_MINI2) \
 || defined(CUBEREVO_MINI) \
 || defined(CUBEREVO_250HD) \
 || defined(CUBEREVO_2000HD) \
 || defined(CUBEREVO_9500HD) \
 || defined(CUBEREVO_MINI_FTA) \
 || defined(CUBEREVO_3000HD) \
 || defined(ATEVIO7500) \
 || defined(HS7110) \
 || defined(HS7810A) \
 || defined(HS7420) \
 || defined(HS7429) \
 || defined(HS7119) \
 || defined(HS7819) \
 || defined(ATEMIO520) \
 || defined(ATEMIO530) \
 || defined(IPBOX9900) \
 || defined(ARIVALINK200) \
 || defined(VITAMIN_HD5000)
		ctrl_outl(0x0, tsm_io + SWTS_CFG(1));
		ctrl_outl(0x0, tsm_io + SWTS_CFG(2));
#endif
		ctrl_outl(0x0, tsm_io + TSM_SYS_CFG);
		ctrl_outl(0x0, tsm_io + TSM_SYS_CFG); /* 2 times ? */
		/* RAM partitioning of streams max 1984kb (31*64) */
#if defined(FORTIS_HDBOX) \
 || defined(UFS922) \
 || defined(UFC960) \
 || defined(HL101) \
 || defined(VIP1_V2)
		ctrl_outl(0x0, tsm_io + TSM_STREAM0_CFG); //320kb (5*64)
		ctrl_outl(0x500, tsm_io + TSM_STREAM1_CFG); //320kb (5*64)
		ctrl_outl(0xa00, tsm_io + TSM_STREAM2_CFG); //256kb (4*64)
		ctrl_outl(0xe00, tsm_io + TSM_STREAM3_CFG); //320kb (5*64)
		ctrl_outl(0x1300, tsm_io + TSM_STREAM4_CFG); //256kb (4*64)
		ctrl_outl(0x1700, tsm_io + TSM_STREAM5_CFG); //192kb (3*64)
		ctrl_outl(0x1a00, tsm_io + TSM_STREAM6_CFG); //384kb (5*64)
#elif defined (UFS912) \
 || defined(HS7420) \
 || defined(HS7429) \
 || defined(HS7810A) \
 || defined(HS7819)
		/* RAM partitioning of streams */
		ctrl_outl(0x0, tsm_io + TSM_STREAM0_CFG); //448kb (8*64)
		ctrl_outl(0x500, tsm_io + TSM_STREAM1_CFG); //448kb (6*64)
		ctrl_outl(0xe00, tsm_io + TSM_STREAM2_CFG); //384kb (8*64)
		ctrl_outl(0x1600, tsm_io + TSM_STREAM3_CFG); //384kb (6*64)
		ctrl_outl(0x1a00, tsm_io + TSM_STREAM4_CFG); //320kb (5*64)
		ctrl_outl(0x1d00, tsm_io + TSM_STREAM5_CFG);
		ctrl_outl(0x1e00, tsm_io + TSM_STREAM6_CFG);
		/* I think this is a fault value !!! 0x1f00 is maximum but
		* this is the lower address. nevertheless, stream7 not needed
		*/
		ctrl_outl(0x1f00, tsm_io + TSM_STREAM7_CFG);
		ctrl_outl(0x0, tsm_io + TSM_STREAM0_CFG2);
		ctrl_outl(0x0, tsm_io + TSM_STREAM1_CFG2);
		ctrl_outl(0x0, tsm_io + TSM_STREAM2_CFG2);
		ctrl_outl(0x0, tsm_io + TSM_STREAM3_CFG2);
		ctrl_outl(0x0, tsm_io + TSM_STREAM4_CFG2);
		ctrl_outl(0x0, tsm_io + TSM_STREAM5_CFG2);
		ctrl_outl(0x0, tsm_io + TSM_STREAM6_CFG2);
		ctrl_outl(0x0, tsm_io + TSM_STREAM7_CFG2);
#elif defined(HS7110) \
 || defined(HS7119) \
 || defined(ATEMIO520) \
 || defined(ATEMIO530)
		/* RAM partitioning of streams */
		ctrl_outl(0x0, tsm_io + TSM_STREAM0_CFG); //448kb (8*64)
		ctrl_outl(0x800, tsm_io + TSM_STREAM1_CFG); //448kb (6*64)
		ctrl_outl(0xe00, tsm_io + TSM_STREAM2_CFG); //384kb (8*64)
		ctrl_outl(0x1400, tsm_io + TSM_STREAM3_CFG); //384kb (6*64)
		ctrl_outl(0x1a00, tsm_io + TSM_STREAM4_CFG); //320kb (5*64)
		ctrl_outl(0x1d00, tsm_io + TSM_STREAM5_CFG);
		ctrl_outl(0x1e00, tsm_io + TSM_STREAM6_CFG);
		/* I think this is a fault value !!! 0x1f00 is maximum but
		 * this is the lower address. nevertheless, stream7 not needed
		 */
		ctrl_outl(0x1f00, tsm_io + TSM_STREAM7_CFG);
		ctrl_outl(0x0, tsm_io + TSM_STREAM0_CFG2);
		ctrl_outl(0x0, tsm_io + TSM_STREAM1_CFG2);
		ctrl_outl(0x0, tsm_io + TSM_STREAM2_CFG2);
		ctrl_outl(0x0, tsm_io + TSM_STREAM3_CFG2);
		ctrl_outl(0x0, tsm_io + TSM_STREAM4_CFG2);
		ctrl_outl(0x0, tsm_io + TSM_STREAM5_CFG2);
		ctrl_outl(0x0, tsm_io + TSM_STREAM6_CFG2);
		ctrl_outl(0x0, tsm_io + TSM_STREAM7_CFG2);
#elif defined(ATEVIO7500)
		/* RAM partitioning of streams */
		ctrl_outl(0x0, tsm_io + TSM_STREAM0_CFG);
		ctrl_outl(0x400, tsm_io + TSM_STREAM1_CFG);
		ctrl_outl(0x800, tsm_io + TSM_STREAM2_CFG);
		ctrl_outl(0xc00, tsm_io + TSM_STREAM3_CFG);
		ctrl_outl(0x1300, tsm_io + TSM_STREAM4_CFG);
		ctrl_outl(0x1d00, tsm_io + TSM_STREAM5_CFG);
		ctrl_outl(0x1e00, tsm_io + TSM_STREAM6_CFG);
		ctrl_outl(0x1f00, tsm_io + TSM_STREAM7_CFG);
		ctrl_outl(0x0, tsm_io + TSM_STREAM0_CFG2);
		ctrl_outl(0x0, tsm_io + TSM_STREAM1_CFG2);
		ctrl_outl(0x0, tsm_io + TSM_STREAM2_CFG2);
		ctrl_outl(0x0, tsm_io + TSM_STREAM3_CFG2);
		ctrl_outl(0x0, tsm_io + TSM_STREAM4_CFG2);
		ctrl_outl(0x0, tsm_io + TSM_STREAM5_CFG2);
		ctrl_outl(0x0, tsm_io + TSM_STREAM6_CFG2);
		ctrl_outl(0x0, tsm_io + TSM_STREAM7_CFG2);
#elif defined(UFS913)
		/* RAM partitioning of streams */
		ctrl_outl(0x0, tsm_io + TSM_STREAM0_CFG);
		ctrl_outl(0x500, tsm_io + TSM_STREAM1_CFG);
		ctrl_outl(0xa00, tsm_io + TSM_STREAM2_CFG);
		ctrl_outl(0xf00, tsm_io + TSM_STREAM3_CFG);
		ctrl_outl(0x1300, tsm_io + TSM_STREAM4_CFG);
		ctrl_outl(0x1700, tsm_io + TSM_STREAM5_CFG);
		ctrl_outl(0x1b00, tsm_io + TSM_STREAM6_CFG);
		ctrl_outl(0x1c00, tsm_io + TSM_STREAM7_CFG);
		ctrl_outl(0x0, tsm_io + TSM_STREAM0_CFG2);
		ctrl_outl(0x0, tsm_io + TSM_STREAM1_CFG2);
		ctrl_outl(0x0, tsm_io + TSM_STREAM2_CFG2);
		ctrl_outl(0x0, tsm_io + TSM_STREAM3_CFG2);
		ctrl_outl(0x0, tsm_io + TSM_STREAM4_CFG2);
		ctrl_outl(0x0, tsm_io + TSM_STREAM5_CFG2);
		ctrl_outl(0x0, tsm_io + TSM_STREAM6_CFG2);
		ctrl_outl(0x0, tsm_io + TSM_STREAM7_CFG2);
#else
		ctrl_outl(0x0, tsm_io + TSM_STREAM0_CFG); //448kb (7*64)
		ctrl_outl(0x700, tsm_io + TSM_STREAM1_CFG); //448kb (7*64)
		ctrl_outl(0xe00, tsm_io + TSM_STREAM2_CFG); //384kb (6*64)
		ctrl_outl(0x1400, tsm_io + TSM_STREAM3_CFG); //384kb (6*64)
		ctrl_outl(0x1a00, tsm_io + TSM_STREAM4_CFG); //320kb (5*64)
#endif
		/* configure streams: */
		/* add tag bytes to stream + stream priority */
#if defined(FORTIS_HDBOX) \
 || defined(OCTAGON1008) \
 || defined(CUBEREVO) \
 || defined(CUBEREVO_MINI2) \
 || defined(CUBEREVO_MINI) \
 || defined(CUBEREVO_250HD) \
 || defined(CUBEREVO_2000HD) \
 || defined(CUBEREVO_9500HD) \
 || defined(CUBEREVO_MINI_FTA) \
 || defined(CUBEREVO_3000HD)
		ret = ctrl_inl(tsm_io + TSM_STREAM0_CFG);
		ctrl_outl(ret | (0x40020), tsm_io + TSM_STREAM0_CFG);
		ret = ctrl_inl(tsm_io + TSM_STREAM4_CFG);
		ctrl_outl(ret | (0x40020), tsm_io + TSM_STREAM4_CFG);
		ret = ctrl_inl(tsm_io + TSM_STREAM5_CFG);
		ctrl_outl(ret | (0x40020), tsm_io + TSM_STREAM5_CFG);
#elif defined(ATEVIO7500)
		/* add tag bytes to stream + stream priority */
		ret = ctrl_inl(tsm_io + TSM_STREAM0_CFG);
		ctrl_outl(ret | (0x40020), tsm_io + TSM_STREAM0_CFG);
#elif !defined(UFS913)
		/* configure streams: */
		/* add tag bytes to stream + stream priority */
		ret = ctrl_inl(tsm_io + TSM_STREAM0_CFG);
		ctrl_outl(ret | (0x20020), tsm_io + TSM_STREAM0_CFG);
#endif
		ctrl_outl(stream_sync, tsm_io + TSM_STREAM0_SYNC);
		ctrl_outl(0x0, tsm_io + 0x18 /* reserved ??? */);
		/* add tag bytes to stream + stream priority */
#if defined(FORTIS_HDBOX) \
 || defined(OCTAGON1008) \
 || defined(ATEVIO7500)
		ret = ctrl_inl(tsm_io + TSM_STREAM1_CFG);
		ctrl_outl(ret | (0x40020), tsm_io + TSM_STREAM1_CFG);
#elif !defined(UFS913)
		ret = ctrl_inl(tsm_io + TSM_STREAM1_CFG);
		ctrl_outl(ret | (0x20020), tsm_io + TSM_STREAM1_CFG);
#endif
		ctrl_outl(stream_sync, tsm_io + TSM_STREAM1_SYNC);
		ctrl_outl(0x0, tsm_io + 0x38 /* reserved ??? */);
		/* add tag bytes to stream + stream priority */
#if defined(FORTIS_HDBOX) || defined(OCTAGON1008) || defined(ATEVIO7500)
		ret = ctrl_inl(tsm_io + TSM_STREAM2_CFG);
		ctrl_outl(ret | (0x40020), tsm_io + TSM_STREAM2_CFG);
#elif defined(UFS912) \
 || defined(SPARK) \
 || defined(HS7810A) \
 || defined(HS7420) \
 || defined(HS7429) \
 || defined(HS7819) \
 || defined(VITAMIN_HD5000)
		ret = ctrl_inl(tsm_io + TSM_STREAM2_CFG);
		ctrl_outl(ret | (0x20020), tsm_io + TSM_STREAM2_CFG);
#elif !defined(UFS913)
		ret = ctrl_inl(tsm_io + TSM_STREAM2_CFG);
		ctrl_outl(ret | (0x30020), tsm_io + TSM_STREAM2_CFG);
#endif
		ctrl_outl(stream_sync, tsm_io + TSM_STREAM2_SYNC);
		ctrl_outl(0x0, tsm_io + 0x58 /* reserved ??? */);
		/* add tag bytes to stream + stream priority */
#if defined(FORTIS_HDBOX) \
 || defined(OCTAGON1008)
		ret = ctrl_inl(tsm_io + TSM_STREAM3_CFG);
		ctrl_outl(ret | (0x40020), tsm_io + TSM_STREAM3_CFG);
#elif defined(UFS912) \
 || defined(SPARK) \
 || defined(ATEVIO7500) \
 || defined(HS7420) \
 || defined(HS7429) \
 || defined(HS7810A) \
 || defined(HS7819) \
 || defined(VITAMIN_HD5000)
		ret = ctrl_inl(tsm_io + TSM_STREAM3_CFG);
		ctrl_outl(ret | (0x20020), tsm_io + TSM_STREAM3_CFG);
		ret = ctrl_inl(tsm_io + TSM_STREAM4_CFG);
		ctrl_outl(ret | (0x20020), tsm_io + TSM_STREAM4_CFG);
		ctrl_outl(0x00, tsm_io + TSM_STREAM5_SYNC);
		ctrl_outl(0x00, tsm_io + TSM_STREAM6_SYNC);
#elif defined(CUBEREVO) \
 || defined(CUBEREVO_MINI2) \
 || defined(CUBEREVO_MINI) \
 || defined(CUBEREVO_250HD) \
 || defined(CUBEREVO_2000HD) \
 || defined(CUBEREVO_9500HD) \
 || defined(CUBEREVO_MINI_FTA) \
 || defined(CUBEREVO_3000HD) \
 || defined(IPBOX9900) \
 || defined(ARIVALINK200)
		/* configure streams: */
		/* add tag bytes to stream + stream priority */
		ret = ctrl_inl(tsm_io + TSM_STREAM3_CFG);
		ctrl_outl(ret | (0x70020), tsm_io + TSM_STREAM3_CFG);
#elif !defined(UFS913)
		ret = ctrl_inl(tsm_io + TSM_STREAM3_CFG);
		ctrl_outl(ret | (0x30020), tsm_io + TSM_STREAM3_CFG);
#endif
		ctrl_outl(stream_sync, tsm_io + TSM_STREAM3_SYNC);
		ctrl_outl(0x0, tsm_io + 0x78 /* reserved ??? */);
#if !defined(FORTIS_HDBOX) \
 && !defined(UFS912) \
 && !defined(UFS913) \
 && !defined(CUBEREVO) \
 && !defined(CUBEREVO_MINI2) \
 && !defined(CUBEREVO_MINI) \
 && !defined(CUBEREVO_250HD) \
 && !defined(CUBEREVO_2000HD) \
 && !defined(CUBEREVO_9500HD) \
 && !defined(CUBEREVO_MINI_FTA) \
 && !defined(CUBEREVO_3000HD) \
 && !defined(ATEVIO7500) \
 && !defined(HS7110) \
 && !defined(HS7810A)\
 && !defined(HS7420) \
 && !defined(HS7429) \
 && !defined(HS7119) \
 && !defined(HS7819) \
 && !defined(ATEMIO520) \
 && !defined(ATEMIO530) \
 && !defined(IPBOX9900) \
 && !defined(ARIVALINK200) \
 && !defined(VITAMIN_HD5000)
		/* swts_req_trigger + pace cycles (1101) */
		ctrl_outl(0x800000d, tsm_io + SWTS_CFG(0));
#elif defined (UFS912) \
 || defined(ATEVIO7500) \
 || defined(HS7110) \
 || defined(HS7810A) \
 || defined(HS7420) \
 || defined(HS7429) \
 || defined(HS7119) \
 || defined(HS7819) \
 || defined(ATEMIO520) \
 || defined(ATEMIO530) \
 || defined(VITAMIN_HD5000)
		ctrl_outl(0x8f0000e, tsm_io + SWTS_CFG(0));
		ctrl_outl(0x8000000, tsm_io + SWTS_CFG(1));
		ctrl_outl(0x8000000, tsm_io + SWTS_CFG(2));
#elif defined(CUBEREVO) \
 || defined(CUBEREVO_MINI2) \
 || defined(CUBEREVO_MINI) \
 || defined(CUBEREVO_250HD) \
 || defined(CUBEREVO_3000HD)
		ctrl_outl(0x88000010, tsm_io + SWTS_CFG(0));
#elif !defined(UFS913)
		ctrl_outl(0x8000010, tsm_io + SWTS_CFG(0));
#endif
		/* auto count */
		ctrl_outl(0x0, tsm_io + TSM_PROG_CNT0);
#if !defined(TF7700) \
 && !defined(UFS922) \
 && !defined(UFC960) \
 && !defined(FORTIS_HDBOX) \
 && !defined(HL101) \
 && !defined(VIP1_V2) \
 && !defined(UFS912) \
 && !defined(UFS913) \
 && !defined(SPARK) \
 && !defined(OCTAGON1008) \
 && !defined(CUBEREVO) \
 && !defined(CUBEREVO_MINI2) \
 && !defined(CUBEREVO_MINI) \
 && !defined(CUBEREVO_250HD) \
 && !defined(CUBEREVO_2000HD) \
 && !defined(CUBEREVO_9500HD) \
 && !defined(CUBEREVO_MINI_FTA) \
 && !defined(CUBEREVO_3000HD) \
 && !defined(ATEVIO7500) \
 && !defined(HS7110) \
 && !defined(HS7810A) \
 && !defined(HS7420) \
 && !defined(HS7429) \
 && !defined(HS7119) \
 && !defined(HS7819) \
 && !defined(ATEMIO520) \
 && !defined(ATEMIO530) \
 && !defined(IPBOX9900) \
 && !defined(ARIVALINK200) \
 && !defined(VITAMIN_HD5000)
		/* UFS910 stream configuration */
		/* route stream 2 to PTI */
		ret = ctrl_inl(tsm_io + TSM_PTI_SEL);
		ctrl_outl(ret | 0x4, tsm_io + TSM_PTI_SEL);
		/* set stream on */
		ret = ctrl_inl(tsm_io + TSM_STREAM2_CFG);
		ctrl_outl(ret | 0x80, tsm_io + TSM_STREAM2_CFG);
		/* set firewire clock */
		/* This solves the SR 30000 Problem */
		/* this must be 0x70010 for SR 30000. for some modules
		 * this must be 0x70012 or 0x70014 :-(
		 * ->also the original pvrmain (fw 106) makes CC trouble
		 * with this setting and some modules.
		 * 0xf from 106 pvrmain
		 */
#ifdef FW1XX
		if (highSR)
			ctrl_outl(0x7000f, tsm_io + TS_1394_CFG);
		else
			ctrl_outl(0x70014, tsm_io + TS_1394_CFG);
#elif defined(UFS910)
		if (camRouting == 1) // CamRouting ufs910
			ctrl_outl(0x70014, tsm_io + TS_1394_CFG);
		else
			ctrl_outl(0x50014, tsm_io + TS_1394_CFG);
#else
		/* logged from fw 202rc ->see also pti */
		ctrl_outl(0x50014, tsm_io + TS_1394_CFG);
#endif
		/* connect TSIN0 to TS1394 for routing tuner TS through the CIMAX */
		ret = ctrl_inl(tsm_io + TSM_1394_DEST);
		ctrl_outl(ret | 0x1, tsm_io + TSM_1394_DEST);
#elif defined(UFS912) \
 || defined(HS7420) \
 || defined(HS7429) \
 || defined(HS7810A) \
 || defined(HS7819)
		ctrl_outl(0x15, tsm_io + TSM_PTI_SEL);
		/* set stream 2 on */
		ret = ctrl_inl(tsm_io + TSM_STREAM2_CFG);
		ctrl_outl(ret | 0x80, tsm_io + TSM_STREAM2_CFG);
		/* set stream 4 on */
		ret = ctrl_inl(tsm_io + TSM_STREAM4_CFG);
		ctrl_outl(ret | 0x80, tsm_io + TSM_STREAM4_CFG);
#elif defined(TF7700) \
 || defined(UFS922) \
 || defined(UFC960) \
 || defined(HL101) \
 || defined(VIP1_V2)
		/* TF7700 stream configuration */
		/* route stream 1 to PTI */
		ret = ctrl_inl(tsm_io + TSM_PTI_SEL);
		ctrl_outl(ret | 0x2, tsm_io + TSM_PTI_SEL);
		/* set stream 1 on */
		ret = ctrl_inl(tsm_io + TSM_STREAM1_CFG);
		ctrl_outl(ret | 0x80, tsm_io + TSM_STREAM1_CFG);
		/* route stream 0 to PTI */
		ret = ctrl_inl(tsm_io + TSM_PTI_SEL);
		ctrl_outl(ret | 0x1, tsm_io + TSM_PTI_SEL);
		/* set firewire clock */
		//ctrl_outl(0x70014 ,tsm_io + TS_1394_CFG);
		/* connect SWTS to TS1394 for routing through the StarCI2Win */
		//ret = ctrl_inl(tsm_io + TSM_1394_DEST);
		//ctrl_outl(ret | 0x8 , tsm_io + TSM_1394_DEST);
#elif defined(FORTIS_HDBOX)
		/* route stream 1 to PTI */
		ret = ctrl_inl(tsm_io + TSM_PTI_SEL);
		ctrl_outl(ret | 0x2, tsm_io + TSM_PTI_SEL);
		/* route stream 2 to PTI */
		ret = ctrl_inl(tsm_io + TSM_PTI_SEL);
		ctrl_outl(ret | 0x4, tsm_io + TSM_PTI_SEL);
		/* set stream on */
		ret = ctrl_inl(tsm_io + TSM_STREAM5_CFG);
		ctrl_outl(ret | 0x80, tsm_io + TSM_STREAM5_CFG);
		/* set stream on */
		ret = ctrl_inl(tsm_io + TSM_STREAM4_CFG);
		ctrl_outl(ret | 0x80, tsm_io + TSM_STREAM4_CFG);
		/* set stream on */
		ret = ctrl_inl(tsm_io + TSM_STREAM3_CFG);
		ctrl_outl(ret | 0x80, tsm_io + TSM_STREAM3_CFG);
		/* set stream on */
		ret = ctrl_inl(tsm_io + TSM_STREAM2_CFG);
		ctrl_outl(ret | 0x80, tsm_io + TSM_STREAM2_CFG);
		/* set stream 1 on */
		ret = ctrl_inl(tsm_io + TSM_STREAM1_CFG);
		ctrl_outl(ret | 0x80, tsm_io + TSM_STREAM1_CFG);
		/* route stream 0 to PTI */
		ret = ctrl_inl(tsm_io + TSM_PTI_SEL);
		ctrl_outl(ret | 0x1, tsm_io + TSM_PTI_SEL);
		ctrl_outl(stream_sync, tsm_io + TSM_STREAM4_SYNC);
		ctrl_outl(stream_sync, tsm_io + TSM_STREAM5_SYNC);
		ctrl_outl(0x00, tsm_io + TSM_STREAM6_SYNC);
		ret = ctrl_inl(tsm_io + TSM_1394_DEST);
		ctrl_outl(ret | 0x38, tsm_io + TSM_1394_DEST);
#elif defined(CUBEREVO) \
 || defined(CUBEREVO_MINI2) \
 || defined(CUBEREVO_MINI) \
 || defined(CUBEREVO_250HD) \
 || defined(CUBEREVO_2000HD) \
 || defined(CUBEREVO_9500HD) \
 || defined(CUBEREVO_MINI_FTA) \
 || defined(CUBEREVO_3000HD)
		/* route stream 1 to PTI */
		ret = ctrl_inl(tsm_io + TSM_PTI_SEL);
		ctrl_outl(ret | 0x2, tsm_io + TSM_PTI_SEL);
		/* set stream 1 on */
		ret = ctrl_inl(tsm_io + TSM_STREAM1_CFG);
		ctrl_outl(ret | 0x80, tsm_io + TSM_STREAM1_CFG);
		/* route stream 0 to PTI */
		ret = ctrl_inl(tsm_io + TSM_PTI_SEL);
		ctrl_outl(ret | 0x1, tsm_io + TSM_PTI_SEL);
		/* stream 5 and 6
		ctrl_outl(0x1, tsm_io + TSM_STREAM5_CFG);
		ctrl_outl(0x1, tsm_io + TSM_STREAM6_CFG);
		ctrl_outl(0xbc4700, tsm_io + TSM_STREAM5_SYNC);
		ctrl_outl(0xbc4700, tsm_io + TSM_STREAM6_SYNC);*/
		/* set 1394 stream Out
		ctrl_outl(0x0001804c ,tsm_io + TS_1394_CFG);
		ret = ctrl_inl(tsm_io + TSM_1394_DEST);
		ctrl_outl(ret | 0x1 , tsm_io + TSM_1394_DEST);*/
#elif defined(OCTAGON1008)
		/* route stream 1 to PTI */
		ret = ctrl_inl(tsm_io + TSM_PTI_SEL);
		ctrl_outl(ret | 0x2, tsm_io + TSM_PTI_SEL);
		/* route stream 2 to PTI */
		ret = ctrl_inl(tsm_io + TSM_PTI_SEL);
		ctrl_outl(ret | 0x4, tsm_io + TSM_PTI_SEL);
		/* set stream on */
		ret = ctrl_inl(tsm_io + TSM_STREAM2_CFG);
		ctrl_outl(ret | 0x80, tsm_io + TSM_STREAM2_CFG);
		/* set stream 1 on */
		ret = ctrl_inl(tsm_io + TSM_STREAM1_CFG);
		ctrl_outl(ret | 0x80, tsm_io + TSM_STREAM1_CFG);
		/* route stream 0 to PTI */
		ret = ctrl_inl(tsm_io + TSM_PTI_SEL);
		ctrl_outl(ret | 0x1, tsm_io + TSM_PTI_SEL);
		ret = ctrl_inl(tsm_io + TSM_1394_DEST);
		ctrl_outl(ret | 0x38, tsm_io + TSM_1394_DEST);
#elif defined(ATEVIO7500)
		/* set stream 1 on */
		ret = ctrl_inl(tsm_io + TSM_STREAM1_CFG);
		ctrl_outl(ret | 0x80, tsm_io + TSM_STREAM1_CFG);
		/* set stream 2 on */
		ret = ctrl_inl(tsm_io + TSM_STREAM2_CFG);
		ctrl_outl(ret | 0x80, tsm_io + TSM_STREAM2_CFG);
		/* route stream 0/1/2 to PTI */
		ctrl_outl(0x7, tsm_io + TSM_PTI_SEL);
#elif defined(UFS913)
		/* configure stream 0 */
		ret = ctrl_inl(tsm_io + TSM_STREAM0_CFG);
		ctrl_outl(ret | 0x20020, tsm_io + TSM_STREAM0_CFG);
		/* configure stream 1 */
		ret = ctrl_inl(tsm_io + TSM_STREAM1_CFG);
		ctrl_outl(ret | 0x20025, tsm_io + TSM_STREAM1_CFG);
		/* configure stream 2 */
		ret = ctrl_inl(tsm_io + TSM_STREAM2_CFG);
		ctrl_outl(ret | 0x20020, tsm_io + TSM_STREAM2_CFG);
		/* configure stream 3 */
		ret = ctrl_inl(tsm_io + TSM_STREAM3_CFG);
		ctrl_outl(ret | 0x20020, tsm_io + TSM_STREAM3_CFG);
		/* configure stream 4 */
		ret = ctrl_inl(tsm_io + TSM_STREAM4_CFG);
		ctrl_outl(ret | 0x20020, tsm_io + TSM_STREAM4_CFG);
		/* configure stream 5 */
		ret = ctrl_inl(tsm_io + TSM_STREAM5_CFG);
		ctrl_outl(ret | 0x20020, tsm_io + TSM_STREAM5_CFG);
		ret = ctrl_inl(tsm_io + TSM_STREAM1_CFG);
		ctrl_outl(ret | 0x80, tsm_io + TSM_STREAM1_CFG);
		ret = ctrl_inl(tsm_io + TSM_STREAM2_CFG);
		ctrl_outl(ret | 0x80, tsm_io + TSM_STREAM2_CFG);
		ret = ctrl_inl(tsm_io + TSM_STREAM3_CFG);
		ctrl_outl(ret | 0x80, tsm_io + TSM_STREAM3_CFG);
		ret = ctrl_inl(tsm_io + TSM_STREAM4_CFG);
		ctrl_outl(ret | 0x80, tsm_io + TSM_STREAM4_CFG);
		ret = ctrl_inl(tsm_io + TSM_STREAM5_CFG);
		ctrl_outl(ret | 0x80, tsm_io + TSM_STREAM5_CFG);
		ret = ctrl_inl(tsm_io + TSM_SWTS_CFG(0));
		ctrl_outl(ret | 0x14, tsm_io + TSM_SWTS_CFG(0));
		ret = ctrl_inl(tsm_io + TSM_SWTS_CFG(1));
		ctrl_outl(ret | 0x26, tsm_io + TSM_SWTS_CFG(1));
		/* stream0 and stream3 (2 and 3 are identical ?!) */
		ctrl_outl(0x9, tsm_io + TSM_PTI_SEL);
		/* orig setting: currently pti does not support this :(

		->used streams:
		->TSIN0 = Tuner0
		->TSIN2 = Tuner1
		->TSIN3 = Tuner1

		 ctrl_outl(0x11, tsm_io + TSM_PTI_SEL);
		 ctrl_outl(0x2f, tsm_io + TSM_PTI1_SEL);
		*/
#elif defined(IPBOX9900) \
 || defined(ARIVALINK200) \
 || defined(VITAMIN_HD5000)
		/* route stream 0/1 to PTI */
		ret = ctrl_inl(tsm_io + TSM_PTI_SEL);
		ctrl_outl(ret | 0x2 | 0x1 | 0x4, tsm_io + TSM_PTI_SEL);
		/* route stream 0/1 to PTI1 */
		ret = ctrl_inl(tsm_io + TSM_PTI1_SEL);
		ctrl_outl(ret | 0x2 | 0x1 | 0x4, tsm_io + TSM_PTI1_SEL);
		/* set stream 1 on */
		ret = ctrl_inl(tsm_io + TSM_STREAM1_CFG);
		ctrl_outl(ret | 0x80, tsm_io + TSM_STREAM1_CFG);
		/* set stream 2 on */
		ret = ctrl_inl(tsm_io + TSM_STREAM2_CFG);
		ctrl_outl(ret | 0x80, tsm_io + TSM_STREAM2_CFG);
#elif defined(ADB_BOX)
		/* route stream 1 to PTI */
		ret = ctrl_inl(tsm_io + TSM_PTI_SEL);
		ctrl_outl(ret | 0x2, tsm_io + TSM_PTI_SEL);
		/* set stream 1 on */
		ret = ctrl_inl(tsm_io + TSM_STREAM1_CFG);
		ctrl_outl(ret | 0x80, tsm_io + TSM_STREAM1_CFG);
		/* route stream 2 to PTI */
		ret = ctrl_inl(tsm_io + TSM_PTI_SEL);
		ctrl_outl(ret | 0x4, tsm_io + TSM_PTI_SEL);
		/* set stream 2 on */
		ret = ctrl_inl(tsm_io + TSM_STREAM2_CFG);
		ctrl_outl(ret | 0x80, tsm_io + TSM_STREAM2_CFG);
		/* route stream 0 to PTI */
		ret = ctrl_inl(tsm_io + TSM_PTI_SEL);
		ctrl_outl(ret | 0x1, tsm_io + TSM_PTI_SEL);
#elif defined(HS7110) \
 || defined(HS7119) \
 || defined(ATEMIO520) \
 || defined(ATEMIO530)
		/* route stream 0 to PTI */
		ret = ctrl_inl(tsm_io + TSM_PTI_SEL);
		ctrl_outl(ret | 0x1, tsm_io + TSM_PTI_SEL);
#endif
		/* set stream0 on */
		ret = ctrl_inl(tsm_io + TSM_STREAM0_CFG);
		ctrl_outl(ret | 0x80, tsm_io + TSM_STREAM0_CFG);
#if !defined(CUBEREVO) \
 && !defined(CUBEREVO_MINI2) \
 && !defined(CUBEREVO_MINI) \
 && !defined(CUBEREVO_250HD) \
 && !defined(CUBEREVO_2000HD) \
 && !defined(CUBEREVO_9500HD) \
 && !defined(CUBEREVO_MINI_FTA) \
 && !defined(CUBEREVO_3000HD) \
 && !defined(HS7110) \
 && !defined(HS7119) \
 && !defined(ATEMIO520) \
 && !defined(ATEMIO530) \
 && !defined(UFS913) /* Dagobert: set-up swts */
		ctrl_outl(TSM_SWTS_REQ_TRIG(128 / 16) | 0x10, tsm_io + TSM_SWTS_CFG(0));
		/* SWTS0 to PTI */
		ret = ctrl_inl(tsm_io + TSM_PTI_SEL);
		ctrl_outl(ret | 0x8, tsm_io + TSM_PTI_SEL);
		ret = ctrl_inl(tsm_io + TSM_STREAM3_CFG);
		ctrl_outl((ret & TSM_RAM_ALLOC_START(0xff)) |
				  TSM_PRIORITY(0x7) | TSM_STREAM_ON | TSM_ADD_TAG_BYTES | TSM_SYNC_NOT_ASYNC | TSM_ASYNC_SOP_TOKEN
				  , tsm_io + TSM_STREAM3_CFG);
		/* don't touch the DMA engine -- seems unnecessary on reinit */
		if (reinit)
			return;
		tsm_handle.swts_channel = 3;
		tsm_handle.tsm_swts = (unsigned long)ioremap(0x1A300000, 0x1000);
		/* Now lets get the SWTS info and setup an FDMA channel */
#if defined(UFS912) \
 || defined(SPARK) \
 || defined(ATEVIO7500) \
 || defined(HS7110) \
 || defined(HS7810A) \
 || defined(HS7420) \
 || defined(HS7429) \
 || defined(HS7119) \
 || defined(HS7819) \
 || defined(ATEMIO520) \
 || defined(ATEMIO530) \
 || defined(VITAMIN_HD5000)
		tsm_handle.fdma_reqline = 31;
#elif defined(UFS910)
		//ufs910 use dma request id 30 for swts, do'nt know what other boxes use
		tsm_handle.fdma_reqline = 28;
#else
		tsm_handle.fdma_reqline = 30;
#endif
		tsm_handle.fdma_channel = request_dma_bycap(fdmac_id, fdma_cap_hb, "swts0");
		tsm_handle.fdma_req = dma_req_config(tsm_handle.fdma_channel, tsm_handle.fdma_reqline, &fdma_req_config);
		/* Initilise the parameters for the FDMA SWTS data injection */
		for (n = 0; n < MAX_SWTS_PAGES; n++)
		{
			dma_params_init(&tsm_handle.swts_params[n], MODE_PACED, STM_DMA_LIST_OPEN);
			dma_params_DIM_1_x_0(&tsm_handle.swts_params[n]);
			dma_params_req(&tsm_handle.swts_params[n], tsm_handle.fdma_req);
		}
#endif
	}
	else
	{
		/* bypass cimax */
		int n;
		printk("Bypass ci\n");
		if (reinit)
		{
			printk("reinit\n");
		}
		else
		{
#if defined(SPARK) \
 || defined(SPARK7162) \
 || defined(HS7110) \
 || defined(HS7119) \
 || defined(ATEMIO520) \
 || defined(ATEMIO530) \
 || defined(VITAMIN_HD5000) \
 || defined(SAGEMCOM88)
			tsm_io = ioremap(/* config->tsm_base_address */ TSMergerBaseAddress, 0x1000);
#else // !defined(SPARK) && !defined(SPARK7162) && !defined(HS7110) && !defined(ATEMIO520) && !defined(ATEMIO530)
			tsm_io = ioremap(/* config->tsm_base_address */ 0x19242000, 0x1000);
#endif
		}
		/* >>> DVBT-USB
		 j00zek: when tuner hangs starting streaming from DVB-T USB, something wrong is with this section */
#if defined(SAGEMCOM88) \
 || defined(SPARK7162)
		printk(">>Init st7105 DVBT-USB\n");
		// STi7105
		// 0-3 - 4xTS
		// 4-6 - 3xSWTS
		ctrl_outl(TSM_SWTS_REQ_TRIG(128 / 16) | 0x12, tsm_io + TSM_SWTS_CFG(0));
		ctrl_outl(0x8000000, tsm_io + SWTS_CFG(1));
		ctrl_outl(0x8000000, tsm_io + SWTS_CFG(2));
		tsm_handle.tsm_io = ioremap(TSMergerBaseAddress, 0x1000);
		tsm_handle.tsm_swts = (unsigned long)ioremap(SWTS_BASE_ADDRESS, 0x1000);
		// pio12
		ctrl_outl(0x0, 0xfe015020);
		ctrl_outl(0x0, 0xfe015030);
		ctrl_outl(0x0, 0xfe015040);
		// alternate 1
		ctrl_outl(0x0, reg_sys_config + 0x1c0); // sys_cfg48/
		// pio6
		ctrl_outl(0x0, 0xfd026020);
		ctrl_outl(0x0, 0xfd026030);
		ctrl_outl(0x0, 0xfd026040);
		// alternate 2
		ctrl_outl(0x00ff, reg_sys_config + 0x190); // sys_cfg36/
		// pio6 = TSIN2
		ret = ctrl_inl(reg_sys_config + 0x110);
		ctrl_outl((ret & ~(1 << 10)), reg_sys_config + 0x110); // sys_cfg4
		// pio6/0 not used PCI mode
		ret = ctrl_inl(reg_sys_config + 0x114);
		ctrl_outl((ret | (1 << 27)), reg_sys_config + 0x114); // sys_cfg5/
		// route
		ctrl_outl(0x8, reg_sys_config + SYS_CFG0); //tsin2>2 and tsin3>3
#elif defined(ADB_BOX) \
 || defined(ARIVALINK200) \
 || defined(IPBOX9900) \
 || defined(IPBOX99) \
 || defined(IPBOX55) \
 || defined(HL101)
		printk(">>Init DVBT-USB\n");
		tsm_handle.tsm_io = ioremap(TSMergerBaseAddress, 0x0900);
		tsm_handle.swts_channel = 3;
		tsm_handle.tsm_swts = (unsigned long)ioremap(0x1A300000, 0x1000);
		//ctrl_outl( TSM_SWTS_REQ_TRIG(128/16) | 8, tsm_io + TSM_SWTS_CFG(0));
		ctrl_outl(TSM_SWTS_REQ_TRIG(128 / 16) | 12, tsm_io + TSM_SWTS_CFG(0));
		tsm_handle.fdma_reqline = 30;
		tsm_handle.fdma_channel = request_dma_bycap(fdmac_id, fdma_cap_hb, "swts0");
		tsm_handle.fdma_req = dma_req_config(tsm_handle.fdma_channel, tsm_handle.fdma_reqline, &fdma_req_config);
		/*
		// Initilise the parameters for the FDMA SWTS data injection
		for (n=0;n<3;n++) {
		//for (n=0;n<MAX_SWTS_PAGES;n++) {
		 dma_params_init(&tsm_handle.swts_params[n], MODE_PACED, STM_DMA_LIST_OPEN);
		 dma_params_DIM_1_x_0(&tsm_handle.swts_params[n]);
		 dma_params_req(&tsm_handle.swts_params[n],tsm_handle.fdma_req);
		}
		*/
#endif
		/* <<< DVBT-USB */
#ifdef LOAD_TSM_DATA
		TSM_NUM_PTI_ALT_OUT = 1/* config->tsm_num_pti_alt_out*/;
		TSM_NUM_1394_ALT_OUT = 1/*config->tsm_num_1394_alt_out */;
#endif // LOAD_TSM_DATA
		writel(0x0, tsm_io + TSM_SW_RESET);
		writel(0x6, tsm_io + TSM_SW_RESET);
		/*
		channels = streams = 5 ? wohl 4 s.u.
		tsm_sram_buffer_size ?
		options? ->s.u.
		lock ->muss es als register geben ->ist lt. meinen logs beides 3
		drop ->dito _>also nachlesen
		#define STM_GET_CHANNEL(x) ((x & 0x0f0000) >> 16)

		Dies sind die Options (also wohl auch view channel):
		 STM_TSM_CHANNEL_0 = 0x000000, ->ok dann liefer stm_get_channel 0,1,2,3 ;-)
		 STM_TSM_CHANNEL_1 = 0x010000,
		 STM_TSM_CHANNEL_2 = 0x020000,
		 STM_TSM_CHANNEL_3 = 0x030000,

		*/
#if defined(SPARK)
		/* RAM partitioning of streams */
		ctrl_outl(0x0, tsm_io + TSM_STREAM0_CFG); //448kb (8*64)
		ctrl_outl(0x500, tsm_io + TSM_STREAM1_CFG); //448kb (6*64)
		ctrl_outl(0xe00, tsm_io + TSM_STREAM2_CFG); //384kb (8*64)
		ctrl_outl(0x1600, tsm_io + TSM_STREAM3_CFG); //384kb (6*64)
		ctrl_outl(0x1a00, tsm_io + TSM_STREAM4_CFG); //320kb (5*64)
		ctrl_outl(0x1d00, tsm_io + TSM_STREAM5_CFG);
		ctrl_outl(0x1e00, tsm_io + TSM_STREAM6_CFG);
		ctrl_outl(0x1f00, tsm_io + TSM_STREAM7_CFG);
#elif defined(HS7110) \
 || defined(HS7119) \
 || defined(ATEMIO520) \
 || defined(ATEMIO530)
		/* RAM partitioning of streams */
		ctrl_outl(0x0, tsm_io + TSM_STREAM0_CFG); //448kb (8*64)
		ctrl_outl(0x800, tsm_io + TSM_STREAM1_CFG); //448kb (6*64)
		ctrl_outl(0xe00, tsm_io + TSM_STREAM2_CFG); //384kb (8*64)
		ctrl_outl(0x1400, tsm_io + TSM_STREAM3_CFG); //384kb (6*64)
		ctrl_outl(0x1a00, tsm_io + TSM_STREAM4_CFG); //320kb (5*64)
		ctrl_outl(0x1d00, tsm_io + TSM_STREAM5_CFG);
		ctrl_outl(0x1e00, tsm_io + TSM_STREAM6_CFG);
		ctrl_outl(0x1f00, tsm_io + TSM_STREAM7_CFG);
#elif defined(VITAMIN_HD5000)
		ctrl_outl(0x0000, tsm_io + TSM_STREAM0_CFG); // 0x0000-0x08FF
		ctrl_outl(0x0900, tsm_io + TSM_STREAM1_CFG); // 0x0900-0x11FF
		ctrl_outl(0x1200, tsm_io + TSM_STREAM2_CFG); // 0x1200-0x12FF
		ctrl_outl(0x1300, tsm_io + TSM_STREAM3_CFG); // 0x1300-0x13FF
		ctrl_outl(0x1400, tsm_io + TSM_STREAM4_CFG); // 0x1400-0x1CFF
		ctrl_outl(0x1D00, tsm_io + TSM_STREAM5_CFG); // 0x1D00-0x1DFF
		ctrl_outl(0x1E00, tsm_io + TSM_STREAM6_CFG); // 0x1E00-0x1EFF
		ctrl_outl(0x1F00, tsm_io + TSM_STREAM7_CFG); // 0x1F00-0x1FFF
#elif defined(SAGEMCOM88) \
 || defined(SPARK7162)
		for (n = 0; n < 6; n++) //4TS + 3SWTS at STi7105
		{
			writel(TSM_RAM_ALLOC_START(0x4 * n), tsm_io + TSM_STREAM_CONF(n));
		}
#else /* !defined(SPARK) && !defined(HS7110) && !defined(HS7119) && !defined(ATEMIO520) && !defined(ATEMIO530) && !defined(VITAMIN_HD5000) && !defined(SAGEMCOM88) && !defined(SPARK7162)*/
		for (n = 0; n < 5; n++)
		{
			writel(TSM_RAM_ALLOC_START(0x3 * n), tsm_io + TSM_STREAM_CONF(n));
		}
#endif // defined(SPARK) || defined(HS7110) || defined(HS7119) || defined(ATEMIO520) || defined(ATEMIO530)
#if defined(SAGEMCOM88) \
 || defined(SPARK7162)
		for (n = 0; n < 6/* config->nr_channels */; n++) //4TS + 3SWTS at STi7105
#else
		for (n = 0; n < 4/* config->nr_channels */; n++)
#endif
		{
#ifdef alt
			enum plat_frontend_options options = config->channels[n].option;
			int chan = STM_GET_CHANNEL(options);// / STM_TSM_CHANNEL_1) & 0xf;
#endif // alt
			int chan = n;
#if defined(SAGEMCOM88)
			int options = (n * 0x10000) + STM_SERIAL_NOT_PARALLEL;
#else
			int options = n * 0x10000;
#endif // alt
			writel(readl(tsm_io + TSM_DESTINATION(0)) | (1 << chan), tsm_io + TSM_DESTINATION(0));
#if defined(ADB_BOX)
			if (TsinMode == SERIAL)
			{
				printk("BZZB TsinMode = SERIAL*st-merger*\n\t");
				writel((readl(tsm_io + TSM_STREAM_CONF(chan)) & TSM_RAM_ALLOC_START(0xff)) |
					   (options & STM_SERIAL_NOT_PARALLEL ? TSM_SERIAL_NOT_PARALLEL : 1) |
					   (options & STM_INVERT_CLOCK ? TSM_INVERT_BYTECLK : 0) |
					   (options & STM_PACKET_CLOCK_VALID ? TSM_SYNC_NOT_ASYNC : 0) |
					   TSM_ALIGN_BYTE_SOP |
					   TSM_PRIORITY(0xf) | TSM_STREAM_ON | TSM_ADD_TAG_BYTES,
					   tsm_io + TSM_STREAM_CONF(chan));
			}
			else if (TsinMode == PARALLEL)
			{
				printk("BSKA,BSLA,BXZB TsinMode = Parallel *st-merger*\n\t");
				writel((readl(tsm_io + TSM_STREAM_CONF(chan)) & TSM_RAM_ALLOC_START(0xff)) |
					   (options & STM_SERIAL_NOT_PARALLEL ? TSM_SERIAL_NOT_PARALLEL : 0) |
					   (options & STM_INVERT_CLOCK ? TSM_INVERT_BYTECLK : 0) |
					   (options & STM_PACKET_CLOCK_VALID ? TSM_SYNC_NOT_ASYNC : 0) |
					   TSM_ALIGN_BYTE_SOP |
					   TSM_PRIORITY(0xf) | TSM_STREAM_ON | TSM_ADD_TAG_BYTES,
					   tsm_io + TSM_STREAM_CONF(chan));
			}
#elif defined(SAGEMCOM88)
			writel((readl(tsm_io + TSM_STREAM_CONF(chan)) & TSM_RAM_ALLOC_START(0xff)) |
				   (options & STM_SERIAL_NOT_PARALLEL ? TSM_SERIAL_NOT_PARALLEL : 0) |
				   (options & STM_INVERT_CLOCK ? TSM_INVERT_BYTECLK : 0) |
				   (options & STM_PACKET_CLOCK_VALID ? TSM_SYNC_NOT_ASYNC : 0) |
				   TSM_ALIGN_BYTE_SOP |
				   TSM_PRIORITY(0xf) | TSM_STREAM_ON | TSM_ADD_TAG_BYTES,
				   tsm_io + TSM_STREAM_CONF(chan));
#else
			printk("TsinMode = Parallel *st-merger*\n\t");
			writel((readl(tsm_io + TSM_STREAM_CONF(chan)) & TSM_RAM_ALLOC_START(0xff)) |
				   (options & STM_SERIAL_NOT_PARALLEL ? TSM_SERIAL_NOT_PARALLEL : 0) |
				   (options & STM_INVERT_CLOCK ? TSM_INVERT_BYTECLK : 0) |
				   (options & STM_PACKET_CLOCK_VALID ? TSM_SYNC_NOT_ASYNC : 0) |
				   TSM_ALIGN_BYTE_SOP |
				   TSM_PRIORITY(0xf) | TSM_STREAM_ON | TSM_ADD_TAG_BYTES,
				   tsm_io + TSM_STREAM_CONF(chan));
#endif
#ifdef alt
			writel(TSM_SYNC(config->channels[n].lock) |
				   TSM_DROP(config->channels[n].drop) |
				   TSM_SOP_TOKEN(0x47) |
				   TSM_PACKET_LENGTH(188)
				   , tsm_io + TSM_STREAM_SYNC(chan));
#endif // alt
			writel(TSM_SYNC(3 /* lock */) |
				   TSM_DROP(3 /*drop*/) |
				   TSM_SOP_TOKEN(0x47) |
				   TSM_PACKET_LENGTH(188)
				   , tsm_io + TSM_STREAM_SYNC(chan));
		}
		/* Put TSMERGER into normal mode */
		writel(TSM_CFG_BYPASS_NORMAL, tsm_io + TSM_SYSTEM_CFG);
	}
}

void stm_tsm_release(void)
{
	iounmap(tsm_io);
}
