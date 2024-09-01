/*
 * Dagobert:
 * e2 proc handling rebuilding.
 * We do not rebuild all files from dm800 but those we need from e2 view.
 * the rest seems to be receiver & driver specific.
 *
 *
 * progress
 * |
 * bus
 * |
 * ----------- nim_sockets
 * |
 * stb
 * |
 * ----------- audio
 * | |
 * | --------- ac3
 * | |
 * | --------- audio_delay_pcm
 * | |
 * | --------- audio_delay_bitstream
 * | |
 * | --------- audio_j1_mute
 * | |
 * | --------- ac3_choices
 * | |
 * | ---------
 * |
 * ----------- video
 * | |
 * | --------- alpha
 * | |
 * | --------- aspect
 * | |
 * | --------- aspect_choices
 * | |
 * | --------- force_dvi
 * | |
 * | --------- hdmi_colorspace
 * | |
 * | --------- hdmi_colorspace_choices
 * | |
 * | --------- policy
 * | |
 * | --------- policy_choices
 * | |
 * | --------- videomode
 * | |
 * | --------- videomode_50hz
 * | |
 * | --------- videomode_60hz
 * | |
 * | --------- videomode_choices
 * | |
 * | --------- videomode_preferred
 * | |
 * | --------- pal_v_start
 * | |
 * | --------- pal_v_end
 * | |
 * | --------- pal_h_start
 * | |
 * | --------- pal_h_end
 * |
 * ---------- avs
 * | |
 * | --------- 0
 * | |
 * | --------- colorformat <-colorformat in generlell, hdmi and scart
 * | |
 * | --------- colorformat_choices
 * | |
 * | --------- fb <-fastblanking
 * | |
 * | --------- sb <-slowblanking
 * | |
 * | --------- volume
 * | |
 * | --------- input <-Input, Scart VCR Input or Encoder
 * | |
 * | --------- input_choices
 * | |
 * | --------- standby
 * |
 * ---------- denc
 * | |
 * | --------- 0
 * | |
 * | --------- wss
 * | |
 * | ---------
 * | |
 * | ---------
 * |
 * ---------- fp (this is wrong used for e2 I think. on dm800 this is frontprocessor and there is another proc entry for frontend)
 * | |
 * | --------- lnb_sense1
 * | |
 * | --------- lnb_sense2
 * | |
 * | --------- led0_pattern
 * | |
 * | --------- led_pattern_speed
 * | |
 * | |
 * | --------- version
 * | |
 * | --------- wakeup_time <- dbox frontpanel wakeuptime
 * | |
 * | --------- was_timer_wakeup
 * |
 * |
 * ---------- hdmi
 * | |
 * | --------- bypass_edid_checking
 * | |
 * | --------- enable_hdmi_resets
 * | |
 * | --------- audio_source
 * | |
 * | ---------
 * |
 * ---------- info
 * | |
 * | --------- model <- Version String of out Box
 * |
 * ---------- tsmux
 * | |
 * | --------- input0
 * | |
 * | --------- input1
 * | |
 * | --------- ci0_input
 * | |
 * | --------- ci1_input
 * | |
 * | --------- lnb_b_input
 * | |
 * | ---------
 * |
 * ---------- misc
 * | |
 * | --------- 12V_output
 * |
 * ---------- tuner (dagoberts tuner entry ;-) )
 * | |
 * | ---------
 * |
 * ---------- vmpeg
 * | |
 * | --------- 0/1
 * | |
 * | --------- dst_left \
 * | | |
 * | --------- dst_top |
 * | | > PIG WINDOW SIZE AND POSITION
 * | --------- dst_width |
 * | | |
 * | --------- dst_height /
 * | |
 * | --------- dst_all (Dagobert: Dont confuse player by setting value one after each other)
 * | |
 * | --------- yres
 * | |
 * | --------- xres
 * | |
 * | --------- framerate
 * | |
 * | --------- aspect
 * | |TODO
 * | | v
 * | --------- progressive
 * |
 * ---------- fb
 * | |
 * | --------- dst_left \
 * | | |
 * | --------- dst_top |
 * | | > PIG WINDOW SIZE AND POSITION
 * | --------- dst_width |
 * | | |
 * | --------- dst_height /
 *
 */

