CCFLAGSY+=-I$(TREE_ROOT)/linux/drivers/sound/pseudocard/

# workaround for stlinux24
ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
endif

manifestor-objs:=       manifestor_base.o \
			manifestor_video.o \
			manifestor_video_stmfb.o \
			manifestor_audio.o \
			manifestor_audio_ksound.o \
			mixer_mme.o \
			pcmplayer_ksound.o \
			manifestor_clone.o

obj-y += $(manifestor-objs)
