TREE_ROOT?=$(DRIVER_TOPDIR)/player2

CCFLAGSY+=-I$(DRIVER_TOPDIR)/stgfb/stmfb/linux/include
CCFLAGSY+=-I$(DRIVER_TOPDIR)/include/player2/
CCFLAGSY+=-I$(TREE_ROOT)/linux/include/linux/stm/
CCFLAGSY+=-I$(TREE_ROOT)/linux/drivers/media/dvb/stm/dvb/
CCFLAGSY+=-I$(TREE_ROOT)/linux/drivers/media/dvb/stm/allocator/
CCFLAGSY+=-I$(TREE_ROOT)/linux/drivers/media/dvb/stm/h264_preprocessor/
CCFLAGSY+=-I$(TREE_ROOT)/linux/drivers/sound/pseudocard/
CCFLAGSY+=-I$(TREE_ROOT)/linux/drivers/stm/mmelog/
CCFLAGSY+=-I$(TREE_ROOT)/linux/drivers/stm/monitor/
CCFLAGSY+=-I$(TREE_ROOT)/linux/drivers/media/sysfs/stm/
CCFLAGSY+=-DREPORT

CCFLAGSY+= -fno-exceptions

## It would be good to make this CXXFLAGS however, this generates some missing cxx symbols???
## Somebody is using c++ in c code.
EXTRA_CXXFLAGS+= -fno-rtti

##A bit of a hack to stop conflicting functions
ifeq ($(CONFIG_STM_UNIFY_PLAYER2),y)
CCFLAGSY+=-DStreamChannelSelect=DEF01
CCFLAGSY+=-DStreamRegisterEventSignalCallback=DEF02
CCFLAGSY+=-DBackendDelete=DEF03
CCFLAGSY+=-DStreamSetPlayInterval=DEF04
CCFLAGSY+=-DPlaybackCreate=DEF05
CCFLAGSY+=-DDisplaySynchronize=DEF06
CCFLAGSY+=-DPlaybackSetSpeed=DEF07
CCFLAGSY+=-DStreamGetPlayInfo=DEF08
CCFLAGSY+=-DStreamGetDecodeBufferPoolStatus=DEF09
CCFLAGSY+=-DPlaybackAddStream=DEF010
CCFLAGSY+=-DStreamSetInputWindow=DEF011
CCFLAGSY+=-DStreamStep=DEF012
CCFLAGSY+=-DPlaybackGetPlayerEnvironment=DEF013
CCFLAGSY+=-DPlaybackSetOption=DEF014
CCFLAGSY+=-DStreamEnable=DEF015
CCFLAGSY+=-DDisplayDelete=DEF016
CCFLAGSY+=-DStreamGetDecodeBuffer=DEF017
CCFLAGSY+=-DStreamGetPlayerEnvironment=DEF018
CCFLAGSY+=-DStreamSetId=DEF019
CCFLAGSY+=-DStreamDiscontinuity=DEF020
CCFLAGSY+=-DStreamReturnDecodeBuffer=DEF021
CCFLAGSY+=-DStreamGetOption=DEF022
CCFLAGSY+=-DStreamSwitch=DEF023
CCFLAGSY+=-DStreamSetOutputWindow=DEF024
CCFLAGSY+=-DStreamDrain=DEF025
CCFLAGSY+=-DBackendInit=DEF026
CCFLAGSY+=-DPlaybackGetSpeed=DEF027
CCFLAGSY+=-DPlaybackSetClockDataPoint=DEF028
CCFLAGSY+=-DPlaybackRemoveStream=DEF029
CCFLAGSY+=-DPlaybackSetNativePlaybackTime=DEF030
CCFLAGSY+=-DStreamSetOption=DEF031
CCFLAGSY+=-DComponentSetAttribute=DEF032
CCFLAGSY+=-DPlayerRegisterEventSignalCallback=DEF033
CCFLAGSY+=-DComponentGetAttribute=DEF034
endif

# workaround for stlinux24
ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
endif

infrastructure-objs := \
			crc32.o \
			report.o \
			ring_generic.o \
			stack_generic.o \
			allocator_simple.o

