/*
 * Compressed RAM based swap device
 *
 * Copyright (C) 2008, 2009  Nitin Gupta
 *
 * This RAM based block device acts as swap disk.
 * Pages swapped to this device are compressed and
 * stored in memory.
 *
 * Released under the terms of GNU General Public License Version 2.0
 *
 * Project home: http://compcache.googlecode.com
 */

#include <linux/version.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/bitops.h>
#include <linux/blkdev.h>
#include <linux/buffer_head.h>
#include <linux/device.h>
#include <linux/genhd.h>
#include <linux/mutex.h>
#include <linux/string.h>
#include <linux/swap.h>
#include <linux/swapops.h>
#include <linux/vmalloc.h>

#define BIT(nr) (1UL << (nr))

#include "compat.h"
#include "ramzswap.h"
#include "lzo-kmod/lzo.h"

/* Globals */
static struct ramzswap rzs;

/* Module params (documentation at end) */
static unsigned long disksize_kb;

static int __init ramzswap_init(void);
static struct block_device_operations ramzswap_devops = {
	.owner = THIS_MODULE,
};

static int test_flag(u32 index, enum rzs_pageflags flag)
{
	return rzs.table[index].flags & BIT(flag);
}

static void set_flag(u32 index, enum rzs_pageflags flag)
{
	rzs.table[index].flags |= BIT(flag);
}

static void clear_flag(u32 index, enum rzs_pageflags flag)
{
	rzs.table[index].flags &= ~BIT(flag);
}

/*
 * Check if request is within bounds and page aligned.
 */
static inline int valid_swap_request(struct bio *bio)
{
	if ( 	(bio->bi_sector >= (rzs.disksize >> SECTOR_SHIFT)) ||
		(bio->bi_sector & (SECTORS_PER_PAGE - 1)) ||
		(bio->bi_vcnt != 1) ||
		(bio->bi_size != PAGE_SIZE) ||
		(bio->bi_io_vec[0].bv_offset != 0)) {

		return 0;
	}

	/* swap request is valid */
	return 1;
}

/*
 * Called when request page is not present in ramzswap.
 * Its either in backing swap device (if present) or
 * this is an attempt to read before any previous write
 * to this location - this happens due to readahead when
 * swap device is read from user-space (e.g. during swapon)
 */
int handle_ramzswap_fault(struct bio *bio)
{
	struct page *page = bio->bi_io_vec[0].bv_page;

	pr_info(C "Read before write on swap device: "
		"sector=%lu, size=%u, offset=%u\n",
		(ulong)(bio->bi_sector), bio->bi_size,
		bio->bi_io_vec[0].bv_offset);
	memset(page_address(page), 0, PAGE_SIZE);

	set_bit(BIO_UPTODATE, &bio->bi_flags);
	BIO_ENDIO(bio, 0);
	return 0;
}

int ramzswap_read(struct bio *bio)
{
	int ret;
	u32 index;
	size_t clen;
	struct page *page;

	page = bio->bi_io_vec[0].bv_page;
	index = bio->bi_sector >> SECTORS_PER_PAGE_SHIFT;

	/* Requested page is not present in compressed area */
	if (!test_flag(index, RZS_INUSE))
		return handle_ramzswap_fault(bio);

	/* Page is stored uncompressed since its incompressible */
	if (test_flag(index, RZS_UNCOMPRESSED)) {
		memcpy(page_address(page), rzs.table[index].pagepos, PAGE_SIZE);
	} else {
		clen = PAGE_SIZE;

		ret = lzo1x_decompress_safe(
			rzs.table[index].pagepos,
			rzs.table[index].size,
			page_address(page), &clen);

		if (ret != LZO_E_OK) {
			pr_info(C "Decompression failed! err=%d, page=%u\n", ret, index);
			goto out;
		}
	}
	//flush_cache_all(); //ok
	__flush_purge_region(page_address(page), PAGE_SIZE); //ok
	__flush_purge_region(rzs.table[index].pagepos, rzs.table[index].size); //ok

	set_bit(BIO_UPTODATE, &bio->bi_flags);
	BIO_ENDIO(bio, 0);
	return 0;
out:
	BIO_IO_ERROR(bio);
	return 0;
}

