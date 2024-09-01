# workaround for stlinux24
ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
endif

mmelog-objs += acc_mme_wrapper.o

obj-$(CONFIG_STM_UNIFY_MMELOG) += mmelog.o
