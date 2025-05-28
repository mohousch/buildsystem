/*
 * buildimage - create a Dreambox NAND boot image
 *
 * contains algorithms ripped from u-boot and first-stage.
 *
 * Copyright (C) 2000-2004 Steven J. Hill (sjhill@realitydiluted.com)
 *                         Toshiba America Electronics Components, Inc.
 *
 * Copyright (C) 2006 Thomas Gleixner <tglx@linutronix.de>
 *
 * Copyright (C) 2005-2009 Felix Domke <tmbinc@elitedvb.net>
 * Copyright (C) 2010-2011 Andreas Oberritter <obi@opendreambox.org>
 *
 * This file is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#ifdef HAVE_CONFIG_H
#include "buildimage_config.h"
#endif

#include <errno.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

typedef void (*fnc_encode_ecc)(unsigned char *dst, const unsigned char *src, unsigned int cnt);

struct partition {
	struct partition *next;
	fnc_encode_ecc encode;
	unsigned int id;
	const char *filename;
	FILE *file;
	size_t part_size;
	size_t data_size;
	size_t sectors;
};

/*
 * Pre-calculated 256-way 1 byte column parity
 */
static const unsigned char nand_ecc_precalc_table[] = {
	0x00, 0x55, 0x56, 0x03, 0x59, 0x0c, 0x0f, 0x5a, 0x5a, 0x0f, 0x0c, 0x59, 0x03, 0x56, 0x55, 0x00,
	0x65, 0x30, 0x33, 0x66, 0x3c, 0x69, 0x6a, 0x3f, 0x3f, 0x6a, 0x69, 0x3c, 0x66, 0x33, 0x30, 0x65,
	0x66, 0x33, 0x30, 0x65, 0x3f, 0x6a, 0x69, 0x3c, 0x3c, 0x69, 0x6a, 0x3f, 0x65, 0x30, 0x33, 0x66,
	0x03, 0x56, 0x55, 0x00, 0x5a, 0x0f, 0x0c, 0x59, 0x59, 0x0c, 0x0f, 0x5a, 0x00, 0x55, 0x56, 0x03,
	0x69, 0x3c, 0x3f, 0x6a, 0x30, 0x65, 0x66, 0x33, 0x33, 0x66, 0x65, 0x30, 0x6a, 0x3f, 0x3c, 0x69,
	0x0c, 0x59, 0x5a, 0x0f, 0x55, 0x00, 0x03, 0x56, 0x56, 0x03, 0x00, 0x55, 0x0f, 0x5a, 0x59, 0x0c,
	0x0f, 0x5a, 0x59, 0x0c, 0x56, 0x03, 0x00, 0x55, 0x55, 0x00, 0x03, 0x56, 0x0c, 0x59, 0x5a, 0x0f,
	0x6a, 0x3f, 0x3c, 0x69, 0x33, 0x66, 0x65, 0x30, 0x30, 0x65, 0x66, 0x33, 0x69, 0x3c, 0x3f, 0x6a,
	0x6a, 0x3f, 0x3c, 0x69, 0x33, 0x66, 0x65, 0x30, 0x30, 0x65, 0x66, 0x33, 0x69, 0x3c, 0x3f, 0x6a,
	0x0f, 0x5a, 0x59, 0x0c, 0x56, 0x03, 0x00, 0x55, 0x55, 0x00, 0x03, 0x56, 0x0c, 0x59, 0x5a, 0x0f,
	0x0c, 0x59, 0x5a, 0x0f, 0x55, 0x00, 0x03, 0x56, 0x56, 0x03, 0x00, 0x55, 0x0f, 0x5a, 0x59, 0x0c,
	0x69, 0x3c, 0x3f, 0x6a, 0x30, 0x65, 0x66, 0x33, 0x33, 0x66, 0x65, 0x30, 0x6a, 0x3f, 0x3c, 0x69,
	0x03, 0x56, 0x55, 0x00, 0x5a, 0x0f, 0x0c, 0x59, 0x59, 0x0c, 0x0f, 0x5a, 0x00, 0x55, 0x56, 0x03,
	0x66, 0x33, 0x30, 0x65, 0x3f, 0x6a, 0x69, 0x3c, 0x3c, 0x69, 0x6a, 0x3f, 0x65, 0x30, 0x33, 0x66,
	0x65, 0x30, 0x33, 0x66, 0x3c, 0x69, 0x6a, 0x3f, 0x3f, 0x6a, 0x69, 0x3c, 0x66, 0x33, 0x30, 0x65,
	0x00, 0x55, 0x56, 0x03, 0x59, 0x0c, 0x0f, 0x5a, 0x5a, 0x0f, 0x0c, 0x59, 0x03, 0x56, 0x55, 0x00,
};

