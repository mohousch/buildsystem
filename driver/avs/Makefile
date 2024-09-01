ifeq ($(KERNELRELEASE),)
DRIVER_TOPDIR:=$(shell pwd)/..
include $(DRIVER_TOPDIR)/kernel.make
else

obj-m += avs.o

avs-objs += \
	tools.o \
	ak4705.o \
	ak4708.o \
	avs_core.o \
	stv6412.o \
	cxa2161.o \
	fake_avs.o \
	stv6417.o \
	stv6418.o \
	avs_none.o \
	avs_pio.o \
	vip2_avs.o \
	vip1_avs.o \
	stv6419.o

ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
else
CFLAGS += $(CCFLAGSY)
endif
endif