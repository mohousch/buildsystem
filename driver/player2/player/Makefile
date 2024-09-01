# Objects we will link into busybox / subdirs we need to visit

CFLAGS+=-g -DREPORT
LDFLAGS+=-g

NDIR?=.

CCFLAGSY += -I$(NDIR)/user_mode_non_player_bits/includes
##CFLAGS += -I/opt/STM/STLinux-2.2/devkit/sh4/target/usr/include/
##CFLAGS += -I/usr/include
CCFLAGSY += -I$(NDIR)/../components/include

CCFLAGSY += \
	-I$(NDIR)/class_definitions        \
	-I$(NDIR)/buffer                   \
	-I$(NDIR)/demultiplexor            \
	-I$(NDIR)/collator                 \
	-I$(NDIR)/frame_parser             \
	-I$(NDIR)/codec                    \
	-I$(NDIR)/standards                \
	-I$(NDIR)/player                   \
	-I$(NDIR)/manifestor               \
	-I$(NDIR)/output_timer             \
	-I$(NDIR)/output_coordinator

CCFLAGSY += -I$(ROOT_DIR)/linux/drivers/media/dvb/stm/h264_preprocessor/
CCFLAGSY += -I$(ROOT_DIR)/linux/drivers/sound/ksound
CCFLAGSY += -I$(ROOT_DIR)/linux/drivers/media/dvb/stm/dvb
CCFLAGSY += -I$(ROOT_DIR)/linux/include

CCFLAGSY += -I$(ROOT_DIR)/virtualisation
CCFLAGSY += -I$(ROOT_DIR)/virtualisation/linux
CCFLAGSY += -I$(ROOT_DIR)/virtualisation/linux/linuxdvb

CCFLAGSY+=-I$(ROOT_DIR)/virtualisation/mpeg2/

# workaround for stlinux24
ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
endif

player-libs += buffer/
player-libs += demultiplexor/
player-libs += collator/
player-libs += frame_parser/
player-libs += codec/
player-libs += player/
player-libs += output_timer/
player-libs += output_coordinator/
player-libs += user_mode_non_player_bits/infrastructure/

player-libs += manifestor/
player-libs += ../virtualisation/

##player-libs += back.o
##player-libs += components/infrastructure/

##obj-y +=$(player-libs)

##applications-cxx+=player2

libraries+=player2.so

player2.so-objs:= $(player-libs) virt.o
##player2-objs:=$(player-libs)


LDFLAGS_player2.so=-lpthread -ldirectfb -lc -lm

##player2-objs:=tmp.o
