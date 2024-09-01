# workaround for stlinux24
ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
endif

obj-$(CONFIG_STM_UNIFY_PLATFORM) += platform.o
obj-$(CONFIG_STM_UNIFY_P2DIV64) += p2div64.o

p2div64-objs:=  divdi3.o \
                udivdi3.o \
                div64.o