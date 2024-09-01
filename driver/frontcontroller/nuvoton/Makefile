ifeq ($(KERNELRELEASE),)
    DRIVER_TOPDIR:=$(shell pwd)/..
    include $(DRIVER_TOPDIR)/kernel.make
else
    nuvoton-objs += nuvoton_file.o
    nuvoton-objs += nuvoton_asc.o
    nuvoton-objs += nuvoton_main.o

    obj-m += nuvoton.o

    ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
        ccflags-y += $(CCFLAGSY)
    else
        CFLAGS += $(CCFLAGSY)
    endif
endif

