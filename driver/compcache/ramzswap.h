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

#ifndef _RAMZSWAP_H_
#define _RAMZSWAP_H_

/*-- Configurable parameters */

/* Default ramzswap disk size: 25% of total RAM */
#define DEFAULT_DISKSIZE_PERC_RAM	25
#define DEFAULT_MEMLIMIT_PERC_RAM	15

/*-- End of configurable params */

#define SECTOR_SHIFT		9
#define SECTOR_SIZE		(1 << SECTOR_SHIFT)
#define SECTORS_PER_PAGE_SHIFT	(PAGE_SHIFT - SECTOR_SHIFT)
#define SECTORS_PER_PAGE	(1 << SECTORS_PER_PAGE_SHIFT)

/* Message prefix */
#define C "ramzswap: "

/* Flags for ramzswap pages (table[page_no].flags) */
enum rzs_pageflags {
	RZS_UNCOMPRESSED,
	RZS_INUSE,
	__NR_RZS_PAGEFLAGS,
};

/*-- Data structures */

/* Indexed by page no. */
struct table {
	void *pagepos;
	u8 flags;
	size_t size;
};

struct ramzswap {
	void *compress_workmem;
	void *compress_buffer;
	struct table *table;
	struct mutex lock;
	struct gendisk *disk;
	size_t disksize;	/* bytes */
};

#endif
