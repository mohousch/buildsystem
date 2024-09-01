/*
 *
 * (c) 2010 konfetti, schischu
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


#include <linux/proc_fs.h>  	/* proc fs */
#include <asm/uaccess.h>    	/* copy_from_user */
#include <linux/delay.h>

#include "cec_worker.h"
#include "cec_opcodes.h"
#include "cec_opcodes_def.h"
#include "cec_debug.h"

/* external functions provided by the module e2_procfs */
extern int install_e2_procs(char *name, read_proc_t *read_proc, write_proc_t *write_proc, void *data);
extern int remove_e2_procs(char *name, read_proc_t *read_proc, write_proc_t *write_proc);

#define INPUT_BUFFER_SIZE 255
static unsigned char inputBuffer[INPUT_BUFFER_SIZE];
static unsigned int sizeOfInputBuffer = 0;

//===================================

#define PW_INIT(buf, count) \
	char *  page; \
	int     pageLen = 0; \
	ssize_t ret     = -ENOMEM; \
	\
	dprintk(2, "%s %ld\n", __FUNCTION__, count); \
	\
	page = (char *)__get_free_page(GFP_KERNEL); \
	if (page) \
	{ \
		ret = -EFAULT; \
		pageLen = count; \
		\
		if (copy_from_user(page, buf, count)) \
			goto out;

#define PW_EXIT(count, ret, page) \
	} \
	ret = count; \
	out: \
	free_page((unsigned long)page); \
	return ret;

#define PR_INIT() \
	int len = 0; \
	dprintk(2, "%s\n", __FUNCTION__);

#define PR_INIT_SILENT() \
	int len = 0; \
	dprintk(2, "%s\n", __FUNCTION__);

#define PR_EXIT(len) \
	return len;

////////////////////////////////////////

#define INC(counter) \
	if(counter != 0xf) \
		counter++; \
	dprintk(2, "Incremented Value: %d\n", counter);

#define RESET(event) \
	event = 0;

////////////////////////////////////////

static unsigned char eventDiscovery = 0;
static unsigned char eventActiveSource = 0;
static unsigned char eventStandby = 0;

////////////////////////////////////////

void setUpdatedDiscovery(void)
{
	INC(eventDiscovery);
}
void setUpdatedActiveSource(void)
{
	INC(eventActiveSource);
}
void setUpdatedStandby(void)
{
	INC(eventStandby);
}

////////////////////////////////////////

int proc_event_poll_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused)
{
	PR_INIT_SILENT();

	len = sprintf(page, "%1d%1d%1d%1d%1d%1d%1d%1d\n"
		      "||||||||-> Event 8\n"
		      "|||||||-> Event 7\n"
		      "||||||-> Event 6\n"
		      "|||||-> Event 5\n"
		      "||||-> Event 4\n"
		      "|||-> Standby Requested\n"
		      "||-> Active Source\n"
		      "|-> Discovery\n",
		      eventDiscovery,
		      eventActiveSource,
		      eventStandby,
		      0, 0, 0, 0, 0);

	PR_EXIT(len)
}

//===================================

int proc_cec_send_write(struct file *filefile, const char __user *buf, unsigned long count, void *data)
{

	int i = 0;
	PW_INIT(buf, count);

	memset(inputBuffer, 0, INPUT_BUFFER_SIZE);
	sizeOfInputBuffer = 0;
	for (i = 0; (i + 2) < pageLen; i += 3)
	{
		char ctmp[3];
		ctmp[0] = page[i];
		ctmp[1] = page[i + 1];
		ctmp[2] = '\0';
		sscanf(ctmp, "%02hhx", &inputBuffer[sizeOfInputBuffer++]);
	}
	dprintk(2, "%s read %d bytes\n", __FUNCTION__, sizeOfInputBuffer);
	sendMessage(sizeOfInputBuffer, inputBuffer);

	PW_EXIT(count, ret, page);
}

//===================================

int proc_cecaddress_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused)
{
	unsigned short physAddr = 0;
	PR_INIT();

	physAddr = getPhysicalAddress();

	len = sprintf(page, "%01x.%01x.%01x.%01x\n", (physAddr >> 12) & 0xf, (physAddr >> 8) & 0xf,
		      (physAddr >> 4)  & 0xf, (physAddr >> 0)  & 0xf);

	PR_EXIT(len)
}

//===================================

int proc_activesource_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused)
{
	unsigned short physAddr = 0;
	PR_INIT();

	physAddr = getActiveSource();

	len = sprintf(page, "%01x.%01x.%01x.%01x\n", (physAddr >> 12) & 0xf, (physAddr >> 8) & 0xf,
		      (physAddr >> 4)  & 0xf, (physAddr >> 0)  & 0xf);

	RESET(eventActiveSource);

	PR_EXIT(len)
}

//===================================

int proc_standby_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused)
{
	PR_INIT();

	len = sprintf(page, "\n");

	RESET(eventStandby);

	PR_EXIT(len)
}

//===================================

//===================================

int proc_onetouchplay_write(struct file *file, const char __user *buf, unsigned long count, void *data)
{
	PW_INIT(buf, count);

	sendOneTouchPlay();

	PW_EXIT(count, ret, page);
}

//===================================

int proc_systemstandby_write(struct file *file, const char __user *buf, unsigned long count, void *data)
{
	unsigned int deviceId = DEVICE_TYPE_TV;
	PW_INIT(buf, count);

	if (count > 0)
		if (page[0] >= '0' && page[0] <= 'f')
			sscanf(page, "%1x", &deviceId);

	sendSystemStandby(deviceId);

	PW_EXIT(count, ret, page);
}

//===================================

struct e2_procs
{
	char *name;
	read_proc_t  *read_proc;
	write_proc_t *write_proc;
	int context;
} e2_procs[] =
{
	{"stb/cec/send",               NULL,                    proc_cec_send_write,       0},

	{"stb/cec/onetouchplay",       NULL,                    proc_onetouchplay_write,   0},
	{"stb/cec/systemstandby",      NULL,                    proc_systemstandby_write,  0},

	{"stb/cec/state_activesource", proc_activesource_read,  NULL,                      0},
	{"stb/cec/state_cecaddress",   proc_cecaddress_read,    NULL,                      0},
	{"stb/cec/state_standby",      proc_standby_read,       NULL,                      0},

	{"stb/cec/event_poll",         proc_event_poll_read,    NULL,                      0}
};

void init_e2_proc(void)
{
	int i;

	for (i = 0; i < sizeof(e2_procs) / sizeof(e2_procs[0]); i++)
	{
		install_e2_procs(e2_procs[i].name, e2_procs[i].read_proc,
				 e2_procs[i].write_proc, NULL);
	}
}

void cleanup_e2_proc(void)
{
	int i;

	for (i = sizeof(e2_procs) / sizeof(e2_procs[0]) - 1; i >= 0; i--)
	{
		remove_e2_procs(e2_procs[i].name, e2_procs[i].read_proc,
				e2_procs[i].write_proc);
	}
}