static const struct option options[] = {
	{ "brcmnand", no_argument, NULL, 'B' },
	{ "hw-ecc", no_argument, NULL, 'H' },
	{ "arch", required_argument, NULL, 'a' },
	{ "boot-partition", required_argument, NULL, 'b' },
	{ "data-partition", required_argument, NULL, 'd' },
	{ "erase-block-size", required_argument, NULL, 'e' },
	{ "flash-size", required_argument, NULL, 'f' },
	{ "help", no_argument, NULL, 'h' },
	{ "raw", no_argument, NULL, 'r' },
	{ "sector-size", required_argument, NULL, 's' },
	{ NULL, 0, NULL, 0 },
};

static struct partition *partitions;
static unsigned int num_partitions;
static size_t erase_block_size, spare_size, sector_size, flash_size;
static bool broadcom_nand;
static bool hw_ecc;
static bool large_page;
static bool raw;

/* reserve to two sectors plus 1% for badblocks, and round down */
static inline size_t badblock_safe(size_t x)
{
	return x ? (x - (erase_block_size * 2) - (x / 100)) & ~erase_block_size : 0;
}

static inline size_t sector_size_with_ecc(void)
{
	return sector_size + spare_size;
}

static inline size_t to_sect(size_t x)
{
	return (x + sector_size - 1) / sector_size;
}

/*
 * Creates non-inverted ECC code from line parity
 */
static void nand_trans_result(unsigned char reg2, unsigned char reg3,
	unsigned char *ecc_code)
{
	unsigned char a, b, i, tmp1, tmp2;

	/* Initialize variables */
	a = b = 0x80;
	tmp1 = tmp2 = 0;

	/* Calculate first ECC byte */
	for (i = 0; i < 4; i++) {
		if (reg3 & a)		/* LP15,13,11,9 --> ecc_code[0] */
			tmp1 |= b;
		b >>= 1;
		if (reg2 & a)		/* LP14,12,10,8 --> ecc_code[0] */
			tmp1 |= b;
		b >>= 1;
		a >>= 1;
	}

	/* Calculate second ECC byte */
	b = 0x80;
	for (i = 0; i < 4; i++) {
		if (reg3 & a)		/* LP7,5,3,1 --> ecc_code[1] */
			tmp2 |= b;
		b >>= 1;
		if (reg2 & a)		/* LP6,4,2,0 --> ecc_code[1] */
			tmp2 |= b;
		b >>= 1;
		a >>= 1;
	}

	/* Store two of the ECC bytes */
	ecc_code[0] = tmp1;
	ecc_code[1] = tmp2;
}

/*
 * Calculate 3 byte ECC code for 256 byte block
 */
static void nand_calculate_ecc(const unsigned char *dat, unsigned char *ecc_code)
{
	unsigned char idx, reg1, reg3;
	int j;

	/* Initialize variables */
	reg1 = reg3 = 0;
	ecc_code[0] = ecc_code[1] = ecc_code[2] = 0;

	/* Build up column parity */
	for(j = 0; j < 256; j++) {
		/* Get CP0 - CP5 from table */
		idx = nand_ecc_precalc_table[dat[j]];
		reg1 ^= idx;

		/* All bit XOR = 1 ? */
		if (idx & 0x40)
			reg3 ^= (unsigned char) j;
	}

	/* Create non-inverted ECC code from line parity */
	nand_trans_result((reg1 & 0x40) ? ~reg3 : reg3, reg3, ecc_code);

	/* Calculate final ECC code */
	ecc_code[0] = ~ecc_code[0];
	ecc_code[1] = ~ecc_code[1];
	ecc_code[2] = ((~reg1) << 2) | 0x03;
}

