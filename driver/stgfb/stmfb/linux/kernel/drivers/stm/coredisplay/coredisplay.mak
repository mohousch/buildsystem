EXTRA_CXXFLAGS := -fno-rtti -fno-exceptions

SRC_TOPDIR := ../../../../../..

# Common stmcoredisplay Linux specific module files
CORESOURCEFILES := $(addprefix  ../,					\
		 	devfbOS.c                                       \
			hdmisysfs.c                                    	\
			hdmidev.c                                       \
			hdmiedid.c                                      \
			hdmikthread.c                                   \
			coredisplay.c)

# Base class files and C API implementation
GENINITSRCS := $(addprefix $(SRC_TOPDIR)/Generic/,                             \
			DisplayDevice.cpp                                      \
			DisplayPlane.cpp                                       \
			Output.cpp                                             \
			MetaDataQueue.cpp)

# Classes common to all ST SoCs regardless of the display architecture
STM_COMMON := $(addprefix $(SRC_TOPDIR)/STMCommon/,                            \
			stmmasteroutput.cpp                                    \
			stmawg.cpp                                             \
			stmdenc.cpp                                            \
			stmfsynth.cpp                                          \
			stmvtg.cpp                                             \
			stmblitter.cpp                                         \
			stmteletext.cpp)

STM_HDMI_COMMON := $(addprefix $(SRC_TOPDIR)/STMCommon/,                       \
			stmhdmi.cpp                                            \
			stmiframemanager.cpp)

# Classes required for all SoCs containing Gamma based hardware video
# composition
STM_GAMMA := $(addprefix $(SRC_TOPDIR)/Gamma/,                                 \
			GenericGammaDevice.cpp                                 \
			GammaMixer.cpp                                         \
			GammaCompositorPlane.cpp                               \
			GammaCompositorGDP.cpp                                 \
			GammaCompositorNULL.cpp                                \
			GammaCompositorCursor.cpp                              \
			GammaCompositorVideoPlane.cpp                          \
			GammaVideoPlug.cpp                                     \
			VBIPlane.cpp)

# Classes for SoCs containing the BDispII IP
STM_BDISP := $(addprefix $(SRC_TOPDIR)/STMCommon/, stmbdisp.cpp stmbdispaq.cpp \
			stmbdispoutput.cpp stmvirtplane.cpp )

# Classes for SoCs containing the HDFormatter/TVOut style video output stage.
STM_HDF_TVOUT := $(addprefix $(SRC_TOPDIR)/STMCommon/,                         \
			stmsdvtg.cpp                                           \
			stmfdvo.cpp)

STM_HDF_TVOUT += $(addprefix $(SRC_TOPDIR)/HDTVOutFormatter/,                  \
			stmhdfoutput.cpp                                       \
			stmauxtvoutput.cpp                                     \
			stmhdfawg.cpp                                          \
			stmhdfdvo.cpp                                          \
			stmtvoutdenc.cpp                                       \
			stmtvoutteletext.cpp)

STM_HDF_HDMI := $(addprefix $(SRC_TOPDIR)/HDTVOutFormatter/, stmhdfhdmi.cpp)

STM_DEI += $(addprefix $(SRC_TOPDIR)/Gamma/,                                   \
			DEIVideoPipe.cpp                                       \
			VDPFilter.cpp)

STM_DEI_IQI := $(STM_DEI)
STM_DEI_IQI += $(addprefix $(SRC_TOPDIR)/STMCommon/, stmiqi.cpp)
STM_DEI_IQI += $(addprefix $(SRC_TOPDIR)/Gamma/, DEIVideoPipeV2.cpp)

ifeq ($(CONFIG_STM_FMDSW),y)
FMDSW_SRC := $(SRC_TOPDIR)/STMCommon/fmdsw.cpp
endif

STM_HDMI_DMA_IFRAME := $(addprefix $(SRC_TOPDIR)/STMCommon/, stmdmaiframes.cpp)
STM_HDMI_V2_9 := $(addprefix $(SRC_TOPDIR)/STMCommon/, stmv29iframes.cpp)

ALLSRCS := $(CORESOURCEFILES) $(FMDSW_SRC) $(GENINITSRCS) $(STM_COMMON) $(STM_GAMMA)

BUILD_SYSTEM_INFO = $(shell /bin/uname -a)
BUILD_USER = $(shell /usr/bin/whoami)
BUILD_DATE = $(shell /bin/date)
STMFB_ORIGINAL_SOURCE_PATH ?= <unknown - are you using a foreign build system?>

# Add build information defines for just the coredisplay object which will
# appear in sysfs. Because of the time information this file will rebuild
# every time.
CFLAGS_coredisplay.o := -DKBUILD_SYSTEM_INFO="KBUILD_STR($(BUILD_SYSTEM_INFO))"     \
                        -DKBUILD_USER="KBUILD_STR($(BUILD_USER))"                   \
                        -DKBUILD_SOURCE="KBUILD_STR($(STMFB_ORIGINAL_SOURCE_PATH))" \
                        -DKBUILD_DATE="KBUILD_STR($(BUILD_DATE))"


# C++ build magic
EXTRA_CFLAGS += -DINSERT_EXTRA_CXXFLAGS_HERE
mould_cxx_cflags = $(subst -ffreestanding,,\
		   $(subst -Wstrict-prototypes,,\
		   $(subst -Wno-pointer-sign,,\
		   $(subst -Wdeclaration-after-statement,,\
		   $(subst -Werror-implicit-function-declaration,,\
		   $(subst -DINSERT_EXTRA_CXXFLAGS_HERE,$(EXTRA_CXXFLAGS),\
		   $(1)))))))


quiet_cmd_cc_o_cpp = CC $(quiet_modtab) $@

cmd_cc_o_cpp = $(call mould_cxx_cflags,$(cmd_cc_o_c))

define rule_cc_o_cpp
	$(call echo-cmd,checksrc) $(cmd_checksrc)                         \
	$(call echo-cmd,cc_o_cpp)                                         \
	$(cmd_cc_o_cpp);                                                  \
	$(cmd_modversions)                                                \
	scripts/basic/fixdep $(depfile) $@ '$(call make-cmd,cc_o_cpp)' > $(@D)/.$(@F).tmp;  \
	rm -f $(depfile);                                                 \
	mv -f $(@D)/.$(@F).tmp $(@D)/.$(@F).cmd
endef

%.o: %.cpp FORCE
	$(call cmd,force_checksrc)
	$(call if_changed_rule,cc_o_cpp)

