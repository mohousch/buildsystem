/*
 * eeprom.c
 *
 * (c) 2011 konfetti
 * partly copied from uboot source!
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

/*
 * Description:
 *
 * _ATTENTION_: Many thinks are untested in this module. Use on
 * your own risk
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/fs.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define CONFIG_CUBEREVO

#define N_TVMODE	6
char *tvmode_name[N_TVMODE] =
{
	"SD-PAL",	"SD-NTSC",
	"720P-50", 	"720P-60",
	"1080I-50",	"1080I-60"
};


typedef enum
{
	db_key_null = 0,
	db_key_end,
	db_key_ethaddr,
	db_key_prodnum,
	db_key_vol,
	db_key_lch,
	db_key_pcbver,
	db_key_tvmode,
	db_key_tuner,
	db_key_time,
} db_key;

typedef struct
{
	unsigned short key;
	unsigned short len;
	unsigned char data[0];
} db_item;

static struct
{
	db_key key;
	char   *name;
	int    isRO;
} db_name_tbl[] =
{
	{ db_key_null,		"null"    , /*1 */ 0},
	{ db_key_end,		"end"     , /*1 */ 0},
	{ db_key_ethaddr,	"ethaddr" , 0},
	{ db_key_prodnum,	"prodnum" , /*1 */ 0},
	{ db_key_vol,		"volume"  , 0},
	{ db_key_lch,		"lastch"  , 0},
	{ db_key_pcbver,	"pcbver"  , /*1 */ 0},
#if defined(CONFIG_CUBEREVO) || defined(CONFIG_CUBEREVO_MINI)
	{ db_key_tvmode,	"tvmode"  , 0},
	{ db_key_tuner,		"tuner"   , /*1 */ 0},
	{ db_key_time,		"time"    , /*1 */ 0},
#endif
};

#define CFG_EEPROM_ADD 0x50
#define CFG_EEPROM_SIZE 512

#define db_name_tbl_size	(sizeof(db_name_tbl)/sizeof(db_name_tbl[0]))

#define DB_MAGIC_SIZE		sizeof(unsigned short)
#define DB_HEADE_SIZE		sizeof(db_item)

#define CFG_EEPROM_PAGE_WRITE_BITS 4
#define CFG_EEPROM_PAGE_WRITE_DELAY_MS	11	/* 10ms. but give more */

#define	EEPROM_PAGE_SIZE	(1 << CFG_EEPROM_PAGE_WRITE_BITS)
#define	EEPROM_PAGE_OFFSET(x)	((x) & (EEPROM_PAGE_SIZE - 1))

/* ********************************* i2c ************************************ */
/*
 * I2C Message - used for pure i2c transaction, also from /dev interface
 */
struct i2c_msg
{
	unsigned short addr;		/* slave address			*/
	unsigned short flags;
	unsigned short len;		/* msg length				*/
	unsigned char  *buf;		/* pointer to msg data			*/
};

/* This is the structure as used in the I2C_RDWR ioctl call */
struct i2c_rdwr_ioctl_data
{
	struct i2c_msg *msgs;	/* pointers to i2c_msgs */
	unsigned int nmsgs;			/* number of i2c_msgs */
};

#define I2C_SLAVE	0x0703	/* Change slave address			*/
/* Attn.: Slave address is 7 or 10 bits */

#define I2C_RDWR	0x0707	/* Combined R/W transfer (one stop only)*/

