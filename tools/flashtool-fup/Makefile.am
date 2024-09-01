ACLOCAL_AMFLAGS = -I m4

bin_PROGRAMS = fup

fup_SOURCES = fup.c crc16.c crc32.cpp
fup_LDADD = -lz

AM_CFLAGS = -std=c99 -DUSE_ZLIB