#ifdef m
Offen:
proc = open("/sys/class/stmcoredisplay/display0/hdmi0.0/modes", "r")
#endif

#include <linux/proc_fs.h> /* proc fs */
#include <asm/uaccess.h> /* copy_from_user */

#include <linux/dvb/video.h> /* Video Format etc */

#include <linux/dvb/audio.h>
#include <linux/smp_lock.h>
#include <linux/string.h>

#include "../backend.h"
#include "../dvb_module.h"
#include "linux/dvb/stm_ioctls.h"

	   /* external functions provided by the module e2_procfs */
	   extern int install_e2_procs(char *name, read_proc_t *read_proc, write_proc_t *write_proc, void *data);
extern int remove_e2_procs(char *name, read_proc_t *read_proc, write_proc_t *write_proc);

extern int proc_progress_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_progress_write(struct file *file, const char __user *buf, unsigned long count, void *data);

extern int proc_bus_nim_sockets_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);

extern int proc_info_model_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);

extern int proc_audio_ac3_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_audio_ac3_write(struct file *file, const char __user *buf, unsigned long count, void *data);
extern int proc_audio_delay_pcm_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_audio_delay_pcm_write(struct file *file, const char __user *buf, unsigned long count, void *data);
extern int proc_audio_delay_bitstream_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_audio_delay_bitstream_write(struct file *file, const char __user *buf, unsigned long count, void *data);
extern int proc_audio_j1_mute_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_audio_j1_mute_write(struct file *file, const char __user *buf, unsigned long count, void *data);
extern int proc_video_aspect_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_video_aspect_write(struct file *file, const char __user *buf, unsigned long count, void *data);
extern int proc_video_aspect_choices_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_video_hdmi_colorspace_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_video_hdmi_colorspace_write(struct file *file, const char __user *buf, unsigned long count, void *data);
extern int proc_video_hdmi_colorspace_choices_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_video_policy_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_video_policy_write(struct file *file, const char __user *buf, unsigned long count, void *data);
extern int proc_video_policy_choices_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_video_videomode_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_video_videomode_write(struct file *file, const char __user *buf, unsigned long count, void *data);
extern int proc_video_pal_h_start_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_video_pal_h_start_write(struct file *file, const char __user *buf, unsigned long count, void *data);
extern int proc_video_pal_h_end_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_video_pal_h_end_write(struct file *file, const char __user *buf, unsigned long count, void *data);
extern int proc_video_pal_v_start_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_video_pal_v_start_write(struct file *file, const char __user *buf, unsigned long count, void *data);
extern int proc_video_pal_v_end_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_video_pal_v_end_write(struct file *file, const char __user *buf, unsigned long count, void *data);
extern int proc_video_alpha_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_video_alpha_write(struct file *file, const char __user *buf, unsigned long count, void *data);
#if defined(ADB_BOX)
extern int proc_video_switch_choices_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_video_switch_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_video_switch_write(struct file *file, const char __user *buf, unsigned long count, void *data);
extern int proc_video_switch_type_write(struct file *file, const char __user *buf, unsigned long count, void *data);
#endif
extern int proc_avs_0_colorformat_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_avs_0_colorformat_write(struct file *file, const char __user *buf, unsigned long count, void *data);
extern int proc_avs_0_colorformat_choices_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_avs_0_fb_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_avs_0_fb_write(struct file *file, const char __user *buf, unsigned long count, void *data);
extern int proc_avs_0_input_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_avs_0_input_write(struct file *file, const char __user *buf, unsigned long count, void *data);
extern int proc_avs_0_sb_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_avs_0_sb_write(struct file *file, const char __user *buf, unsigned long count, void *data);
extern int proc_avs_0_volume_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_avs_0_volume_write(struct file *file, const char __user *buf, unsigned long count, void *data);
extern int proc_avs_0_input_choices_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);

extern int proc_avs_0_standby_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_avs_0_standby_write(struct file *file, const char __user *buf, unsigned long count, void *data);

extern int proc_denc_0_wss_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_denc_0_wss_write(struct file *file, const char __user *buf, unsigned long count, void *data);