static bool safe_write(int fd, const unsigned char *buf, size_t count)
{
	size_t pos = 0;
	ssize_t ret;

	while (pos < count) {
		errno = 0;
		ret = write(fd, &buf[pos], count - pos);
		if (ret < 0) {
			if (errno != EINTR)
				return false;
		} else {
			pos += ret;
		}
	}

	return true;
}

static bool emit_4(unsigned int val)
{
	unsigned char buf[4];

	/* don't output headers in raw mode */
	if (raw)
		return true;

	buf[0] = (val >> 24) & 0xff;
	buf[1] = (val >> 16) & 0xff;
	buf[2] = (val >>  8) & 0xff;
	buf[3] = (val >>  0) & 0xff;

	return safe_write(1, buf, 4);
}

static void encode_hevers(unsigned char *dst, const unsigned char *src, unsigned int count)
{
	if (!large_page) {
		dst[0] = count >> 8;
		dst[1] = count & 0xFF;
		if (!hw_ecc) {
			unsigned char temp;
			size_t cnt;
			for (cnt = 0; cnt < sector_size; cnt++) {
				temp = src[cnt];
				dst[2] ^= temp;
				if (cnt & 1)
					dst[6 + 0] ^= temp;
				else
					dst[6 + 1] ^= temp;
				if (cnt & 2) dst[6 + 2] ^= temp;
				if (cnt & 4) dst[6 + 3] ^= temp;
				if (cnt & 8) dst[6 + 4] ^= temp;
				if (cnt & 16) dst[6 + 5] ^= temp;
				if (cnt & 32) dst[6 + 6] ^= temp;
				if (cnt & 64) dst[6 + 7] ^= temp;
				if (cnt & 128) dst[6 + 8] ^= temp;
				if (cnt & 256) dst[6 + 9] ^= temp;
			}
		}
	} else {
		dst[2] = count >> 8;
		dst[3] = count & 0xFF;
		if (!hw_ecc) {
			unsigned char temp;
			size_t cnt;
			for (cnt = 0; cnt < sector_size; cnt++) {
				temp = src[cnt];
				dst[40] ^= temp;
				if (cnt & 1)
					dst[41] ^= temp;
				else
					dst[42] ^= temp;
				if (cnt & 2) dst[43] ^= temp;
				if (cnt & 4) dst[44] ^= temp;
				if (cnt & 8) dst[45] ^= temp;
				if (cnt & 16) dst[46] ^= temp;
				if (cnt & 32) dst[47] ^= temp;
				if (cnt & 64) dst[48] ^= temp;
				if (cnt & 128) dst[49] ^= temp;
				if (cnt & 256) dst[50] ^= temp;
			}
		}
	}
}

static void encode_jffs2(unsigned char *dst, const unsigned char *src, unsigned int cnt)
{
	if (broadcom_nand) // hamming (broadcom slc nand hw ecc)
	{
		if (!(cnt & ((erase_block_size/sector_size)-1)))
		{
			if (large_page)
			{
				/* 0,1 is badblock indication
				   6,7,8 is hw ecc
				*/
				dst[2]  = 0x85;
				dst[3]  = 0x19;
				dst[4] = 0x03;
				dst[5] = 0x20;
				dst[9] = 0x08;
				dst[10] = 0x00;
				dst[11] = 0x00;
				dst[12] = 0x00;
			}
			else
			{
				/* 5 is badblock indication
				   6,7,8 is hw ecc
				*/
				dst[0]  = 0x85;
				dst[1]  = 0x19;
				dst[2] = 0x03;
				dst[3] = 0x20;
				dst[4] = 0x08;
				dst[9] = 0x00;
				dst[10] = 0x00;
				dst[11] = 0x00;
			}
		}
	}
	else if (!large_page)
	{
		unsigned char ecc_code[8];
		nand_calculate_ecc (src, ecc_code);
		nand_calculate_ecc (src+256, ecc_code+3);
		dst[0] = ecc_code[0];
		dst[1] = ecc_code[1];
		dst[2] = ecc_code[2];
		dst[3] = ecc_code[3];
		dst[4] = 0xFF;
		dst[5] = 0xFF;
		dst[6] = ecc_code[4];
		dst[7] = ecc_code[5];

		if (!(cnt & ((erase_block_size/sector_size)-1)))
		{
			dst[8]  = 0x85;
			dst[9]  = 0x19;
			dst[10] = 0x03;
			dst[11] = 0x20;
			dst[12] = 0x08;
			dst[13] = 0x00;
			dst[14] = 0x00;
			dst[15] = 0x00;
		}
	} else
	{
		int i;
		for (i=0; i<8; ++i)
			nand_calculate_ecc (src + i * 256, dst + 40 + i * 3);

		if (!(cnt & ((erase_block_size/sector_size)-1)))
		{
			dst[2] = 0x85;
			dst[3] = 0x19;
			dst[4] = 0x03;
			dst[5] = 0x20;
			dst[6] = 0x08;
			dst[7] = 0x00;
			dst[8] = 0x00;
			dst[9] = 0x00;
		}
	}
}

