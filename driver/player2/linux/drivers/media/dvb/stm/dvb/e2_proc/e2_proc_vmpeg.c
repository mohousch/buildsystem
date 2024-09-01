/*
 * e2_proc_vmpeg.c
 */

#include <linux/proc_fs.h> /* proc fs */
#include <asm/uaccess.h> /* copy_from_user */

#include <linux/dvb/video.h> /* Video Format etc */

#include <linux/dvb/audio.h>
#include <linux/smp_lock.h>
#include <linux/string.h>
#include <linux/fb.h>

#include "../backend.h"
#include "../dvb_module.h"
#include "linux/dvb/stm_ioctls.h"

extern struct stmfb_info *stmfb_get_fbinfo_ptr(void);

int proc_vmpeg_0_dst_left_write(struct file *file, const char __user *buf,
								unsigned long count, void *data)
{
	char *page;
	ssize_t ret = -ENOMEM;
	int value, err, x, y;
	void *fb;
	struct fb_info *info;
	struct fb_var_screeninfo screen_info;
	char *myString = kmalloc(count + 1, GFP_KERNEL);
	struct DeviceContext_s *pContext = (struct DeviceContext_s *)data;
#ifdef VERY_VERBOSE
	printk("%s %d - ", __FUNCTION__, (unsigned int)count);
#endif
	fb = stmfb_get_fbinfo_ptr();
	info = (struct fb_info *) fb;
	memcpy(&screen_info, &info->var, sizeof(struct fb_var_screeninfo));
	if (fb != NULL)
	{
		y = screen_info.yres;
		x = screen_info.xres;
	}
	else
	{
		y = 576;
		x = 720;
	}
	page = (char *)__get_free_page(GFP_KERNEL);
	if (page)
	{
		int l, t, w, h;
		ret = -EFAULT;
		if (copy_from_user(page, buf, count))
			goto out;
		strncpy(myString, page, count);
		myString[count] = '\0';
#ifdef VERY_VERBOSE
		printk("%s\n", myString);
#endif
		sscanf(myString, "%x", &value);
		if (pContext != NULL)
		{
			mutex_lock(&(pContext->DvbContext->Lock));
			err = DvbStreamGetOutputWindow(pContext->VideoStream, &l, &t, &w, &h);
			if (err != 0)
				printk("failed to get output window %d\n", err);
#ifdef VERY_VERBOSE
			else
				printk("get output window to %d %d %d, %d ok\n", l, t, w, h);
#endif
			l = x * value / 720;
			err = DvbStreamSetOutputWindow(pContext->VideoStream, l, t, w, h);
			if (err != 0)
				printk("failed to set output window %d\n", err);
#ifdef VERY_VERBOSE
			else
				printk("set output window ok %d %d %d %d\n", l, t, w, h);
#endif
			mutex_unlock(&(pContext->DvbContext->Lock));
		}
		/* always return count to avoid endless loop */
		ret = count;
	}
out:
	free_page((unsigned long)page);
	kfree(myString);
	return ret;
}

int proc_vmpeg_0_dst_left_read(char *page, char **start, off_t off, int count,
							   int *eof, void *data)
{
	int len = 0;
	int l, t, w, h;
	int err, x, y;
	void *fb;
	struct fb_info *info;
	struct fb_var_screeninfo screen_info;
	struct DeviceContext_s *pContext = (struct DeviceContext_s *)data;
#ifdef VERY_VERBOSE
	printk("%s\n", __FUNCTION__);
#endif
	fb = stmfb_get_fbinfo_ptr();
	info = (struct fb_info *) fb;
	memcpy(&screen_info, &info->var, sizeof(struct fb_var_screeninfo));
	if (fb != NULL)
	{
		y = screen_info.yres;
		x = screen_info.xres;
	}
	else
	{
		y = 576;
		x = 720;
	}
	if (pContext != NULL)
	{
		mutex_lock(&(pContext->DvbContext->Lock));
		err = DvbStreamGetOutputWindow(pContext->VideoStream, &l, &t, &w, &h);
		if (err != 0)
		{
			printk("failed to get output window %d\n", err);
			l = t = w = h = 0;
		}
#ifdef VERY_VERBOSE
		else
			printk("get output window to %d %d %d, %d ok\n", l, t, w, h);
#endif
		mutex_unlock(&(pContext->DvbContext->Lock));
	}
	len = sprintf(page, "%x\n", 720 * l / x);
	return len;
}