inline-objs := \
			osinline.o \
			osdev_user.o \
			osdev_device.o \
			sh4cache.o 

backend-objs := \
			player_module.o \
			display.o \
			player_backend.o \
			player_factory.o \
			havana_player.o \
			havana_factory.o \
			havana_playback.o \
			havana_display.o \
			havana_demux.o \
			havana_stream.o

buffer-objs := \
			buffer_individual_generic.o \
			buffer_manager_generic.o \
			buffer_pool_generic.o

player-objs := \
			player_coded_frame.o \
			player_construct.o \
			player_events.o \
			player_extraction.o \
			player_in_sequence.o \
			player_input.o \
			player_playback.o \
			player_process_collate_to_parse.o \
			player_process_decode_to_manifest.o \
			player_process_parse_to_decode.o \
			player_process_post_manifest.o \
			player_settings.o \
			player_statistics.o \
			player_switch.o \
			player_time.o \
			player_tostring.o

demultiplexor-objs := \
			demultiplexor_base.o \
			demultiplexor_ts.o

collator-objs := \
			collator_base.o \
			collator_packet.o \
			collator_packet_dvp.o \
			collator_pes.o \
			collator_pes_video.o \
			collator_pes_video_mpeg2.o \
			collator_pes_video_h264.o \
			collator_pes_video_vc1.o \
			collator_pes_video_mjpeg.o \
			collator_pes_frame.o \
			collator_pes_video_divx.o \
			collator_pes_video_raw.o \
			collator_pes_audio.o \
			collator_pes_audio_aac.o \
			collator_pes_audio_dvd.o \
			collator_pes_audio_mpeg.o \
			collator_pes_audio_eac3.o \
			collator_pes_audio_dtshd.o \
			collator_pes_audio_lpcm.o \
			collator_pes_audio_wma.o \
			collator_pes_audio_mlp.o

collator2-objs := \
			collator2_base.o \
			collator2_pes.o \
			collator2_pes_video.o \
			collator2_pes_video_h264.o

frame_parser-objs := \
			frame_parser_base.o \
			frame_parser_video.o \
			frame_parser_video_mpeg2.o \
			frame_parser_video_h264.o \
			frame_parser_video_vc1.o \
			frame_parser_video_wmv.o \
			frame_parser_video_divx.o \
			frame_parser_video_divx_hd.o \
			frame_parser_video_dvp.o \
			frame_parser_video_h263.o \
			frame_parser_video_flv1.o \
			frame_parser_video_vp6.o \
			frame_parser_video_rmv.o \
			frame_parser_video_theora.o \
			frame_parser_video_mjpeg.o \
			frame_parser_audio.o \
			frame_parser_audio_aac.o \
			frame_parser_audio_mpeg.o \
			frame_parser_audio_eac3.o \
			frame_parser_audio_dtshd.o \
			frame_parser_audio_lpcm.o \
			asf_guids.o \
			wma_properties.o \
			frame_parser_audio_wma.o \
			frame_parser_audio_mlp.o \
			frame_parser_audio_rma.o \
			frame_parser_audio_vorbis.o \
			frame_parser_audio_pcm.o

codec-objs := \
			codec_mme_base.o \
			codec_mme_video.o \
			codec_mme_video_mpeg2.o \
			codec_mme_video_h264.o \
			codec_mme_video_divx.o \
			codec_mme_video_divx_hd.o \
			codec_mme_video_vc1.o \
			codec_mme_video_h263.o \
			codec_mme_video_flv1.o \
			codec_mme_video_vp6.o \
			codec_mme_video_rmv.o \
			codec_mme_video_theora.o \
			codec_mme_video_mjpeg.o \
			codec_dvp_video.o \
			codec_mme_audio.o \
			codec_mme_audio_stream.o \
			codec_mme_audio_aac.o \
			codec_mme_audio_mpeg.o \
			codec_mme_audio_eac3.o \
			codec_mme_audio_dtshd.o \
			codec_mme_audio_lpcm.o \
			codec_mme_audio_wma.o \
			codec_mme_audio_spdifin.o \
			codec_mme_audio_silence.o \
			codec_mme_audio_mlp.o \
			codec_mme_audio_rma.o \
			codec_mme_audio_vorbis.o \
			codec_mme_audio_pcm.o

