/*
 * e2_proc_avs.c
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

//Dagobert Hack
#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
#include <sound/driver.h>
#endif
#include <sound/core.h>
#include <sound/control.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/rawmidi.h>
#include <sound/initval.h>

struct stmfb_info;
struct stmfbio_output_configuration;

extern struct snd_kcontrol **pseudoGetControls(int *numbers);
extern int snd_pseudo_integer_get(struct snd_kcontrol *kcontrol,
								  struct snd_ctl_elem_value *ucontrol);
extern int snd_pseudo_integer_put(struct snd_kcontrol *kcontrol,
								  struct snd_ctl_elem_value *ucontrol);
extern int avs_command_kernel(unsigned int cmd, void *arg);

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

#if defined(ADB_BOX)
#define SAAIOSWSS 10 /* set wide screen signaling data */
#define SAA_WSS_OFF 8
#define SAA_WSS_43F 0

#define STMFBIO_OUTPUT_HDMI_ENABLED (0)
#define STMFBIO_OUTPUT_HDMI_DISABLED (1L<<0)
#endif

#define STMFBIO_OUTPUT_CAPS_HDMI_CONFIG (1L<<3)
#define STMFBIO_OUTPUT_CAPS_ANALOGUE_CONFIG (1L<<1)
#define STMFBIO_OUTPUT_HDMI_RGB (0)
#define STMFBIO_OUTPUT_HDMI_YUV (1L<<1)
#define STMFBIO_OUTPUT_HDMI_444 (0)
#define STMFBIO_OUTPUT_HDMI_422 (1L<<2)

#define STMFBIO_OUTPUT_ANALOGUE_RGB (1L<<0)
#define STMFBIO_OUTPUT_ANALOGUE_YPrPb (1L<<1)
#define STMFBIO_OUTPUT_ANALOGUE_YC (1L<<2)
#define STMFBIO_OUTPUT_ANALOGUE_CVBS (1L<<3)

#include "../../../../../sound/pseudocard/pseudo_mixer.h"

#define PSEUDO_ADDR(x) (offsetof(struct snd_pseudo_mixer_settings, x))

#include "../../../../../../../../avs/avs_core.h"

#define AVSIOSET 0x1000
#define AVSIOSTANDBY (99|AVSIOSET)

#define ENCODER 0
#define SCART 1

extern struct DeviceContext_s *DeviceContext;

static int current_standby = 0;
static int current_input = ENCODER;
#if defined(CUBEREVO) \
 || defined(CUBEREVO_MINI) \
 || defined(CUBEREVO_MINI2) \
 || defined(CUBEREVO_MINI_FTA) \
 || defined(CUBEREVO_250HD) \
 || defined(CUBEREVO_2000HD) \
 || defined(CUBEREVO_9500HD) \
 || defined(CUBEREVO_3000HD) \
 || defined(TF7700) \
 || defined(UFS912) \
 || defined(UFS922) \
 || defined(UFC960) \
 || defined(HL101) \
 || defined(VIP1_V2) \
 || defined(VIP2_V1) \
 || defined(ATEVIO7500) \
 || defined(HS7110) \
 || defined(HS7810A) \
 || defined(HS7420) \
 || defined(HS7429) \
 || defined(HS7119) \
 || defined(HS7819) \
 || defined(ATEMIO520) \
 || defined(ATEMIO530) \
 || defined(IPBOX9900) \
 || defined(IPBOX99) \
 || defined(IPBOX55) \
 || defined(ADB_BOX) \
 || defined(VITAMIN_HD5000)
static int current_volume = 0;
#else
static int current_volume = 31;
#endif

static int current_e2_volume = 31;