extern int proc_tsmux_input0_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_tsmux_input0_write(struct file *file, const char __user *buf, unsigned long count, void *data);
extern int proc_tsmux_input1_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_tsmux_input1_write(struct file *file, const char __user *buf, unsigned long count, void *data);
extern int proc_tsmux_ci0_input_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_tsmux_ci0_input_write(struct file *file, const char __user *buf, unsigned long count, void *data);
extern int proc_tsmux_ci1_input_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_tsmux_ci1_input_write(struct file *file, const char __user *buf, unsigned long count, void *data);
extern int proc_tsmux_lnb_b_input_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_tsmux_lnb_b_input_write(struct file *file, const char __user *buf, unsigned long count, void *data);
#if defined(IPBOX9900)
extern int proc_misc_12V_output_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_misc_12V_output_write(struct file *file, const char __user *buf, unsigned long count, void *data);
#endif
extern int proc_audio_ac3_choices_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_video_videomode_choices_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_video_videomode_preferred_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_video_videomode_preferred_write(struct file *file, const char __user *buf, unsigned long count, void *data);
extern int proc_fp_led0_pattern_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_fp_led0_pattern_write(struct file *file, const char __user *buf, unsigned long count, void *data);
extern int proc_fp_led_pattern_speed_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_fp_led_pattern_speed_write(struct file *file, const char __user *buf, unsigned long count, void *data);
extern int proc_fp_was_timer_wakeup_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_fp_was_timer_wakeup_write(struct file *file, const char __user *buf, unsigned long count, void *data);

#if defined(IPBOX9900) || defined(IPBOX99)
extern int proc_fp_wakeup_time_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_fp_wakeup_time_write(struct file *file, const char __user *buf, unsigned long count, void *data);
#endif

extern int proc_vmpeg_0_dst_left_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_vmpeg_0_dst_left_write(struct file *file, const char __user *buf, unsigned long count, void *data);
extern int proc_vmpeg_0_dst_top_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_vmpeg_0_dst_top_write(struct file *file, const char __user *buf, unsigned long count, void *data);
extern int proc_vmpeg_0_dst_width_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_vmpeg_0_dst_width_write(struct file *file, const char __user *buf, unsigned long count, void *data);
extern int proc_vmpeg_0_dst_height_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_vmpeg_0_dst_height_write(struct file *file, const char __user *buf, unsigned long count, void *data);
extern int proc_vmpeg_0_dst_all_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_vmpeg_0_dst_all_write(struct file *file, const char __user *buf, unsigned long count, void *data);

extern int proc_vmpeg_0_yres_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_vmpeg_0_xres_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_vmpeg_0_aspect_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_vmpeg_0_framerate_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);

extern int proc_hdmi_audio_source_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_hdmi_audio_source_write(struct file *file, const char __user *buf, unsigned long count, void *data);
extern int proc_hdmi_audio_source_choices_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);

extern int proc_hdmi_edid_handling_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_hdmi_edid_handling_write(struct file *file, const char __user *buf, unsigned long count, void *data);

extern int proc_hdmi_output_write(struct file *file, const char __user *buf, unsigned long count, void *data);
extern int proc_hdmi_output_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_hdmi_output_choices_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);