int ramzswap_write(struct bio *bio)
{
	int ret;
	size_t clen, index;
	struct page *page;
	void *lzopage;

	page = bio->bi_io_vec[0].bv_page;
	index = bio->bi_sector >> SECTORS_PER_PAGE_SHIFT;

	lzopage = rzs.compress_buffer;

	/*
	 * System swaps to same sector again when the stored page
	 * is no longer referenced by any process. So, its now safe
	 * to free the memory that was allocated for this page.
	 */
	if (test_flag(index, RZS_INUSE))
		kfree(rzs.table[index].pagepos);

	clear_flag(index, RZS_INUSE);
	clear_flag(index, RZS_UNCOMPRESSED);

	mutex_lock(&rzs.lock);

	ret = lzo1x_1_compress(page_address(page), PAGE_SIZE, lzopage, &clen, rzs.compress_workmem);

	if (ret != LZO_E_OK) {
		pr_info(C "Compression failed! err=%d\n", ret);
		goto out;
	}

	/*
	 * Page is incompressible. Store it as-is (uncompressed)
	 * since we do not want to return too many swap write
	 * errors which has side effect of hanging the system.
	 */
	if (clen >= PAGE_SIZE) {
		clen = PAGE_SIZE;
		set_flag(index, RZS_UNCOMPRESSED);
		lzopage = page_address(page);
	}

	rzs.table[index].pagepos = kmalloc(clen, GFP_KERNEL);
	if (rzs.table[index].pagepos == NULL) {
		pr_info(C "Error allocating memory");
		goto out;
	}
	memset(rzs.table[index].pagepos, 0, clen);

	set_flag(index, RZS_INUSE);
	rzs.table[index].size = clen;
	memcpy(rzs.table[index].pagepos, lzopage, clen);

	mutex_unlock(&rzs.lock);

	set_bit(BIO_UPTODATE, &bio->bi_flags);
	BIO_ENDIO(bio, 0);
	return 0;
out:
	clear_flag(index, RZS_INUSE);
	clear_flag(index, RZS_UNCOMPRESSED);
	mutex_unlock(&rzs.lock);
	BIO_IO_ERROR(bio);
	return 0;
}

/*
 * Handler function for all ramzswap I/O requests.
 */
static int ramzswap_make_request(struct request_queue *queue, struct bio *bio)
{
	int ret = 0;

	if (!valid_swap_request(bio)) {
		BIO_IO_ERROR(bio);
		return 0;
	}

	switch (bio_data_dir(bio)) {
	case READ:
		ret = ramzswap_read(bio);
		break;

	case WRITE:
		ret = ramzswap_write(bio);
		break;
	}

	return ret;
}

/*
 * Swap header (1st page of swap device) contains information
 * to indentify it as a swap partition. Prepare such a header
 * for ramzswap device (ramzswap0) so that swapon can identify
 * it as swap partition. In case backing swap device is provided,
 * copy its swap header.
 */
static int setup_swap_header(union swap_header *s)
{
	s->info.version = 1;
	s->info.last_page = rzs.disksize >> PAGE_SHIFT;
	s->info.nr_badpages = 0;
	memcpy(s->magic.magic, "SWAPSPACE2", 10);
	return 0;
}

static void ramzswap_set_disksize(size_t totalram_bytes)
{
	rzs.disksize = disksize_kb << 10;

	if (!disksize_kb) {
		pr_info(C
		"disk size not provided. You can use disksize_kb module "
		"param to specify size.\nUsing default: (%u%% of RAM).\n",
		DEFAULT_DISKSIZE_PERC_RAM
		);
		rzs.disksize = DEFAULT_DISKSIZE_PERC_RAM *
					(totalram_bytes / 100);
	}

	if (disksize_kb > 2 * (totalram_bytes >> 10)) {
		pr_info(C
		"There is little point creating a ramzswap of greater than "
		"twice the size of memory since we expect a 2:1 compression "
		"ratio. Note that ramzswap uses about 0.1%% of the size of "
		"the swap device when not in use so a huge ramzswap is "
		"wasteful.\n"
		"\tMemory Size: %zu kB\n"
		"\tSize you selected: %lu kB\n"
		"Continuing anyway ...\n",
		totalram_bytes >> 10, disksize_kb
		);
	}

	rzs.disksize &= PAGE_MASK;
	pr_info(C "disk size set to %zu kB\n", rzs.disksize >> 10);
}