static bool emit_partition(struct partition *p)
{
	unsigned int cnt;

	if (!emit_4(to_sect(p->data_size) * sector_size_with_ecc())) {
		fprintf(stderr, "Couldn't write partition size\n");
		return false;
	}

	for (cnt = 0; cnt < p->sectors; cnt++) {
		unsigned char sector[sector_size + spare_size];
		memset(sector, 0xFF, sector_size + spare_size);
		int r = fread(sector, 1, sector_size, p->file);
		if (!r)
			break;
		p->encode(sector + sector_size, sector, cnt);
		if (!safe_write(1, sector, sector_size_with_ecc())) {
			fprintf(stderr, "Couldn't write sector\n");
			return false;
		}
	}

	return true;
}

static bool partition_append(const char *option, fnc_encode_ecc encode)
{
	const char *filename;
	struct partition *p;
	size_t val;
	struct stat st;
	FILE *f;
	char *end;

	errno = 0;
	val = strtoull(option, &end, 0);
	if ((errno != 0) || (end == NULL) || (*end != ':'))
		return false;

	filename = (end + 1);
	if (stat(filename, &st) < 0) {
		perror(filename);
		return false;
	}

	fprintf(stderr, "Partition #%u: %llu of %llu bytes (%s)\n",
			num_partitions, (unsigned long long)st.st_size,
			(unsigned long long)badblock_safe(val), filename);
	if (st.st_size > badblock_safe(val)) {
		fprintf(stderr, "Partition #%u (%s) is too big. This doesn't work. Sorry.", num_partitions, filename);
		return false;
	}

	f = fopen(filename, "r");
	if (f == NULL) {
		perror(filename);
		return false;
	}

	p = malloc(sizeof(struct partition));
	if (p == NULL) {
		perror("malloc");
		fclose(f);
		return false;
	}

	p->id = num_partitions++;
	p->encode = encode;
	p->filename = filename;
	p->part_size = val;
	p->data_size = st.st_size;
	p->sectors = to_sect(st.st_size);
	p->file = f;

	p->next = NULL;
	if (partitions == NULL) {
		partitions = p;
	} else {
		struct partition *q = partitions;
		while (q->next)
			q = q->next;
		q->next = p;
	}

	return true;
}

static bool parse_size(const char *option, size_t *size)
{
	errno = 0;
	*size = strtoull(optarg, NULL, 0);
	return (errno == 0);
}

static const char usage[] =
"Usage: buildimage <parameters> [...]\n"
"\n"
"  Mandatory parameters:\n"
"    -a STRING      --arch=STRING\n"
"    -e SIZE        --erase-block-size=SIZE\n"
"    -s SIZE        --sector-size=SIZE\n"
"\n"
"  Optional parameters\n"
"    -b SIZE:FILE   --boot-partition=SIZE:FILE\n"
"    -d SIZE:FILE   --data-partition=SIZE:FILE\n"
"    -f SIZE        --flash-size=SIZE\n"
"    -r             --raw\n"
"    -B             --brcmnand\n"
"    -H             --hw-ecc (for boot partition)\n"
"\n"
"  buildimage -a dm8000 -e 0x20000 -f 0x4000000 -s 2048 \\\n"
"             -b 0x100000:secondstage-dm8000.bin -d 0x300000:boot.jffs2 \\\n"
"             -d 0x3C00000:rootfs.jffs2 > dreambox-image-dm8000.nfi\n"
"\n";

