# Makefile for NeutrinoNG buildsystem
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
#

SHELL = /bin/bash
UID := $(shell id -u)

ifeq ($(UID), 0)
warn:
	@echo "You are running as root. Do not do this, it is dangerous."
	@echo "Aborting the build. Log in as a regular user and retry."
	@exit 1
endif

LC_ALL := C
LANG := C

export LC_ALL LANG

all: init

# Boxtype
init:
	@echo ""
	@echo "Target receivers:"
	@echo "  Kathrein"
	@echo "    1)  UFS-912"
	@echo ""
	@echo "  Fortis"
	@echo "    10)  Fortis HDbox / FS9000 / FS9200"
	@echo "    11)  Octagon SF1008P / HS9510"
	@echo "    12)  Atevio AV7500 / HS8200"
	@echo "    13)  Hl101"
	@echo ""
	@echo "  AB IPBox/cuberevo/Xsarius"
	@echo "   20)  cuberevo / 9000"
	@echo "   21)  mini / 900HD"
	@echo "   22)  mini2 / 910HD / 3000HD / Xsarius Alpha"
	@echo "   23)  2000HD"
	@echo
	@echo "  Fulan"
	@echo "   30)  Spark"
	@echo "   31)  Spark7162"
	@echo ""
	@echo "  VU Plus"
	@echo "   40)  VU+ Duo"
	@echo "   41)  VU+ Duo2"
	@echo "   42)  VU+ Duo4k"
	@echo "   43)  VU+ Ultimo4k"
	@echo "   44)  VU+ Uno4k"
	@echo "   45)  VU+ Uno4kse"
	@echo "   46)  VU+ Zero4k"
	@echo "   47)  Vu+ Solo4K"
	@echo ""
	@echo "  AX Mutant"
	@echo "   50)  Mut@nt HD51"
	@echo "   51)  Mut@nt HD60"
	@echo ""
	@echo "  Edision"
	@echo "   60)  osnino"
	@echo "   61)  osninoplus" 
	@echo "   62)  osninopro" 
	@echo "   63)  osmio4k"
	@echo "   64)  osmio4kplus"
	@echo "   65)  osmini4k"  
	@echo ""
	@echo "  Giga Blue"
	@echo "   70)  gb800se"
	@echo "   71)  gbue4k"
	@echo -e "\033[01;32m   72)  gbultraue\033[00m"
	@echo ""
	@echo "  WWIO"
	@echo "   80)  WWIO BRE2ZE 4K"
	@echo "   81)  WWIO BRE2ZE T2C"
	@echo ""
	@echo "  Air Digital"
	@echo "   90)  Zgemma h7"
	@echo ""
	@echo "  AXAS"
	@echo "   100)  AXAS E4HD 4K Ultra"
	@echo ""
	@echo "  Dream Media"
	@echo "   110)  dm800se"
	@echo "   111)  dm800sev2"
	@echo "   112)  dm820"
	@echo "   113)  dm900"
	@echo "   114)  dm920"
	@echo "   115)  dm7020hd"
	@echo "   116)  dm7020hdv2"
	@echo "   117)  dm7080"
	@echo "   118)  dm8000"
	@echo ""
	@echo "  Maxytec"
	@echo "   120)  multiboxse"
	@echo ""
	@echo "  Octagon"
	@echo "   130)  sf8008"
	@echo ""
	@echo "  Protek"
	@echo "   140)  protek4k"
	@echo ""
	@echo "  Uclan"
	@echo "   150)  ustym4kpro"
	@echo ""
	@read -p "Select target (1-150)? " BOXTYPE; \
	BOXTYPE=$${BOXTYPE}; \
	case "$$BOXTYPE" in \
		1) BOXTYPE="ufs912";; \
		10) BOXTYPE="fortis_hdbox";; \
		11) BOXTYPE="octagon1008";; \
		12) BOXTYPE="atevio7500";; \
		13) BOXTYPE="hl101";; \
		20) BOXTYPE="cuberevo";; \
		21) BOXTYPE="cuberevo_mini";; \
		22) BOXTYPE="cuberevo_mini2";; \
		23) BOXTYPE="cuberevo_2000hd";; \
		30) BOXTYPE="spark";; \
		31) BOXTYPE="spark7162";; \
		40) BOXTYPE="vuduo";; \
		41) BOXTYPE="vuduo2";; \
		42) BOXTYPE="vuduo4k";; \
		43) BOXTYPE="vuultimo4k";; \
		44) BOXTYPE="vuuno4k";; \
		45) BOXTYPE="vuuno4kse";; \
		46) BOXTYPE="vuzero4k";; \
		47) BOXTYPE="vusolo4k";; \
		50) BOXTYPE="hd51";; \
		51) BOXTYPE="hd60";; \
		60) BOXTYPE="osnino";; \
		61) BOXTYPE="osninoplus";; \
		62) BOXTYPE="osninopro";; \
		63) BOXTYPE="osmio4k";; \
		64) BOXTYPE="osmio4kplus";; \
		65) BOXTYPE="osmini4k";; \
		70) BOXTYPE="gb800se";; \
		71) BOXTYPE="gbue4k";; \
		72) BOXTYPE="gbultraue";; \
		80) BOXTYPE="bre2ze4k";; \
		81) BOXTYPE="bre2zet2c";; \
		90) BOXTYPE="h7";; \
		100) BOXTYPE="e4hdultra";; \
		110) BOXTYPE="dm800se";; \
		111) BOXTYPE="dm800sev2";; \
		112) BOXTYPE="dm820";; \
		113) BOXTYPE="dm900";; \
		114) BOXTYPE="dm920";; \
		115) BOXTYPE="dm7020hd";; \
		116) BOXTYPE="dm7020hdv2";; \
		117) BOXTYPE="dm7080";; \
		118) BOXTYPE="dm8000";; \
		120) BOXTYPE="multiboxse";; \
		130) BOXTYPE="sf8008";; \
		140) BOXTYPE="protek4k";; \
		150) BOXTYPE="ustym4kpro";; \
		*) BOXTYPE="gbultraue";; \
	esac; \
	echo "BOXTYPE?=$$BOXTYPE" > .config
	@echo ""		
