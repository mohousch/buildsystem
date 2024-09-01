CCFLAGSY += -I$(CONFIG_KERNEL_PATH)/components/include
CCFLAGSY += -I$(DRIVER_TOPDIR)/include/player2

# workaround for stlinux24
ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
endif

mpeg2hw-objs:= mpeg2_hard_host_transformer.o mpeg2hard.o

obj-$(CONFIG_STM_UNIFY_MPEG2HW) += mpeg2hw.o
