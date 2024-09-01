ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
else
CFLAGS += $(CCFLAGSY)
endif

obj-m := dvb-as102.o

dvb-as102-objs := as102_drv.o as102_fw.o as10x_cmd.o as10x_cmd_stream.o as102_fe.o as102_usb_drv.o as10x_cmd_cfg.o

EXTRA_CFLAGS += -DLINUX -DCONFIG_AS102_USB -Idrivers/media/dvb/dvb-core
