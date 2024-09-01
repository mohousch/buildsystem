/*
 * e2_proc_denc.c
 */

#include <linux/proc_fs.h> /* proc fs */
#include <asm/uaccess.h> /* copy_from_user */

#include <linux/dvb/video.h> /* Video Format etc */

#include <linux/dvb/audio.h>
#include <linux/smp_lock.h>
#include <linux/string.h>

#include "../backend.h"
#include "../dvb_module.h"
#include "linux/dvb/stm_ioctls.h"

extern struct DeviceContext_s *DeviceContext;

int proc_denc_0_wss_write(struct file *file, const char __user *buf,
						  unsigned long count, void *data)
{
	char *page;
	char *myString;
	ssize_t ret = -ENOMEM;
	/* int result; */
	printk("%s %ld - ", __FUNCTION__, count);
	page = (char *)__get_free_page(GFP_KERNEL);
	if (page)
	{
		ret = -EFAULT;
		if (copy_from_user(page, buf, count))
			goto out;
		myString = (char *) kmalloc(count + 1, GFP_KERNEL);
		strncpy(myString, page, count);
		myString[count] = '\0';
		printk("%s\n", myString);
		kfree(myString);
		//result = sscanf(page, "%3s %3s %3s %3s %3s", s1, s2, s3, s4, s5);
	}
	ret = count;
out:
	free_page((unsigned long)page);
	return ret;
}

int proc_denc_0_wss_read(char *page, char **start, off_t off, int count,
						 int *eof, void *data_unused)
{
	int len = 0;
	printk("%s\n", __FUNCTION__);
	len = sprintf(page, "auto(4:3_off)\n");
	return len;
}
