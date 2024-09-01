# The 7105 is almost identical to 7111, the only real difference being an
# added 2nd DVO (not currently supported). However we have to also support
# 7106 here, which has a different HDMI, because it hasn't been given its own
# CPU subtype in KConfig, you can only tell it is a 7106 at runtime.
ifeq ($(CONFIG_CPU_SUBTYPE_STX7105),y)

include $(STG_TOPDIR)/linux/kernel/drivers/stm/coredisplay/coredisplay.mak

STI7111SRCS = $(STM_HDF_TVOUT) $(STM_HDF_HDMI) $(STM_HDMI_COMMON) $(STM_HDMI_DMA_IFRAME) $(STM_BDISP) $(STM_DEI_IQI)

STI7111SRCS += $(addprefix $(SRC_TOPDIR)/Gamma/, GDPBDispOutput.cpp)

STI7111SRCS += $(addprefix $(SRC_TOPDIR)/soc/sti7111/,                         \
			sti7111gdp.cpp                                         \
			sti7111bdisp.cpp                                       \
			sti7111mixer.cpp                                       \
			sti7111mainoutput.cpp                                  \
			sti7111auxoutput.cpp                                   \
			sti7111dvo.cpp                                         \
			sti7111hdmi.cpp                                        \
			sti7111device.cpp)

STI7105SRCS = $(STI7111SRCS)
STI7105SRCS += $(addprefix $(SRC_TOPDIR)/soc/sti7105/,                         \
			sti7105device.cpp                                      \
			sti7105devicecreate.cpp)

STI7106SRCS = $(STI7111SRCS) $(STM_HDMI_V2_9)
STI7106SRCS += $(addprefix $(SRC_TOPDIR)/soc/sti7105/, sti7105device.cpp)
STI7106SRCS += $(addprefix $(SRC_TOPDIR)/soc/sti7106/,                         \
			sti7106hdmi.cpp                                        \
			sti7106device.cpp)

sti7105-display-objs := $(patsubst %.cpp,%.o,$(ALLSRCS) $(STI7105SRCS))
sti7105-display-objs := $(patsubst %.c,%.o,$(sti7105-display-objs))

sti7106-display-objs := $(patsubst %.cpp,%.o,$(ALLSRCS) $(STI7106SRCS))
sti7106-display-objs := $(patsubst %.c,%.o,$(sti7106-display-objs))


obj-m += stmcore-display-sti7105.o
obj-m += stmcore-display-sti7106.o

stmcore-display-sti7105-objs := $(sti7105-display-objs) sti7105.o
stmcore-display-sti7106-objs := $(sti7106-display-objs) sti7106.o

endif
