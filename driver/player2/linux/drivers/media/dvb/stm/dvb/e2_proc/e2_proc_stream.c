/*
 * e2_proc_stream.c
 *
 * TODO/ISSUES:
 * - must be updated with every new player ->options are defined in player_types.h and backend_ops.h
 * - currently missing is PolicyH264AllowBadPreProcessedFrames in backend_ops.h
 * - HavanaStream_c::GetOption in havana_stream.cpp misses many of the otions so the GetOption
 * will fail here.
 * - a _choices proc for each policy should be implemented.
 */

#include <linux/proc_fs.h> /* proc fs */
#include <asm/uaccess.h> /* copy_from_user */

#include <linux/dvb/video.h> /* Video Format etc */
#include <linux/dvb/audio.h>
#include <linux/string.h>

#include "../backend.h"
#include "../dvb_module.h"
#include "linux/dvb/stm_ioctls.h"

extern struct DeviceContext_s *ProcDeviceContext;

int proc_stream_AV_SYNC_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused)
{
	int len = 0;
	unsigned int value;
	printk("%s >\n", __FUNCTION__);
	if (DvbStreamGetOption(ProcDeviceContext->VideoStream, PLAY_OPTION_AV_SYNC, &value) == 0)
	{
		if (value == PLAY_OPTION_VALUE_ENABLE)
			len = sprintf(page, "apply\n");
		else
			len = sprintf(page, "disapply\n");
	}
	else
		len = sprintf(page, "failed to get value\n");
	return len;
}

int proc_stream_AV_SYNC_write(struct file *file, const char __user *buf, unsigned long count, void *data)
{
	char *page;
	char *myString;
	ssize_t ret = -ENOMEM;
	int result;
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
		if (strncmp("apply", myString, count - 1) == 0)
		{
			result = DvbStreamSetOption(ProcDeviceContext->VideoStream, PLAY_OPTION_AV_SYNC, PLAY_OPTION_VALUE_ENABLE);
			if (result != 0)
			{
				printk("failed to set policy\n");
			}
		}
		else if (strncmp("disapply", myString, count - 1) == 0)
		{
			result = DvbStreamSetOption(ProcDeviceContext->VideoStream, PLAY_OPTION_AV_SYNC, PLAY_OPTION_VALUE_DISABLE);
			if (result != 0)
			{
				printk("failed to set policy\n");
			}
		}
		else
		{
			printk("invalid value\n");
		}
		kfree(myString);
	}
	ret = count;
out:
	free_page((unsigned long)page);
	return ret;
}

int proc_stream_TRICK_MODE_AUDIO_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused)
{
	int len = 0;
	unsigned int value;
	printk("%s\n", __FUNCTION__);
	if (DvbStreamGetOption(ProcDeviceContext->VideoStream, PLAY_OPTION_TRICK_MODE_AUDIO, &value) == 0)
	{
		if (value == PLAY_OPTION_VALUE_ENABLE)
			len = sprintf(page, "apply\n");
		else
			len = sprintf(page, "disapply\n");
	}
	else
		len = sprintf(page, "failed to get value\n");
	return len;
}

int proc_stream_TRICK_MODE_AUDIO_write(struct file *file, const char __user *buf, unsigned long count, void *data)
{
	char *page;
	char *myString;
	ssize_t ret = -ENOMEM;
	int result;
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
		if (strncmp("apply", myString, count - 1) == 0)
		{
			result = DvbStreamSetOption(ProcDeviceContext->VideoStream, PLAY_OPTION_TRICK_MODE_AUDIO, PLAY_OPTION_VALUE_ENABLE);
			if (result != 0)
			{
				printk("failed to set policy\n");
			}
		}
		else if (strncmp("disapply", myString, count - 1) == 0)
		{
			result = DvbStreamSetOption(ProcDeviceContext->VideoStream, PLAY_OPTION_TRICK_MODE_AUDIO, PLAY_OPTION_VALUE_DISABLE);
			if (result != 0)
			{
				printk("failed to set policy\n");
			}
		}
		else
		{
			printk("invalid value\n");
		}
		kfree(myString);
	}
	ret = count;
out:
	free_page((unsigned long)page);
	return ret;
}

int proc_stream_PLAY_24FPS_VIDEO_AT_25FPS_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused)
{
	int len = 0;
	unsigned int value;
	printk("%s\n", __FUNCTION__);
	if (DvbStreamGetOption(ProcDeviceContext->VideoStream, PLAY_OPTION_PLAY_24FPS_VIDEO_AT_25FPS, &value) == 0)
	{
		if (value == PLAY_OPTION_VALUE_ENABLE)
			len = sprintf(page, "apply\n");
		else
			len = sprintf(page, "disapply\n");
	}
	else
		len = sprintf(page, "failed to get value\n");
	return len;
}