int proc_avs_0_volume_write(struct file *file, const char __user *buf,
							unsigned long count, void *data)
{
#define cMaxAttenuationE2 64
	int logarithmicAttenuation[cMaxAttenuationE2] =
	{
		0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4,
		5, 5, 5, 6, 6, 6, 7, 7, 8, 8, 9, 9, 9, 10, 10, 11,
		11, 12, 13, 13, 14, 14, 15, 16, 16, 17, 18, 19, 19, 20, 21, 22,
		23, 24, 25, 27, 28, 29, 31, 33, 35, 37, 40, 43, 47, 51, 58, 70
	};
	char *page;
	char *myString;
	ssize_t ret = -ENOMEM;
#ifdef VERY_VERBOSE
	printk("%s %ld - ", __FUNCTION__, count);
#endif
	page = (char *)__get_free_page(GFP_KERNEL);
	if (page)
	{
		int number = 0;
		struct snd_kcontrol **kcontrol = pseudoGetControls(&number);
		struct snd_kcontrol *single_control = NULL;
		int vLoop;
		int volume = 0;
		ret = -EFAULT;
		if (file == NULL && data == NULL)
			strncpy(page, buf, count);
		else
		{
			if (copy_from_user(page, buf, count))
				goto out;
		}
		myString = (char *) kmalloc(count + 1, GFP_KERNEL);
		strncpy(myString, page, count);
		myString[count] = '\0';
#ifdef VERY_VERBOSE
		printk("%s\n", myString);
#endif
		sscanf(myString, "%d", &volume);
#if defined(CUBEREVO) \
 || defined(CUBEREVO_MINI) \
 || defined(CUBEREVO_MINI2) \
 || defined(CUBEREVO_MINI_FTA) \
 || defined(CUBEREVO_250HD) \
 || defined(CUBEREVO_2000HD) \
 || defined(CUBEREVO_9500HD) \
 || defined(CUBEREVO_3000HD) \
 || defined(TF7700) \
 || defined(UFS912) \
 || defined(UFS922) \
 || defined(UFC960) \
 || defined(HL101) \
 || defined(VIP1_V2) \
 || defined(VIP2_V1) \
 || defined(ATEVIO7500) \
 || defined(HS7110) \
 || defined(HS7810A) \
 || defined(HS7420) \
 || defined(HS7429) \
 || defined(HS7119) \
 || defined(HS7819) \
 || defined(ATEMIO520) \
 || defined(ATEMIO530) \
 || defined(IPBOX9900) \
 || defined(IPBOX99) \
 || defined(IPBOX55) \
 || defined(ADB_BOX) \
 || defined(VITAMIN_HD5000)
		current_volume = volume;
#else
		/* Dagobert: 04.10.2009: e2 delivers values from 0 to 63 db. the ak4705
		 * needs values which are a little bit more sophisticated.
		 * the range is from mute (value 0) to -60 db (value 1) to +6db (value 34)
		 * which is represented by 6 bits in 2db steps
		 *
		 * so we have a value range of 0 - 34. in my opinion the algo must be:
		 *
		 * current_volume = ((volume * 100) / 63 * 34) / 100;
		 * current_volume = 34 - current_volume;
		 *
		 * mybe someone could test it.
		 *
		 */
		current_volume = 31 - volume / 4;
#endif
		for (vLoop = 0; vLoop < number; vLoop++)
		{
			if (kcontrol[vLoop]->private_value == PSEUDO_ADDR(master_volume))
			{
				single_control = kcontrol[vLoop];
				//printk("Find master_volume control at %p\n", single_control);
				break;
			}
		}
		if ((kcontrol != NULL) && (single_control != NULL))
		{
			struct snd_ctl_elem_value ucontrol;
			if (volume > cMaxAttenuationE2 - 1)
				volume = cMaxAttenuationE2 - 1;
			else if (volume < 0)
				volume = 0;
			current_e2_volume = volume;
			/* Dagobert: 04.10.2009: e2 delivers values from 0 to 63 db. the current
			 * pseudo_mixer needs a value from 0 to "-70".
			 * ->see pseudo_mixer.c line 722.
			 */
			/* Dagobert: 06.10.2009: Volume is a logarithmical value ...

			 scale range

			 volume = ((volume * 100) / cMaxVolumeE2 * cMaxVolumePlayer) / 100;
			 volume = 0 - volume;
			*/
			volume = 0 - logarithmicAttenuation[current_e2_volume];
			//printk("Pseudo Mixer controls = %p\n", kcontrol);
			ucontrol.value.integer.value[0] = volume;
			ucontrol.value.integer.value[1] = volume;
			ucontrol.value.integer.value[2] = volume;
			ucontrol.value.integer.value[3] = volume;
			ucontrol.value.integer.value[4] = volume;
			ucontrol.value.integer.value[5] = volume;
			snd_pseudo_integer_put(single_control, &ucontrol);
		}
		else
		{
			printk("Pseudo Mixer does not deliver controls\n");
		}
		if (current_input == SCART)
			avs_command_kernel(AVSIOSVOL, (void *) current_volume);
		kfree(myString);
	}
	ret = count;
out:
	free_page((unsigned long)page);
	return ret;
}