int proc_vmpeg_0_dst_top_write(struct file *file, const char __user *buf,
							   unsigned long count, void *data)
{
	char *page;
	ssize_t ret = -ENOMEM;
	int value, err, x, y;
	void *fb;
	struct fb_info *info;
	struct fb_var_screeninfo screen_info;
	char *myString = kmalloc(count + 1, GFP_KERNEL);
	struct DeviceContext_s *pContext = (struct DeviceContext_s *)data;
#ifdef VERY_VERBOSE
	printk("%s %d - ", __FUNCTION__, (unsigned int)count);
#endif
	fb = stmfb_get_fbinfo_ptr();
	info = (struct fb_info *) fb;
	memcpy(&screen_info, &info->var, sizeof(struct fb_var_screeninfo));
	if (fb != NULL)
	{
		y = screen_info.yres;
		x = screen_info.xres;
	}
	else
	{
		y = 576;
		x = 720;
	}
	page = (char *)__get_free_page(GFP_KERNEL);
	if (page)
	{
		ret = -EFAULT;
		if (copy_from_user(page, buf, count))
			goto out;
		strncpy(myString, page, count);
		myString[count] = '\0';
#ifdef VERY_VERBOSE
		printk("%s\n", myString);
#endif
		sscanf(myString, "%x", &value);
		if (pContext != NULL)
		{
			int l, t, w, h;
			mutex_lock(&(pContext->DvbContext->Lock));
			err = DvbStreamGetOutputWindow(pContext->VideoStream, &l, &t, &w, &h);
			if (err != 0)
				printk("failed to get output window %d\n", err);
#ifdef VERY_VERBOSE
			else
				printk("get output window to %d %d %d, %d ok\n", l, t, w, h);
#endif
			t = y * value / 576;
			err = DvbStreamSetOutputWindow(pContext->VideoStream, l, t, w, h);
			if (err != 0)
				printk("failed to set output window %d\n", err);
#ifdef VERY_VERBOSE
			else
				printk("set output window ok %d %d %d %d\n", l, t, w, h);
#endif
			mutex_unlock(&(pContext->DvbContext->Lock));
		}
		/* always return count to avoid endless loop */
		ret = count;
	}
out:
	free_page((unsigned long)page);
	kfree(myString);
	return ret;
}

int proc_vmpeg_0_dst_top_read(char *page, char **start, off_t off, int count,
							  int *eof, void *data)
{
	int len = 0;
	int l, t, w, h;
	int err, x, y;
	void *fb;
	struct fb_info *info;
	struct fb_var_screeninfo screen_info;
	struct DeviceContext_s *pContext = (struct DeviceContext_s *)data;
#ifdef VERY_VERBOSE
	printk("%s\n", __FUNCTION__);
#endif
	fb = stmfb_get_fbinfo_ptr();
	info = (struct fb_info *) fb;
	memcpy(&screen_info, &info->var, sizeof(struct fb_var_screeninfo));
	if (fb != NULL)
	{
		y = screen_info.yres;
		x = screen_info.xres;
	}
	else
	{
		y = 576;
		x = 720;
	}
	if (pContext != NULL)
	{
		mutex_lock(&(pContext->DvbContext->Lock));
		err = DvbStreamGetOutputWindow(pContext->VideoStream, &l, &t, &w, &h);
		if (err != 0)
		{
			printk("failed to get output window %d\n", err);
			l = t = w = h = 0;
		}
#ifdef VERY_VERBOSE
		else
			printk("get output window to %d %d %d, %d ok\n", l, t, w, h);
#endif
		mutex_unlock(&(pContext->DvbContext->Lock));
	}
	len = sprintf(page, "%x\n", 576 * t / y);
	return len;
}