int i2c_read(int fd_i2c, unsigned char addr, unsigned char reg, unsigned char *buffer, int len)
{
	struct 	i2c_rdwr_ioctl_data i2c_rdwr;
	int	err;
	unsigned char b0[] = { reg };

	//printf("%s> 0x%0x - %d\n", __func__, reg, len);

	i2c_rdwr.nmsgs = 2;
	i2c_rdwr.msgs = malloc(2 * sizeof(struct i2c_msg));
	i2c_rdwr.msgs[0].addr = addr;
	i2c_rdwr.msgs[0].flags = 0;
	i2c_rdwr.msgs[0].len = 1;
	i2c_rdwr.msgs[0].buf = b0;

	i2c_rdwr.msgs[1].addr = addr;
	i2c_rdwr.msgs[1].flags = 1;
	i2c_rdwr.msgs[1].len = len;
	i2c_rdwr.msgs[1].buf = malloc(len);

	memset(i2c_rdwr.msgs[1].buf, 0, len);

	if ((err = ioctl(fd_i2c, I2C_RDWR, &i2c_rdwr)) < 0)
	{
		//printf("i2c_read failed %d %d\n", err, errno);
		//printf("%s\n", strerror(errno));
		free(i2c_rdwr.msgs[0].buf);
		free(i2c_rdwr.msgs);

		return -1;
	}

	//printf("reg 0x%02x ->ret 0x%02x\n", reg, (i2c_rdwr.msgs[1].buf[0] & 0xff));
	memcpy(buffer, i2c_rdwr.msgs[1].buf, len);

	free(i2c_rdwr.msgs[1].buf);
	free(i2c_rdwr.msgs);

//    printf("%s<\n", __func__);
	return 0;
}

int i2c_write(int fd_i2c, unsigned char addr, unsigned char reg, unsigned char *buffer, int len)
{
	struct 	i2c_rdwr_ioctl_data i2c_rdwr;
	int	err;
	unsigned char buf[256];

	//printf("%s> 0x%0x - %s - %d\n", __func__, reg, buffer, len);

	buf[0] = reg;
	memcpy(&buf[1], buffer, len);

	i2c_rdwr.nmsgs = 1;
	i2c_rdwr.msgs = malloc(1 * sizeof(struct i2c_msg));
	i2c_rdwr.msgs[0].addr = addr;
	i2c_rdwr.msgs[0].flags = 0;
	i2c_rdwr.msgs[0].len = len + 1;
	i2c_rdwr.msgs[0].buf = buf;

	if ((err = ioctl(fd_i2c, I2C_RDWR, &i2c_rdwr)) < 0)
	{
		//printf("i2c_read failed %d %d\n", err, errno);
		//printf("%s\n", strerror(errno));
		free(i2c_rdwr.msgs[0].buf);
		free(i2c_rdwr.msgs);

		return -1;
	}

	free(i2c_rdwr.msgs);

//    printf("%s<\n", __func__);
	return 0;
}

/* *************************** uboot copied func ************************************** */

int isRO(const char *name)
{
	int a;

	for (a = 0; a < db_name_tbl_size; a++)
	{
		if (!strcmp(db_name_tbl[a].name, name))
		{
			return db_name_tbl[a].isRO;
		}
	}

	return 1;
}

int get_keyvalue(const char *name, db_key *key)
{
	int a;

	for (a = 0; a < db_name_tbl_size; a++)
	{
		if (!strcmp(db_name_tbl[a].name, name))
		{
			*key = db_name_tbl[a].key;
			return 0;
		}
	}

	return 1;
}

static int get_keyname(db_key key, char **name)
{
	int a;

	for (a = 0; a < db_name_tbl_size; a++)
	{
		if (db_name_tbl[a].key == key)
		{
			*name = db_name_tbl[a].name;
			return 0;
		}
	}

	//printf("unknown key %d\n", key);
	*name = "unknown";

	return 1;
}

int eeprom_write(int fd, unsigned dev_addr, unsigned offset, unsigned char *buffer, unsigned cnt)
{
	unsigned end = offset + cnt;
	unsigned blk_off;
	int rcode = 0;

	/* Write data until done or would cross a write page boundary.
	 * We must write the address again when changing pages
	 * because the address counter only increments within a page.
	 */

	while (offset < end)
	{
		unsigned alen, len, maxlen;
		unsigned char addr[2];

		blk_off = offset & 0xFF;	/* block offset */

		addr[0] = offset >> 8;		/* block number */
		addr[1] = blk_off;		/* block offset */
		alen	= 2;
		addr[0] |= dev_addr;		/* insert device address */

		maxlen = EEPROM_PAGE_SIZE - EEPROM_PAGE_OFFSET(blk_off);

		len = end - offset;

		if (i2c_write(fd, addr[0], offset, buffer, len) != 0)
			rcode = 1;

		buffer += len;
		offset += len;

		usleep(CFG_EEPROM_PAGE_WRITE_DELAY_MS * 1000);
	}
	return rcode;
}

