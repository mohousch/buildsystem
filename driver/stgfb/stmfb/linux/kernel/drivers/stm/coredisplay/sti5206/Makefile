ifeq ($(CONFIG_CPU_SUBTYPE_STX5206),y)

include $(STG_TOPDIR)/linux/kernel/drivers/stm/coredisplay/coredisplay.mak

STI5206SRCS = $(STM_HDF_TVOUT) $(STM_BDISP) $(STM_DEI_IQI) 

STI5206SRCS += $(addprefix $(SRC_TOPDIR)/soc/sti5206/,                         \
			sti5206bdisp.cpp                                       \
			sti5206dvo.cpp                                         \
			sti5206mainoutput.cpp                                  \
			sti5206auxoutput.cpp                                   \
			sti5206clkdivider.cpp                                  \
			sti5206videopipes.cpp                                  \
			sti5206device.cpp)


sti5206-display-objs := $(patsubst %.cpp,%.o,$(ALLSRCS) $(STI5206SRCS))
sti5206-display-objs := $(patsubst %.c,%.o,$(sti5206-display-objs))

obj-m += stmcore-display-sti5206.o

stmcore-display-sti5206-objs := $(sti5206-display-objs) sti5206.o

endif
