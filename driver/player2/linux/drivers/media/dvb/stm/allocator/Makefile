stmalloc-objs := allocator.o

ifeq ($(CONFIG_CPU_SUBTYPE_STX7200),y)
CCFLAGSY += -DLMI_BASE_ADDRESS=0x19000000 -DLMI_BLOCK_SIZE=0x03000000
else
ifeq ($(CONFIG_CPU_SUBTYPE_STB7100),y)
CCFLAGSY += -DLMI_BASE_ADDRESS=0x10800000 -DLMI_BLOCK_SIZE=0x03800000
else
endif
endif

# workaround for stlinux24
ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
endif

obj-$(CONFIG_STM_UNIFY_STMALLOC) += stmalloc.o