int eeprom_read(int fd, unsigned dev_addr, unsigned offset, unsigned char *buffer, unsigned cnt)
{
	unsigned end = offset + cnt;
	unsigned blk_off;
	int rcode = 0;
	int i;

	//printf("%s> offset %d cnt %d\n", __func__, offset, cnt);

	while (offset < end)
	{
		unsigned len, maxlen;
		unsigned char addr[2];

		blk_off = offset & 0xFF;	/* block offset */

		addr[0] = offset >> 8;		/* block number */
		addr[1] = blk_off;		/* block offset */
		addr[0] |= dev_addr;		/* insert device address */

		len = cnt;

		for (i = 0; i < len; i++)
		{
			if (i2c_read(fd, addr[0], offset + i, &buffer[i], 1) != 0)
			{
				rcode = 1;
				break;
			}
		}

		buffer += len;
		offset += len;
	}
	//printf("%s< %d\n", __func__, rcode);
	return rcode;
}

static int read_item(int fd, int offset, db_key *key, unsigned char *buf, int *buflen)
{
	int rcode;
	db_item item;

	//printf("%s>\n", __func__);

	if (offset >= CFG_EEPROM_SIZE - DB_HEADE_SIZE)
		return -1;

	rcode = eeprom_read(fd, CFG_EEPROM_ADD, offset, (unsigned char *)&item, sizeof(item));
	if (rcode)
		return -1;

	//printf("item.len %d\n", item.len);

	if (item.len > 0 && buf != NULL)
	{
		if (*buflen > item.len)
			*buflen = item.len;

		rcode = eeprom_read(fd, CFG_EEPROM_ADD, offset + DB_HEADE_SIZE, buf, *buflen);
		if (rcode)
			return -1;

		buf[*buflen] = 0;
	}

	*key = item.key;

	//printf("%s< %d\n", __func__, item.len);
	return item.len;
}

int search_item(int fd, int offset, db_key key, unsigned char *buf, unsigned char *buflen, int *offret, int *lenret)
{
	int rcode;
	int len;
	db_key keytmp;

	//printf("key %d\n", key);

	do
	{
		len = read_item(fd,  offset, &keytmp, NULL, NULL);
		if (len < 0)
			return 1;

		if (keytmp == key)
		{
			/* found it */
			if (offret != NULL)
				*offret = offset;
			if (lenret != NULL)
				*lenret = len;

			if (len > 0 && buf != NULL)
			{
				if (*buflen > len)
					*buflen = len;

				rcode = eeprom_read(
						fd,
						CFG_EEPROM_ADD,
						offset + DB_HEADE_SIZE,
						buf,
						*buflen);
				if (rcode)
					return 1;

				buf[*buflen] = 0;
			}

			return 0;
		}

		offset += DB_HEADE_SIZE + len;
	}
	while (keytmp != db_key_end);

	/* we coundn`t find the key.
	 * return the end key
	 */
	if (offret != NULL)
		*offret = offset - (DB_HEADE_SIZE + len);
	if (lenret != NULL)
		*lenret = len;

	return 2;
}

static int save_item(int fd, int offset, db_key key, const char *buf, int len)
{
	int rcode;
	db_item item;

	//printf("%s> %d - %s\n", __func__, key, buf);

	/* write item data */
	if (buf != NULL && len != 0)
	{

		rcode = eeprom_write(
				fd,
				CFG_EEPROM_ADD,
				offset + DB_HEADE_SIZE,
				(unsigned char *)buf,
				len);

		if (rcode)
			return rcode;
	}

	/* write item header */
	item.key = key;
	item.len = len;
	rcode = eeprom_write(fd, CFG_EEPROM_ADD, offset, (unsigned char *)&item, sizeof(item));

	//printf("%s< rcode %d\n", __func__, rcode);
	return rcode;
}

