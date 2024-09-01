ifeq ($(CONFIG_FB),)
$(warning Framebuffer not configured in kernel, skipping stmfb module)
else
stmfb-objs :=	stmfbops.o \
		stmfbioctl.o \
		stmfbsysfs.o \
		stmfbvar.o \
		stmfboutconfig.o \
		stmfbplatform.o

obj-m += stmfb.o

endif
