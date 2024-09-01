#
# Makefile for module providing procfs functionality to enigma2
#

CCFLAGSY+= -DPLAYER_191

ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
else
CFLAGS += $(CCFLAGSY)
endif

obj-m 		+= e2_proc.o
e2_proc-objs	+= e2_proc_main.o
