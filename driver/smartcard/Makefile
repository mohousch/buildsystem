ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
else
CFLAGS += $(CCFLAGSY)
endif

obj-m      := smartcard.o
smartcard-objs := sci_drv.o sci_params.o atr.o

CCFLAGSY	+= -DDEBUG

