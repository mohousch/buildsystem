/*
 * e2_proc_hdmi.c
 */

#include <linux/proc_fs.h> /* proc fs */
#include <asm/uaccess.h> /* copy_from_user */

#include <linux/dvb/video.h> /* Video Format etc */

#include <linux/dvb/audio.h>
#include <linux/smp_lock.h>
#include <linux/string.h>
#include <linux/kernel.h> /* sscanf */
#include <linux/fb.h>

#include "../backend.h"
#include "../dvb_module.h"
#include "linux/dvb/stm_ioctls.h"

#define STMHDMIIO_AUDIO_SOURCE_2CH_I2S (0)
#define STMHDMIIO_AUDIO_SOURCE_PCM STMHDMIIO_AUDIO_SOURCE_2CH_I2S
#define STMHDMIIO_AUDIO_SOURCE_SPDIF (1)
#define STMHDMIIO_AUDIO_SOURCE_8CH_I2S (2)
#define STMHDMIIO_AUDIO_SOURCE_NONE (0xffffffff)

#define STMHDMIIO_EDID_STRICT_MODE_HANDLING 0
#define STMHDMIIO_EDID_NON_STRICT_MODE_HANDLING 1

#define STMFBIO_ACTIVATE_IMMEDIATE 0
#define STMFBIO_OUTPUT_CAPS_HDMI_CONFIG (1L<<3)
#define STMFBIO_OUTPUT_HDMI_DISABLED (1L<<0)

struct stmfb_info;
struct stmfbio_output_configuration;

extern long stmhdmiio_set_audio_source(unsigned int arg);
extern long stmhdmiio_get_audio_source(unsigned int *arg);

extern long stmhdmiio_set_edid_handling(unsigned int arg);
extern long stmhdmiio_get_edid_handling(unsigned int *arg);

extern int stmfb_set_output_configuration(struct stmfbio_output_configuration *c, struct stmfb_info *i);
extern int stmfb_get_output_configuration(struct stmfbio_output_configuration *c, struct stmfb_info *i);
struct stmfb_info *stmfb_get_fbinfo_ptr(void);

//stgfb.h
struct stmfbio_output_configuration
{
	__u32 outputid;
	__u32 caps;
	__u32 failed;
	__u32 activate;

	__u32 sdtv_encoding;
	__u32 analogue_config;
	__u32 dvo_config;
	__u32 hdmi_config;
	__u32 mixer_background;
	__u8 brightness;
	__u8 saturation;
	__u8 contrast;
	__u8 hue;
};

extern struct DeviceContext_s *ProcDeviceContext;

int proc_hdmi_audio_source_write(struct file *file, const char __user *buf,
								 unsigned long count, void *data)
{
	char *page;
	ssize_t ret = -ENOMEM;
	unsigned int value;
	char *myString = kmalloc(count + 1, GFP_KERNEL);
	printk("%s %ld - ", __FUNCTION__, count);
	mutex_lock(&(ProcDeviceContext->DvbContext->Lock));
	page = (char *)__get_free_page(GFP_KERNEL);
	if (page)
	{
		ret = -EFAULT;
		if (copy_from_user(page, buf, count))
			goto out;
		strncpy(myString, page, count);
		myString[count] = '\0';
		printk("%s\n", myString);
		if (strncmp("spdif", myString, count - 1) == 0)
		{
			value = STMHDMIIO_AUDIO_SOURCE_SPDIF;
		}
		else if (strncmp("pcm", myString, count - 1) == 0)
		{
			value = STMHDMIIO_AUDIO_SOURCE_PCM;
		}
		else if (strncmp("8ch", myString, count - 1) == 0)
		{
			value = STMHDMIIO_AUDIO_SOURCE_8CH_I2S;
		}
		else
		{
			value = STMHDMIIO_AUDIO_SOURCE_NONE;
		}
		stmhdmiio_set_audio_source(value);
		/* always return count to avoid endless loop */
		ret = count;
	}
out:
	free_page((unsigned long)page);
	kfree(myString);
	mutex_unlock(&(ProcDeviceContext->DvbContext->Lock));
	return ret;
}

int proc_hdmi_audio_source_read(char *page, char **start, off_t off, int count,
								int *eof, void *data_unused)
{
	int len = 0;
	unsigned int value = 0;
	printk("%s\n", __FUNCTION__);
	mutex_lock(&(ProcDeviceContext->DvbContext->Lock));
	stmhdmiio_get_audio_source(&value);
	printk("%s - %u\n", __FUNCTION__, value);
	switch (value)
	{
		case STMHDMIIO_AUDIO_SOURCE_2CH_I2S:
			len = sprintf(page, "pcm\n");
			break;
		case STMHDMIIO_AUDIO_SOURCE_SPDIF:
			len = sprintf(page, "spdif\n");
			break;
		case STMHDMIIO_AUDIO_SOURCE_8CH_I2S:
			len = sprintf(page, "8ch\n");
			break;
		case STMHDMIIO_AUDIO_SOURCE_NONE:
		default:
			len = sprintf(page, "none\n");
			break;
	}
	mutex_unlock(&(ProcDeviceContext->DvbContext->Lock));
	return len;
}

int proc_hdmi_audio_source_choices_read(char *page, char **start, off_t off, int count,
										int *eof, void *data_unused)
{
	int len = 0;
	printk("%s\n", __FUNCTION__);
	mutex_lock(&(ProcDeviceContext->DvbContext->Lock));
	len = sprintf(page, "pcm spdif 8ch none\n");
	mutex_unlock(&(ProcDeviceContext->DvbContext->Lock));
	return len;
}