int proc_vmpeg_0_dst_width_write(struct file *file, const char __user *buf,
								 unsigned long count, void *data)
{
	char *page;
	ssize_t ret = -ENOMEM;
	int value, err, x, y;
	void *fb;
	struct fb_info *info;
	struct fb_var_screeninfo screen_info;
	struct DeviceContext_s *pContext = (struct DeviceContext_s *)data;
	char *myString = kmalloc(count + 1, GFP_KERNEL);
#ifdef VERY_VERBOSE
	printk("%s %d - ", __FUNCTION__, (unsigned int)count);
#endif
	fb = stmfb_get_fbinfo_ptr();
	info = (struct fb_info *) fb;
	memcpy(&screen_info, &info->var, sizeof(struct fb_var_screeninfo));
	if (fb != NULL)
	{
		y = screen_info.yres;
		x = screen_info.xres;
	}
	else
	{
		y = 576;
		x = 720;
	}
	page = (char *)__get_free_page(GFP_KERNEL);
	if (page)
	{
		ret = -EFAULT;
		if (copy_from_user(page, buf, count))
			goto out;
		strncpy(myString, page, count);
		myString[count] = '\0';
#ifdef VERY_VERBOSE
		printk("%s\n", myString);
#endif
		sscanf(myString, "%x", &value);
		if (pContext != NULL)
		{
			int l, t, w, h;
			mutex_lock(&(pContext->DvbContext->Lock));
			err = DvbStreamGetOutputWindow(pContext->VideoStream, &l, &t, &w, &h);
			if (err != 0)
				printk("failed to get output window %d\n", err);
#ifdef VERY_VERBOSE
			else
				printk("get output window to %d %d %d, %d ok\n", l, t, w, h);
#endif
			w = x * value / 720;
			err = DvbStreamSetOutputWindow(pContext->VideoStream, l, t, w, h);
			if (err != 0)
				printk("failed to set output window %d\n", err);
#ifdef VERY_VERBOSE
			else
				printk("set output window ok %d %d %d %d\n", l, t, w, h);
#endif
			mutex_unlock(&(pContext->DvbContext->Lock));
		}
		/* always return count to avoid endless loop */
		ret = count;
	}
out:
	free_page((unsigned long)page);
	kfree(myString);
	return ret;
}

int proc_vmpeg_0_dst_width_read(char *page, char **start, off_t off, int count,
								int *eof, void *data)
{
	int len = 0;
	int l, t, w, h;
	int err, x, y;
	void *fb;
	struct fb_info *info;
	struct fb_var_screeninfo screen_info;
	struct DeviceContext_s *pContext = (struct DeviceContext_s *)data;
#ifdef VERY_VERBOSE
	printk("%s\n", __FUNCTION__);
#endif
	fb = stmfb_get_fbinfo_ptr();
	info = (struct fb_info *) fb;
	memcpy(&screen_info, &info->var, sizeof(struct fb_var_screeninfo));
	if (fb != NULL)
	{
		y = screen_info.yres;
		x = screen_info.xres;
	}
	else
	{
		y = 576;
		x = 720;
	}
	if (pContext != NULL)
	{
		mutex_lock(&(pContext->DvbContext->Lock));
		err = DvbStreamGetOutputWindow(pContext->VideoStream, &l, &t, &w, &h);
		if (err != 0)
		{
			printk("failed to get output window %d\n", err);
			l = t = w = h = 0;
		}
#ifdef VERY_VERBOSE
		else
			printk("get output window to %d %d %d, %d ok\n", l, t, w, h);
#endif
		mutex_unlock(&(pContext->DvbContext->Lock));
	}
	len = sprintf(page, "%x\n", 720 * w / x);
	return len;
}