int proc_avs_0_volume_read(char *page, char **start, off_t off, int count,
						   int *eof, void *data_unused)
{
	int len = 0;
#ifdef VERY_VERBOSE
	printk("%s\n", __FUNCTION__);
#endif
	len = sprintf(page, "%d\n", current_e2_volume);
	return len;
}

int proc_avs_0_input_choices_read(char *page, char **start, off_t off, int count,
								  int *eof, void *data_unused)
{
	int len = 0;
#ifdef VERY_VERBOSE
	printk("%s\n", __FUNCTION__);
#endif
	len = sprintf(page, "encoder scart\n");
	return len;
}

int proc_avs_0_input_write(struct file *file, const char __user *buf,
						   unsigned long count, void *data)
{
	char *page;
	char *myString;
	ssize_t ret = -ENOMEM;
	/* int result; */
#if defined(ADB_BOX)
	struct stmfbio_output_configuration outputConfig;
	struct stmfb_info *info = stmfb_get_fbinfo_ptr();
	int err;
	outputConfig.outputid = 1;
	stmfb_get_output_configuration(&outputConfig, info);
	outputConfig.caps = 0;
	outputConfig.activate = 0;//STMFBIO_ACTIVATE_IMMEDIATE;
	outputConfig.analogue_config = 0;
	outputConfig.caps |= STMFBIO_OUTPUT_CAPS_HDMI_CONFIG;
#endif
#ifdef VERY_VERBOSE
	printk("%s %ld - ", __FUNCTION__, count);
#endif
	page = (char *)__get_free_page(GFP_KERNEL);
	if (page)
	{
		ret = -EFAULT;
		if (copy_from_user(page, buf, count))
			goto out;
		myString = (char *) kmalloc(count + 1, GFP_KERNEL);
		strncpy(myString, page, count);
		myString[count] = '\0';
#ifdef VERY_VERBOSE
		printk("%s\n", myString);
#endif
		if (!strncmp("encoder", myString, count - 1))
		{
			avs_command_kernel(SAAIOSSRCSEL, SAA_SRC_ENC);
			// Note: Volumne is not changed directly but by using the MIXER instead of the AVS.
			// So this should always be set to the maximum
#if defined(UFS910) \
 || defined(ADB_BOX)
			avs_command_kernel(AVSIOSVOL, (void *) 31);
#else
			avs_command_kernel(AVSIOSVOL, (void *) 0);
#endif
#if defined(ADB_BOX)
			avs_command_kernel(SAAIOSWSS, (void *) SAA_WSS_43F);
			outputConfig.hdmi_config &= ~STMFBIO_OUTPUT_HDMI_DISABLED;
#endif
			current_input = ENCODER;
		}
		if (!strncmp("scart", myString, count - 1))
		{
			avs_command_kernel(SAAIOSSRCSEL, (void *) SAA_SRC_SCART);
			avs_command_kernel(AVSIOSVOL, (void *) current_volume);
#if defined(ADB_BOX)
			avs_command_kernel(SAAIOSWSS, (void *) SAA_WSS_OFF);
			outputConfig.hdmi_config |= STMFBIO_OUTPUT_HDMI_DISABLED;
#endif
			current_input = SCART;
		}
#if defined(ADB_BOX)
		err = stmfb_set_output_configuration(&outputConfig, info);
		if (err != 0)
		{
			printk("HDMI Config Disabled - Failed !!!!!!!!!!");
		}
#endif
		kfree(myString);
		//result = sscanf(page, "%3s %3s %3s %3s %3s", s1, s2, s3, s4, s5);
	}
	ret = count;
out:
	free_page((unsigned long)page);
	return ret;
}

