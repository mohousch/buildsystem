# workaround for stlinux24
ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
endif

buffer-objs := 	buffer_individual_generic.o \
		buffer_manager_generic.o \
		buffer_pool_generic.o

obj-y += $(buffer-objs)