int main(int argc, char *argv[])
{
	struct partition *p;
	const char *arch = NULL;
	int opt;

	while ((opt = getopt_long(argc, argv, "BHa:b:d:e:f:ho:rs:", options, NULL)) != -1) {
		switch (opt) {
		case 'B':
			broadcom_nand = true;
			break;
		case 'H':
			hw_ecc = true;
			break;
		case 'a':
			if (strlen(optarg) > 27) {
				fprintf(stderr, "Invalid arch!\n");
				return EXIT_FAILURE;
			}
			arch = optarg;
			break;
		case 'b':
			if (!partition_append(optarg, encode_hevers)) {
				fprintf(stderr, "Invalid boot partition!\n");
				return EXIT_FAILURE;
			}
			break;
		case 'd':
			if (!partition_append(optarg, encode_jffs2)) {
				fprintf(stderr, "Invalid data partition!\n");
				return EXIT_FAILURE;
			}
			break;
		case 'e':
			/* minimum: 16 KiB */
			if (!parse_size(optarg, &erase_block_size) || (erase_block_size & 0x3fff)) {
				fprintf(stderr, "Invalid erase block size!\n");
				return EXIT_FAILURE;
			}
			break;
		case 'f':
			/* minimum: 32 MiB */
			if (!parse_size(optarg, &flash_size) || (flash_size & 0x1ffffff)) {
				fprintf(stderr, "Invalid flash size!\n");
				return EXIT_FAILURE;
			}
			break;
		case 'h':
			fprintf(stdout, usage);
			return EXIT_SUCCESS;
		case 'r':
			raw = true;
			break;
		case 's':
			/* minimum: 512 bytes */
			if (!parse_size(optarg, &sector_size) || (sector_size & 0x1ff)) {
				fprintf(stderr, "Invalid sector size!\n");
				return EXIT_FAILURE;
			}
			break;
		default:
			fprintf(stderr, usage);
			return EXIT_FAILURE;
		}
	}

	if ((optind < argc) ||
	    (erase_block_size == 0) ||
	    (sector_size == 0)) {
		fprintf(stderr, usage);
		return EXIT_FAILURE;
	}

	if (sector_size > 512)
		large_page = true;

	spare_size = sector_size / 32;

	/* write NFI1/2 header */
	if (!raw && arch != NULL) {
		char header[32];
		memset(header, 0, 32);
		strcpy(header, "NFI1");

		/* This is what the bootloader expects */
		if (!strcmp(arch, "dm7020hdv2"))
			arch = "dm7020hd";

		/* DM7020HD with 128K eraseblock size nand flash needs NFI3 header */
		if (erase_block_size == 128*1024 && sector_size == 2*1024 && !strcmp(arch, "dm7020hd"))
			header[3] = '3';
		else if (broadcom_nand)
			header[3] = '2';

		strncpy(header + 4, arch, 28);

		if (erase_block_size == 128*1024 && sector_size == 2*1024)
			header[31] |= 1; // all images with this marker are ready for v2 boxes with toshiba nand flash (needs new kernel and new 2nd stage)

		if (!safe_write(1, header, 32)) {
			fprintf(stderr, "Couldn't write NFI header!\n");
			return EXIT_FAILURE;
		}
	}

	unsigned int total_size = 4 + num_partitions * 4;
	for (p = partitions; p != NULL; p = p->next)
		total_size += 4 + p->sectors * sector_size_with_ecc();

	/* global header */
	if (!emit_4(total_size)) {
		fprintf(stderr, "Couldn't write total size\n");
		return EXIT_FAILURE;
	}

	/* partition */
	if (!emit_4(num_partitions * 4)) {
		fprintf(stderr, "Couldn't write number of partitions\n");
		return EXIT_FAILURE;
	}

	unsigned int endptr = 0;
	for (p = partitions; p != NULL; p = p->next) {
		endptr += p->part_size;
		if (!emit_4(p->part_size ? endptr : 0)) {
			fprintf(stderr, "Couldn't write partition pointer\n");
			return EXIT_FAILURE;
		}
	}

	for (p = partitions; p != NULL; p = p->next)
		if (!emit_partition(p))
			return EXIT_FAILURE;

	return EXIT_SUCCESS;
}