extern int proc_stream_AV_SYNC_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_stream_AV_SYNC_write(struct file *file, const char __user *buf, unsigned long count, void *data);
extern int proc_stream_TRICK_MODE_AUDIO_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_stream_TRICK_MODE_AUDIO_write(struct file *file, const char __user *buf, unsigned long count, void *data);
extern int proc_stream_PLAY_24FPS_VIDEO_AT_25FPS_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_stream_PLAY_24FPS_VIDEO_AT_25FPS_write(struct file *file, const char __user *buf, unsigned long count, void *data);
extern int proc_stream_MASTER_CLOCK_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_stream_MASTER_CLOCK_write(struct file *file, const char __user *buf, unsigned long count, void *data);
extern int proc_stream_EXTERNAL_TIME_MAPPING_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_stream_EXTERNAL_TIME_MAPPING_write(struct file *file, const char __user *buf, unsigned long count, void *data);
extern int proc_stream_DISPLAY_FIRST_FRAME_EARLY_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_stream_DISPLAY_FIRST_FRAME_EARLY_write(struct file *file, const char __user *buf, unsigned long count, void *data);
extern int proc_stream_STREAM_ONLY_KEY_FRAMES_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_stream_STREAM_ONLY_KEY_FRAMES_write(struct file *file, const char __user *buf, unsigned long count, void *data);
extern int proc_stream_STREAM_SINGLE_GROUP_BETWEEN_DISCONTINUITIES_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_stream_STREAM_SINGLE_GROUP_BETWEEN_DISCONTINUITIES_write(struct file *file, const char __user *buf, unsigned long count, void *data);
extern int proc_stream_PLAYOUT_ON_TERMINATE_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_stream_PLAYOUT_ON_TERMINATE_write(struct file *file, const char __user *buf, unsigned long count, void *data);
extern int proc_stream_PLAYOUT_ON_SWITCH_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_stream_PLAYOUT_ON_SWITCH_write(struct file *file, const char __user *buf, unsigned long count, void *data);
extern int proc_stream_PLAYOUT_ON_DRAIN_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_stream_PLAYOUT_ON_DRAIN_write(struct file *file, const char __user *buf, unsigned long count, void *data);
extern int proc_stream_TRICK_MODE_DOMAIN_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_stream_TRICK_MODE_DOMAIN_write(struct file *file, const char __user *buf, unsigned long count, void *data);
extern int proc_stream_DISCARD_LATE_FRAMES_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_stream_DISCARD_LATE_FRAMES_write(struct file *file, const char __user *buf, unsigned long count, void *data);
extern int proc_stream_REBASE_ON_DATA_DELIVERY_LATE_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_stream_REBASE_ON_DATA_DELIVERY_LATE_write(struct file *file, const char __user *buf, unsigned long count, void *data);
extern int proc_stream_REBASE_ON_FRAME_DECODE_LATE_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_stream_REBASE_ON_FRAME_DECODE_LATE_write(struct file *file, const char __user *buf, unsigned long count, void *data);
extern int proc_stream_LOWER_CODEC_DECODE_LIMITS_ON_FRAME_DECODE_LATE_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_stream_LOWER_CODEC_DECODE_LIMITS_ON_FRAME_DECODE_LATE_write(struct file *file, const char __user *buf, unsigned long count, void *data);
extern int proc_stream_H264_ALLOW_NON_IDR_RESYNCHRONIZATION_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_stream_H264_ALLOW_NON_IDR_RESYNCHRONIZATION_write(struct file *file, const char __user *buf, unsigned long count, void *data);
extern int proc_stream_MPEG2_IGNORE_PROGESSIVE_FRAME_FLAG_read(char *page, char **start, off_t off, int count, int *eof, void *data_unused);
extern int proc_stream_MPEG2_IGNORE_PROGESSIVE_FRAME_FLAG_write(struct file *file, const char __user *buf, unsigned long count, void *data);