int proc_avs_0_input_read(char *page, char **start, off_t off, int count,
						  int *eof, void *data_unused)
{
	int len = 0;
#ifdef VERY_VERBOSE
	printk("%s\n", __FUNCTION__);
#endif
	if (current_input == ENCODER)
		len = sprintf(page, "encoder\n");
	else
		len = sprintf(page, "scart\n");
	return len;
}

int proc_avs_0_fb_write(struct file *file, const char __user *buf,
						unsigned long count, void *data)
{
	char *page;
	char *myString;
	ssize_t ret = -ENOMEM;
	/* int result; */
#ifdef VERY_VERBOSE
	printk("%s %ld - ", __FUNCTION__, count);
#endif
	page = (char *)__get_free_page(GFP_KERNEL);
	if (page)
	{
		ret = -EFAULT;
		if (copy_from_user(page, buf, count))
			goto out;
		myString = (char *) kmalloc(count + 1, GFP_KERNEL);
		strncpy(myString, page, count);
		myString[count] = '\0';
#ifdef VERY_VERBOSE
		printk("%s\n", myString);
#endif
		kfree(myString);
		//result = sscanf(page, "%3s %3s %3s %3s %3s", s1, s2, s3, s4, s5);
	}
	ret = count;
out:
	free_page((unsigned long)page);
	return ret;
}

int proc_avs_0_fb_read(char *page, char **start, off_t off, int count,
					   int *eof, void *data_unused)
{
	int len = 0;
#ifdef VERY_VERBOSE
	printk("%s\n", __FUNCTION__);
#endif
	len = sprintf(page, "low\n");
	return len;
}

