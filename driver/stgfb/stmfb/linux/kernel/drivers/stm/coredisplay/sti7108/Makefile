ifeq ($(CONFIG_CPU_SUBTYPE_STX7108),y)

include $(STG_TOPDIR)/linux/kernel/drivers/stm/coredisplay/coredisplay.mak

STI7108SRCS = $(STM_HDF_TVOUT) $(STM_HDF_HDMI) $(STM_HDMI_COMMON) $(STM_HDMI_V2_9) $(STM_BDISP) $(STM_DEI)

STI7108SRCS += $(addprefix $(SRC_TOPDIR)/soc/sti7108/,                         \
			sti7108bdisp.cpp                                       \
			sti7108dvo.cpp                                         \
			sti7108hdmi.cpp                                        \
			sti7108mainoutput.cpp                                  \
			sti7108auxoutput.cpp                                   \
			sti7108clkdivider.cpp                                  \
			sti7108device.cpp)


sti7108-display-objs := $(patsubst %.cpp,%.o,$(ALLSRCS) $(STI7108SRCS))
sti7108-display-objs := $(patsubst %.c,%.o,$(sti7108-display-objs))

obj-m += stmcore-display-sti7108.o

stmcore-display-sti7108-objs := $(sti7108-display-objs) sti7108.o

endif

