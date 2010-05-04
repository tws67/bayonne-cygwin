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

INCLUDES = -I$(srcdir)/../../src

moduledir = @script_libdir@
module_PROGRAMS = odbc.dso
noinst_HEADERS = module.h

odbc_dso_SOURCES = binder.cpp checks.cpp database.cpp thread.cpp methods.cpp
odbc_dso_LDFLAGS = @COMMON_MODFLAGS@
odbc_dso_LDADD = @ODBC_LIB@ ../../src/libccscript3.la

noinst_DATA = link

link:	odbc.dso
	rm -rf ../../modules/odbc.dso
	ln -sf "../optional/odbc/odbc.dso" ../../modules/odbc.dso
	if test -d .libs ; then rm -rf ../../modules/.libs/odbc.dso ; ln -sf "../../optional/odbc/.libs/odbc.dso" ../../modules/.libs/odbc.dso ; fi
	touch link

