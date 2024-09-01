/*
 * ALSA library for kernel clients (ksound)
 *
 * Copyright (c) 2007 STMicroelectronics R&D Limited <daniel.thompson@st.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <linux/version.h>
#include <linux/init.h>
#include <linux/smp_lock.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/vmalloc.h>
#include <sound/core.h>
#include <sound/minors.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/info.h>
#include <linux/soundcard.h>
#include <sound/initval.h>
#include <linux/delay.h>
#include <sound/asoundef.h>
#include <sound/control.h>
#include <linux/mutex.h>
#include <linux/slab.h>

#include <asm/io.h>

#include "ksound.h"

/*
 * todo Need to remove these typedefs and externs.
 */
typedef struct snd_pcm_runtime snd_pcm_runtime_t;
typedef struct snd_pcm snd_pcm_t;
typedef struct snd_mask snd_mask_t;
typedef struct snd_pcm_sw_params snd_pcm_sw_params_t;
typedef struct snd_pcm_hw_params snd_pcm_hw_params_t;

/*
 In STM2.4 all these helper functions were moved to the OSS lib, which has lots of dependencies and therefore
 needs lots of space. However, since we don't need all that stuff i copied the relevant functions into this file.
*/
#if defined(__TDT__) && (defined(CONFIG_SND_PCM_OSS) || (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 30)))
extern int _snd_pcm_hw_param_setinteger(struct snd_pcm_hw_params *params,
										snd_pcm_hw_param_t var);

extern int _snd_pcm_hw_param_min(struct snd_pcm_hw_params *params,
								 snd_pcm_hw_param_t var, unsigned int val,
								 int dir);
extern int snd_pcm_hw_param_mask(struct snd_pcm_substream *pcm,
								 struct snd_pcm_hw_params *params,
								 snd_pcm_hw_param_t var,
								 const struct snd_mask *val);
extern int snd_pcm_hw_param_set(struct snd_pcm_substream *pcm,
								struct snd_pcm_hw_params *params,
								snd_pcm_hw_param_t var, unsigned int val,
								int dir);

extern int snd_pcm_hw_param_near(struct snd_pcm_substream *pcm,
								 struct snd_pcm_hw_params *params,
								 snd_pcm_hw_param_t var, unsigned int best,
								 int *dir);
#else

/*
 * helper functions to process hw_params
 */
static int snd_interval_refine_min(struct snd_interval *i, unsigned int min, int openmin)
{
	int changed = 0;
	if (i->min < min)
	{
		i->min = min;
		i->openmin = openmin;
		changed = 1;
	}
	else if (i->min == min && !i->openmin && openmin)
	{
		i->openmin = 1;
		changed = 1;
	}
	if (i->integer)
	{
		if (i->openmin)
		{
			i->min++;
			i->openmin = 0;
		}
	}
	if (snd_interval_checkempty(i))
	{
		snd_interval_none(i);
		return -EINVAL;
	}
	return changed;
}

static int snd_interval_refine_max(struct snd_interval *i, unsigned int max, int openmax)
{
	int changed = 0;
	if (i->max > max)
	{
		i->max = max;
		i->openmax = openmax;
		changed = 1;
	}
	else if (i->max == max && !i->openmax && openmax)
	{
		i->openmax = 1;
		changed = 1;
	}
	if (i->integer)
	{
		if (i->openmax)
		{
			i->max--;
			i->openmax = 0;
		}
	}
	if (snd_interval_checkempty(i))
	{
		snd_interval_none(i);
		return -EINVAL;
	}
	return changed;
}

static int snd_interval_refine_set(struct snd_interval *i, unsigned int val)
{
	struct snd_interval t;
	t.empty = 0;
	t.min = t.max = val;
	t.openmin = t.openmax = 0;
	t.integer = 1;
	return snd_interval_refine(i, &t);
}

/**
 * snd_pcm_hw_param_value_min
 * @params: the hw_params instance
 * @var: parameter to retrieve
 * @dir: pointer to the direction (-1,0,1) or NULL
 *
 * Return the minimum value for field PAR.
 */
static unsigned int
snd_pcm_hw_param_value_min(const struct snd_pcm_hw_params *params,
						   snd_pcm_hw_param_t var, int *dir)
{
	if (hw_is_mask(var))
	{
		if (dir)
			*dir = 0;
		return snd_mask_min(hw_param_mask_c(params, var));
	}
	if (hw_is_interval(var))
	{
		const struct snd_interval *i = hw_param_interval_c(params, var);
		if (dir)
			*dir = i->openmin;
		return snd_interval_min(i);
	}
	return -EINVAL;
}

/**
 * snd_pcm_hw_param_value_max
 * @params: the hw_params instance
 * @var: parameter to retrieve
 * @dir: pointer to the direction (-1,0,1) or NULL
 *
 * Return the maximum value for field PAR.
 */
static unsigned int
snd_pcm_hw_param_value_max(const struct snd_pcm_hw_params *params,
						   snd_pcm_hw_param_t var, int *dir)
{
	if (hw_is_mask(var))
	{
		if (dir)
			*dir = 0;
		return snd_mask_max(hw_param_mask_c(params, var));
	}
	if (hw_is_interval(var))
	{
		const struct snd_interval *i = hw_param_interval_c(params, var);
		if (dir)
			*dir = - (int) i->openmax;
		return snd_interval_max(i);
	}
	return -EINVAL;
}

static int _snd_pcm_hw_param_mask(struct snd_pcm_hw_params *params,
								  snd_pcm_hw_param_t var,
								  const struct snd_mask *val)
{
	int changed;
	changed = snd_mask_refine(hw_param_mask(params, var), val);
	if (changed)
	{
		params->cmask |= 1 << var;
		params->rmask |= 1 << var;
	}
	return changed;
}

static int snd_pcm_hw_param_mask(struct snd_pcm_substream *pcm,
								 struct snd_pcm_hw_params *params,
								 snd_pcm_hw_param_t var,
								 const struct snd_mask *val)
{
	int changed = _snd_pcm_hw_param_mask(params, var, val);
	if (changed < 0)
		return changed;
	if (params->rmask)
	{
		int err = snd_pcm_hw_refine(pcm, params);
		if (err < 0)
			return err;
	}
	return 0;
}

EXPORT_SYMBOL_GPL(snd_pcm_hw_param_mask);

static int _snd_pcm_hw_param_min(struct snd_pcm_hw_params *params,
								 snd_pcm_hw_param_t var, unsigned int val,
								 int dir)
{
	int changed;
	int open = 0;
	if (dir)
	{
		if (dir > 0)
		{
			open = 1;
		}
		else if (dir < 0)
		{
			if (val > 0)
			{
				open = 1;
				val--;
			}
		}
	}
	if (hw_is_mask(var))
		changed = snd_mask_refine_min(hw_param_mask(params, var),
									  val + !!open);
	else if (hw_is_interval(var))
		changed = snd_interval_refine_min(hw_param_interval(params, var),
										  val, open);
	else
		return -EINVAL;
	if (changed)
	{
		params->cmask |= 1 << var;
		params->rmask |= 1 << var;
	}
	return changed;
}

/**
 * snd_pcm_hw_param_min
 * @pcm: PCM instance
 * @params: the hw_params instance
 * @var: parameter to retrieve
 * @val: minimal value
 * @dir: pointer to the direction (-1,0,1) or NULL
 *
 * Inside configuration space defined by PARAMS remove from PAR all
 * values < VAL. Reduce configuration space accordingly.
 * Return new minimum or -EINVAL if the configuration space is empty
 */
static int snd_pcm_hw_param_min(struct snd_pcm_substream *pcm,
								struct snd_pcm_hw_params *params,
								snd_pcm_hw_param_t var, unsigned int val,
								int *dir)
{
	int changed = _snd_pcm_hw_param_min(params, var, val, dir ? *dir : 0);
	if (changed < 0)
		return changed;
	if (params->rmask)
	{
		int err = snd_pcm_hw_refine(pcm, params);
		if (err < 0)
			return err;
	}
	return snd_pcm_hw_param_value_min(params, var, dir);
}