int proc_hdmi_edid_handling_write(struct file *file, const char __user *buf,
								  unsigned long count, void *data)
{
	char *page;
	ssize_t ret = -ENOMEM;
	unsigned int value;
	int argument = -1;
	int test = -1;
	char *myString = kmalloc(count + 1, GFP_KERNEL);
	printk("%s %ld - ", __FUNCTION__, count);
	mutex_lock(&(ProcDeviceContext->DvbContext->Lock));
	page = (char *)__get_free_page(GFP_KERNEL);
	if (page)
	{
		ret = -EFAULT;
		if (copy_from_user(page, buf, count))
			goto out;
		strncpy(myString, page, count);
		myString[count] = '\0';
		printk("[HDMI] EDID handling arg: %s\n", myString);
		test = sscanf(myString, "%d", &argument);
		if ((0 < test) && (0 == argument))
		{
			value = STMHDMIIO_EDID_STRICT_MODE_HANDLING;
		}
		else
		{
			value = STMHDMIIO_EDID_NON_STRICT_MODE_HANDLING;
		}
		printk("[HDMI] EDID handling: %s\n", (value == STMHDMIIO_EDID_STRICT_MODE_HANDLING) ? "strict" : "none strict");
		stmhdmiio_set_edid_handling(value);
		/* always return count to avoid endless loop */
		ret = count;
	}
out:
	free_page((unsigned long)page);
	kfree(myString);
	mutex_unlock(&(ProcDeviceContext->DvbContext->Lock));
	return ret;
}

int proc_hdmi_edid_handling_read(char *page, char **start, off_t off, int count,
								 int *eof, void *data_unused)
{
	int len = 0;
	unsigned int value = 0;
	printk("%s\n", __FUNCTION__);
	mutex_lock(&(ProcDeviceContext->DvbContext->Lock));
	stmhdmiio_get_edid_handling(&value);
	printk("%s - %u\n", __FUNCTION__, value);
	switch (value)
	{
		case STMHDMIIO_EDID_NON_STRICT_MODE_HANDLING:
			len = sprintf(page, "00000001\n");
			break;
		case STMHDMIIO_EDID_STRICT_MODE_HANDLING:
		default:
			len = sprintf(page, "00000000\n");
			break;
	}
	mutex_unlock(&(ProcDeviceContext->DvbContext->Lock));
	return len;
}

int proc_hdmi_output_write(struct file *file, const char __user *buf,
						   unsigned long count, void *data)
{
	char *page;
	ssize_t ret = -ENOMEM;
	struct stmfbio_output_configuration outputConfig = {0};
	struct stmfb_info *info = NULL;
	char *myString = kmalloc(count + 1, GFP_KERNEL);
	printk("%s %ld - ", __FUNCTION__, count);
	mutex_lock(&(ProcDeviceContext->DvbContext->Lock));
	page = (char *)__get_free_page(GFP_KERNEL);
	if (page)
	{
		ret = -EFAULT;
		if (copy_from_user(page, buf, count))
			goto out;
		strncpy(myString, page, count);
		myString[count] = '\0';
		printk("%s\n", myString);
		outputConfig.outputid = 1;
		info = stmfb_get_fbinfo_ptr();
		stmfb_get_output_configuration(&outputConfig, info);
		outputConfig.caps = 0;
		outputConfig.activate = STMFBIO_ACTIVATE_IMMEDIATE;
		outputConfig.analogue_config = 0;
		outputConfig.caps |= STMFBIO_OUTPUT_CAPS_HDMI_CONFIG;
		if (!strncmp("off", myString, count))
		{
			outputConfig.hdmi_config |= STMFBIO_OUTPUT_HDMI_DISABLED;
		}
		else if (!strncmp("on", myString, count))
		{
			outputConfig.hdmi_config &= ~STMFBIO_OUTPUT_HDMI_DISABLED;
		}
		stmfb_set_output_configuration(&outputConfig, info);
		/* always return count to avoid endless loop */
		ret = count;
	}
out:
	free_page((unsigned long)page);
	kfree(myString);
	mutex_unlock(&(ProcDeviceContext->DvbContext->Lock));
	return ret;
}

int proc_hdmi_output_read(char *page, char **start, off_t off, int count,
						  int *eof, void *data_unused)
{
	int len = 0;
	unsigned int disabled = 0;
	struct stmfbio_output_configuration outputConfig = {0};
	struct stmfb_info *info = NULL;
	mutex_lock(&(ProcDeviceContext->DvbContext->Lock));
	outputConfig.outputid = 1;
	info = stmfb_get_fbinfo_ptr();
	stmfb_get_output_configuration(&outputConfig, info);
	disabled = (outputConfig.hdmi_config & STMFBIO_OUTPUT_HDMI_DISABLED) ? 1 : 0;
	printk("%s - %u\n", __FUNCTION__, disabled);
	switch (disabled)
	{
		case 0:
			len = sprintf(page, "on\n");
			break;
		case 1:
		default:
			len = sprintf(page, "off\n");
			break;
	}
	mutex_unlock(&(ProcDeviceContext->DvbContext->Lock));
	return len;
}

int proc_hdmi_output_choices_read(char *page, char **start, off_t off, int count,
								  int *eof, void *data_unused)
{
	int len = 0;
	printk("%s\n", __FUNCTION__);
	len = sprintf(page, "on off\n");
	return len;
}