struct e2_procs
{
	char *name;
	read_proc_t *read_proc;
	write_proc_t *write_proc;
	int context;
} e2_procs[] =
{
	{"progress", proc_progress_read, proc_progress_write, 0},

	{"bus/nim_sockets", proc_bus_nim_sockets_read, NULL, 0},
	{"stb/audio/ac3", proc_audio_ac3_read, proc_audio_ac3_write, 0},
	//{"stb/audio/audio_delay_pcm", proc_audio_delay_pcm_read, proc_audio_delay_pcm_write, 0},
	{"stb/audio/audio_delay_pcm", proc_audio_delay_bitstream_read, proc_audio_delay_bitstream_write, 0},
	{"stb/audio/audio_delay_bitstream", proc_audio_delay_bitstream_read, proc_audio_delay_bitstream_write, 0},
	{"stb/audio/j1_mute", proc_audio_j1_mute_read, proc_audio_j1_mute_write, 0},
	{"stb/audio/ac3_choices", proc_audio_ac3_choices_read, NULL, 0},

	{"stb/video/alpha", proc_video_alpha_read, proc_video_alpha_write, 0},
	{"stb/video/aspect", proc_video_aspect_read, proc_video_aspect_write, 0},
	{"stb/video/aspect_choices", proc_video_aspect_choices_read, NULL, 0},
	/*
	 {"stb/video/force_dvi", NULL, NULL, 0},
	*/
	{"stb/video/hdmi_colorspace", proc_video_hdmi_colorspace_read, proc_video_hdmi_colorspace_write, 0},
	{"stb/video/hdmi_colorspace_choices", proc_video_hdmi_colorspace_choices_read, NULL, 0},
	{"stb/video/policy", proc_video_policy_read, proc_video_policy_write, 0},
	{"stb/video/policy_choices", proc_video_policy_choices_read, NULL, 0},
	{"stb/video/videomode", proc_video_videomode_read, proc_video_videomode_write, 0},
	{"stb/video/videomode_50hz", proc_video_videomode_read, proc_video_videomode_write, 0},
	{"stb/video/videomode_60hz", proc_video_videomode_read, proc_video_videomode_write, 0},
	{"stb/video/videomode_choices", proc_video_videomode_choices_read, NULL, 0},
	{"stb/video/videomode_preferred", proc_video_videomode_preferred_read, proc_video_videomode_preferred_write, 0},
	{"stb/video/pal_v_start", proc_video_pal_v_start_read, proc_video_pal_v_start_write, 0},
	{"stb/video/pal_v_end", proc_video_pal_v_end_read, proc_video_pal_v_end_write, 0},
	{"stb/video/pal_h_start", proc_video_pal_h_start_read, proc_video_pal_h_start_write, 0},
	{"stb/video/pal_h_end", proc_video_pal_h_end_read, proc_video_pal_h_end_write, 0},

#if defined(ADB_BOX)
	{"stb/video/switch_type", NULL, proc_video_switch_type_write, 0},
	{"stb/video/switch", proc_video_switch_read, proc_video_switch_write, 0},
	{"stb/video/switch_choices", proc_video_switch_choices_read, NULL, 0},
#endif

	{"stb/avs/0/colorformat", proc_avs_0_colorformat_read, proc_avs_0_colorformat_write, 0},
	{"stb/avs/0/colorformat_choices", proc_avs_0_colorformat_choices_read, NULL, 0},
	{"stb/avs/0/fb", proc_avs_0_fb_read, proc_avs_0_fb_write, 0},
	{"stb/avs/0/input", proc_avs_0_input_read, proc_avs_0_input_write, 0},
	{"stb/avs/0/sb", proc_avs_0_sb_read, proc_avs_0_sb_write, 0},
	{"stb/avs/0/volume", proc_avs_0_volume_read, proc_avs_0_volume_write, 0},
	{"stb/avs/0/input_choices", proc_avs_0_input_choices_read, NULL, 0},
	{"stb/avs/0/standby", proc_avs_0_standby_read, proc_avs_0_standby_write, 0},

	{"stb/denc/0/wss", proc_denc_0_wss_read, proc_denc_0_wss_write, 0},

	{"stb/fp/was_timer_wakeup", proc_fp_was_timer_wakeup_read, proc_fp_was_timer_wakeup_write, 0},
#if defined(IPBOX9900) || defined(IPBOX99)
	{"stb/fp/wakeup_time", proc_fp_wakeup_time_read, proc_fp_wakeup_time_write, 0},
#endif

	{"stb/tsmux/input0", proc_tsmux_input0_read, proc_tsmux_input0_write, 0},
	{"stb/tsmux/input1", proc_tsmux_input1_read, proc_tsmux_input1_write, 0},
	{"stb/tsmux/ci0_input", proc_tsmux_ci0_input_read, proc_tsmux_ci0_input_write, 0},
	{"stb/tsmux/ci1_input", proc_tsmux_ci1_input_read, proc_tsmux_ci1_input_write, 0},
	{"stb/tsmux/lnb_b_input", proc_tsmux_lnb_b_input_read, proc_tsmux_lnb_b_input_write, 0},
#if defined(IPBOX9900)
	{"stb/misc/12V_output", proc_misc_12V_output_read, proc_misc_12V_output_write, 0},
#endif
	{"stb/vmpeg/0/dst_left", proc_vmpeg_0_dst_left_read, proc_vmpeg_0_dst_left_write, 0},
	{"stb/vmpeg/0/dst_top", proc_vmpeg_0_dst_top_read, proc_vmpeg_0_dst_top_write, 0},
	{"stb/vmpeg/0/dst_width", proc_vmpeg_0_dst_width_read, proc_vmpeg_0_dst_width_write, 0},
	{"stb/vmpeg/0/dst_height", proc_vmpeg_0_dst_height_read, proc_vmpeg_0_dst_height_write, 0},
	{"stb/vmpeg/0/dst_all", NULL, proc_vmpeg_0_dst_all_write, 0},
	{"stb/vmpeg/0/yres", proc_vmpeg_0_yres_read, NULL, 0},
	{"stb/vmpeg/0/xres", proc_vmpeg_0_xres_read, NULL, 0},
	{"stb/vmpeg/0/aspect", proc_vmpeg_0_aspect_read, NULL, 0},
	{"stb/vmpeg/0/framerate", proc_vmpeg_0_framerate_read, NULL, 0},

	{"stb/vmpeg/1/dst_left", proc_vmpeg_0_dst_left_read, proc_vmpeg_0_dst_left_write, 1},
	{"stb/vmpeg/1/dst_top", proc_vmpeg_0_dst_top_read, proc_vmpeg_0_dst_top_write, 1},
	{"stb/vmpeg/1/dst_width", proc_vmpeg_0_dst_width_read, proc_vmpeg_0_dst_width_write, 1},
	{"stb/vmpeg/1/dst_height", proc_vmpeg_0_dst_height_read, proc_vmpeg_0_dst_height_write, 1},
	{"stb/vmpeg/1/dst_all", NULL, proc_vmpeg_0_dst_all_write, 1},
	{"stb/vmpeg/1/yres", proc_vmpeg_0_yres_read, NULL, 1},
	{"stb/vmpeg/1/xres", proc_vmpeg_0_xres_read, NULL, 1},
	{"stb/vmpeg/1/aspect", proc_vmpeg_0_aspect_read, NULL, 1},
	{"stb/vmpeg/1/framerate", proc_vmpeg_0_framerate_read, NULL, 1},

	{"stb/fb/dst_left", proc_vmpeg_0_dst_left_read, proc_vmpeg_0_dst_left_write, 0},
	{"stb/fb/dst_top", proc_vmpeg_0_dst_top_read, proc_vmpeg_0_dst_top_write, 0},
	{"stb/fb/dst_width", proc_vmpeg_0_dst_width_read, proc_vmpeg_0_dst_width_write, 0},
	{"stb/fb/dst_height", proc_vmpeg_0_dst_height_read, proc_vmpeg_0_dst_height_write, 0},

	{"stb/hdmi/bypass_edid_checking", proc_hdmi_edid_handling_read, proc_hdmi_edid_handling_write, 0},
	/*
	 {"stb/hdmi/enable_hdmi_resets", NULL, NULL, 0},
	*/
	{"stb/hdmi/audio_source", proc_hdmi_audio_source_read, proc_hdmi_audio_source_write, 0},
	{"stb/hdmi/audio_source_choices", proc_hdmi_audio_source_choices_read, NULL, 0},

	{"stb/hdmi/output", proc_hdmi_output_read, proc_hdmi_output_write, 0},
	{"stb/hdmi/output_choices", proc_hdmi_output_choices_read, NULL, 0},

	{"stb/stream/policy/AV_SYNC", proc_stream_AV_SYNC_read, proc_stream_AV_SYNC_write, 0},
	{"stb/stream/policy/TRICK_MODE_AUDIO", proc_stream_TRICK_MODE_AUDIO_read, proc_stream_TRICK_MODE_AUDIO_write, 0},
	{"stb/stream/policy/PLAY_24FPS_VIDEO_AT_25FPS", proc_stream_PLAY_24FPS_VIDEO_AT_25FPS_read, proc_stream_PLAY_24FPS_VIDEO_AT_25FPS_write, 0},
	{"stb/stream/policy/MASTER_CLOCK", proc_stream_MASTER_CLOCK_read, proc_stream_MASTER_CLOCK_write, 0},
	{"stb/stream/policy/EXTERNAL_TIME_MAPPING", proc_stream_EXTERNAL_TIME_MAPPING_read, proc_stream_EXTERNAL_TIME_MAPPING_write, 0},
	{"stb/stream/policy/DISPLAY_FIRST_FRAME_EARLY", proc_stream_DISPLAY_FIRST_FRAME_EARLY_read, proc_stream_DISPLAY_FIRST_FRAME_EARLY_write, 0},
	{"stb/stream/policy/STREAM_ONLY_KEY_FRAMES", proc_stream_STREAM_ONLY_KEY_FRAMES_read, proc_stream_STREAM_ONLY_KEY_FRAMES_write, 0},
	{"stb/stream/policy/STREAM_SINGLE_GROUP_BETWEEN_DISCONTINUITIES", proc_stream_STREAM_SINGLE_GROUP_BETWEEN_DISCONTINUITIES_read, proc_stream_STREAM_SINGLE_GROUP_BETWEEN_DISCONTINUITIES_write, 0},
	{"stb/stream/policy/PLAYOUT_ON_TERMINATE", proc_stream_PLAYOUT_ON_TERMINATE_read, proc_stream_PLAYOUT_ON_TERMINATE_write, 0},
	{"stb/stream/policy/PLAYOUT_ON_SWITCH", proc_stream_PLAYOUT_ON_SWITCH_read, proc_stream_PLAYOUT_ON_SWITCH_write, 0},
	{"stb/stream/policy/PLAYOUT_ON_DRAIN", proc_stream_PLAYOUT_ON_DRAIN_read, proc_stream_PLAYOUT_ON_DRAIN_write, 0},
	{"stb/stream/policy/TRICK_MODE_DOMAIN", proc_stream_TRICK_MODE_DOMAIN_read, proc_stream_TRICK_MODE_DOMAIN_write, 0},
	{"stb/stream/policy/DISCARD_LATE_FRAMES", proc_stream_DISCARD_LATE_FRAMES_read, proc_stream_DISCARD_LATE_FRAMES_write, 0},
	{"stb/stream/policy/REBASE_ON_DATA_DELIVERY_LATE", proc_stream_REBASE_ON_DATA_DELIVERY_LATE_read, proc_stream_REBASE_ON_DATA_DELIVERY_LATE_write, 0},
	{"stb/stream/policy/REBASE_ON_FRAME_DECODE_LATE", proc_stream_REBASE_ON_FRAME_DECODE_LATE_read, proc_stream_REBASE_ON_FRAME_DECODE_LATE_write, 0},
	{"stb/stream/policy/LOWER_CODEC_DECODE_LIMITS_ON_FRAME_DECODE_LATE", proc_stream_LOWER_CODEC_DECODE_LIMITS_ON_FRAME_DECODE_LATE_read, proc_stream_LOWER_CODEC_DECODE_LIMITS_ON_FRAME_DECODE_LATE_write, 0},
	{"stb/stream/policy/H264_ALLOW_NON_IDR_RESYNCHRONIZATION", proc_stream_H264_ALLOW_NON_IDR_RESYNCHRONIZATION_read, proc_stream_H264_ALLOW_NON_IDR_RESYNCHRONIZATION_write, 0},
	{"stb/stream/policy/MPEG2_IGNORE_PROGESSIVE_FRAME_FLAG", proc_stream_MPEG2_IGNORE_PROGESSIVE_FRAME_FLAG_read, proc_stream_MPEG2_IGNORE_PROGESSIVE_FRAME_FLAG_write}
};

struct DeviceContext_s *ProcDeviceContext = NULL;

#if defined(IPBOX9900)
#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,23)
#include <linux/stpio.h>
#else
#include <linux/stm/pio.h>
#endif
extern struct stpio_pin *output_pin;

void setup_stpio_pin()
{
	output_pin = stpio_request_pin(3, 5, "12V_output", STPIO_OUT);
	return;
}
#endif

void init_e2_proc(struct DeviceContext_s *DC)
{
#if defined(IPBOX9900)
	setup_stpio_pin();
#endif
	int i;
	for (i = 0; i < sizeof(e2_procs) / sizeof(e2_procs[0]); i++)
	{
		install_e2_procs(e2_procs[i].name, e2_procs[i].read_proc,
						 e2_procs[i].write_proc,
						 &DC->DvbContext->DeviceContext[e2_procs[i].context]);
	}
	/* save players device context */
	ProcDeviceContext = DC;
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
