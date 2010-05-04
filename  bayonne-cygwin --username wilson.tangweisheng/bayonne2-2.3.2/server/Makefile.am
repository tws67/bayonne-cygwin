# Copyright (C) 2005 Open Source Telecom Corporation.
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

AUTOMAKE_OPTIONS = no-dependencies dist-shar dist-zip
EXTRA_DIST=bayonne.1 *.bat 

INCLUDES = -I$(top_srcdir)/engine

noinst_HEADERS = server.h
man1_MANS = bayonne.1
man_MANS = bayonne.1

bayonnedir=@module_libdir@
bayonne_PROGRAMS = libexec
sbin_PROGRAMS = bayonne

bayonne_SOURCES = server.cpp runtime.cpp methods.cpp checks.cpp \
	config.cpp property.cpp thread.cpp
bayonne_LDFLAGS = @STREAM_LIBS@
bayonne_LDADD = ../engine/libbayonne.la @COMMON_LIBFLAGS@

libexec_SOURCES = runner.c
libexec_LDFLAGS = @PERL_LDFLAGS@
libexec_CFLAGS = @PERL_CCFLAGS@

