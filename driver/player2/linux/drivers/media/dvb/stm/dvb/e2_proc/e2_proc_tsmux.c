/*
 * e2_proc_tsmux.c
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

#if defined(HS7110) \
 || defined(ATEMIO520) \
 || defined(ATEMIO530)
extern int setMuxSource(int source);
#endif
int setCiSource(int slot, int source);
void getCiSource(int slot, int *source);

int proc_tsmux_input0_write(struct file *file, const char __user *buf,
							unsigned long count, void *data)
{
	char *page;
	ssize_t ret = -ENOMEM;
	printk("%s %ld ", __FUNCTION__, count);
	page = (char *)__get_free_page(GFP_KERNEL);
	if (page)
	{
		ret = -EFAULT;
		if (copy_from_user(page, buf, count))
			goto out;
		page[count] = 0;
		printk("%s\n", page);
		ret = count;
#if defined(IPBOX9900)
		if (strcmp(page, "A") == 0)
			setCiSource(0, -1);
#endif
		/* ciN for source ciN, e.g. A for TUNER */
#if defined(HS7110) \
 || defined(ATEMIO520) \
 || defined(ATEMIO530)
		if (strcmp(page, "A") == 0)
			setMuxSource(0);
		else if (strcmp(page, "CI0") == 0)
			setMuxSource(1);
#endif
	}
out:
	free_page((unsigned long)page);
	return ret;
}

int proc_tsmux_input0_read(char *page, char **start, off_t off, int count,
						   int *eof, void *data_unused)
{
	int len = 0;
	printk("%s %d\n", __FUNCTION__, count);
	return len;
}

int proc_tsmux_input1_write(struct file *file, const char __user *buf,
							unsigned long count, void *data)
{
	char *page;
	ssize_t ret = -ENOMEM;
	printk("%s %ld ", __FUNCTION__, count);
	page = (char *)__get_free_page(GFP_KERNEL);
	if (page)
	{
		ret = -EFAULT;
		if (copy_from_user(page, buf, count))
			goto out;
		page[count] = 0;
		printk("%s\n", page);
		ret = count;
		/* Dagobert: fixme: currently just to see what e2 sends here
		 * so maybe I understand what the developer
		 * wants to do here.
		 */
#if defined(UFS922) \
 || defined(UFC960)
		if (strcmp(page, "CI0") == 0)
			printk("%s ->CIO received\n", __func__);
		else if (strcmp(page, "CI1") == 0)
			printk("%s ->CI1 received\n", __func__);
		else
			ret = -EINVAL;
#endif
	}
out:
	free_page((unsigned long)page);
	return ret;
}

int proc_tsmux_input1_read(char *page, char **start, off_t off, int count,
						   int *eof, void *data_unused)
{
	int len = 0;
	printk("%s %d\n", __FUNCTION__, count);
	return len;
}

int proc_tsmux_lnb_b_input_write(struct file *file, const char __user *buf,
								 unsigned long count, void *data)
{
	char *page;
	ssize_t ret = -ENOMEM;
	printk("%s %ld\n", __FUNCTION__, count);
	page = (char *)__get_free_page(GFP_KERNEL);
	if (page)
	{
		ret = -EFAULT;
		if (copy_from_user(page, buf, count))
			goto out;
	}
	ret = count;
out:
	free_page((unsigned long)page);
	return ret;
}

int proc_tsmux_lnb_b_input_read(char *page, char **start, off_t off, int count,
								int *eof, void *data_unused)
{
	int len = 0;
	printk("%s %d\n", __FUNCTION__, count);
	return len;
}

