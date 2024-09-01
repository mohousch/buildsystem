/************************************************************************
Copyright (C) 2005 STMicroelectronics. All Rights Reserved.

This file is part of the Player2 Library.

Player2 is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License version 2 as published by the
Free Software Foundation.

Player2 is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with player2; see the file COPYING. If not, write to the Free Software
Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

The Player2 Library may alternatively be licensed under a proprietary
license from ST.

Source file name : player_module.c
Author : Julian

Implementation of the LinuxDVB interface to the DVB streamer

Date Modification Name
---- ------------ --------
24-Mar-05 Created Julian

************************************************************************/

#include <linux/sched.h>
#include <linux/syscalls.h>
#include <linux/module.h>

#include "report.h"
#include "display.h"
#include "player_module.h"
#include "backend_ops.h"
#include "player_interface_ops.h"
#include "alsa_backend_ops.h"
#include "player_backend.h"

static int __init PlayerLoadModule(void);
static void __exit PlayerUnloadModule(void);

module_init(PlayerLoadModule);
module_exit(PlayerUnloadModule);

MODULE_DESCRIPTION("Player2 backend implementation for STM streaming architecture.");
MODULE_AUTHOR("Julian Wilson");
MODULE_LICENSE("GPL");

#ifdef __TDT__
int noaudiosync = 0;
module_param(noaudiosync, int, 0444);
MODULE_PARM_DESC(noaudiosync, "Workaround, if there are problems with audio dropouts set it to 1.");

int discardlateframe = 2;
module_param(discardlateframe, int, 0444);
MODULE_PARM_DESC(discardlateframe, "Set start value for Discard Late Frame 0=never, 1=aftersync, 2=always");

int useoldaudiofw = 0;
module_param(useoldaudiofw, int, 0444);
MODULE_PARM_DESC(useoldaudiofw, "Set to 1 if you will use old audio firmware (audio.elf)");
#endif

static struct dvb_backend_operations DvbBackendOps =
{
	.owner = THIS_MODULE,

	.playback_create = PlaybackCreate,
	.playback_delete = PlaybackDelete,
	.playback_add_demux = PlaybackAddDemux,
	.playback_remove_demux = PlaybackRemoveDemux,
	.playback_add_stream = PlaybackAddStream,
	.playback_remove_stream = PlaybackRemoveStream,
	.playback_get_speed = PlaybackGetSpeed,
	.playback_set_speed = PlaybackSetSpeed,
	.playback_set_native_playback_time = PlaybackSetNativePlaybackTime,
	.playback_set_option = PlaybackSetOption,
	.playback_get_player_environment = PlaybackGetPlayerEnvironment,
	.playback_set_clock_data_point = PlaybackSetClockDataPoint,

	.demux_inject_data = DemuxInjectData,

	.stream_inject_data = StreamInjectData,
	.stream_inject_data_packet = StreamInjectDataPacket,
	.stream_discontinuity = StreamDiscontinuity,
	.stream_drain = StreamDrain,
	.stream_check_drained = StreamCheckDrained,
	.stream_enable = StreamEnable,
	.stream_set_id = StreamSetId,
	.stream_channel_select = StreamChannelSelect,
	.stream_set_option = StreamSetOption,
	.stream_get_option = StreamGetOption,
	.stream_step = StreamStep,
	.stream_set_alarm = StreamSetAlarm,
	.stream_get_play_info = StreamGetPlayInfo,
	.stream_switch = StreamSwitch,
	.stream_get_decode_buffer = StreamGetDecodeBuffer,
	.stream_return_decode_buffer = StreamReturnDecodeBuffer,
	.stream_set_output_window = StreamSetOutputWindow,
	.stream_get_output_window = StreamGetOutputWindow,
	.stream_set_input_window = StreamSetInputWindow,
	.stream_set_play_interval = StreamSetPlayInterval,
	.stream_get_decode_buffer_pool_status = StreamGetDecodeBufferPoolStatus,
	.stream_get_player_environment = StreamGetPlayerEnvironment,
	.stream_register_event_signal_callback = StreamRegisterEventSignalCallback,

	.display_create = DisplayCreate,
	.display_delete = DisplayDelete,
	.display_synchronize = DisplaySynchronize
#ifdef __TDT__
	, .is_display_created = isDisplayCreated
#endif

};

#ifndef __TDT__
static struct player_interface_operations PlayerInterfaceOps =
{
	.owner = THIS_MODULE,

	.component_get_attribute = ComponentGetAttribute,
	.component_set_attribute = ComponentSetAttribute,
	.player_register_event_signal_callback = PlayerRegisterEventSignalCallback
};
#endif

static struct alsa_backend_operations AlsaBackendOps =
{
	.owner = THIS_MODULE,

	.mixer_get_instance = MixerGetInstance,
	.mixer_set_module_parameters = ComponentSetModuleParameters,

	.mixer_alloc_substream = MixerAllocSubStream,
	.mixer_free_substream = MixerFreeSubStream,
	.mixer_setup_substream = MixerSetupSubStream,
	.mixer_prepare_substream = MixerPrepareSubStream,
	.mixer_start_substream = MixerStartSubStream,
	.mixer_stop_substream = MixerStopSubStream,
};

extern void SysfsInit(void);

#ifndef __TDT__
#define CONFIG_EXPORT_PLAYER_INTERFACE
#endif
static int __init PlayerLoadModule(void)
{
	report_init();
	report_restricted_severity_levels(severity_fatal, severity_interrupt * 10);
	DisplayInit();
	BackendInit();
	register_dvb_backend(MODULE_NAME, &DvbBackendOps);
	register_alsa_backend(MODULE_NAME, &AlsaBackendOps);
#if defined (CONFIG_EXPORT_PLAYER_INTERFACE)
	register_player_interface(MODULE_NAME, &PlayerInterfaceOps);
#endif
	PLAYER_DEBUG("Player2 backend loaded\n");
	return 0;
}

static void __exit PlayerUnloadModule(void)
{
	BackendDelete();
	PLAYER_DEBUG("Player2 backend unloaded\n");
	return;
}

