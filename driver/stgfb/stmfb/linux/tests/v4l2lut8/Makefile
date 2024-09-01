PKG_CONFIG ?= /opt/STM/STLinux-2.4/host/bin/pkg-config
PKG_CONFIG_LIBDIR ?= /opt/STM/STLinux-2.4/devkit/sh4/target/usr/lib/pkgconfig
PKG_CONFIG_SYSROOT_DIR ?= /opt/STM/STLinux-2.4/devkit/sh4/target

INC		= -I $(STG_TOPDIR) \
		  `PKG_CONFIG_LIBDIR=$(PKG_CONFIG_LIBDIR) \
			PKG_CONFIG_SYSROOT_DIR=$(PKG_CONFIG_SYSROOT_DIR) \
			$(PKG_CONFIG) --cflags \
			directfb`
CFLAGS		= -Wall $(INC)
LDFLAGS 	= `PKG_CONFIG_LIBDIR=$(PKG_CONFIG_LIBDIR) \
			PKG_CONFIG_SYSROOT_DIR=$(PKG_CONFIG_SYSROOT_DIR) \
			$(PKG_CONFIG) --libs \
			directfb`
CROSS_COMPILE	:= sh4-linux-
AS		= $(CROSS_COMPILE)as
LD		= $(CROSS_COMPILE)ld
CC		= $(CROSS_COMPILE)gcc
CPP		= $(CROSS_COMPILE)gcc
TARGET		= v4l2lut8
TARGET2		= v4l2lut8_2
SRCS		= $(addprefix ./,$(TARGET).c)
SRCS2		= $(addprefix ./,$(TARGET2).c)
OBJS		= $(patsubst %.c,%.o,$(SRCS))
OBJS2		= $(patsubst %.c,%.o,$(SRCS2))

INCLUDES = $(STG_TOPDIR)/linux/kernel/drivers/video/stmfb.h \
           $(STG_TOPDIR)/linux/kernel/drivers/media/video/stmvout.h \
           $(STG_TOPDIR)/linux/tests/v4l2_helper.h

all: $(TARGET) $(TARGET2)

$(TARGET): $(OBJS)
$(TARGET2): $(OBJS2)

dep:

clean :
	find . -name "*.o" -exec rm \{\} \;
	find . -name $(TARGET) -exec rm \{\} \;

$(OBJS): $(INCLUDES)
$(OBJS2): $(INCLUDES)