int proc_tsmux_ci0_input_write(struct file *file, const char __user *buf,
							   unsigned long count, void *data)
{
	char *page;
	ssize_t ret = -ENOMEM;
	printk("%s %ld ", __FUNCTION__, count);
	page = (char *)__get_free_page(GFP_KERNEL);
	if (page)
	{
		ret = -EFAULT;
		if (copy_from_user(page, buf, count))
			goto out;
		page[count] = 0;
		printk("%s\n", page);
		ret = count;
#if defined(TF7700) \
 || defined(UFS922) \
 || defined(UFC960) \
 || defined(CUBEREVO) \
 || defined(CUBEREVO_MINI) \
 || defined(CUBEREVO_MINI2) \
 || defined(CUBEREVO_9500HD) \
 || defined(CUBEREVO_3000HD) \
 || defined(FORTIS_HDBOX) \
 || defined(HL101) \
 || defined(VIP1_V2) \
 || defined(ATEVIO7500) \
 || defined(IPBOX9900) \
 || defined(IPBOX99) \
 || defined(IPBOX55) \
 || defined(UFS913)
		if (strcmp(page, "A") == 0)
			setCiSource(0, 0);
		else if (strcmp(page, "B") == 0)
			setCiSource(0, 1);
		else
			ret = -EINVAL;
#endif
	}
out:
	free_page((unsigned long)page);
	return ret;
}

int proc_tsmux_ci0_input_read(char *page, char **start, off_t off, int count,
							  int *eof, void *data_unused)
{
	int len = 0;
	printk("%s\n", __FUNCTION__);
#if defined(TF7700) \
 || defined(UFS922) \
 || defined(UFC960) \
 || defined(CUBEREVO) \
 || defined(CUBEREVO_MINI) \
 || defined(CUBEREVO_MINI2) \
 || defined(CUBEREVO_9500HD) \
 || defined(CUBEREVO_3000HD) \
 || defined(FORTIS_HDBOX) \
 || defined(HL101) \
 || defined(VIP1_V2) \
 || defined(ATEVIO7500) \
 || defined(IPBOX9900) \
 || defined(IPBOX99) \
 || defined(IPBOX55) \
 || defined(UFS913)
	{
		int source = 0;
		getCiSource(0, &source);
		if (source == 0)
			len = sprintf(page, "A\n");
		else
			len = sprintf(page, "B\n");
	}
#endif
	return len;
}

int proc_tsmux_ci1_input_write(struct file *file, const char __user *buf,
							   unsigned long count, void *data)
{
	char *page;
	ssize_t ret = -ENOMEM;
	printk("%s %ld ", __FUNCTION__, count);
	page = (char *)__get_free_page(GFP_KERNEL);
	if (page)
	{
		ret = -EFAULT;
		if (copy_from_user(page, buf, count))
			goto out;
		page[count] = 0;
		printk("%s\n", page);
		ret = count;
#if defined(TF7700) \
 || defined(UFS922) \
 || defined(UFC960) \
 || defined(CUBEREVO) \
 || defined(CUBEREVO_MINI) \
 || defined(CUBEREVO_MINI2) \
 || defined(CUBEREVO_9500HD) \
 || defined(CUBEREVO_3000HD) \
 || defined(FORTIS_HDBOX) \
 || defined(HL101) \
 || defined(VIP1_V2) \
 || defined(ATEVIO7500) \
 || defined(IPBOX9900) \
 || defined(IPBOX99) \
 || defined(IPBOX55) \
 || defined(UFS913)
		if (strcmp(page, "A") == 0)
			setCiSource(1, 0);
		else if (strcmp(page, "B") == 0)
			setCiSource(1, 1);
		else
			ret = -EINVAL;
#endif
	}
out:
	free_page((unsigned long)page);
	return ret;
}

int proc_tsmux_ci1_input_read(char *page, char **start, off_t off, int count,
							  int *eof, void *data_unused)
{
	int len = 0;
	printk("%s\n", __FUNCTION__);
#if defined(TF7700) \
 || defined(UFS922) \
 || defined(UFC960) \
 || defined(CUBEREVO) \
 || defined(CUBEREVO_MINI) \
 || defined(CUBEREVO_MINI2) \
 || defined(CUBEREVO_9500HD) \
 || defined(CUBEREVO_3000HD) \
 || defined(FORTIS_HDBOX) \
 || defined(HL101) \
 || defined(VIP1_V2) \
 || defined(ATEVIO7500) \
 || defined(IPBOX9900) \
 || defined(IPBOX99) \
 || defined(IPBOX55) \
 || defined(UFS913)
	{
		int source = 0;
		getCiSource(1, &source);
		if (source == 0)
			len = sprintf(page, "A\n");
		else
			len = sprintf(page, "B\n");
	}
#endif
	return len;
}