int proc_avs_0_colorformat_write(struct file *file, const char __user *buf,
								 unsigned long count, void *data)
{
	char *page;
	char *myString;
	ssize_t ret = -ENOMEM;
#ifdef VERY_VERBOSE
	printk("%s %ld - ", __FUNCTION__, count);
#endif
	page = (char *)__get_free_page(GFP_KERNEL);
	if (page)
	{
		struct stmfb_info *info = stmfb_get_fbinfo_ptr();
		struct stmfbio_output_configuration outputConfig;
		int err = 0;
		int alpha = 0;
		int hdmi_colour = 0;
		int scart_colour = 0;
		int hdmi0scart1yuv2 = 1;
		ret = -EFAULT;
		if (copy_from_user(page, buf, count))
			goto out;
		myString = (char *) kmalloc(count + 1, GFP_KERNEL);
		strncpy(myString, page, count);
		myString[count] = '\0';
#ifdef VERY_VERBOSE
		printk("%s\n", myString);
#endif
		sscanf(myString, "%d", &alpha);
//0rgb 1yuv 2422
		if (strncmp("hdmi_rgb", page, count - 1) == 0)
		{
			hdmi_colour = 0;
			hdmi0scart1yuv2 = 0;
		}
		else if (strncmp("hdmi_yuv", page, count - 1) == 0)
		{
			hdmi_colour = 1;
			hdmi0scart1yuv2 = 0;
		}
		else if (strncmp("hdmi_422", page, count - 1) == 0)
		{
			hdmi_colour = 2;
			hdmi0scart1yuv2 = 0;
		}
		else if (strncmp("rgb", page, count - 1) == 0)
		{
			scart_colour = SAA_MODE_RGB;
			hdmi0scart1yuv2 = 1;
		}
		else if (strncmp("cvbs", page, count - 1) == 0)
		{
			scart_colour = SAA_MODE_FBAS;
			hdmi0scart1yuv2 = 1;
		}
		else if (strncmp("svideo", page, count - 1) == 0)
		{
			scart_colour = SAA_MODE_SVIDEO;
			hdmi0scart1yuv2 = 1;
		}
		else if (strncmp("yuv", page, count - 1) == 0)
		{
			hdmi0scart1yuv2 = 2;
		}
		if (hdmi0scart1yuv2 == 0)
		{
			outputConfig.outputid = 1;
			stmfb_get_output_configuration(&outputConfig, info);
			outputConfig.caps = 0;
			outputConfig.activate = 0; //STMFBIO_ACTIVATE_IMMEDIATE;
			outputConfig.caps |= STMFBIO_OUTPUT_CAPS_HDMI_CONFIG;
			outputConfig.hdmi_config &= ~(STMFBIO_OUTPUT_HDMI_YUV | STMFBIO_OUTPUT_HDMI_422);
			switch (hdmi_colour)
			{
				case 1:
					outputConfig.hdmi_config |= STMFBIO_OUTPUT_HDMI_YUV;
					break;
				case 2:
					outputConfig.hdmi_config |= (STMFBIO_OUTPUT_HDMI_YUV | STMFBIO_OUTPUT_HDMI_422);
					break;
				default:
					break;
			}
			err = stmfb_set_output_configuration(&outputConfig, info);
		}
		else if (hdmi0scart1yuv2 == 1)
		{
			avs_command_kernel(SAAIOSMODE, (void *) scart_colour);
			outputConfig.outputid = 1;
			stmfb_get_output_configuration(&outputConfig, info);
			outputConfig.caps = 0;
			outputConfig.activate = 0; //STMFBIO_ACTIVATE_IMMEDIATE;
			outputConfig.caps |= STMFBIO_OUTPUT_CAPS_ANALOGUE_CONFIG;
			outputConfig.analogue_config = 0;
			switch (scart_colour)
			{
				case SAA_MODE_RGB:
					outputConfig.analogue_config |= (STMFBIO_OUTPUT_ANALOGUE_RGB | STMFBIO_OUTPUT_ANALOGUE_CVBS);
					break;
				case SAA_MODE_FBAS:
					outputConfig.analogue_config |= STMFBIO_OUTPUT_ANALOGUE_CVBS;
					break;
				case SAA_MODE_SVIDEO:
					outputConfig.analogue_config |= STMFBIO_OUTPUT_ANALOGUE_YC;
					break;
				default:
					break;
			}
			err = stmfb_set_output_configuration(&outputConfig, info);
			if (err != 0)
			{
				printk("SET SCART COLOR - %ld - ", count);
			}
		}
		else
		{
			outputConfig.outputid = 1;
			stmfb_get_output_configuration(&outputConfig, info);
			outputConfig.caps = 0;
			outputConfig.activate = 0; //STMFBIO_ACTIVATE_IMMEDIATE;
			outputConfig.caps |= STMFBIO_OUTPUT_CAPS_ANALOGUE_CONFIG;
			outputConfig.analogue_config = 0;
			outputConfig.analogue_config |= STMFBIO_OUTPUT_ANALOGUE_YPrPb;
			err = stmfb_set_output_configuration(&outputConfig, info);
			if (err != 0)
			{
				printk("SET SCART COLOR - %ld - ", count);
			}
		}
		//if(ioctl(fbfd, STMFBIO_SET_OUTPUT_CONFIG, &outputConfig)<0)
		//perror("setting output configuration failed");
		kfree(myString);
	}
	ret = count;
out:
	free_page((unsigned long)page);
	return ret;
}