EXPORT_SYMBOL_GPL(_snd_pcm_hw_param_min);

static int _snd_pcm_hw_param_max(struct snd_pcm_hw_params *params,
								 snd_pcm_hw_param_t var, unsigned int val,
								 int dir)
{
	int changed;
	int open = 0;
	if (dir)
	{
		if (dir < 0)
		{
			open = 1;
		}
		else if (dir > 0)
		{
			open = 1;
			val++;
		}
	}
	if (hw_is_mask(var))
	{
		if (val == 0 && open)
		{
			snd_mask_none(hw_param_mask(params, var));
			changed = -EINVAL;
		}
		else
			changed = snd_mask_refine_max(hw_param_mask(params, var),
										  val - !!open);
	}
	else if (hw_is_interval(var))
		changed = snd_interval_refine_max(hw_param_interval(params, var),
										  val, open);
	else
		return -EINVAL;
	if (changed)
	{
		params->cmask |= 1 << var;
		params->rmask |= 1 << var;
	}
	return changed;
}

/**
 * snd_pcm_hw_param_max
 * @pcm: PCM instance
 * @params: the hw_params instance
 * @var: parameter to retrieve
 * @val: maximal value
 * @dir: pointer to the direction (-1,0,1) or NULL
 *
 * Inside configuration space defined by PARAMS remove from PAR all
 * values >= VAL + 1. Reduce configuration space accordingly.
 * Return new maximum or -EINVAL if the configuration space is empty
 */
static int snd_pcm_hw_param_max(struct snd_pcm_substream *pcm,
								struct snd_pcm_hw_params *params,
								snd_pcm_hw_param_t var, unsigned int val,
								int *dir)
{
	int changed = _snd_pcm_hw_param_max(params, var, val, dir ? *dir : 0);
	if (changed < 0)
		return changed;
	if (params->rmask)
	{
		int err = snd_pcm_hw_refine(pcm, params);
		if (err < 0)
			return err;
	}
	return snd_pcm_hw_param_value_max(params, var, dir);
}

static int boundary_sub(int a, int adir,
						int b, int bdir,
						int *c, int *cdir)
{
	adir = adir < 0 ? -1 : (adir > 0 ? 1 : 0);
	bdir = bdir < 0 ? -1 : (bdir > 0 ? 1 : 0);
	*c = a - b;
	*cdir = adir - bdir;
	if (*cdir == -2)
	{
		(*c)--;
	}
	else if (*cdir == 2)
	{
		(*c)++;
	}
	return 0;
}

static int boundary_lt(unsigned int a, int adir,
					   unsigned int b, int bdir)
{
	if (adir < 0)
	{
		a--;
		adir = 1;
	}
	else if (adir > 0)
		adir = 1;
	if (bdir < 0)
	{
		b--;
		bdir = 1;
	}
	else if (bdir > 0)
		bdir = 1;
	return a < b || (a == b && adir < bdir);
}

/* Return 1 if min is nearer to best than max */
static int boundary_nearer(int min, int mindir,
						   int best, int bestdir,
						   int max, int maxdir)
{
	int dmin, dmindir;
	int dmax, dmaxdir;
	boundary_sub(best, bestdir, min, mindir, &dmin, &dmindir);
	boundary_sub(max, maxdir, best, bestdir, &dmax, &dmaxdir);
	return boundary_lt(dmin, dmindir, dmax, dmaxdir);
}

/**
 * snd_pcm_hw_param_near
 * @pcm: PCM instance
 * @params: the hw_params instance
 * @var: parameter to retrieve
 * @best: value to set
 * @dir: pointer to the direction (-1,0,1) or NULL
 *
 * Inside configuration space defined by PARAMS set PAR to the available value
 * nearest to VAL. Reduce configuration space accordingly.
 * This function cannot be called for SNDRV_PCM_HW_PARAM_ACCESS,
 * SNDRV_PCM_HW_PARAM_FORMAT, SNDRV_PCM_HW_PARAM_SUBFORMAT.
 * Return the value found.
 */
static int snd_pcm_hw_param_near(struct snd_pcm_substream *pcm,
								 struct snd_pcm_hw_params *params,
								 snd_pcm_hw_param_t var, unsigned int best,
								 int *dir)
{
	struct snd_pcm_hw_params *save = NULL;
	int v;
	unsigned int saved_min;
	int last = 0;
	int min, max;
	int mindir, maxdir;
	int valdir = dir ? *dir : 0;
	/* FIXME */
	if (best > INT_MAX)
		best = INT_MAX;
	min = max = best;
	mindir = maxdir = valdir;
	if (maxdir > 0)
		maxdir = 0;
	else if (maxdir == 0)
		maxdir = -1;
	else
	{
		maxdir = 1;
		max--;
	}
	save = kmalloc(sizeof(*save), GFP_KERNEL);
	if (save == NULL)
		return -ENOMEM;
	*save = *params;
	saved_min = min;
	min = snd_pcm_hw_param_min(pcm, params, var, min, &mindir);
	if (min >= 0)
	{
		struct snd_pcm_hw_params *params1;
		if (max < 0)
			goto _end;
		if ((unsigned int)min == saved_min && mindir == valdir)
			goto _end;
		params1 = kmalloc(sizeof(*params1), GFP_KERNEL);
		if (params1 == NULL)
		{
			kfree(save);
			return -ENOMEM;
		}
		*params1 = *save;
		max = snd_pcm_hw_param_max(pcm, params1, var, max, &maxdir);
		if (max < 0)
		{
			kfree(params1);
			goto _end;
		}
		if (boundary_nearer(max, maxdir, best, valdir, min, mindir))
		{
			*params = *params1;
			last = 1;
		}
		kfree(params1);
	}
	else
	{
		*params = *save;
		max = snd_pcm_hw_param_max(pcm, params, var, max, &maxdir);
		if (max < 0)
		{
			kfree(save);
			return max;
		}
		last = 1;
	}
_end:
	kfree(save);
	if (last)
		v = snd_pcm_hw_param_last(pcm, params, var, dir);
	else
		v = snd_pcm_hw_param_first(pcm, params, var, dir);
	snd_BUG_ON(v < 0);
	return v;
}

EXPORT_SYMBOL_GPL(snd_pcm_hw_param_near);

static int _snd_pcm_hw_param_set(struct snd_pcm_hw_params *params,
								 snd_pcm_hw_param_t var, unsigned int val,
								 int dir)
{
	int changed;
	if (hw_is_mask(var))
	{
		struct snd_mask *m = hw_param_mask(params, var);
		if (val == 0 && dir < 0)
		{
			changed = -EINVAL;
			snd_mask_none(m);
		}
		else
		{
			if (dir > 0)
				val++;
			else if (dir < 0)
				val--;
			changed = snd_mask_refine_set(hw_param_mask(params, var), val);
		}
	}
	else if (hw_is_interval(var))
	{
		struct snd_interval *i = hw_param_interval(params, var);
		if (val == 0 && dir < 0)
		{
			changed = -EINVAL;
			snd_interval_none(i);
		}
		else if (dir == 0)
			changed = snd_interval_refine_set(i, val);
		else
		{
			struct snd_interval t;
			t.openmin = 1;
			t.openmax = 1;
			t.empty = 0;
			t.integer = 0;
			if (dir < 0)
			{
				t.min = val - 1;
				t.max = val;
			}
			else
			{
				t.min = val;
				t.max = val + 1;
			}
			changed = snd_interval_refine(i, &t);
		}
	}
	else
		return -EINVAL;
	if (changed)
	{
		params->cmask |= 1 << var;
		params->rmask |= 1 << var;
	}
	return changed;
}

/**
 * snd_pcm_hw_param_set
 * @pcm: PCM instance
 * @params: the hw_params instance
 * @var: parameter to retrieve
 * @val: value to set
 * @dir: pointer to the direction (-1,0,1) or NULL
 *
 * Inside configuration space defined by PARAMS remove from PAR all
 * values != VAL. Reduce configuration space accordingly.
 * Return VAL or -EINVAL if the configuration space is empty
 */