static int __init ramzswap_init(void)
{
	int ret;
	size_t num_pages, totalram_bytes;
	struct sysinfo i;
	void *page;

	mutex_init(&rzs.lock);

	si_meminfo(&i);
	/* Here is a trivia: guess unit used for i.totalram !! */
	totalram_bytes = i.totalram << PAGE_SHIFT;

	ramzswap_set_disksize(totalram_bytes);

	rzs.compress_workmem = kmalloc(LZO1X_MEM_COMPRESS, GFP_KERNEL);
	if (rzs.compress_workmem == NULL) {
		pr_info(C "Error allocating compressor working memory\n");
		ret = -ENOMEM;
		goto fail;
	}
	memset(rzs.compress_workmem, 0, LZO1X_MEM_COMPRESS);

	rzs.compress_buffer = kmalloc(2 * PAGE_SIZE, GFP_KERNEL);
	if (rzs.compress_buffer == NULL) {
		pr_info(C "Error allocating compressor buffer space\n");
		ret = -ENOMEM;
		goto fail;
	}
	memset(rzs.compress_buffer, 0, 2 * PAGE_SIZE);

	num_pages = rzs.disksize >> PAGE_SHIFT;
	rzs.table = vmalloc(num_pages * sizeof(*rzs.table));
	if (rzs.table == NULL) {
		pr_info(C "Error allocating ramzswap address table\n");
		ret = -ENOMEM;
		goto fail;
	}
	memset(rzs.table, 0, num_pages * sizeof(*rzs.table));

	page = kmalloc(PAGE_SIZE, GFP_KERNEL);
	if (page == NULL) {
		pr_info(C "Error allocating swap header page\n");
		ret = -ENOMEM;
		goto fail;
	}
	memset(page, 0, PAGE_SIZE);
	rzs.table[0].pagepos = page;
	set_flag(0, RZS_UNCOMPRESSED);
	set_flag(0, RZS_INUSE);

	ret = setup_swap_header((union swap_header *)(rzs.table[0].pagepos));
	if (ret) {
		pr_info(C "Error setting swap header\n");
		goto fail;
	}

	rzs.disk = alloc_disk(1);
	if (rzs.disk == NULL) {
		pr_info(C "Error allocating disk structure\n");
		ret = -ENOMEM;
		goto fail;
	}

	rzs.disk->first_minor = 0;
	rzs.disk->fops = &ramzswap_devops;
	/*
	 * It is named like this to prevent distro installers
	 * from offering ramzswap as installation target. They
	 * seem to ignore all devices beginning with 'ram'
	 */
	strcpy(rzs.disk->disk_name, "ramzswap0");

	rzs.disk->major = register_blkdev(0, rzs.disk->disk_name);
	if (rzs.disk->major < 0) {
		pr_info(C "Cannot register block device\n");
		ret = -EFAULT;
		goto fail;
	}

	rzs.disk->queue = blk_alloc_queue(GFP_KERNEL);
	if (rzs.disk->queue == NULL) {
		pr_info(C "Cannot register disk queue\n");
		ret = -EFAULT;
		goto fail;
	}

	set_capacity(rzs.disk, rzs.disksize >> SECTOR_SHIFT);
	blk_queue_make_request(rzs.disk->queue, ramzswap_make_request);

#ifdef QUEUE_FLAG_NONROT
	/*
	 * Assuming backing device is "rotational" type.
	 * TODO: check if its actually "non-rotational" (SSD).
	 *
	 * We have ident mapping of sectors for ramzswap and
	 * and the backing swap device. So, this queue flag
	 * should be according to backing dev.
	 */
	queue_flag_set_unlocked(QUEUE_FLAG_NONROT, rzs.disk->queue);
#endif
#ifdef SWAP_DISCARD_SUPPORTED
	blk_queue_set_discard(rzs.disk->queue, ramzswap_prepare_discard);
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
	blk_queue_hardsect_size(rzs.disk->queue, PAGE_SIZE);
#else
	blk_queue_logical_block_size(rzs.disk->queue, PAGE_SIZE);
#endif
	add_disk(rzs.disk);

	pr_info(C "Initialization done!\n");
	return 0;

fail:
	if (rzs.disk != NULL) {
		if (rzs.disk->major > 0)
			unregister_blkdev(rzs.disk->major, rzs.disk->disk_name);
		del_gendisk(rzs.disk);
	}

	if (test_flag(0, RZS_INUSE))
		kfree(rzs.table[0].pagepos);
	kfree(rzs.compress_workmem);
	kfree(rzs.compress_buffer);
	vfree(rzs.table);

	pr_info(C "Initialization failed: err=%d\n", ret);
	return ret;
}

static void __exit ramzswap_exit(void)
{
	size_t index, num_pages;
	num_pages = rzs.disksize >> PAGE_SHIFT;

	unregister_blkdev(rzs.disk->major, rzs.disk->disk_name);
	del_gendisk(rzs.disk);

	kfree(rzs.compress_workmem);
	kfree(rzs.compress_buffer);

	/* Free all pages that are still in ramzswap */
	for (index = 0; index < num_pages; index++) {

		if (test_flag(index, RZS_INUSE))
			kfree(rzs.table[index].pagepos);
	}

	vfree(rzs.table);
	pr_info(C "cleanup done!\n");
}

/*
 * This param is applicable only when there is no backing swap device.
 * We ignore this param in case backing dev is provided since then its
 * always equal to size of the backing swap device.
 *
 * This size refers to amount of (uncompressed) data it can hold.
 * For e.g. disksize_kb=1024 means it can hold 1024kb worth of
 * uncompressed data even if this data compresses to just, say, 100kb.
 *
 * Default value is used if this param is missing or 0 (if its applicable).
 * Default: [DEFAULT_DISKSIZE_PERC_RAM]% of RAM
 */
module_param(disksize_kb, ulong, 0);
MODULE_PARM_DESC(disksize_kb, "ramzswap device size (kB)");

module_init(ramzswap_init);
module_exit(ramzswap_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nitin Gupta <ngupta@vflare.org>");
MODULE_DESCRIPTION("Compressed RAM Based Swap Device");
