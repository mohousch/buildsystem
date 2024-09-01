#
# Makefile for module fan controlling on ufs922
#
ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
else
CFLAGS += $(CCFLAGSY)
endif

obj-m += cooler.o
cooler-objs += adb_box_fan.o