static int snd_pcm_hw_param_set(struct snd_pcm_substream *pcm,
								struct snd_pcm_hw_params *params,
								snd_pcm_hw_param_t var, unsigned int val,
								int dir)
{
	int changed = _snd_pcm_hw_param_set(params, var, val, dir);
	if (changed < 0)
		return changed;
	if (params->rmask)
	{
		int err = snd_pcm_hw_refine(pcm, params);
		if (err < 0)
			return err;
	}
	return snd_pcm_hw_param_value(params, var, NULL);
}

EXPORT_SYMBOL_GPL(snd_pcm_hw_param_set);

static int _snd_pcm_hw_param_setinteger(struct snd_pcm_hw_params *params,
										snd_pcm_hw_param_t var)
{
	int changed;
	changed = snd_interval_setinteger(hw_param_interval(params, var));
	if (changed)
	{
		params->cmask |= 1 << var;
		params->rmask |= 1 << var;
	}
	return changed;
}

#endif

MODULE_LICENSE("GPL");

EXPORT_SYMBOL(ksnd_pcm_avail_update);
EXPORT_SYMBOL(ksnd_pcm_wait);
EXPORT_SYMBOL(ksnd_pcm_mmap_begin);
EXPORT_SYMBOL(ksnd_pcm_mmap_commit);
EXPORT_SYMBOL(ksnd_pcm_delay);
EXPORT_SYMBOL(ksnd_pcm_start);
EXPORT_SYMBOL(ksnd_pcm_stop);
EXPORT_SYMBOL(ksnd_pcm_open);
EXPORT_SYMBOL(ksnd_pcm_close);
EXPORT_SYMBOL(ksnd_pcm_writei);
EXPORT_SYMBOL(ksnd_pcm_prepare);
EXPORT_SYMBOL(ksnd_pcm_hw_params);
EXPORT_SYMBOL(ksnd_pcm_hw_params_any);
EXPORT_SYMBOL(ksnd_pcm_set_params);
EXPORT_SYMBOL(ksnd_pcm_get_params);
EXPORT_SYMBOL(ksnd_pcm_hw_params_malloc);
EXPORT_SYMBOL(ksnd_pcm_hw_params_free);
EXPORT_SYMBOL(ksnd_pcm_hw_params_get_period_size);
EXPORT_SYMBOL(ksnd_pcm_hw_params_get_buffer_size);

EXPORT_SYMBOL(ksnd_ctl_elem_id_alloca);
EXPORT_SYMBOL(ksnd_ctl_elem_id_set_interface);
EXPORT_SYMBOL(ksnd_ctl_elem_id_set_name);
EXPORT_SYMBOL(ksnd_ctl_elem_id_set_device);
EXPORT_SYMBOL(ksnd_ctl_elem_id_set_index);
EXPORT_SYMBOL(ksnd_substream_find_elem);
EXPORT_SYMBOL(ksnd_ctl_elem_value_alloca);
EXPORT_SYMBOL(ksnd_ctl_elem_value_set_id);
EXPORT_SYMBOL(ksnd_ctl_elem_value_set_integer);
EXPORT_SYMBOL(ksnd_ctl_elem_value_set_iec958);
EXPORT_SYMBOL(ksnd_hctl_elem_write);

/*#define CONFIG_KSOUND_DEBUG*/

#ifdef CONFIG_KSOUND_DEBUG
#define ENABLE_KSND_DEBUG 1
#else
#define ENABLE_KSND_DEBUG 0
#endif

