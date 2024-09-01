# 7100, 7109 and 5202 are all configured in the Linux kernel as the 7100.
# The 7100 and 7109 can be distinguished at runtime by their device IDs, however
# the 5202 cannot as it also reports with the same ID as 7100. This is why the
# 5202 build is only built when its ST board is configured. Unfortunately this
# means a customer board config for a 5202 would need to be added to the test
# below.
ifeq ($(CONFIG_CPU_SUBTYPE_STX7100),y)

include $(STG_TOPDIR)/linux/kernel/drivers/stm/coredisplay/coredisplay.mak

STX7100SRCS = $(STM_BDISP) $(STM_HDMI_COMMON) $(STM_HDMI_DMA_IFRAME)

STX7100SRCS += $(addprefix $(SRC_TOPDIR)/STMCommon/, stmdmaiframes.cpp)

STX7100SRCS += $(addprefix $(SRC_TOPDIR)/Gamma/,                               \
			GammaDisplayFilter.cpp                                 \
			DEIVideoPipe.cpp                                       \
			VDPFilter.cpp                                          \
			GammaCompositorDISP.cpp                                \
			GammaBlitter.cpp                                       \
			GDPBDispOutput.cpp)

STX7100SRCS += $(addprefix $(SRC_TOPDIR)/soc/stb7100/,                         \
			stb7100device.cpp                                      \
			stb7100AWGAnalog.cpp                                   \
			stb7100analogueouts.cpp                                \
			stb7100dvo.cpp                                         \
			stb7100hdmi.cpp                                        \
			stb7100denc.cpp                                        \
			stb7100vtg.cpp                                         \
			stb7100mixer.cpp                                       \
			stb710xgdp.cpp                                         \
			stb710xcursor.cpp)

stx7100-display-objs := $(patsubst %.cpp,%.o,$(ALLSRCS) $(STX7100SRCS))
stx7100-display-objs := $(patsubst %.c,%.o,$(stx7100-display-objs))

ifeq ($(CONFIG_SH_ST_MB602),y)
obj-m += stmcore-display-sti5202.o
stmcore-display-sti5202-objs   := $(stx7100-display-objs) sti5202.o
else
obj-m += stmcore-display-stx7100.o
obj-m += stmcore-display-stx7109c2.o
obj-m += stmcore-display-stx7109c3.o

stmcore-display-stx7100-objs   := $(stx7100-display-objs) stx7100.o
stmcore-display-stx7109c2-objs := $(stx7100-display-objs) stx7109c2.o
stmcore-display-stx7109c3-objs := $(stx7100-display-objs) stx7109c3.o
endif

endif
