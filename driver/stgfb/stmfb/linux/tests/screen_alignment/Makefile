PKG_CONFIG ?= /opt/STM/STLinux-2.4/host/bin/pkg-config
PKG_CONFIG_LIBDIR ?= /opt/STM/STLinux-2.4/devkit/sh4/target/usr/lib/pkgconfig
PKG_CONFIG_SYSROOT_DIR ?= /opt/STM/STLinux-2.4/devkit/sh4/target

INC		= `PKG_CONFIG_LIBDIR=$(PKG_CONFIG_LIBDIR) \
			PKG_CONFIG_SYSROOT_DIR=$(PKG_CONFIG_SYSROOT_DIR) \
			$(PKG_CONFIG) --cflags \
			directfb`
CFLAGS		= -Wall $(INC)
LDFLAGS		= `PKG_CONFIG_LIBDIR=$(PKG_CONFIG_LIBDIR) \
			PKG_CONFIG_SYSROOT_DIR=$(PKG_CONFIG_SYSROOT_DIR) \
			$(PKG_CONFIG) --libs \
			directfb`
CROSS_COMPILE	:= sh4-linux-
AS		= $(CROSS_COMPILE)as
LD		= $(CROSS_COMPILE)ld
CC		= $(CROSS_COMPILE)gcc
CPP		= $(CROSS_COMPILE)gcc
TARGET		= screen_alignment
SRCS		= $(addprefix ./,$(TARGET).c)
OBJS		= $(patsubst %.c,%.o,$(SRCS))

INCLUDES = $(STG_TOPDIR)/linux/kernel/drivers/video/stmfb.h

all: $(TARGET)

$(TARGET): $(OBJS)

dep:

clean :
	find . -name "*.o" -exec rm \{\} \;
	find . -name $(TARGET) -exec rm \{\} \;

$(OBJS): $(INCLUDES)
