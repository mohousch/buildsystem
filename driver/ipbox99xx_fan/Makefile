#
# Makefile for module fan controlling on ufs922
#
ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
else
CFLAGS += $(CCFLAGSY)
endif

obj-m 		+= ipbox_fan.o
fan_ctrl-objs	+= fan_ctrl.o