int proc_avs_0_colorformat_read(char *page, char **start, off_t off, int count,
								int *eof, void *data_unused)
{
	struct stmfb_info *info = stmfb_get_fbinfo_ptr();
	struct stmfbio_output_configuration outputConfig;
	int len = 0;
#ifdef VERY_VERBOSE
	printk("%s %d\n", __FUNCTION__, count);
#endif
	outputConfig.outputid = 1;
	stmfb_get_output_configuration(&outputConfig, info);
	if (outputConfig.hdmi_config & STMFBIO_OUTPUT_HDMI_422)
		len = sprintf(page, "hdmi_422\n");
	else if (outputConfig.hdmi_config & STMFBIO_OUTPUT_HDMI_YUV)
		len = sprintf(page, "hdmi_yuv\n");
	else
		len = sprintf(page, "hdmi_rgb\n");
	if (outputConfig.analogue_config & STMFBIO_OUTPUT_ANALOGUE_RGB)
		len += sprintf(page + len, "rgb\n");
	else if (outputConfig.analogue_config & STMFBIO_OUTPUT_ANALOGUE_CVBS)
		len += sprintf(page + len, "cvbs\n");
	else if (outputConfig.analogue_config & STMFBIO_OUTPUT_ANALOGUE_YC)
		len += sprintf(page + len, "svideo\n");
	else if (outputConfig.analogue_config & STMFBIO_OUTPUT_ANALOGUE_YPrPb)
		len += sprintf(page + len, "yuv\n");
	else
		len += sprintf(page + len, "not defined\n");
	return len;
}

int proc_avs_0_colorformat_choices_read(char *page, char **start, off_t off, int count,
										int *eof, void *data_unused)
{
	int len = 0;
#ifdef VERY_VERBOSE
	printk("%s\n", __FUNCTION__);
#endif
	len = sprintf(page, "rgb cvbs svideo yuv hdmi_rgb hdmi_yuv hdmi_422\n");
	return len;
}

int proc_avs_0_sb_write(struct file *file, const char __user *buf,
						unsigned long count, void *data)
{
	char *page;
	char *myString;
	ssize_t ret = -ENOMEM;
	/* int result; */
#ifdef VERY_VERBOSE
	printk("%s %ld - ", __FUNCTION__, count);
#endif
	page = (char *)__get_free_page(GFP_KERNEL);
	if (page)
	{
		ret = -EFAULT;
		if (copy_from_user(page, buf, count))
			goto out;
		myString = (char *) kmalloc(count + 1, GFP_KERNEL);
		strncpy(myString, page, count);
		myString[count] = '\0';
#ifdef VERY_VERBOSE
		printk("%s\n", myString);
#endif
		kfree(myString);
		//result = sscanf(page, "%3s %3s %3s %3s %3s", s1, s2, s3, s4, s5);
	}
	ret = count;
out:
	free_page((unsigned long)page);
	return ret;
}

int proc_avs_0_sb_read(char *page, char **start, off_t off, int count,
					   int *eof, void *data_unused)
{
	int len = 0;
#ifdef VERY_VERBOSE
	printk("%s\n", __FUNCTION__);
#endif
	len = sprintf(page, "auto\n");
	return len;
}

int proc_avs_0_standby_write(struct file *file, const char __user *buf,
							 unsigned long count, void *data)
{
	char *page;
	char *myString;
	ssize_t ret = -ENOMEM;
	/* int result; */
#ifdef VERY_VERBOSE
	printk("%s %ld - ", __FUNCTION__, count);
#endif
	page = (char *)__get_free_page(GFP_KERNEL);
	if (page)
	{
		ret = -EFAULT;
		if (copy_from_user(page, buf, count))
			goto out;
		myString = (char *) kmalloc(count + 1, GFP_KERNEL);
		strncpy(myString, page, count);
		myString[count] = '\0';
#ifdef VERY_VERBOSE
		printk("%s\n", myString);
#endif
		if (strncmp("on", page, count - 1) == 0)
		{
			current_standby = 1;
		}
		else if (strncmp("off", page, count - 1) == 0)
		{
			current_standby = 0;
		}
		avs_command_kernel(AVSIOSTANDBY, (void *) current_standby);
		kfree(myString);
		//result = sscanf(page, "%3s %3s %3s %3s %3s", s1, s2, s3, s4, s5);
	}
	ret = count;
out:
	free_page((unsigned long)page);
	return ret;
}