int proc_vmpeg_0_dst_height_write(struct file *file, const char __user *buf,
								  unsigned long count, void *data)
{
	char *page;
	ssize_t ret = -ENOMEM;
	int value, err, x, y;
	void *fb;
	struct fb_info *info;
	struct fb_var_screeninfo screen_info;
	struct DeviceContext_s *pContext = (struct DeviceContext_s *)data;
	char *myString = kmalloc(count + 1, GFP_KERNEL);
#ifdef VERY_VERBOSE
	printk("%s %d - ", __FUNCTION__, (unsigned int)count);
#endif
	fb = stmfb_get_fbinfo_ptr();
	info = (struct fb_info *) fb;
	memcpy(&screen_info, &info->var, sizeof(struct fb_var_screeninfo));
	if (fb != NULL)
	{
		y = screen_info.yres;
		x = screen_info.xres;
	}
	else
	{
		y = 576;
		x = 720;
	}
	page = (char *)__get_free_page(GFP_KERNEL);
	if (page)
	{
		ret = -EFAULT;
		if (copy_from_user(page, buf, count))
			goto out;
		strncpy(myString, page, count);
		myString[count] = '\0';
#ifdef VERY_VERBOSE
		printk("%s\n", myString);
#endif
		sscanf(myString, "%x", &value);
		if (pContext != NULL)
		{
			int l, t, w, h;
			mutex_lock(&(pContext->DvbContext->Lock));
			err = DvbStreamGetOutputWindow(pContext->VideoStream, &l, &t, &w, &h);
			if (err != 0)
				printk("failed to get output window %d\n", err);
#ifdef VERY_VERBOSE
			else
				printk("get output window to %d %d %d, %d ok\n", l, t, w, h);
#endif
			h = y * value / 576;
			err = DvbStreamSetOutputWindow(pContext->VideoStream, l, t, w, h);
			if (err != 0)
				printk("failed to set output window %d\n", err);
#ifdef VERY_VERBOSE
			else
				printk("set output window ok %d %d %d %d\n", l, t, w, h);
#endif
			mutex_unlock(&(pContext->DvbContext->Lock));
		}
		/* always return count to avoid endless loop */
		ret = count;
	}
out:
	free_page((unsigned long)page);
	kfree(myString);
	return ret;
}

int proc_vmpeg_0_dst_height_read(char *page, char **start, off_t off, int count,
								 int *eof, void *data)
{
	int len = 0;
	int l, t, w, h;
	int err, x, y;
	void *fb;
	struct fb_info *info;
	struct fb_var_screeninfo screen_info;
	struct DeviceContext_s *pContext = (struct DeviceContext_s *)data;
#ifdef VERY_VERBOSE
	printk("%s\n", __FUNCTION__);
#endif
	fb = stmfb_get_fbinfo_ptr();
	info = (struct fb_info *) fb;
	memcpy(&screen_info, &info->var, sizeof(struct fb_var_screeninfo));
	if (fb != NULL)
	{
		y = screen_info.yres;
		x = screen_info.xres;
	}
	else
	{
		y = 576;
		x = 720;
	}
	if (pContext != NULL)
	{
		mutex_lock(&(pContext->DvbContext->Lock));
		err = DvbStreamGetOutputWindow(pContext->VideoStream, &l, &t, &w, &h);
		if (err != 0)
		{
			printk("failed to get output window %d\n", err);
			l = t = w = h = 0;
		}
#ifdef VERY_VERBOSE
		else
			printk("get output window to %d %d %d, %d ok\n", l, t, w, h);
#endif
		mutex_unlock(&(pContext->DvbContext->Lock));
	}
	len = sprintf(page, "%x\n", 576 * h / y);
	return len;
}

int proc_vmpeg_0_yres_read(char *page, char **start, off_t off, int count,
						   int *eof, void *data)
{
	int len = 0;
	struct DeviceContext_s *pContext = (struct DeviceContext_s *)data;
#ifdef VERY_VERBOSE
	printk("%s\n", __FUNCTION__);
#endif
	if (pContext != NULL)
	{
		mutex_lock(&(pContext->DvbContext->Lock));
		len = sprintf(page, "%x\n", pContext->VideoSize.h);
		mutex_unlock(&(pContext->DvbContext->Lock));
	}
	return len;
}