# Gstreamer
	@echo -e "\nGstreamer as mediaplayer for neutrino2 (only for mipsel / arm)"
	@echo "   1) yes"
	@echo -e "   \033[01;32m2) no\033[00m"
	@read -p "Select Gstreamer (1-2)?" GSTREAMER; \
	GSTREAMER=$${GSTREAMER}; \
	case "$$GSTREAMER" in \
		1) echo "GSTREAMER=gstreamer" >> .config;; \
		2|*) echo "GSTREAMER=" >> .config;; \
	esac; \
	echo ""
# python
	@echo -e "\npython plugins support in neutrino2 (experimental and only for mipsel / arm)?:"
	@echo "   1)  yes"
	@echo -e "   \033[01;32m2)  no\033[00m"
	@read -p "Select python support (1-2)?" PYTHON; \
	PYTHON=$${PYTHON}; \
	case "$$PYTHON" in \
		1) echo "PYTHON=python" >> .config;; \
		2|*) echo "PYTHON=" >> .config;; \
	esac; \
	echo ""
# GraphLCD
	@echo -e "\nGraphLCD (neutrino2 / neutrino-DDT):"
	@echo -e "   \033[01;32m1)  yes\033[00m"
	@echo "   2) no"
	@read -p "Select  GraphLCD (1-2)?" GRAPHLCD; \
	GRAPHLCD=$${GRAPHLCD}; \
	case "$$GRAPHLCD" in \
		1) echo "GRAPHLCD=graphlcd" >> .config;; \
		2) echo "GRAPHLCD=" >> .config;; \
		*) echo "GRAPHLCD=graphlcd" >> .config;; \
	esac; \
	echo ""
# LCD4Linux
	@echo -e "\nLCD4linux (neutrino-DDT):"
	@echo -e "   \033[01;32m1)  no\033[00m"
	@echo "   2) yes"
	@read -p "Select  LCD4Linux (1-2)?" LCD4LINUX; \
	LCD4LINUX=$${LCD4LINUX}; \
	case "$$LCD4LINUX" in \
		1) echo "LCD4LINUX=" >> .config;; \
		2) echo "LCD4LINUX=lcd4linux" >> .config;; \
		*) echo "LCD4LINUX=" >> .config;; \
	esac; \
	echo ""	
#	
	@echo ""
	@make printenv

init-clean:
	rm -f .config

include make/buildenv.mk

PARALLEL_JOBS := $(shell echo $$((1 + `getconf _NPROCESSORS_ONLN 2>/dev/null || echo 1`)))
override MAKE = make $(if $(findstring j,$(filter-out --%,$(MAKEFLAGS))),,-j$(PARALLEL_JOBS))

#
#  A print out of environment variables
#
# maybe a help about all supported targets would be nice here, too...
#
printenv:
	@echo
	@echo '================================================================================'
	@echo "Build Environment Variables:"
	@echo "PATH             : `type -p fmt>/dev/null&&echo $(PATH)|sed 's/:/ /g' |fmt -65|sed 's/ /:/g; 2,$$s/^/                 : /;'||echo $(PATH)`"
	@echo "ARCHIVE_DIR      : $(ARCHIVE)"
	@echo "BASE_DIR         : $(BASE_DIR)"
	@echo "TOOLS_DIR        : $(TOOLS_DIR)"
ifeq ($(BOXARCH), sh4)
	@echo "DRIVER_DIR       : $(DRIVER_DIR)"
endif
	@echo "IMAGE_DIR        : $(IMAGE_DIR)"
	@echo "PKGS_DIR         : $(PKGS_DIR)"
	@echo "CROSS_DIR        : $(CROSS_DIR)"
	@echo "RELEASE_DIR      : $(RELEASE_DIR)"
	@echo "HOST_DIR         : $(HOST_DIR)"
	@echo "TARGET_DIR       : $(TARGET_DIR)"
	@echo "KERNEL_DIR       : $(KERNEL_DIR)"
	@echo "MAINTAINER       : $(MAINTAINER)"
	@echo "BUILD            : $(BUILD)"
