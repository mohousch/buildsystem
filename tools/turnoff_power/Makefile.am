AM_CPPFLAGS =  \
	-I$(top_srcdir)/include

noinst_PROGRAMS = turnoff_power

test_SOURCES = \
	turnoff_power.c

install-exec-local:
	install -d $(DESTDIR)/usr/bin
	install -m 0755 turnoff_power $(DESTDIR)/usr/bin/turnoff_power