int proc_stream_PLAY_24FPS_VIDEO_AT_25FPS_write(struct file *file, const char __user *buf, unsigned long count, void *data)
{
	char *page;
	char *myString;
	ssize_t ret = -ENOMEM;
	int result;
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
		if (strncmp("apply", myString, count - 1) == 0)
		{
			result = DvbStreamSetOption(ProcDeviceContext->VideoStream, PLAY_OPTION_PLAY_24FPS_VIDEO_AT_25FPS, PLAY_OPTION_VALUE_ENABLE);
			if (result != 0)
			{
				printk("failed to set policy\n");
			}
		}
		else if (strncmp("disapply", myString, count - 1) == 0)
		{
			result = DvbStreamSetOption(ProcDeviceContext->VideoStream, PLAY_OPTION_PLAY_24FPS_VIDEO_AT_25FPS, PLAY_OPTION_VALUE_DISABLE);
			if (result != 0)
			{
				printk("failed to set policy\n");
			}
		}
		else
		{
			printk("invalid value\n");
		}
		kfree(myString);
	}
	ret = count;
out:
	free_page((unsigned long)page);
	return ret;
}

int proc_stream_MASTER_CLOCK_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused)
{
	int len = 0;
	unsigned int value;
	printk("%s\n", __FUNCTION__);
	if (DvbStreamGetOption(ProcDeviceContext->VideoStream, PLAY_OPTION_MASTER_CLOCK, &value) == 0)
	{
		if (value == PLAY_OPTION_VALUE_VIDEO_CLOCK_MASTER)
			len = sprintf(page, "video\n");
		else
			len = sprintf(page, "audio\n");
	}
	else
		len = sprintf(page, "failed to get value\n");
	return len;
}

int proc_stream_MASTER_CLOCK_write(struct file *file, const char __user *buf, unsigned long count, void *data)
{
	char *page;
	char *myString;
	ssize_t ret = -ENOMEM;
	int result;
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
		if (strncmp("video", myString, count - 1) == 0)
		{
			result = DvbStreamSetOption(ProcDeviceContext->VideoStream, PLAY_OPTION_MASTER_CLOCK, PLAY_OPTION_VALUE_VIDEO_CLOCK_MASTER);
			if (result != 0)
			{
				printk("failed to set policy\n");
			}
		}
		else if (strncmp("audio", myString, count - 1) == 0)
		{
			result = DvbStreamSetOption(ProcDeviceContext->VideoStream, PLAY_OPTION_MASTER_CLOCK, PLAY_OPTION_VALUE_AUDIO_CLOCK_MASTER);
			if (result != 0)
			{
				printk("failed to set policy\n");
			}
		}
		else
		{
			printk("invalid value\n");
		}
		kfree(myString);
	}
	ret = count;
out:
	free_page((unsigned long)page);
	return ret;
}

int proc_stream_EXTERNAL_TIME_MAPPING_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused)
{
	int len = 0;
	unsigned int value;
	printk("%s\n", __FUNCTION__);
	if (DvbStreamGetOption(ProcDeviceContext->VideoStream, PLAY_OPTION_EXTERNAL_TIME_MAPPING, &value) == 0)
	{
		if (value == PLAY_OPTION_VALUE_ENABLE)
			len = sprintf(page, "apply\n");
		else
			len = sprintf(page, "disapply\n");
	}
	else
		len = sprintf(page, "failed to get value\n");
	return len;
}

int proc_stream_EXTERNAL_TIME_MAPPING_write(struct file *file, const char __user *buf, unsigned long count, void *data)
{
	char *page;
	char *myString;
	ssize_t ret = -ENOMEM;
	int result;
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
		if (strncmp("apply", myString, count - 1) == 0)
		{
			result = DvbStreamSetOption(ProcDeviceContext->VideoStream, PLAY_OPTION_EXTERNAL_TIME_MAPPING, PLAY_OPTION_VALUE_ENABLE);
			if (result != 0)
			{
				printk("failed to set policy\n");
			}
		}
		else if (strncmp("disapply", myString, count - 1) == 0)
		{
			result = DvbStreamSetOption(ProcDeviceContext->VideoStream, PLAY_OPTION_EXTERNAL_TIME_MAPPING, PLAY_OPTION_VALUE_DISABLE);
			if (result != 0)
			{
				printk("failed to set policy\n");
			}
		}
		else
		{
			printk("invalid value\n");
		}
		kfree(myString);
	}
	ret = count;
