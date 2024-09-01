ifeq ($(KERNELRELEASE),)
DRIVER_TOPDIR:=$(shell pwd)/..
include $(DRIVER_TOPDIR)/kernel.make
else
obj-m           += embxshm.o
embxshm-objs    += embxshm_connection.o  embxshm_heap.o  embxshm_localport.o  embxshm_lowlevel.o        embxshm_module.o      embxshm_transport.o \
		   embxshm_death.o       embxshm_init.o  embxshm_locks.o      embxshm_mailboxFactory.o  embxshm_remoteport.o

ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
else
CFLAGS += $(CCFLAGSY)
endif
endif