int proc_avs_0_standby_read(char *page, char **start, off_t off, int count,
							int *eof, void *data_unused)
{
	int len = 0;
#ifdef VERY_VERBOSE
	printk("%s\n", __FUNCTION__);
#endif
	if (current_standby == 0)
		len = sprintf(page, "off\n");
	if (current_standby == 1)
		len = sprintf(page, "on\n");
	return len;
}

int proc_video_hdmi_colorspace_write(struct file *file, const char __user *buf, unsigned long count, void *data)
{
	char *page;
	char *myString;
	ssize_t ret = -ENOMEM;
#ifdef VERY_VERBOSE
	printk("%s %ld - ", __FUNCTION__, count);
#endif
	page = (char *)__get_free_page(GFP_KERNEL);
	if (page)
	{
		struct stmfb_info *info = stmfb_get_fbinfo_ptr();
		struct stmfbio_output_configuration outputConfig;
		int err = 0;
		//int alpha = 0;
		int hdmi_colour = 0;
		ret = -EFAULT;
		if (copy_from_user(page, buf, count))
			goto out;
		myString = (char *) kmalloc(count + 1, GFP_KERNEL);
		strncpy(myString, page, count);
		myString[count] = '\0';
#ifdef VERY_VERBOSE
		printk("%s\n", myString);
#endif
		//sscanf(myString, "%d", &alpha);
		//0rgb 1yuv 2422
		if (strncmp("hdmi_rgb", page, count - 1) == 0)
		{
			hdmi_colour = 0;
		}
		else if (strncmp("hdmi_yuv", page, count - 1) == 0)
		{
			hdmi_colour = 1;
		}
		else if (strncmp("hdmi_422", page, count - 1) == 0)
		{
			hdmi_colour = 2;
		}
		outputConfig.outputid = 1;
		stmfb_get_output_configuration(&outputConfig, info);
		outputConfig.caps = 0;
		outputConfig.activate = 0; //STMFBIO_ACTIVATE_IMMEDIATE;
		outputConfig.caps |= STMFBIO_OUTPUT_CAPS_HDMI_CONFIG;
		outputConfig.hdmi_config &= ~(STMFBIO_OUTPUT_HDMI_YUV | STMFBIO_OUTPUT_HDMI_422);
		switch (hdmi_colour)
		{
			case 1:
				outputConfig.hdmi_config |= STMFBIO_OUTPUT_HDMI_YUV;
				break;
			case 2:
				outputConfig.hdmi_config |= (STMFBIO_OUTPUT_HDMI_YUV | STMFBIO_OUTPUT_HDMI_422);
				break;
			default:
				break;
		}
		err = stmfb_set_output_configuration(&outputConfig, info);
		//if(ioctl(fbfd, STMFBIO_SET_OUTPUT_CONFIG, &outputConfig)<0)
		//perror("setting output configuration failed");
		kfree(myString);
	}
	ret = count;
out:
	free_page((unsigned long)page);
	return ret;
}

int proc_video_hdmi_colorspace_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused)
{
	struct stmfb_info *info = stmfb_get_fbinfo_ptr();
	struct stmfbio_output_configuration outputConfig;
	int len = 0;
#ifdef VERY_VERBOSE
	printk("%s %d\n", __FUNCTION__, count);
#endif
	outputConfig.outputid = 1;
	stmfb_get_output_configuration(&outputConfig, info);
	if (outputConfig.hdmi_config & STMFBIO_OUTPUT_HDMI_422)
		len = sprintf(page, "hdmi_422\n");
	else if (outputConfig.hdmi_config & STMFBIO_OUTPUT_HDMI_YUV)
		len = sprintf(page, "hdmi_yuv\n");
	else
		len = sprintf(page, "hdmi_rgb\n");
	return len;
}

int proc_video_hdmi_colorspace_choices_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused)
{
	int len = 0;
#ifdef VERY_VERBOSE
	printk("%s\n", __FUNCTION__);
#endif
	len = sprintf(page, "hdmi_rgb hdmi_yuv hdmi_422\n");
	return len;
}