static int del_item(int fd, db_key key)
{
	int offset;
	db_key keytmp;
	int len;

	int last_offset;
	db_key last_keytmp;
	int last_len;

	int next_offset;
	db_key next_keytmp;
	int next_len;

	/* find the item */
	offset = DB_MAGIC_SIZE;
	last_offset = 0;
	last_keytmp = db_key_end;
	last_len = 0;
	do
	{
		len = read_item(fd, offset, &keytmp, NULL, NULL);
		if (len < 0)
			return -1;

		if (keytmp == key)
		{
			/* merge with next one if it is null item */
			next_offset = offset + DB_HEADE_SIZE + len;
			next_len = read_item(fd, next_offset, &next_keytmp, NULL, NULL);
			if (next_len >= 0)
			{
				if (next_keytmp == db_key_null)
					len += DB_HEADE_SIZE + next_len;
			}

			/* merge with privious one if it was null item */
			if (last_keytmp == db_key_null)
			{
				offset = last_offset;
				len += last_len + DB_HEADE_SIZE;
			}

			return save_item(fd, offset, db_key_null, NULL, len);
		}

		last_offset = offset;
		last_keytmp = keytmp;
		last_len = len;

		offset += DB_HEADE_SIZE + len;
	}
	while (key != db_key_end);

	return 0;
}

int add_item(int fd, db_key key, const char *data)
{
	int data_len;
	int rcode;

	int offset;
	int len;

	if (data)
		data_len = strlen(data);
	else
		data_len = 0;

	/* delete same key */
	del_item(fd, key);

	/* search enough space to store the item */
	offset = DB_MAGIC_SIZE;
	do
	{
		rcode = search_item(fd,  offset, db_key_null, NULL, NULL, &offset, &len);
		if (rcode == 1)	/* device error */
			return 1;
		if (rcode == 2)	/* it`s the end */
			break;

		if (len == data_len || len >= data_len + DB_HEADE_SIZE)
		{
			rcode = save_item(fd, offset, key, data, data_len);
			if (rcode)
				return rcode;

			if (len > data_len)
				rcode = save_item(
						fd,
						offset + DB_HEADE_SIZE + data_len,
						db_key_null,
						NULL,
						len - data_len - DB_HEADE_SIZE
					);

			return rcode;
		}

		offset += DB_HEADE_SIZE + len;
	}
	while (1);

	/* store at the end */
	rcode = save_item(fd, offset, key, data, data_len);
	if (rcode)
	{
		save_item(fd, offset, db_key_end, NULL, 0);
		return 1;
	}
	else
	{
		offset += DB_HEADE_SIZE + data_len;

		rcode = save_item(fd, offset, db_key_end, NULL, 0);
	}

	return rcode;
}


int main(int argc, char *argv[])
{
	int fd_i2c;
	unsigned char reg = sizeof(unsigned short); //DB_MAGIC_SIZE
	int vLoop, ret = 0;
	db_item item;
	unsigned char *buffer = (unsigned char *) &item;
	db_key key;
	int offset = DB_MAGIC_SIZE;

	//printf("%s >\n", argv[0]);

	fd_i2c = open("/dev/i2c-2", O_RDWR);

	//printf("fd = %d\n", fd_i2c);

	if (argc > 2)
	{
#ifdef write_works
		char *key_item = argv[1];
		char *value = argv[2];

		db_key key;
		int rc_code;

		rc_code = get_keyvalue(key_item, &key);

		if (rc_code)
		{
			printf("unknown key\n");
			goto failed;
		}

		if (!isRO(key_item))
		{
			printf("writing \"%s\" to \"%s\".\n", key_item, value);
			add_item(fd_i2c, key, value);
		}
		else
		{
			printf("trying to write RO element denied.\n");
		}
#endif
	}
	else if (argc > 1)
	{
		printf("offset\tkey\tlen\tdata\n");
		do
		{
			int rcode;
			char *name;
			unsigned char buf[256];
			int buflen;

			buf[0] = 0;
			buflen = 255;
			rcode = read_item(fd_i2c, offset, &key, buf, &buflen);
			if (rcode < 0)
			{
				rcode = 1;
				goto failed;
			}

			get_keyname(key, &name);

			if (strcmp(name, argv[1]) == 0)
			{
				printf("found key %s ->value %s\n", name, buf);

				if (strcmp(name, "tvmode") == 0)
					return atoi((unsigned char *) buf);
			}

			//printf( "%d\t%s\t%d\t%s\n", offset, name, rcode, (key == db_key_null) ? "" : buf );

			offset += DB_HEADE_SIZE + rcode;
		}
		while (key != db_key_end);
	}

	return 0;
failed:
	printf("failed\n");
	return -1;
}
