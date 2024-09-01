ifeq ($(KERNELRELEASE),)
    DRIVER_TOPDIR:=$(shell pwd)/..
    include $(DRIVER_TOPDIR)/kernel.make
else
    cn_micom-objs += cn_micom_file.o
    cn_micom-objs += cn_micom_asc.o
    cn_micom-objs += cn_micom_main.o

    obj-m += cn_micom.o

    ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
        ccflags-y += $(CCFLAGSY)
    else
        CFLAGS += $(CCFLAGSY)
    endif
endif

