#
# Makefile for Topfield frontpanel drivers.
#
ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
else
CFLAGS += $(CCFLAGSY)
endif

obj-m 		+= tffp.o
tffp-objs	+= frontpanel.o tffp_procfs.o tffp_config.o
