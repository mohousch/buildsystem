CCFLAGSY += -Wno-uninitialized -D__TDT__

# workaround for stlinux24
ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
endif

obj-$(CONFIG_STM_UNIFY_KREPLAY) := kreplay.o kreplay-fdma.o

kreplay-fdma-y := ksound-streaming.o ksound-streaming_demo.o
