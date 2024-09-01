# workaround for stlinux24
ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
endif

strelay-objs += st_relay.o

ifdef CONFIG_RELAY
obj-$(CONFIG_STM_UNIFY_STRELAY) += strelay.o
endif

obj-y+=dummy.o