# workaround for stlinux24
ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
endif

silencegen-objs:= silencegen_module.o silencegen_transformer.o

obj-$(CONFIG_STM_UNIFY_SILENCEGEN) += silencegen.o
