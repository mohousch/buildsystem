pseudocard-objs := pseudo_mixer.o

CCFLAGSY+=-I$(CONFIG_STGFB_PATH)/include

# workaround for stlinux24
ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
endif

obj-$(CONFIG_STM_UNIFY_PSEUDOCARD) := pseudocard.o
