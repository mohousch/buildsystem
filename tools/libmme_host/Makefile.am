ACLOCAL_AMFLAGS = -I m4

lib_LTLIBRARIES = libmme_host.la

libmme_host_la_SOURCES = \
	$(DRIVER_TOPDIR)/multicom/mme/mme_linux_user.c \
	$(DRIVER_TOPDIR)/multicom/mme/ExecutionLoop.c \
	$(DRIVER_TOPDIR)/multicom/mme/LocalTransformer.c \
	$(DRIVER_TOPDIR)/multicom/mme/LookupTable.c \
	$(DRIVER_TOPDIR)/multicom/mme/MMEMessageQueue.c \
	$(DRIVER_TOPDIR)/multicom/mme/mme_tune.c \
	$(DRIVER_TOPDIR)/multicom/embxshell/embx_linux.c

AM_CFLAGS = -Wall -D__LINUX__ -I $(DRIVER_TOPDIR)/include/multicom -I$(DRIVER_TOPDIR)/multicom/mme