out:
	free_page((unsigned long)page);
	return ret;
}

int proc_stream_DISPLAY_FIRST_FRAME_EARLY_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused)
{
	int len = 0;
	unsigned int value;
	printk("%s\n", __FUNCTION__);
	if (DvbStreamGetOption(ProcDeviceContext->VideoStream, PLAY_OPTION_DISPLAY_FIRST_FRAME_EARLY, &value) == 0)
	{
		if (value == PLAY_OPTION_VALUE_ENABLE)
			len = sprintf(page, "apply\n");
		else
			len = sprintf(page, "disapply\n");
	}
	else
		len = sprintf(page, "failed to get value\n");
	return len;
}

int proc_stream_DISPLAY_FIRST_FRAME_EARLY_write(struct file *file, const char __user *buf, unsigned long count, void *data)
{
	char *page;
	char *myString;
	ssize_t ret = -ENOMEM;
	int result;
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
		if (strncmp("apply", myString, count - 1) == 0)
		{
			result = DvbStreamSetOption(ProcDeviceContext->VideoStream, PLAY_OPTION_DISPLAY_FIRST_FRAME_EARLY, PLAY_OPTION_VALUE_ENABLE);
			if (result != 0)
			{
				printk("failed to set policy\n");
			}
		}
		else if (strncmp("disapply", myString, count - 1) == 0)
		{
			result = DvbStreamSetOption(ProcDeviceContext->VideoStream, PLAY_OPTION_DISPLAY_FIRST_FRAME_EARLY, PLAY_OPTION_VALUE_DISABLE);
			if (result != 0)
			{
				printk("failed to set policy\n");
			}
		}
		else
		{
			printk("invalid value\n");
		}
		kfree(myString);
	}
	ret = count;
out:
	free_page((unsigned long)page);
	return ret;
}

int proc_stream_STREAM_ONLY_KEY_FRAMES_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused)
{
	int len = 0;
	unsigned int value;
	printk("%s\n", __FUNCTION__);
	if (DvbStreamGetOption(ProcDeviceContext->VideoStream, PLAY_OPTION_STREAM_ONLY_KEY_FRAMES, &value) == 0)
	{
		if (value == PLAY_OPTION_VALUE_ENABLE)
			len = sprintf(page, "apply\n");
		else
			len = sprintf(page, "disapply\n");
	}
	else
		len = sprintf(page, "failed to get value\n");
	return len;
}

int proc_stream_STREAM_ONLY_KEY_FRAMES_write(struct file *file, const char __user *buf, unsigned long count, void *data)
{
	char *page;
	char *myString;
	ssize_t ret = -ENOMEM;
	int result;
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
		if (strncmp("apply", myString, count - 1) == 0)
		{
			result = DvbStreamSetOption(ProcDeviceContext->VideoStream, PLAY_OPTION_STREAM_ONLY_KEY_FRAMES, PLAY_OPTION_VALUE_ENABLE);
			if (result != 0)
			{
				printk("failed to set policy\n");
			}
		}
		else if (strncmp("disapply", myString, count - 1) == 0)
		{
			result = DvbStreamSetOption(ProcDeviceContext->VideoStream, PLAY_OPTION_STREAM_ONLY_KEY_FRAMES, PLAY_OPTION_VALUE_DISABLE);
			if (result != 0)
			{
				printk("failed to set policy\n");
			}
		}
		else
		{
			printk("invalid value\n");
		}
		kfree(myString);
	}
	ret = count;
out:
	free_page((unsigned long)page);
	return ret;
}

int proc_stream_STREAM_SINGLE_GROUP_BETWEEN_DISCONTINUITIES_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused)
{
	int len = 0;
	unsigned int value;
	printk("%s\n", __FUNCTION__);
	if (DvbStreamGetOption(ProcDeviceContext->VideoStream, PLAY_OPTION_STREAM_SINGLE_GROUP_BETWEEN_DISCONTINUITIES, &value) == 0)
	{
		if (value == PLAY_OPTION_VALUE_ENABLE)
			len = sprintf(page, "apply\n");
		else
			len = sprintf(page, "disapply\n");
	}
	else
		len = sprintf(page, "failed to get value\n");
	return len;
}

int proc_stream_STREAM_SINGLE_GROUP_BETWEEN_DISCONTINUITIES_write(struct file *file, const char __user *buf, unsigned long count, void *data)
{
	char *page;
	char *myString;
	ssize_t ret = -ENOMEM;
	int result;
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
		if (strncmp("apply", myString, count - 1) == 0)
		{
			result = DvbStreamSetOption(ProcDeviceContext->VideoStream, PLAY_OPTION_STREAM_SINGLE_GROUP_BETWEEN_DISCONTINUITIES, PLAY_OPTION_VALUE_ENABLE);
			if (result != 0)
			{
				printk("failed to set policy\n");
			}
		}
		else if (strncmp("disapply", myString, count - 1) == 0)
		{
			result = DvbStreamSetOption(ProcDeviceContext->VideoStream, PLAY_OPTION_STREAM_SINGLE_GROUP_BETWEEN_DISCONTINUITIES, PLAY_OPTION_VALUE_DISABLE);
			if (result != 0)
			{
				printk("failed to set policy\n");
			}
		}
		else
		{
			printk("invalid value\n");
		}
		kfree(myString);
	}
	ret = count;
out:
	free_page((unsigned long)page);
	return ret;
}

int proc_stream_PLAYOUT_ON_TERMINATE_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused)
{
	int len = 0;
	unsigned int value;
	printk("%s\n", __FUNCTION__);
	if (DvbStreamGetOption(ProcDeviceContext->VideoStream, PLAY_OPTION_PLAYOUT_ON_TERMINATE, &value) == 0)
	{
		if (value == PLAY_OPTION_VALUE_ENABLE)
			len = sprintf(page, "apply\n");
		else
			len = sprintf(page, "disapply\n");
	}
	else
		len = sprintf(page, "failed to get value\n");
	return len;
}

int proc_stream_PLAYOUT_ON_TERMINATE_write(struct file *file, const char __user *buf, unsigned long count, void *data)
{
	char *page;
	char *myString;
	ssize_t ret = -ENOMEM;
	int result;
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
		if (strncmp("apply", myString, count - 1) == 0)
		{
			result = DvbStreamSetOption(ProcDeviceContext->VideoStream, PLAY_OPTION_PLAYOUT_ON_TERMINATE, PLAY_OPTION_VALUE_ENABLE);
			if (result != 0)
			{
				printk("failed to set policy\n");
			}
		}
		else if (strncmp("disapply", myString, count - 1) == 0)
		{
			result = DvbStreamSetOption(ProcDeviceContext->VideoStream, PLAY_OPTION_PLAYOUT_ON_TERMINATE, PLAY_OPTION_VALUE_DISABLE);
			if (result != 0)
			{
				printk("failed to set policy\n");
			}
		}
		else
		{
			printk("invalid value\n");
		}
		kfree(myString);
	}
	ret = count;
out:
	free_page((unsigned long)page);
	return ret;
}

int proc_stream_PLAYOUT_ON_SWITCH_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused)
{
	int len = 0;
	unsigned int value;
	printk("%s\n", __FUNCTION__);
	if (DvbStreamGetOption(ProcDeviceContext->VideoStream, PLAY_OPTION_PLAYOUT_ON_SWITCH, &value) == 0)
	{
		if (value == PLAY_OPTION_VALUE_ENABLE)
			len = sprintf(page, "apply\n");
		else
			len = sprintf(page, "disapply\n");
	}
	else
		len = sprintf(page, "failed to get value\n");
	return len;
}

int proc_stream_PLAYOUT_ON_SWITCH_write(struct file *file, const char __user *buf, unsigned long count, void *data)
{
	char *page;
	char *myString;
	ssize_t ret = -ENOMEM;
	int result;
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
		if (strncmp("apply", myString, count - 1) == 0)
		{
			result = DvbStreamSetOption(ProcDeviceContext->VideoStream, PLAY_OPTION_PLAYOUT_ON_SWITCH, PLAY_OPTION_VALUE_ENABLE);
			if (result != 0)
			{
				printk("failed to set policy\n");
			}
		}
		else if (strncmp("disapply", myString, count - 1) == 0)
		{
			result = DvbStreamSetOption(ProcDeviceContext->VideoStream, PLAY_OPTION_PLAYOUT_ON_SWITCH, PLAY_OPTION_VALUE_DISABLE);
			if (result != 0)
			{
				printk("failed to set policy\n");
			}
		}
		else
		{
			printk("invalid value\n");
		}
		kfree(myString);
	}
	ret = count;
out:
	free_page((unsigned long)page);
	return ret;
}

int proc_stream_PLAYOUT_ON_DRAIN_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused)
{
	int len = 0;
	unsigned int value;
	printk("%s\n", __FUNCTION__);
	if (DvbStreamGetOption(ProcDeviceContext->VideoStream, PLAY_OPTION_PLAYOUT_ON_DRAIN, &value) == 0)
	{
		if (value == PLAY_OPTION_VALUE_ENABLE)
			len = sprintf(page, "apply\n");
		else
			len = sprintf(page, "disapply\n");
	}
	else
		len = sprintf(page, "failed to get value\n");
	return len;
}

int proc_stream_PLAYOUT_ON_DRAIN_write(struct file *file, const char __user *buf, unsigned long count, void *data)
{
	char *page;
	char *myString;
	ssize_t ret = -ENOMEM;
	int result;
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
		if (strncmp("apply", myString, count - 1) == 0)
		{
			result = DvbStreamSetOption(ProcDeviceContext->VideoStream, PLAY_OPTION_PLAYOUT_ON_DRAIN, PLAY_OPTION_VALUE_ENABLE);
			if (result != 0)
			{
				printk("failed to set policy\n");
			}
		}
		else if (strncmp("disapply", myString, count - 1) == 0)
		{
			result = DvbStreamSetOption(ProcDeviceContext->VideoStream, PLAY_OPTION_PLAYOUT_ON_DRAIN, PLAY_OPTION_VALUE_DISABLE);
			if (result != 0)
			{
				printk("failed to set policy\n");
			}
		}
		else
		{
			printk("invalid value\n");
		}
		kfree(myString);
	}
	ret = count;
out:
	free_page((unsigned long)page);
	return ret;
}

int proc_stream_TRICK_MODE_DOMAIN_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused)
{
	int len = 0;
	unsigned int value;
	printk("%s\n", __FUNCTION__);
	if (DvbStreamGetOption(ProcDeviceContext->VideoStream, PLAY_OPTION_TRICK_MODE_DOMAIN, &value) == 0)
	{
		if (value == PLAY_OPTION_VALUE_TRICK_MODE_AUTO)
			len = sprintf(page, "auto\n");
		if (value == PLAY_OPTION_VALUE_TRICK_MODE_DECODE_ALL)
			len = sprintf(page, "decode_all\n");
		if (value == PLAY_OPTION_VALUE_TRICK_MODE_DECODE_ALL_DEGRADE_NON_REFERENCE_FRAMES)
			len = sprintf(page, "decode_all_degrade_non_reference_frames\n");
		if (value == PLAY_OPTION_VALUE_TRICK_MODE_START_DISCARDING_NON_REFERENCE_FRAMES)
			len = sprintf(page, "start_discarding_non_reference_frames\n");
		if (value == PLAY_OPTION_VALUE_TRICK_MODE_DECODE_REFERENCE_FRAMES_DEGRADE_NON_KEY_FRAMES)
			len = sprintf(page, "decode_reference_frames_degrade_non_key_frames\n");
		if (value == PLAY_OPTION_VALUE_TRICK_MODE_DECODE_KEY_FRAMES)
			len = sprintf(page, "decode_key_frames\n");
		if (value == PLAY_OPTION_VALUE_TRICK_MODE_DISCONTINUOUS_KEY_FRAMES)
			len = sprintf(page, "discontinuous_key_frames\n");
		else
			len = sprintf(page, "disapply\n");
	}
	else
		len = sprintf(page, "failed to get value\n");
	return len;
}

int proc_stream_TRICK_MODE_DOMAIN_write(struct file *file, const char __user *buf, unsigned long count, void *data)
{
	char *page;
	char *myString;
	ssize_t ret = -ENOMEM;
	int result;
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
		if (strncmp("auto", myString, count - 1) == 0)
		{
			result = DvbStreamSetOption(ProcDeviceContext->VideoStream, PLAY_OPTION_TRICK_MODE_DOMAIN, PLAY_OPTION_VALUE_TRICK_MODE_AUTO);
			if (result != 0)
			{
				printk("failed to set policy\n");
			}
		}
		else if (strncmp("decode_all", myString, count - 1) == 0)
		{
			result = DvbStreamSetOption(ProcDeviceContext->VideoStream, PLAY_OPTION_TRICK_MODE_DOMAIN, PLAY_OPTION_VALUE_TRICK_MODE_DECODE_ALL);
			if (result != 0)
			{
				printk("failed to set policy\n");
			}
		}
		else if (strncmp("decode_all_degrade_non_reference_frames", myString, count - 1) == 0)
		{
			result = DvbStreamSetOption(ProcDeviceContext->VideoStream, PLAY_OPTION_TRICK_MODE_DOMAIN, PLAY_OPTION_VALUE_TRICK_MODE_DECODE_ALL_DEGRADE_NON_REFERENCE_FRAMES);
			if (result != 0)
			{
				printk("failed to set policy\n");
			}
		}
		else if (strncmp("start_discarding_non_reference_frames", myString, count - 1) == 0)
		{
			result = DvbStreamSetOption(ProcDeviceContext->VideoStream, PLAY_OPTION_TRICK_MODE_DOMAIN, PLAY_OPTION_VALUE_TRICK_MODE_START_DISCARDING_NON_REFERENCE_FRAMES);
			if (result != 0)
			{
				printk("failed to set policy\n");
			}
		}
		else if (strncmp("decode_reference_frames_degrade_non_key_frames", myString, count - 1) == 0)
		{
			result = DvbStreamSetOption(ProcDeviceContext->VideoStream, PLAY_OPTION_TRICK_MODE_DOMAIN, PLAY_OPTION_VALUE_TRICK_MODE_DECODE_REFERENCE_FRAMES_DEGRADE_NON_KEY_FRAMES);
			if (result != 0)
			{
				printk("failed to set policy\n");
			}
		}
		else if (strncmp("decode_key_frames", myString, count - 1) == 0)
		{
			result = DvbStreamSetOption(ProcDeviceContext->VideoStream, PLAY_OPTION_TRICK_MODE_DOMAIN, PLAY_OPTION_VALUE_TRICK_MODE_DECODE_KEY_FRAMES);
			if (result != 0)
			{
				printk("failed to set policy\n");
			}
		}
		else if (strncmp("discontinuous_key_frames", myString, count - 1) == 0)
		{
			result = DvbStreamSetOption(ProcDeviceContext->VideoStream, PLAY_OPTION_TRICK_MODE_DOMAIN, PLAY_OPTION_VALUE_TRICK_MODE_DISCONTINUOUS_KEY_FRAMES);
			if (result != 0)
			{
				printk("failed to set policy\n");
			}
		}
		else
		{
			printk("invalid value\n");
		}
		kfree(myString);
	}
	ret = count;
out:
	free_page((unsigned long)page);
	return ret;
}

int proc_stream_DISCARD_LATE_FRAMES_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused)
{
	int len = 0;
	unsigned int value;
	printk("%s\n", __FUNCTION__);
	if (DvbStreamGetOption(ProcDeviceContext->VideoStream, PLAY_OPTION_DISCARD_LATE_FRAMES, &value) == 0)
	{
		if (value == PLAY_OPTION_VALUE_DISCARD_LATE_FRAMES_NEVER)
			len = sprintf(page, "never\n");
		else if (value == PLAY_OPTION_VALUE_DISCARD_LATE_FRAMES_ALWAYS)
			len = sprintf(page, "always\n");
		else
			len = sprintf(page, "aftersync\n");
	}
	else
		len = sprintf(page, "failed to get value\n");
	return len;
}

int proc_stream_DISCARD_LATE_FRAMES_write(struct file *file, const char __user *buf, unsigned long count, void *data)
{
	char *page;
	char *myString;
	ssize_t ret = -ENOMEM;
	int result;
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
		if (strncmp("always", myString, count - 1) == 0)
		{
			result = DvbStreamSetOption(ProcDeviceContext->VideoStream, PLAY_OPTION_DISCARD_LATE_FRAMES, PLAY_OPTION_VALUE_DISCARD_LATE_FRAMES_ALWAYS);
			if (result != 0)
			{
				printk("failed to set policy\n");
			}
		}
		else if (strncmp("never", myString, count - 1) == 0)
		{
			result = DvbStreamSetOption(ProcDeviceContext->VideoStream, PLAY_OPTION_DISCARD_LATE_FRAMES, PLAY_OPTION_VALUE_DISCARD_LATE_FRAMES_NEVER);
			if (result != 0)
			{
				printk("failed to set policy\n");
			}
		}
		else if (strncmp("aftersync", myString, count - 1) == 0)
		{
			result = DvbStreamSetOption(ProcDeviceContext->VideoStream, PLAY_OPTION_DISCARD_LATE_FRAMES, PLAY_OPTION_VALUE_DISCARD_LATE_FRAMES_AFTER_SYNCHRONIZE);
			if (result != 0)
			{
				printk("failed to set policy\n");
			}
		}
		else
		{
			printk("invalid value\n");
		}
		kfree(myString);
	}
	ret = count;
out:
	free_page((unsigned long)page);
	return ret;
}

int proc_stream_REBASE_ON_DATA_DELIVERY_LATE_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused)
{
	int len = 0;
	unsigned int value;
	printk("%s\n", __FUNCTION__);
	if (DvbStreamGetOption(ProcDeviceContext->VideoStream, PLAY_OPTION_REBASE_ON_DATA_DELIVERY_LATE, &value) == 0)
	{
		if (value == PLAY_OPTION_VALUE_ENABLE)
			len = sprintf(page, "apply\n");
		else
			len = sprintf(page, "disapply\n");
	}
	else
		len = sprintf(page, "failed to get value\n");
	return len;
}

int proc_stream_REBASE_ON_DATA_DELIVERY_LATE_write(struct file *file, const char __user *buf, unsigned long count, void *data)
{
	char *page;
	char *myString;
	ssize_t ret = -ENOMEM;
	int result;
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
		if (strncmp("apply", myString, count - 1) == 0)
		{
			result = DvbStreamSetOption(ProcDeviceContext->VideoStream, PLAY_OPTION_REBASE_ON_DATA_DELIVERY_LATE, PLAY_OPTION_VALUE_ENABLE);
			if (result != 0)
			{
				printk("failed to set policy\n");
			}
		}
		else if (strncmp("disapply", myString, count - 1) == 0)
		{
			result = DvbStreamSetOption(ProcDeviceContext->VideoStream, PLAY_OPTION_REBASE_ON_DATA_DELIVERY_LATE, PLAY_OPTION_VALUE_DISABLE);
			if (result != 0)
			{
				printk("failed to set policy\n");
			}
		}
		else
		{
			printk("invalid value\n");
		}
		kfree(myString);
	}
	ret = count;
out:
	free_page((unsigned long)page);
	return ret;
}

int proc_stream_REBASE_ON_FRAME_DECODE_LATE_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused)
{
	int len = 0;
	unsigned int value;
	printk("%s\n", __FUNCTION__);
	if (DvbStreamGetOption(ProcDeviceContext->VideoStream, PLAY_OPTION_REBASE_ON_FRAME_DECODE_LATE, &value) == 0)
	{
		if (value == PLAY_OPTION_VALUE_ENABLE)
			len = sprintf(page, "apply\n");
		else
			len = sprintf(page, "disapply\n");
	}
	else
		len = sprintf(page, "failed to get value\n");
	return len;
}

int proc_stream_REBASE_ON_FRAME_DECODE_LATE_write(struct file *file, const char __user *buf, unsigned long count, void *data)
{
	char *page;
	char *myString;
	ssize_t ret = -ENOMEM;
	int result;
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
		if (strncmp("apply", myString, count - 1) == 0)
		{
			result = DvbStreamSetOption(ProcDeviceContext->VideoStream, PLAY_OPTION_REBASE_ON_FRAME_DECODE_LATE, PLAY_OPTION_VALUE_ENABLE);
			if (result != 0)
			{
				printk("failed to set policy\n");
			}
		}
		else if (strncmp("disapply", myString, count - 1) == 0)
		{
			result = DvbStreamSetOption(ProcDeviceContext->VideoStream, PLAY_OPTION_REBASE_ON_FRAME_DECODE_LATE, PLAY_OPTION_VALUE_DISABLE);
			if (result != 0)
			{
				printk("failed to set policy\n");
			}
		}
		else
		{
			printk("invalid value\n");
		}
		kfree(myString);
	}
	ret = count;
out:
	free_page((unsigned long)page);
	return ret;
}

int proc_stream_LOWER_CODEC_DECODE_LIMITS_ON_FRAME_DECODE_LATE_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused)
{
	int len = 0;
	unsigned int value;
	printk("%s\n", __FUNCTION__);
	if (DvbStreamGetOption(ProcDeviceContext->VideoStream, PLAY_OPTION_LOWER_CODEC_DECODE_LIMITS_ON_FRAME_DECODE_LATE, &value) == 0)
	{
		if (value == PLAY_OPTION_VALUE_ENABLE)
			len = sprintf(page, "apply\n");
		else
			len = sprintf(page, "disapply\n");
	}
	else
		len = sprintf(page, "failed to get value\n");
	return len;
}

