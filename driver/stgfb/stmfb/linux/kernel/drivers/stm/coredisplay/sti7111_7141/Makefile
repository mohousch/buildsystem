# The display hardware and clocking in 7111 and 7141 is identical enough that
# we can use exactly the same driver objects, we just need a different
# coredriver file to deal with board and HDMI hotplug PIO differences.
ifeq ($(CONFIG_CPU_SUBTYPE_STX7111)$(CONFIG_CPU_SUBTYPE_STX7141),y)

include $(STG_TOPDIR)/linux/kernel/drivers/stm/coredisplay/coredisplay.mak

STI7111SRCS = $(STM_HDF_TVOUT) $(STM_HDF_HDMI) $(STM_HDMI_COMMON) $(STM_HDMI_DMA_IFRAME) $(STM_BDISP) $(STM_DEI_IQI)

STI7111SRCS += $(addprefix $(SRC_TOPDIR)/Gamma/, GDPBDispOutput.cpp)

STI7111SRCS += $(addprefix $(SRC_TOPDIR)/soc/sti7111/,                         \
			sti7111gdp.cpp                                         \
			sti7111bdisp.cpp                                       \
			sti7111mixer.cpp                                       \
			sti7111dvo.cpp                                         \
			sti7111mainoutput.cpp                                  \
			sti7111auxoutput.cpp                                   \
			sti7111hdmi.cpp                                        \
			sti7111device.cpp                                      \
			sti7111devicecreate.cpp)

sti7111-display-objs := $(patsubst %.cpp,%.o,$(ALLSRCS) $(STI7111SRCS))
sti7111-display-objs := $(patsubst %.c,%.o,$(sti7111-display-objs))


ifeq ($(CONFIG_CPU_SUBTYPE_STX7111),y)
obj-m += stmcore-display-sti7111.o
stmcore-display-sti7111-objs := $(sti7111-display-objs) sti7111.o
endif

ifeq ($(CONFIG_CPU_SUBTYPE_STX7141),y)
obj-m += stmcore-display-sti7141.o
stmcore-display-sti7141-objs := $(sti7111-display-objs) sti7141.o
endif

endif
