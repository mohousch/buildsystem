ifeq ($(KERNELRELEASE),)
DRIVER_TOPDIR:=$(shell pwd)/..
include $(DRIVER_TOPDIR)/kernel.make
else
obj-m           += embxshell.o
embxshell-objs  += embx_buffer.o embx_connect.o embx_deinit.o embx_init.o \
                   embx_module.o embx_open.o embx_port.o embx_transport.o \
		   embx_cache.o embx_debug.o embx_handle.o embx_linux.o \
                   embx_object.o embx_sendrec.o embx_support.o embx_tune.o


ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
else
CFLAGS += $(CCFLAGSY)
endif
endif
