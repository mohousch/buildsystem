ifeq ($(KERNELRELEASE),)
DRIVER_TOPDIR:=$(shell pwd)/..
include $(DRIVER_TOPDIR)/kernel.make
else

obj-m           += mme_host.o
mme_host-objs   += ExecutionLoop.o
mme_host-objs   += LookupTable.o
mme_host-objs   += mme_debug.o
mme_host-objs   += mme_linux_module.o
mme_host-objs   += RemoteTransformer.o 
mme_host-objs   += LocalTransformer.o
mme_host-objs   += Manager.o
mme_host-objs   += mme_host_release.o
#mme_host-objs += mme_companion_release.o                
mme_host-objs += MMEMessageQueue.o                
mme_host-objs += mme_tune.o                        

#EXTRA_CFLAGS   := -DDEBUG
EXTRA_CFLAGS    += -D__TDT__ -D__LINUX__ -D__SH4__ -D__KERNEL__ -DMODULE -DEXPORT_SYMTAB -DMULTICOM_GPL -I$(DRIVER_TOPDIR)/include/multicom
EXTRA_CFLAGS    += -fno-common -ffreestanding -fno-omit-frame-pointer -fno-optimize-sibling-calls
EXTRA_CFLAGS    += -fno-strict-aliasing -fno-stack-protector
EXTRA_CFLAGS    += -Wall
EXTRA_CFLAGS    += -Wundef -Wstrict-prototypes -Wno-trigraphs 
EXTRA_CFLAGS    += -Wdeclaration-after-statement -Wno-pointer-sign
EXTRA_CFLAGS    += -O2

endif