manifestor-objs := \
			manifestor_base.o \
			manifestor_video.o \
			manifestor_video_stmfb.o \
			manifestor_audio.o \
			manifestor_audio_ksound.o \
			mixer_mme.o \
			pcmplayer_ksound.o \
			manifestor_clone.o

output_coordinator-objs := \
			output_coordinator_base.o

output_timer-objs := \
			output_timer_base.o \
			output_timer_video.o \
			output_timer_audio.o

PLAYER_ROOT=../../../../../../

player2-objs :=         $(backend-objs) \
			$(addprefix $(PLAYER_ROOT)/player/demultiplexor/, $(demultiplexor-objs)) \
			$(addprefix $(PLAYER_ROOT)/player/collator/, $(collator-objs)) \
			$(addprefix $(PLAYER_ROOT)/player/collator2/, $(collator2-objs)) \
			$(addprefix $(PLAYER_ROOT)/player/frame_parser/, $(frame_parser-objs)) \
			$(addprefix $(PLAYER_ROOT)/player/codec/, $(codec-objs)) \
			$(addprefix $(PLAYER_ROOT)/player/output_coordinator/, $(output_coordinator-objs)) \
			$(addprefix $(PLAYER_ROOT)/player/output_timer/, $(output_timer-objs)) \
			$(addprefix $(PLAYER_ROOT)/player/manifestor/, $(manifestor-objs)) \
			$(addprefix $(PLAYER_ROOT)/player/buffer/, $(buffer-objs)) \
			$(addprefix $(PLAYER_ROOT)/player/player/, $(player-objs))  \
			$(addprefix $(PLAYER_ROOT)/components/infrastructure/, $(infrastructure-objs)) \
			$(addprefix $(PLAYER_ROOT)/linux/components/inline/, $(inline-objs))


obj-$(CONFIG_STM_UNIFY_PLAYER2) += player2.o

ifeq ($(CONFIG_DEBUG_INFO_CPP),y)
debug_info_cpp = -O0
endif

# C++ build magic
EXTRA_CFLAGS += -DINSERT_EXTRA_CXXFLAGS_HERE
mould_cxx_cflags = $(subst -ffreestanding,,\
		   $(subst -Wstrict-prototypes,,\
		   $(subst -Wno-pointer-sign,,\
		   $(subst -Wdeclaration-after-statement,,\
		   $(subst -Werror-implicit-function-declaration,,\
		   $(subst  -g , -g $(debug_info_cpp) ,\
		   $(subst -DINSERT_EXTRA_CXXFLAGS_HERE,$(EXTRA_CXXFLAGS),\
		   $(1))))))))


quiet_cmd_cc_o_cpp = CC $(quiet_modtab) $@

cmd_cc_o_cpp = $(call mould_cxx_cflags,$(cmd_cc_o_c))

define rule_cc_o_cpp
	$(call echo-cmd,checksrc) $(cmd_checksrc)                         \
	$(call echo-cmd,cc_o_cpp)                                         \
	$(cmd_cc_o_cpp);                                                  \
	$(cmd_modversions)                                                \
	scripts/basic/fixdep $(depfile) $@ '$(call make-cmd,cc_o_cpp)' > $(@D)/.$(@F).tmp;  \
	rm -f $(depfile);                                                 \
	mv -f $(@D)/.$(@F).tmp $(@D)/.$(@F).cmd
endef

%.o: %.cpp FORCE
	$(call cmd,force_checksrc)
	$(call if_changed_rule,cc_o_cpp)

$(single-used-m): %.o: %.cpp FORCE
	$(call cmd,force_checksrc)
	$(call if_changed_rule,cc_o_cpp)
	@{ echo $(@:.o=.ko); echo $@; } > $(MODVERDIR)/$(@F:.o=.mod)

