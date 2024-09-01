CCFLAGSY    += -D__LINUX__ -D__SH4__ -D__KERNEL__ -DMODULE -DEXPORT_SYMTAB -DMULTICOM_GPL -I$(DRIVER_TOPDIR)/include/multicom
CCFLAGSY    += -I$(DRIVER_TOPDIR)/multicom/include/
CCFLAGSY    += -I$(DRIVER_TOPDIR)/multicom/mme/include
CCFLAGSY    += -I$(DRIVER_TOPDIR)/multicom/embx/include
CCFLAGSY    += -fno-common -ffreestanding -fno-omit-frame-pointer -fno-optimize-sibling-calls
CCFLAGSY    += -fno-strict-aliasing -fno-stack-protector
CCFLAGSY    += -Wall
CCFLAGSY    += -Wundef -Wstrict-prototypes -Wno-trigraphs 
CCFLAGSY    += -Wdeclaration-after-statement -Wno-pointer-sign
CCFLAGSY    += -O2

ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
else
CFLAGS += $(CCFLAGSY)
endif

obj-y	+= embxshell/
obj-y	+= embxmailbox/
obj-y	+= embxshm/
obj-y	+= mme/
