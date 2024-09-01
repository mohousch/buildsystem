/*
 * e2_proc_misc.c
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

#if defined(IPBOX9900)

#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,23)
#include <linux/stpio.h>
#else
#include <linux/stm/pio.h>
#endif

static int output_stat = 0;
struct stpio_pin *output_pin;

int proc_misc_12V_output_write(struct file *file, const char __user *buf,
							   unsigned long count, void *data)
{
	char *page;
	ssize_t ret = -ENOMEM;
	char *myString;
//	printk("%s %d\n", __FUNCTION__, count);
	page = (char *)__get_free_page(GFP_KERNEL);
	if (page)
	{
		ret = -EFAULT;
		if (copy_from_user(page, buf, count))
			goto out;
		myString = (char *) kmalloc(count + 1, GFP_KERNEL);
		strncpy(myString, page, count);
		myString[count] = '\0';
		printk("12V %s\n", myString);
		if (strncmp("on", myString, count - 1) == 0) //12V ON
		{
			output_stat = 1;
		}
		else
		{
			output_stat = 0;
		}
		if (output_pin)
			stpio_set_pin(output_pin, output_stat);
		kfree(myString);
	}
	ret = count;
out:
	free_page((unsigned long)page);
	return ret;
}

int proc_misc_12V_output_read(char *page, char **start, off_t off, int count,
							  int *eof, void *data_unused)
{
	int len = 0;
//	printk("%s %d %d\n", __FUNCTION__, count, current_input);
	if (output_stat == 1)
		len = sprintf(page, "on\n");
	else
		len = sprintf(page, "off\n");
	return len;
}

#endif