int proc_stream_LOWER_CODEC_DECODE_LIMITS_ON_FRAME_DECODE_LATE_write(struct file *file, const char __user *buf, unsigned long count, void *data)
{
	char *page;
	char *myString;
	ssize_t ret = -ENOMEM;
	int result;
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
		if (strncmp("apply", myString, count - 1) == 0)
		{
			result = DvbStreamSetOption(ProcDeviceContext->VideoStream, PLAY_OPTION_LOWER_CODEC_DECODE_LIMITS_ON_FRAME_DECODE_LATE, PLAY_OPTION_VALUE_ENABLE);
			if (result != 0)
			{
				printk("failed to set policy\n");
			}
		}
		else if (strncmp("disapply", myString, count - 1) == 0)
		{
			result = DvbStreamSetOption(ProcDeviceContext->VideoStream, PLAY_OPTION_LOWER_CODEC_DECODE_LIMITS_ON_FRAME_DECODE_LATE, PLAY_OPTION_VALUE_DISABLE);
			if (result != 0)
			{
				printk("failed to set policy\n");
			}
		}
		else
		{
			printk("invalid value\n");
		}
		kfree(myString);
	}
	ret = count;
out:
	free_page((unsigned long)page);
	return ret;
}

int proc_stream_H264_ALLOW_NON_IDR_RESYNCHRONIZATION_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused)
{
	int len = 0;
	unsigned int value;
	printk("%s\n", __FUNCTION__);
	if (DvbStreamGetOption(ProcDeviceContext->VideoStream, PLAY_OPTION_H264_ALLOW_NON_IDR_RESYNCHRONIZATION, &value) == 0)
	{
		if (value == PLAY_OPTION_VALUE_ENABLE)
			len = sprintf(page, "apply\n");
		else
			len = sprintf(page, "disapply\n");
	}
	else
		len = sprintf(page, "failed to get value\n");
	return len;
}

int proc_stream_H264_ALLOW_NON_IDR_RESYNCHRONIZATION_write(struct file *file, const char __user *buf, unsigned long count, void *data)
{
	char *page;
	char *myString;
	ssize_t ret = -ENOMEM;
	int result;
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
		if (strncmp("apply", myString, count - 1) == 0)
		{
			result = DvbStreamSetOption(ProcDeviceContext->VideoStream, PLAY_OPTION_H264_ALLOW_NON_IDR_RESYNCHRONIZATION, PLAY_OPTION_VALUE_ENABLE);
			if (result != 0)
			{
				printk("failed to set policy\n");
			}
		}
		else if (strncmp("disapply", myString, count - 1) == 0)
		{
			result = DvbStreamSetOption(ProcDeviceContext->VideoStream, PLAY_OPTION_H264_ALLOW_NON_IDR_RESYNCHRONIZATION, PLAY_OPTION_VALUE_DISABLE);
			if (result != 0)
			{
				printk("failed to set policy\n");
			}
		}
		else
		{
			printk("invalid value\n");
		}
		kfree(myString);
	}
	ret = count;
out:
	free_page((unsigned long)page);
	return ret;
}

int proc_stream_MPEG2_IGNORE_PROGESSIVE_FRAME_FLAG_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused)
{
	int len = 0;
	unsigned int value;
	printk("%s\n", __FUNCTION__);
	if (DvbStreamGetOption(ProcDeviceContext->VideoStream, PLAY_OPTION_MPEG2_IGNORE_PROGESSIVE_FRAME_FLAG, &value) == 0)
	{
		if (value == PLAY_OPTION_VALUE_ENABLE)
			len = sprintf(page, "apply\n");
		else
			len = sprintf(page, "disapply\n");
	}
	else
		len = sprintf(page, "failed to get value\n");
	return len;
}

int proc_stream_MPEG2_IGNORE_PROGESSIVE_FRAME_FLAG_write(struct file *file, const char __user *buf, unsigned long count, void *data)
{
	char *page;
	char *myString;
	ssize_t ret = -ENOMEM;
	int result;
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
		if (strncmp("apply", myString, count - 1) == 0)
		{
			result = DvbStreamSetOption(ProcDeviceContext->VideoStream, PLAY_OPTION_MPEG2_IGNORE_PROGESSIVE_FRAME_FLAG, PLAY_OPTION_VALUE_ENABLE);
			if (result != 0)
			{
				printk("failed to set policy\n");
			}
		}
		else if (strncmp("disapply", myString, count - 1) == 0)
		{
			result = DvbStreamSetOption(ProcDeviceContext->VideoStream, PLAY_OPTION_MPEG2_IGNORE_PROGESSIVE_FRAME_FLAG, PLAY_OPTION_VALUE_DISABLE);
			if (result != 0)
			{
				printk("failed to set policy\n");
			}
		}
		else
		{
			printk("invalid value\n");
		}
		kfree(myString);
	}
	ret = count;
out:
	free_page((unsigned long)page);
	return ret;
}
