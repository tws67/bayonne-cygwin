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
AM_CXXFLAGS = -fPIC -DPIC
AM_CPPFLAGS = @OPENH323_FLAGS@

moduledir = @module_libdir@
module_PROGRAMS = h323.ivr
noinst_HEADERS = driver.h

h323_ivr_SOURCES = driver.cpp session.cpp connection.cpp endpoint.cpp \
	gatekeeper.cpp audio.cpp
h323_ivr_LDFLAGS = @COMMON_MODFLAGS@
h323_ivr_LDADD = ../../engine/libbayonne.la @COMMON_LIBFLAGS@ @OPENH323_LIBS@


