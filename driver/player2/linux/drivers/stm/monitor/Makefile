TREE_ROOT?=$(DRIVER_TOPDIR)/player2
CCFLAGSY += -I$(TREE_ROOT)/linux/include/linux/stm/

CONFIG_MONITOR_HAVANA=$(CONFIG_STM_UNIFY_MONITOR_HAVANA)

CCFLAGSY += -DDEBUG_MONITOR

# workaround for stlinux24
ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
endif

obj-$(CONFIG_MONITOR_HAVANA)+= stm_monitor.o

stm_monitor-objs:= \
		monitor_module.o \
		monitor_device.o \
		monitor_mme.o