int proc_vmpeg_0_xres_read(char *page, char **start, off_t off, int count,
						   int *eof, void *data)
{
	int len = 0;
	struct DeviceContext_s *pContext = (struct DeviceContext_s *)data;
#ifdef VERY_VERBOSE
	printk("%s\n", __FUNCTION__);
#endif
	if (pContext != NULL)
	{
		mutex_lock(&(pContext->DvbContext->Lock));
		len = sprintf(page, "%x\n", pContext->VideoSize.w);
		mutex_unlock(&(pContext->DvbContext->Lock));
	}
	return len;
}

int proc_vmpeg_0_framerate_read(char *page, char **start, off_t off, int count,
								int *eof, void *data)
{
	int len = 0;
	struct DeviceContext_s *pContext = (struct DeviceContext_s *)data;
#ifdef VERY_VERBOSE
	printk("%s\n", __FUNCTION__);
#endif
	if (pContext != NULL)
	{
		mutex_lock(&(pContext->DvbContext->Lock));
		len = sprintf(page, "%u\n", pContext->FrameRate);
		mutex_unlock(&(pContext->DvbContext->Lock));
	}
	return len;
}

int proc_vmpeg_0_aspect_read(char *page, char **start, off_t off, int count,
							 int *eof, void *data)
{
	int len = 0;
	struct DeviceContext_s *pContext = (struct DeviceContext_s *)data;
#ifdef VERY_VERBOSE
	printk("%s\n", __FUNCTION__);
#endif
	if (pContext != NULL)
	{
		mutex_lock(&(pContext->DvbContext->Lock));
		len = sprintf(page, "%x\n", pContext->VideoSize.aspect_ratio);
		mutex_unlock(&(pContext->DvbContext->Lock));
	}
	return len;
}

int proc_vmpeg_0_dst_all_write(struct file *file, const char __user *buf,
							   unsigned long count, void *data)
{
	char *page;
	ssize_t ret = -ENOMEM;
	int err, x, y;
	void *fb;
	struct fb_info *info;
	struct fb_var_screeninfo screen_info;
	char *myString = kmalloc(count + 1, GFP_KERNEL);
	struct DeviceContext_s *pContext = (struct DeviceContext_s *)data;
#ifdef VERY_VERBOSE
	printk("%s %d - ", __FUNCTION__, (unsigned int)count);
#endif
	fb = stmfb_get_fbinfo_ptr();
	info = (struct fb_info *) fb;
	memcpy(&screen_info, &info->var, sizeof(struct fb_var_screeninfo));
	if (fb != NULL)
	{
		y = screen_info.yres;
		x = screen_info.xres;
	}
	else
	{
		y = 576;
		x = 720;
	}
	page = (char *)__get_free_page(GFP_KERNEL);
	if (page)
	{
		int l, t, w, h;
		ret = -EFAULT;
		if (copy_from_user(page, buf, count))
			goto out;
		strncpy(myString, page, count);
		myString[count] = '\0';
#ifdef VERY_VERBOSE
		printk("%s\n", myString);
#endif
		sscanf(myString, "%x %x %x %x", &l, &t, &w, &h);
#ifdef VERY_VERBOSE
		printk("%x, %x, %x, %x\n", l, t, w, h);
#endif
		if (pContext != NULL)
		{
			mutex_lock(&(pContext->DvbContext->Lock));
			h = y * h / 576;
			w = x * w / 720;
			l = x * l / 720;
			t = y * t / 576;
#ifdef VERY_VERBOSE
			printk("%x, %x, %x, %x\n", l, t, w, h);
#endif
			err = DvbStreamSetOutputWindow(pContext->VideoStream, l, t, w, h);
			if (err != 0)
				printk("failed to set output window %d\n", err);
#ifdef VERY_VERBOSE
			else
				printk("set output window ok %d %d %d %d\n", l, t, w, h);
#endif
			mutex_unlock(&(pContext->DvbContext->Lock));
		}
		/* always return count to avoid endless loop */
		ret = count;
	}
out:
	free_page((unsigned long)page);
	kfree(myString);
	return ret;
}