ifeq ($(BOXTYPE),)
	@echo -e "\033[00;31mBOXTYPE		 :specify a valid BOXTYPE please run 'make init' or 'make'\033[0m"
else
	@echo "BOXTYPE          : $(BOXTYPE)"
endif
	@echo "BOXARCH          : $(BOXARCH)"
	@echo "TARGET           : $(TARGET)"
	@echo "GCC              : $(GCC_VER)"
	@echo "KERNEL_VERSION   : $(KERNEL_VER)"
	@echo "GSTREAMER        : $(GSTREAMER)"
	@echo "PYTHON           : $(PYTHON)"
	@echo "GRAPHLCD         : $(GRAPHLCD)"
	@echo "LCD4LINUX        : $(LCD4LINUX)"
	@echo "PARALLEL_JOBS    : $(PARALLEL_JOBS)"
	@echo '================================================================================'
	@make --no-print-directory toolcheck
ifeq ($(MAINTAINER),)
	@echo "##########################################################################"
	@echo "# The MAINTAINER variable is not set. It defaults to your name from the  #"
	@echo "# passwd entry, but this seems to have failed. Please set it in '.config'.#"
	@echo "##########################################################################"
	@echo
endif
	@echo
	@echo -e "\033[01;33mIf you want to create or modify the configuration, run 'make init' or 'make'\033[0m"
	@echo
	@echo "Your next step could be:"
	@echo "  make image-neutrino2"
	@echo ""
	@echo "if you want to build neutrino-DDT image"
	@echo "  make image-neutrino"
	@echo ""
	@echo "for more details:"
	@echo "  make help"

help:
	@echo "target configuration:"
	@echo " make				- setup target configuration"
	@echo " make init			- setup target configuration"
	@echo ""
	@echo "image:"
	@echo " make image-neutrino2		- build neutrino2 image"
	@echo " make image-neutrino		- build neutrino-DDT image"
	@echo ""
	@echo "show board configuration:"
	@echo " make printenv			- show board build configuration"
	@echo ""
	@echo "show all supported boards:"
	@echo " make print-boards		- show all supported boards"
	@echo ""
	@echo "toolchains:"
	@echo " make crosstool			- build cross toolchain"
	@echo " make bootstrap			- prepares for building"
	@echo ""
	@echo "show all build-targets:"
	@echo " make print-targets		- show all available targets"
	@echo ""
	@echo "later, you might find these useful:"
	@echo " make update			- update the buildsystem"
	@echo ""
	@echo "release:"
	@echo " make release-neutrino2		- build neutrino2 release  with full release dir"
	@echo " make release-neutrino		- build neutrino release  with full release dir"
	@echo ""
	@echo "GUI:"
	@echo " make neutrino2			- build neutrino2"
	@echo " make neutrino			- build neutrino"
	@echo ""
	@echo "cleantargets:"
	@echo " make clean			- clears everything except toolchain."
	@echo " make distclean			- clears the whole construction."
	@echo ""
	@echo "feed packages:"
	@echo " make package_name-ipk		- build package."
	@echo " make packages			- build all feed packages."
	@echo " make packges-clean		- clean all packages."
	@echo ""
	@echo "for development:"
	@echo " make release			- build release without any GUI."
	@echo " make image			- build image without any GUI."
	@echo ""

ifeq ($(BOXARCH), sh4)
include make/crosstool-sh4.mk
else
include make/crosstool.mk
endif
include make/bootstrap.mk
include make/contrib-libs.mk
include make/contrib-apps.mk
include make/ffmpeg.mk
include make/gstreamer.mk
include make/root-etc.mk
include make/python.mk
include make/lua.mk
include make/tools.mk
include make/cleantargets.mk
include make/release.mk
include make/neutrino2.mk
include make/neutrino.mk
include make/flashimage.mk
include make/packages.mk

update:
	git stash && git stash show -p > ./pull-stash-NeutrinoNG.patch || true && git pull || true;
	@echo;

# print all present targets...
print-targets:
	@sed -n 's/^\$$.D.\/\(.*\):.*/\1/p' \
	@ls make/*.mk | grep -v make/buildenv.mk | sort -u | fold -s -w 65
		
# print all supported boards ...
print-boards:
	@ls -1C machine | sed 's/.mk//g'
	
# print all builds
print-builds:
	@ls -1C tufsbox

# for local extensions, e.g. special plugins or similar...
# put them into $(BASE_DIR)/local since that is ignored in .gitignore
-include ./Makefile.local

# debug target, if you need that, you know it. If you don't know if you need
# that, you don't need it.
.print-phony:
	@echo $(PHONY)

PHONY += print-targets
PHONY += all printenv .print-phony
PHONY += update
.PHONY: $(PHONY)

# this makes sure we do not build top-level dependencies in parallel
# (which would not be too helpful anyway, running many configure and
# downloads in parallel...), but the sub-targets are still built in
# parallel, which is useful on multi-processor / multi-core machines
.NOTPARALLEL:

