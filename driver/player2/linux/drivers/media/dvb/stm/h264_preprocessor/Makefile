# workaround for stm24
ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
endif

sth264pp-objs := h264pp.o

obj-$(CONFIG_STM_UNIFY_STH264PP) += sth264pp.o

