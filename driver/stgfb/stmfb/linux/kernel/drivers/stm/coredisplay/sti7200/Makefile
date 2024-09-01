ifeq ($(CONFIG_CPU_SUBTYPE_STX7200),y)

include $(STG_TOPDIR)/linux/kernel/drivers/stm/coredisplay/coredisplay.mak

STI7200SRCS = $(STM_HDF_TVOUT) $(STM_HDF_HDMI) $(STM_HDMI_COMMON) $(STM_HDMI_DMA_IFRAME) $(STM_BDISP) $(STM_DEI_IQI)

STI7200SRCS += $(addprefix $(SRC_TOPDIR)/Gamma/, GDPBDispOutput.cpp)

STI7200SRCS += $(addprefix $(SRC_TOPDIR)/soc/sti7200/,                         \
			sti7200device.cpp                                      \
			sti7200remoteoutput.cpp                                \
			sti7200hdmi.cpp                                        \
			sti7200denc.cpp                                        \
			sti7200gdp.cpp                                         \
			sti7200mixer.cpp                                       \
			sti7200hdfdvo.cpp                                      \
			sti7200flexvp.cpp                                      \
			sti7200xvp.cpp)

STI7200C2SRCS = $(addprefix $(SRC_TOPDIR)/soc/sti7200/,                        \
			sti7200cut2devicefactory.cpp                           \
			sti7200cut2remotedevice.cpp                            \
			sti7200cut2bdisp.cpp                                   \
			sti7200cut2localdevice.cpp                             \
			sti7200cut2localauxoutput.cpp                          \
			sti7200cut2localmainoutput.cpp)

sti7200-display-objs := $(patsubst %.cpp,%.o,$(ALLSRCS) $(STI7200SRCS))
sti7200-display-objs := $(patsubst %.c,%.o,$(sti7200-display-objs))

sti7200c2-display-objs := $(patsubst %.cpp,%.o,$(STI7200C2SRCS))
sti7200c2-display-objs := $(patsubst %.c,%.o,$(sti7200c2-display-objs))

obj-m += stmcore-display-sti7200.o

stmcore-display-sti7200-objs := $(sti7200-display-objs) $(sti7200c2-display-objs) sti7200.o

endif
