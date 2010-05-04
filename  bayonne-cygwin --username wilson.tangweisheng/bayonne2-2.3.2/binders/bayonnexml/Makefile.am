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

moduledir = @module_libdir@
module_PROGRAMS = bayonnexml.bin
noinst_HEADERS = module.h

bayonnexml_bin_SOURCES = binder.cpp property.cpp checks.cpp \
	methods.cpp parse.cpp image.cpp xml.cpp
bayonnexml_bin_LDFLAGS = @COMMON_MODFLAGS@
bayonnexml_bin_LDADD = ../../engine/libbayonne.la @EXTRAS_LIBFLAGS@

# ivrscript1dir = @bayonnefiles@/ivrscript1
# ivrscript1_DATA = README
