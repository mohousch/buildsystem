TREE_ROOT?=$(DRIVER_TOPDIR)/player2
CONFIG_STGFB_PATH?=$(DRIVER_TOPDIR)/stgfb/stmfb

CCFLAGSY += -I$(DRIVER_TOPDIR)/include
CCFLAGSY += -I$(DRIVER_TOPDIR)/include/player2
CCFLAGSY += -I$(TREE_ROOT)/linux/drivers/media/video/stm
CCFLAGSY += -I$(TREE_ROOT)/linux/drivers/sound/pseudocard
CCFLAGSY += -I$(TREE_ROOT)/linux/drivers/stm/mmelog/
CCFLAGSY += -I$(CONFIG_STGFB_PATH)/include
CCFLAGSY += -I$(CONFIG_STGFB_PATH)/linux/drivers/media/video
CCFLAGSY += -I$(KERNEL_LOCATION)/drivers/media/dvb/dvb-core
CCFLAGSY += -I$(TREE_ROOT)/linux/include/linux/stm
CCFLAGSY += -I$(TREE_ROOT)/linux/include/linux/dvb
CCFLAGSY += -DDEBUG_DVB

# workaround for stlinux24
ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
endif

stmdvb-objs:= \
		backend.o \
		dvb_audio.o \
		dvb_demux.o \
		dvb_dvr.o \
		dvb_module.o \
		dvb_video.o \
		dvb_ca.o \
		dvb_v4l2.o \
		dvb_v4l2_audout.o \
		e2_proc/e2_proc_bus.o \
		e2_proc/e2_proc_misc.o \
		e2_proc/e2_proc_audio.o \
		e2_proc/e2_proc_avs.o \
		e2_proc/e2_proc_denc.o \
		e2_proc/e2_proc_hdmi.o \
		e2_proc/e2_proc_main.o \
		e2_proc/e2_proc_tsmux.o \
		e2_proc/e2_proc_video.o \
		e2_proc/e2_proc_vmpeg.o \
		e2_proc/e2_proc_progress.o \
		e2_proc/e2_proc_stream.o \
		e2_proc/e2_proc_fp.o \
		st-pti.o \
		st-merger.o \
		st-common.o
##		stmdvp.o

ifeq (,$(findstring 2.6.3,$(KERNELVERSION)))
# these object files are required for stm22/stm23 according to BPanther 
stmdvb-objs += \
		dvb_avr.o \
		dvb_avr_audio.o \
		dvb_avr_video.o \
		dvb_cap_video.o \
		dvb_cap.o
endif

obj-$(CONFIG_STM_UNIFY_STMDVB) += stmdvb.o