/// Used to output trace information on critical paths, typically disabled
#define KSND_DEBUG(fmt, args...) \
	((void) (ENABLE_KSND_DEBUG && \
			 (printk(KERN_DEBUG "%s: " fmt, __FUNCTION__, ##args), 0)))
/// Output trace information off the critical path
#define KSND_TRACE(fmt, args...) \
	(printk(KERN_TRACE "%s: " fmt, \
			(ENABLE_KSND_DEBUG ? __FUNCTION__ : "ksound"), ##args))
/// Output errors, should never be output in 'normal' operation
#define KSND_ERR(fmt, args...) \
	(printk(KERN_WARNING "%s: " fmt, \
			(ENABLE_KSND_DEBUG ? __FUNCTION__ : "ksound"), ##args))

/* Scheduled for demolition! */
int snd_pcm_format_iec60958_copy(snd_pcm_substream_t *substream,
								 int data_channels,
								 snd_pcm_uframes_t pos,
								 void __user *buffer, snd_pcm_uframes_t count);
/*
struct _ksnd_pcm {
	snd_pcm_substream_t *substream;
	snd_pcm_channel_area_t hwareas[10];
	ksnd_pcm_hw_params_t actual_hwparams;
};
*/
static inline snd_pcm_state_t _ksnd_pcm_state(snd_pcm_substream_t *substream)
{
	return substream->runtime->status->state;
}

snd_pcm_state_t ksnd_pcm_state(ksnd_pcm_t *pcm)
{
	return _ksnd_pcm_state(pcm->substream);
}

/* CAUTION: call it with irq disabled (due to internal call to snd_pcm_update_hw_ptr) */
static inline snd_pcm_uframes_t _ksnd_pcm_avail_update(snd_pcm_substream_t
													   *substream)
{
	snd_pcm_runtime_t *runtime = substream->runtime;
	/*NICK added if to remove real updates which we do not want*/
#if defined(__TDT__) \
 && (defined(FORTIS_HDBOX) \
 || defined(UFS922) \
 || defined(UFC960) \
 || defined(HL101) \
 || defined(VIP1_V2) \
 || defined(VIP2_V1) \
 || defined(OCTAGON1008) \
 || defined(IPBOX9900) \
 || defined(IPBOX99) \
 || defined(IPBOX55) \
 || defined(CUBEREVO_250HD) \
 || defined(CUBEREVO))
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
	if (runtime->sleep_min == 0 &&
			_ksnd_pcm_state(substream) == SNDRV_PCM_STATE_RUNNING)
		snd_pcm_update_hw_ptr(substream);
#else
	if (_ksnd_pcm_state(substream) == SNDRV_PCM_STATE_RUNNING)
		snd_pcm_update_hw_ptr(substream);
#endif
#endif
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		return snd_pcm_playback_avail(runtime);
	else
		return snd_pcm_capture_avail(runtime);
}

snd_pcm_uframes_t ksnd_pcm_avail_update(ksnd_pcm_t *kpcm)
{
	snd_pcm_substream_t *substream = kpcm->substream;
	snd_pcm_uframes_t avail;
	snd_pcm_stream_lock_irq(substream);
	avail = _ksnd_pcm_avail_update(substream);
	snd_pcm_stream_unlock_irq(substream);
	return avail;
}

/**
 * \brief Obtain last position update hi-res timestamp
 * \param pcm PCM handle
 * \param avail Number of available frames when timestamp was grabbed
 * \param tstamp Hi-res timestamp
 * \return 0 on success otherwise a negative error code
 *
 * The alsa-lib doxygen comments include the following:
 * Note this function does not update the actual r/w pointer
 * for applications.
 *
 * However examining the implementation of snd_pcm_hw_htimestamp()
 * there is a call to snd_pcm_hw_avail_update(). This implementation
 * therefore makes an equivalent call.
 */
int ksnd_pcm_htimestamp(ksnd_pcm_t *kpcm, snd_pcm_uframes_t *avail, struct timespec *tstamp)
{
	snd_pcm_substream_t *substream = kpcm->substream;
	snd_pcm_runtime_t *runtime;
	snd_pcm_uframes_t myavail;
	struct timespec mystamp;
	/* we can use a radically different approach to the userspace library (which loops making sure
	 * avail is not modified). this is primarily because we can lock out interrupts.
	 */
	snd_pcm_stream_lock_irq(substream);
	runtime = substream->runtime;
	/*NICK made changes here, first he did as suggested by Dan and changed myavail
	 to be obtained by checking the data structure without doing any update.
	 Then he realised that myavail actually just confuses the issue, it is
	 almost always 8, because the timestamp was taken after the hardware started
	 to read the first 8 sample block. Even when it isn't 8, this is simply
	 showing the jitter in the timestamp. The purpose of myavail is to try
	 and correct that jitter, however because it is quantized to 8 samples, it
	 actually makes the jitter much worse, since you have a jitter that varies by
	 10 or 20 us, and you correct for it by adjusting by lumps of around 160 us.
	 So I set myavail to be zero always.
	 Also I added a a return failure if the timestamp is set to zero. */
	/* Dagobert: Also add UFS922 here for the "bad" 7101BWC cpu
	 * currently it seems so that this works for both cpu types
	 * (BWC vs. BWD)
	 */
#if defined(__TDT__) \
 && (defined(FORTIS_HDBOX) \
 || defined(UFS922) \
 || defined(UFC960) \
 || defined(HL101) \
 || defined(VIP1_V2) \
 || defined(VIP2_V1) \
 || defined(OCTAGON1008) \
 || defined(IPBOX9900) \
 || defined(IPBOX99) \
 || defined(IPBOX55) \
 || defined(CUBEREVO_250HD) \
 || defined(CUBEREVO))
	myavail = _ksnd_pcm_avail_update(kpcm->substream);
#else
	myavail = 0;
#endif
	/*NICK*/
	mystamp = runtime->status->tstamp;
	snd_pcm_stream_unlock_irq(substream);
	if ((mystamp.tv_sec == 0) && (mystamp.tv_nsec == 0))
		return -1;
	if (myavail < 0)
		return myavail;
	*avail = myavail;
	*tstamp = mystamp;
	return 0;
}
EXPORT_SYMBOL(ksnd_pcm_htimestamp);

/**
 * \brief Obtain last position update hi-res monotonic timestamp
 * \param pcm PCM handle
 * \param avail Number of available frames when timestamp was grabbed
 * \param mstamp Hi-res timestamp based on CLOCK_MONOTONIC rather then wall time.
 * \return 0 on success otherwise a negative error code
 *
 * This function is an extension to alsa-lib (which serves as a template for
 * all the ksound functions). However since all the in-kernel ksound clients
 * measure time using the monotonic clock this function is required if the
 * timestamp is to be meaningful.
 */
int ksnd_pcm_mtimestamp(ksnd_pcm_t *pcm, snd_pcm_uframes_t *avail, struct timespec *mstamp)
{
	unsigned long seq;
	int res;
	struct timespec tstamp;
	struct timespec tomono;
	do
	{
		seq = read_seqbegin(&xtime_lock);
		res = ksnd_pcm_htimestamp(pcm, avail, &tstamp);
		tomono = wall_to_monotonic;
	}
	while (read_seqretry(&xtime_lock, seq));
	if (res < 0)
		return res;
	mstamp->tv_sec = tstamp.tv_sec + tomono.tv_sec;
	mstamp->tv_nsec = tstamp.tv_nsec + tomono.tv_nsec;
	while (mstamp->tv_nsec >= NSEC_PER_SEC)
	{
		mstamp->tv_nsec -= NSEC_PER_SEC;
		++mstamp->tv_sec;
	}
	while (mstamp->tv_nsec < 0)
	{
		mstamp->tv_nsec += NSEC_PER_SEC;
		--mstamp->tv_sec;
	}
	return res;
}
EXPORT_SYMBOL(ksnd_pcm_mtimestamp);

static int _ksnd_pcm_wait(snd_pcm_substream_t *substream, int timeout)
{
	snd_pcm_runtime_t *runtime = substream->runtime;
	snd_pcm_uframes_t avail;
	int res = 1; /* success is a positive integer */
	snd_pcm_stream_lock_irq(substream);
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		avail = snd_pcm_playback_avail(runtime);
	else
		avail = snd_pcm_capture_avail(runtime);
	if (avail < runtime->control->avail_min)
	{
		wait_queue_t wait;
		long jiffies;
		if (timeout >= 0)
			jiffies = (timeout * HZ) / 1000;
		else
			jiffies = 10 * HZ;
		init_waitqueue_entry(&wait, current);
		add_wait_queue(&runtime->sleep, &wait);
		do
		{
			if (signal_pending(current))
			{
				res = -ERESTARTSYS;
				break;
			}
			set_current_state(TASK_INTERRUPTIBLE);
			snd_pcm_stream_unlock_irq(substream);
			jiffies = schedule_timeout(jiffies);
			snd_pcm_stream_lock_irq(substream);
			if (jiffies == 0)
			{
				if (timeout < 0)
				{
					snd_printd("ksnd: [hw:%d,%d] playback write error "
							   "(DMA or IRQ trouble?)\n",
							   substream->pcm->card->number,
							   substream->pcm->device);
					jiffies = 10 * HZ;
				}
				else
				{
					res = 0; /* timeout */
				}
			}
			switch (_ksnd_pcm_state(substream))
			{
				case SNDRV_PCM_STATE_SETUP:
				case SNDRV_PCM_STATE_XRUN:
				case SNDRV_PCM_STATE_DRAINING:
					res = -EPIPE;
					break;
				case SNDRV_PCM_STATE_SUSPENDED:
					printk("%s: result ESTRPIPE %d\n",
						   __FUNCTION__, __LINE__);
					res = -ESTRPIPE;
					break;
				case SNDRV_PCM_STATE_PAUSED:
					printk("%s: Waiting for buffer %d\n",
						   __FUNCTION__, __LINE__);
					break;
				default:
					break;
			}
			if (1 != res)
				break;
			if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
				avail = snd_pcm_playback_avail(runtime);
			else
				avail = snd_pcm_capture_avail(runtime);
		}
		while (avail < runtime->control->avail_min);
		remove_wait_queue(&runtime->sleep, &wait);
	}
	snd_pcm_stream_unlock_irq(substream);
	return res;
}

int ksnd_pcm_wait(ksnd_pcm_t *kpcm, int timeout)
{
	return _ksnd_pcm_wait(kpcm->substream, timeout);
}

static void _ksnd_pcm_mmap_begin(snd_pcm_substream_t *substream,
								 snd_pcm_uframes_t *offset,
								 snd_pcm_uframes_t *frames)
{
	snd_pcm_runtime_t *runtime = substream->runtime;
	snd_pcm_uframes_t avail, f, cont, appl_ptr;
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		avail = snd_pcm_playback_avail(runtime);
	else
		avail = snd_pcm_capture_avail(runtime);
	f = *frames;
	if (f > avail)
		f = avail;
	cont =
		runtime->buffer_size -
		runtime->control->appl_ptr % runtime->buffer_size;
	if (f > cont)
		f = cont;
	appl_ptr = runtime->control->appl_ptr;
	*frames = f;
	*offset = appl_ptr % runtime->buffer_size;
}

int ksnd_pcm_mmap_begin(ksnd_pcm_t *pcm, const snd_pcm_channel_area_t **areas,
						snd_pcm_uframes_t *offset, snd_pcm_uframes_t *frames)
{
	snd_pcm_substream_t *substream = pcm->substream;
	snd_pcm_channel_area_t *xareas = pcm->hwareas;
	if (snd_BUG_ON(!substream || !areas || !offset || !frames))
		return -EFAULT;
	snd_pcm_stream_lock_irq(substream);
	_ksnd_pcm_mmap_begin(substream, offset, frames);
	snd_pcm_stream_unlock_irq(substream);
	*areas = xareas;
	KSND_DEBUG("Allocated %lu frames offset by %lu\n", *frames, *offset);
	return 0;
}

/* call with interrupts locked? */
static int _ksnd_pcm_update_appl_ptr(snd_pcm_substream_t *substream,
									 snd_pcm_uframes_t appl_ptr)
{
	snd_pcm_runtime_t *runtime = substream->runtime;
	snd_pcm_sframes_t hw_avail;
	int err;
	runtime->control->appl_ptr = appl_ptr;
	if (substream->ops->ack)
		substream->ops->ack(substream);
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		hw_avail = snd_pcm_playback_hw_avail(runtime);
	else
		hw_avail = snd_pcm_capture_hw_avail(runtime);
	if (runtime->status->state == SNDRV_PCM_STATE_PREPARED &&
			hw_avail >= (snd_pcm_sframes_t) runtime->start_threshold)
	{
		err = snd_pcm_start(substream);
		if (err < 0)
			return err;
	}
	return 0;
}

snd_pcm_sframes_t ksnd_pcm_mmap_commit(ksnd_pcm_t *pcm,
									   snd_pcm_uframes_t offset,
									   snd_pcm_uframes_t frames)
{
	snd_pcm_substream_t *substream = pcm->substream;
	snd_pcm_runtime_t *runtime = substream->runtime;
	snd_pcm_uframes_t appl_ptr;
	snd_pcm_sframes_t res = frames;
	int err;
	if (snd_BUG_ON(!substream))
		return -EFAULT;
	/* for SPDIF we need to run though the just committed PCM samples and
	 * add formating (unless raw mode is enabled)
	 */
// BUG_ON(substream->pcm->card->number == 2); /* TODO: magic number */
	snd_pcm_stream_lock_irq(substream);
	switch (_ksnd_pcm_state(substream))
	{
		case SNDRV_PCM_STATE_XRUN:
			res = -EPIPE;
			goto _end_unlock;
		case SNDRV_PCM_STATE_SUSPENDED:
			res = -ESTRPIPE;
			goto _end_unlock;
	}
	appl_ptr = runtime->control->appl_ptr;
	/* verify no-one is interleaving access to the playback */
	// TODO: what about capture?
	BUG_ON(substream->stream == SNDRV_PCM_STREAM_PLAYBACK &&
		   (appl_ptr % runtime->buffer_size) != offset);
	appl_ptr += frames;
	if (appl_ptr >= runtime->boundary)
		appl_ptr = 0;
	err = _ksnd_pcm_update_appl_ptr(substream, appl_ptr);
	if (err < 0)
		res = err;
_end_unlock:
	snd_pcm_stream_unlock_irq(substream);
	return res;
}

/*
 * original alsa-wrapper routines
 */

int ksnd_pcm_get_samplerate(ksnd_pcm_t *pcm)
{
	snd_pcm_substream_t *substream = pcm->substream;
	return substream->runtime->rate;
}

static inline int _ksnd_pcm_drop(snd_pcm_substream_t *substream)
{
	snd_pcm_kernel_ioctl(substream, SNDRV_PCM_IOCTL_DROP, NULL);
	return 0;
}

static inline int _ksnd_pcm_drain(snd_pcm_substream_t *substream)
{
	snd_pcm_kernel_ioctl(substream, SNDRV_PCM_IOCTL_DRAIN, NULL);
	return 0;
}

static inline int _ksnd_pcm_pause(snd_pcm_substream_t *substream,
								  unsigned int push)
{
	snd_pcm_kernel_ioctl(substream, SNDRV_PCM_IOCTL_PAUSE, (void *) push);
	return 0;
}

int ksnd_pcm_delay(ksnd_pcm_t *pcm, snd_pcm_sframes_t *delay)
{
	snd_pcm_substream_t *substream = pcm->substream;
	snd_pcm_sframes_t frames;
	int err;
	err = snd_pcm_kernel_ioctl(substream, SNDRV_PCM_IOCTL_DELAY, &frames);
	if (err < 0)
	{
		return err;
	}
	*delay = frames;
	return 0;
}

static struct file default_file = {.f_flags = 0 };

int ksnd_pcm_open(ksnd_pcm_t **kpcm,
				  int card,
				  int device,
				  snd_pcm_stream_t stream)
{
	int err = 0;
	ksnd_pcm_t *xkpcm;
	int minor;
	int stream_type, device_type;
	snd_pcm_t *pcm;
	xkpcm = kzalloc(sizeof(ksnd_pcm_t), GFP_KERNEL);
	if (!xkpcm)
	{
		err = -ENOMEM;
		goto _error_do_nothing;
	}
	if (stream == SND_PCM_STREAM_PLAYBACK)
	{
		stream_type = SNDRV_PCM_STREAM_PLAYBACK;
		device_type = SNDRV_DEVICE_TYPE_PCM_PLAYBACK;
	}
	else if (stream == SND_PCM_STREAM_CAPTURE)
	{
		stream_type = SNDRV_PCM_STREAM_CAPTURE;
		device_type = SNDRV_DEVICE_TYPE_PCM_CAPTURE;
	}
	else
	{
		err = -ENODEV;
		goto _error_do_free;
	}
#if defined(__TDT__)
	minor = snd_get_minor(device_type, card, device);
#else
	// This assumes snd_find_minor has been added to the kernel
	minor = snd_find_minor(device_type, card, device);
#endif
	if (minor < 0)
	{
		err = -ENODEV;
		goto _error_do_free;
	}
#if 0
	printk(KERN_DEBUG "Opening ALSA device hw:%d,%d for %s...\n",
		   card, device, stream == SND_PCM_STREAM_PLAYBACK ?
		   "playback" : "capture");
#endif
	pcm = snd_lookup_minor_data(minor, device_type);
	if (pcm == NULL)
	{
		err = -ENODEV;
		goto _error_do_free;
	}
	if (!try_module_get(pcm->card->module))
	{
		err = -EFAULT;
		goto _error_do_free;
	}
	mutex_lock(&pcm->open_mutex);
	err = snd_pcm_open_substream(pcm, stream_type, &default_file, &(xkpcm->substream));
	/* We don't support blocking open here, just fail if busy */
	if (err == -EAGAIN)
	{
		err = -EBUSY;
	}
	mutex_unlock(&pcm->open_mutex);
	if (err < 0)
		goto _error_do_put_and_free;
	*kpcm = xkpcm;
	return err;
_error_do_put_and_free:
	module_put(pcm->card->module);
_error_do_free:
	kfree(xkpcm);
_error_do_nothing:
	return err;
}

void ksnd_pcm_close(ksnd_pcm_t *kpcm)
{
	snd_pcm_t *pcm;
	snd_pcm_substream_t *substream = kpcm->substream;
	if (kpcm->hwareas[0].addr)
		iounmap(kpcm->hwareas[0].addr);
	pcm = substream->pcm;
	_ksnd_pcm_drop(substream);
	mutex_lock(&pcm->open_mutex);
	snd_pcm_release_substream(substream);
	mutex_unlock(&pcm->open_mutex);
	wake_up(&pcm->open_wait);
	module_put(pcm->card->module);
	kfree(kpcm);
}

static int _ksnd_pcm_write_transfer(snd_pcm_substream_t *substream,
									unsigned int hwoff,
									unsigned long data,
									unsigned int off,
									snd_pcm_uframes_t frames,
									unsigned int srcchannels)
{
	snd_pcm_runtime_t *runtime = substream->runtime;
	char *buf = (char *) data + samples_to_bytes(runtime, off * srcchannels);
	char *uncachedbuf = ioremap_nocache(runtime->dma_addr, runtime->dma_bytes);
	char *hwbuf = uncachedbuf + frames_to_bytes(runtime, hwoff);
#ifdef VERY_VERBOSE
	printk("offset %d base %p samples %p\n", hwoff, uncachedbuf, hwbuf);
#endif
	if (srcchannels == runtime->channels)
	{
		memcpy(hwbuf, buf, frames_to_bytes(runtime, frames));
	}
	else
	{
		int srcwidth = samples_to_bytes(runtime, srcchannels);
		int dstwidth = frames_to_bytes(runtime, 1);
		int transfersize = srcwidth > dstwidth ? dstwidth : srcwidth;
		int i;
		for (i = 0; i < frames; i++)
		{
			memcpy(hwbuf, buf, transfersize);
			buf += srcwidth;
			hwbuf += dstwidth;
		}
	}
	iounmap(uncachedbuf);
	return 0;
}

static int _ksnd_pcm_IEC60958_transfer(snd_pcm_substream_t *substream,
									   unsigned int hwoffset,
									   unsigned long data,
									   unsigned int offset,
									   snd_pcm_uframes_t frames,
									   unsigned int srcchannels)
{
	int ret = 0;
	mm_segment_t fs;
	fs = get_fs();
	set_fs(get_ds());
	set_fs(fs);
	return ret;
}

typedef int (*transfer_f)(snd_pcm_substream_t *substream,
						  unsigned int hwoff,
						  unsigned long data,
						  unsigned int off,
						  snd_pcm_uframes_t size, unsigned int srcchannels);

static int _ksnd_pcm_writei1(snd_pcm_substream_t *substream,
							 unsigned long data,
							 snd_pcm_uframes_t size,
							 int srcchannels, transfer_f transfer)
{
	snd_pcm_runtime_t *runtime = substream->runtime;
	snd_pcm_uframes_t xfer = 0;
	snd_pcm_uframes_t offset = 0;
	int err = 0;
	if (size == 0)
		return 0;
	snd_pcm_stream_lock_irq(substream);
	switch (_ksnd_pcm_state(substream))
	{
		case SNDRV_PCM_STATE_PREPARED:
		case SNDRV_PCM_STATE_RUNNING:
		case SNDRV_PCM_STATE_PAUSED:
			break;
		case SNDRV_PCM_STATE_XRUN:
			err = -EPIPE;
			goto _end_unlock;
		case SNDRV_PCM_STATE_SUSPENDED:
			err = -ESTRPIPE;
			goto _end_unlock;
		default:
			err = -EBADFD;
			goto _end_unlock;
	}
	while (size > 0)
	{
		snd_pcm_uframes_t frames, appl_ptr, appl_ofs;
		snd_pcm_uframes_t avail;
		snd_pcm_uframes_t cont;
		avail = _ksnd_pcm_avail_update(substream);
#if defined(__TDT__) && (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 30))
		// attribute xfer_align is not used any more
		/* #warning Do we have to check the values 'size' and 'avail'? */
		if ((avail < runtime->control->avail_min) && (size > avail))
		{
#else
		if (((avail < runtime->control->avail_min && size > avail) ||
				(size >= runtime->xfer_align
				 && avail < runtime->xfer_align)))
		{
#endif
			int res;
			snd_pcm_stream_unlock_irq(substream);
			do
			{
				res = _ksnd_pcm_wait(substream, 10000);
			}
			while (res == 0 &&
					_ksnd_pcm_state(substream) !=
					SNDRV_PCM_STATE_PREPARED
					&& _ksnd_pcm_state(substream) !=
					SNDRV_PCM_STATE_PAUSED);
			snd_pcm_stream_lock_irq(substream);
			if (res == 0) /* timeout */
			{
				if (_ksnd_pcm_state(substream) ==
						SNDRV_PCM_STATE_SUSPENDED)
				{
					err = -ESTRPIPE;
					goto _end_unlock;
				}
				else
				{
					snd_printd("playback write error "
							   "(DMA or IRQ trouble?)\n");
					err = -EIO;
					goto _end_unlock;
				}
			}
			else if (res < 0) /* error */
			{
				err = res;
				goto _end_unlock;
			}
			avail = snd_pcm_playback_avail(runtime);
		}
		if (avail > runtime->min_align)
			avail -= avail % runtime->min_align;
		frames = size > avail ? avail : size;
		cont = runtime->buffer_size - runtime->control->appl_ptr % runtime->buffer_size;
		if (frames > cont)
			frames = cont;
		if (snd_BUG_ON(!frames))
		{
			snd_pcm_stream_unlock_irq(substream);
			return -EINVAL;
		}
		appl_ptr = runtime->control->appl_ptr;
		appl_ofs = appl_ptr % runtime->buffer_size;
		snd_pcm_stream_unlock_irq(substream);
		err = transfer(substream, appl_ofs, data, offset, frames, srcchannels);
		snd_pcm_stream_lock_irq(substream);
		if (err < 0)
			goto _end;
		switch (_ksnd_pcm_state(substream))
		{
			case SNDRV_PCM_STATE_XRUN:
				err = -EPIPE;
				goto _end_unlock;
			case SNDRV_PCM_STATE_SUSPENDED:
				err = -ESTRPIPE;
				goto _end_unlock;
			default:
				break;
		}
		appl_ptr += frames;
		if (appl_ptr >= runtime->boundary)
		{
			runtime->control->appl_ptr = 0;
		}
		else
		{
			runtime->control->appl_ptr = appl_ptr;
		}
		if (substream->ops->ack)
			substream->ops->ack(substream);
		offset += frames;
		size -= frames;
		xfer += frames;
		if (_ksnd_pcm_state(substream) == SNDRV_PCM_STATE_PREPARED &&
				snd_pcm_playback_hw_avail(runtime) >= (snd_pcm_sframes_t) runtime->start_threshold)
		{
			err = snd_pcm_start(substream);
			if (err < 0)
				goto _end_unlock;
		}
	}
_end_unlock:
	snd_pcm_stream_unlock_irq(substream);
_end:
	return xfer > 0 ? (snd_pcm_sframes_t) xfer : err;
}

int ksnd_pcm_writei(ksnd_pcm_t *kpcm,
					int *data, unsigned int size, unsigned int srcchannels)
{
	snd_pcm_substream_t *substream = kpcm->substream;
	snd_pcm_runtime_t *runtime;
	int err;
	transfer_f out_func = 0;
	runtime = substream->runtime;
	if (substream->pcm->card->number == 2)
	{
		out_func = _ksnd_pcm_IEC60958_transfer;
	}
	else
	{
		out_func = _ksnd_pcm_write_transfer;
	}
	if (_ksnd_pcm_state(substream) == SNDRV_PCM_STATE_OPEN)
		return -EBADFD;
	if (runtime->access != SNDRV_PCM_ACCESS_RW_INTERLEAVED
			&& runtime->channels > 1)
		return -EINVAL;
	if (substream->stream != SNDRV_PCM_STREAM_PLAYBACK)
		return -EINVAL;
	if (size == 0)
		return 0;
	do
	{
		err =
			_ksnd_pcm_writei1(substream, (unsigned long)data, size,
							  srcchannels, out_func);
		if (err < 0)
		{
			if (err == -EAGAIN)
			{
				continue;
			}
			if (err == -EPIPE)
			{
				printk("ALSA Aud underrun for hw:%d,%d\n",
					   substream->pcm->card->number,
					   substream->pcm->device);
				if ((err = ksnd_pcm_prepare(kpcm)) < 0)
					return err;
				continue;
			}
			return err;
		}
		else
		{
			data += samples_to_bytes(runtime, err * srcchannels);
			size -= err;
		}
	}
	while (size > 0);
	return 0;
}

int ksnd_pcm_stop(ksnd_pcm_t *kpcm)
{
	snd_pcm_substream_t *substream = kpcm->substream;
	return _ksnd_pcm_drop(substream);
}

int ksnd_pcm_drain(ksnd_pcm_t *kpcm)
{
	snd_pcm_substream_t *substream = kpcm->substream;
	return _ksnd_pcm_drain(substream);
}

int ksnd_pcm_pause(ksnd_pcm_t *kpcm, unsigned int push)
{
	snd_pcm_substream_t *substream = kpcm->substream;
	return _ksnd_pcm_pause(substream, push);
}

int ksnd_pcm_mute(ksnd_pcm_t *kpcm, unsigned int push)
{
	int err;
	snd_pcm_substream_t *substream = kpcm->substream;
	if (push == 0)
	{
		err = ksnd_pcm_prepare(kpcm);
		if (err < 0)
			return err;
		ksnd_pcm_start(kpcm);
	}
	else
	{
		_ksnd_pcm_pause(substream, push);
		_ksnd_pcm_drop(substream);
	}
	return 0;
}

int ksnd_pcm_prepare(ksnd_pcm_t *kpcm)
{
	int err;
	snd_pcm_substream_t *substream = kpcm->substream;
	err = snd_pcm_kernel_ioctl(substream, SNDRV_PCM_IOCTL_PREPARE, NULL);
	if (err < 0)
	{
		snd_printd("alsa_prepare: SNDRV_PCM_IOCTL_PREPARE failed\n");
		return err;
	}
	return 0;
}

int ksnd_pcm_start(ksnd_pcm_t *kpcm)
{
	snd_pcm_substream_t *substream = kpcm->substream;
	if (substream != NULL)
		snd_pcm_kernel_ioctl(substream, SNDRV_PCM_IOCTL_START, NULL);
	return 0;
}

/**
 * \brief _ksnd_pcm_hw_param_value
 *
 * Implementation is copied directly from Linux's not-exported-to-modules
 * snd_pcm_hw_param_value function.
 *
 * \param params the hw_params instance
 * \param var parameter to retrieve
 * \param dir pointer to the direction (-1,0,1) or NULL
 *
 * \return Return the value for field PAR if it's fixed in
 * configuration space defined by PARAMS. Return -EINVAL otherwise
 */
static int _ksnd_pcm_hw_param_value(const ksnd_pcm_hw_params_t *params,
									snd_pcm_hw_param_t var, int *dir)
{
	if (hw_is_mask(var))
	{
		const struct snd_mask *mask = hw_param_mask_c(params, var);
		if (!snd_mask_single(mask))
			return -EINVAL;
		if (dir)
			*dir = 0;
		return snd_mask_value(mask);
	}
	if (hw_is_interval(var))
	{
		const struct snd_interval *i = hw_param_interval_c(params, var);
		if (!snd_interval_single(i))
			return -EINVAL;
		if (dir)
			*dir = i->openmin;
		return snd_interval_value(i);
	}
	return -EINVAL;
}

/* Return the value for field PAR if it's fixed in configuration space
 * defined by PARAMS. Return -EINVAL otherwise
 */
static int _ksnd_pcm_hw_param_get(const snd_pcm_hw_params_t *params, snd_pcm_hw_param_t var,
								  unsigned int *val, int *dir)
{
	int err = _ksnd_pcm_hw_param_value(params, var, dir);
	if (err < 0)
		return err;
	*val = err;
	return 0;
}

/**
 * Get the parameters the hardware is currently using (if any).
 *
 * The the alsa-lib implementation of this function does a lot of heroics
 * regenerating the hardware parameters from the runtime structures.
 * that might be a little brittle for a 'hack' like ksound so we cached
 * the last set of parameters stored in ::ksnd_pcm_hw_params() instead.
 */
int ksnd_pcm_hw_params_current(ksnd_pcm_t *kpcm, ksnd_pcm_hw_params_t *params)
{
#if 0
	if (kpcm->actual_hwparams it not valid)
		return -EBADFD;
#endif
	*params = kpcm->actual_hwparams;
	return 0;
}

int ksnd_pcm_hw_params(ksnd_pcm_t *kpcm, ksnd_pcm_hw_params_t *params)
{
	snd_pcm_substream_t *substream = kpcm->substream;
	int err;
	if (substream == NULL)
		return -EFAULT;
	err = snd_pcm_kernel_ioctl(substream, SNDRV_PCM_IOCTL_HW_PARAMS,
							   params);
	if (0 != err)
		return err;
	kpcm->actual_hwparams = *params;
	return 0;
}

int ksnd_pcm_hw_params_any(ksnd_pcm_t *kpcm, ksnd_pcm_hw_params_t *params)
{
	snd_pcm_substream_t *substream = kpcm->substream;
	_snd_pcm_hw_params_any(params);
	return snd_pcm_hw_refine(substream, params);
}

int ksnd_pcm_set_params(ksnd_pcm_t *pcm,
						int nrchannels, int sampledepth, int samplerate,
						int periodsize, int buffersize)
{
	snd_pcm_substream_t *substream = pcm->substream;
	snd_pcm_runtime_t *runtime = substream->runtime;
	snd_pcm_hw_params_t *hw_params = NULL;
	snd_pcm_sw_params_t *sw_params = NULL;
	int err;
	int format;
	snd_mask_t mask;
	int i;
	void *hwbuf;
	err = ksnd_pcm_hw_params_malloc(&hw_params);
	if (0 != err)
		goto failure;
	sw_params = kmalloc(sizeof(*sw_params), GFP_KERNEL);
	if (!sw_params)
	{
		err = -ENOMEM;
		goto failure;
	}
	switch (sampledepth)
	{
		case 16:
			format = SNDRV_PCM_FORMAT_S16_LE;
			break;
		case 24:
			sampledepth = 32;
		/*FALLTHRU*/
		case 32:
			format = SNDRV_PCM_FORMAT_S32_LE;
			break;
		default:
			snd_printd("%s Unsupported sampledepth %d\n",
					   __FUNCTION__, sampledepth);
			err = -EINVAL;
			goto failure;
	}
	err = ksnd_pcm_hw_params_any(pcm, hw_params);
	if (snd_BUG_ON(err < 0))
		goto failure;
	_snd_pcm_hw_param_setinteger(hw_params, SNDRV_PCM_HW_PARAM_PERIODS);
	_snd_pcm_hw_param_min(hw_params, SNDRV_PCM_HW_PARAM_PERIODS, 2, 0);
	snd_mask_none(&mask);
	snd_mask_set(&mask, SNDRV_PCM_ACCESS_RW_INTERLEAVED);
	err =
		snd_pcm_hw_param_mask(substream, hw_params,
							  SNDRV_PCM_HW_PARAM_ACCESS, &mask);
	if (err < 0)
	{
		err = -EINVAL;
		goto failure;
	}
	err = snd_pcm_hw_param_set(substream, hw_params, SNDRV_PCM_HW_PARAM_RATE, samplerate, 0);
	if (snd_BUG_ON(err < 0))
		goto failure;
	err = snd_pcm_hw_param_near(substream, hw_params, SNDRV_PCM_HW_PARAM_CHANNELS, nrchannels, NULL);
	if (snd_BUG_ON(err < 0))
		goto failure;
	err = snd_pcm_hw_param_near(substream, hw_params, SNDRV_PCM_HW_PARAM_FORMAT, format, 0);
	if (snd_BUG_ON(err < 0))
		goto failure;
	err = snd_pcm_hw_param_near(substream, hw_params, SNDRV_PCM_HW_PARAM_PERIOD_SIZE, periodsize, NULL);
	if (snd_BUG_ON(err < 0))
		goto failure;
	err = snd_pcm_hw_param_near(substream, hw_params, SNDRV_PCM_HW_PARAM_BUFFER_SIZE, buffersize, NULL);
	if (snd_BUG_ON(err < 0))
		goto failure;
	_ksnd_pcm_drop(substream);
	/*now we re-use the 61937 control to enable the HW sync mechanism */
	if (0 != (err = ksnd_pcm_hw_params(pcm, hw_params) < 0))
	{
		snd_printd("HW_PARAMS failed: for %d:%d code is %i\n",
				   substream->pcm->card->number, substream->pcm->device,
				   err);
		goto failure;
	}
	memset(sw_params, 0, sizeof(*sw_params));
	sw_params->start_threshold =
		(runtime->buffer_size - (runtime->period_size * 2));
	sw_params->stop_threshold = runtime->buffer_size;
	sw_params->tstamp_mode = SNDRV_PCM_TSTAMP_ENABLE;
	sw_params->period_step = 1;
	sw_params->sleep_min = 0;
	sw_params->avail_min = runtime->period_size;
	sw_params->silence_threshold = runtime->period_size;
	sw_params->silence_size = runtime->period_size;
	if ((err =
				snd_pcm_kernel_ioctl(substream, SNDRV_PCM_IOCTL_SW_PARAMS,
									 sw_params)) < 0)
	{
		snd_printd("SW_PARAMS failed: for %d:%d code is %i\n",
				   substream->pcm->card->number, substream->pcm->device,
				   err);
		goto failure;
	}
	if ((err = ksnd_pcm_prepare(pcm)) < 0)
		goto failure;
	if (pcm->hwareas[0].addr)
		iounmap(pcm->hwareas[0].addr);
	hwbuf = ioremap_nocache(runtime->dma_addr, runtime->dma_bytes);
	for (i = 0; i < nrchannels; i++)
	{
		pcm->hwareas[i].addr = hwbuf;
		pcm->hwareas[i].first = i * sampledepth;
		pcm->hwareas[i].step = nrchannels * sampledepth;
	}
	nrchannels = _ksnd_pcm_hw_param_value(hw_params, SNDRV_PCM_HW_PARAM_CHANNELS, 0);
	samplerate = _ksnd_pcm_hw_param_value(hw_params, SNDRV_PCM_HW_PARAM_RATE, 0);
	periodsize = _ksnd_pcm_hw_param_value(hw_params, SNDRV_PCM_HW_PARAM_PERIOD_SIZE, 0);
	buffersize = _ksnd_pcm_hw_param_value(hw_params, SNDRV_PCM_HW_PARAM_BUFFER_SIZE, 0);
	printk(KERN_DEBUG "ksound: Set parameters for hw:%d,%d to %d x %dhz with period %d (of %d)\n",
		   substream->pcm->card->number, substream->pcm->device,
		   nrchannels, samplerate, periodsize, buffersize);
	err = 0;
failure:
	if (hw_params)
		ksnd_pcm_hw_params_free(hw_params);
	if (sw_params)
		kfree(sw_params);
	return err;
}

/**
 * \brief Get the transfer size parameters in a simple way
 * \param kpcm PCM handle
 * \param buffer_size PCM ring buffer size in frames
 * \param period_size PCM period size in frames
 * \return 0 on success otherwise a negative error code
 */
int ksnd_pcm_get_params(ksnd_pcm_t *kpcm,
						snd_pcm_uframes_t *buffer_size,
						snd_pcm_uframes_t *period_size)
{
	snd_pcm_hw_params_t *hw;
	int err;
	err = ksnd_pcm_hw_params_malloc(&hw);
	if (err < 0)
		return err;
	err = ksnd_pcm_hw_params_current(kpcm, hw);
	if (err >= 0)
		err = ksnd_pcm_hw_params_get_buffer_size(hw, buffer_size);
	if (err >= 0)
		err = ksnd_pcm_hw_params_get_period_size(hw, period_size, NULL);
	ksnd_pcm_hw_params_free(hw);
	if (err < 0)
		return err;
	return 0;
}

int ksnd_pcm_hw_params_malloc(ksnd_pcm_hw_params_t **ptr)
{
	ksnd_pcm_hw_params_t *p;
	p = kzalloc(sizeof(*p), GFP_KERNEL);
	if (!p)
		return -ENOMEM;
	*ptr = p;
	return 0;
}

void ksnd_pcm_hw_params_free(ksnd_pcm_hw_params_t *obj)
{
	kfree(obj);
}

/**
 * \brief Extract buffer size from a configuration space
 * \param params Configuration space
 * \param val Returned buffer size in frames
 * \return 0 otherwise a negative error code if not exactly one is present
 */
int ksnd_pcm_hw_params_get_buffer_size(const ksnd_pcm_hw_params_t *params, snd_pcm_uframes_t *val)
{
	unsigned int _val;
	int err = _ksnd_pcm_hw_param_get(params, SNDRV_PCM_HW_PARAM_BUFFER_SIZE, &_val, NULL);
	if (err >= 0)
		*val = _val;
	return err;
}

/**
 * \brief Extract period size from a configuration space
 * \param params Configuration space
 * \param val Returned approximate period size in frames
 * \param dir Sub unit direction
 * \return 0 otherwise a negative error code if not exactly one is present
 *
 * Actual exact value is <,=,> the approximate one following dir (-1, 0, 1)
 */
int ksnd_pcm_hw_params_get_period_size(const ksnd_pcm_hw_params_t *params, snd_pcm_uframes_t *val, int *dir)
{
	unsigned int _val;
	int err = _ksnd_pcm_hw_param_get(params, SNDRV_PCM_HW_PARAM_PERIOD_SIZE, &_val, dir);
	if (err >= 0)
		*val = _val;
	return err;
}

/******************************
 **** control access stuff ****
 ******************************/

void ksnd_ctl_elem_id_alloca(snd_ctl_elem_id_t **id)
{
	*id = kmalloc(sizeof(snd_ctl_elem_id_t), GFP_KERNEL);
	if (*id != NULL)
	{
		memset(*id, 0, sizeof(snd_ctl_elem_id_t));
	}
}

/**
 * \brief Set name part for a CTL element identifier
 * \param obj CTL element identifier
 * \param val CTL element name
 */
void ksnd_ctl_elem_id_set_name(snd_ctl_elem_id_t *obj, const char *val)
{
	if (snd_BUG_ON(!obj))
		return;
	strncpy((char *)obj->name, val, sizeof(obj->name) - 1);
	obj->name[sizeof(obj->name) - 1] = '\0';
}

/**
 * \brief Set interface part for a CTL element identifier
 * \param obj CTL element identifier
 * \param val CTL element related interface
 */
void ksnd_ctl_elem_id_set_interface(snd_ctl_elem_id_t *obj, snd_ctl_elem_iface_t val)
{
	if (snd_BUG_ON(!obj))
		return;
	obj->iface = val;
}

/**
 * \brief Set device part for a CTL element identifier
 * \param obj CTL element identifier
 * \param val CTL element related device
 */
void ksnd_ctl_elem_id_set_device(snd_ctl_elem_id_t *obj, unsigned int val)
{
	if (snd_BUG_ON(!obj))
		return;
	obj->device = val;
}
/**
 * \brief Set index part for a CTL element identifier
 * \param obj CTL element identifier
 * \param val CTL element index
 */
void ksnd_ctl_elem_id_set_index(snd_ctl_elem_id_t *obj, unsigned int val)
{
	if (snd_BUG_ON(!obj))
		return;
	obj->index = val;
}

snd_kcontrol_t *ksnd_substream_find_elem(snd_pcm_substream_t *substream, snd_ctl_elem_id_t *id)
{
	snd_kcontrol_t *ret;
	down_read(&substream->pcm->card->controls_rwsem);
	ret = snd_ctl_find_id(substream->pcm->card, id);
	up_read(&substream->pcm->card->controls_rwsem);
	return ret;
}

void ksnd_ctl_elem_value_alloca(snd_ctl_elem_value_t **id)
{
	*id = kmalloc(sizeof(snd_ctl_elem_value_t), GFP_KERNEL);
	if (*id != NULL)
	{
		memset(*id, 0, sizeof(snd_ctl_elem_value_t));
	}
}

/**
 * \brief Set CTL element identifier of a CTL element id/value
 * \param obj CTL element id/value
 * \param ptr CTL element identifier
 */
void ksnd_ctl_elem_value_set_id(snd_ctl_elem_value_t *obj, const snd_ctl_elem_id_t *ptr)
{
	if (obj && ptr) obj->id = *ptr;
}

/**
 * \brief Set value for an entry of a #SND_CTL_ELEM_TYPE_INTEGER CTL element id/value
 * \param obj CTL element id/value
 * \param idx Entry index
 * \param val value for the entry
 */
void ksnd_ctl_elem_value_set_integer(snd_ctl_elem_value_t *obj, unsigned int idx, long val)
{
	if (obj) obj->value.integer.value[idx] = val;
}

/**
 * \brief Set value for a #SND_CTL_ELEM_TYPE_IEC958 CTL element id/value
 * \param obj CTL element id/value
 * \param ptr Pointer to CTL element value
 */
void ksnd_ctl_elem_value_set_iec958(snd_ctl_elem_value_t *obj, const struct snd_aes_iec958 *ptr)
{
	if (snd_BUG_ON(!obj || !ptr))
		return;
	memcpy(&obj->value.iec958, ptr, sizeof(obj->value.iec958));
}

/**
 * \brief Set CTL element value
 * \param ctl CTL handle
 * \param control CTL element id/value pointer
 * \retval 0 on success
 * \retval >0 on success when value was changed
 * \retval <0 a negative error code
 */
int ksnd_hctl_elem_write(snd_kcontrol_t *elem, snd_ctl_elem_value_t *control)
{
	int ret = -EINVAL;
	if (elem->put)
		ret = elem->put(elem, control);
	return ret;
}

static int __init ksnd_module_init(void)
{
#ifdef VERY_VERBOSE
	printk(KERN_DEBUG "ksound: Built %s %s\n", __DATE__, __TIME__);
#endif
	return 0;
}

static void __exit ksnd_module_exit(void)
{
}

module_init(ksnd_module_init);
module_exit(ksnd_module_exit);
