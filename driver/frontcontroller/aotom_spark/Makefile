ifeq ($(KERNELRELEASE),)
DRIVER_TOPDIR:=$(shell pwd)/..
include $(DRIVER_TOPDIR)/kernel.make
else
    aotom-objs += aotom_trace.o
    aotom-objs += aotom_i2csoft.o
    aotom-objs += aotom_vfd.o
    aotom-objs += aotom_main.o

    obj-m += aotom.o
    
ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
else
CFLAGS += $(CCFLAGSY)
endif
endif

