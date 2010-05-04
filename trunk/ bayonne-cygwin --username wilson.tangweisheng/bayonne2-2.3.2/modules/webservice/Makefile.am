# Copyright (C) 2006 David Sugar, Tycho Softworks.
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

AUTOMAKE_OPTIONS = no-dependencies dist-shar dist-zip
AM_CXXFLAGS = -fPIC -DPIC
EXTRA_DIST = *.part *.dtd

moduledir = @module_libdir@
webdir = $(datadir)/bayonne/webserver
module_PROGRAMS = web.mon
noinst_HEADERS = module.h

web_mon_SOURCES = service.cpp tcp.cpp session.cpp html.cpp xmlrpc.cpp \
	event.cpp
web_mon_LDFLAGS = @COMMON_MODFLAGS@
web_mon_LDADD = ../../engine/libbayonne.la @COMMON_LIBFLAGS@

web_DATA = *.part *.dtd
