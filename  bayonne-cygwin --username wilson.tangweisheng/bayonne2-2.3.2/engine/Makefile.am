# Copyright (C) 2005 Open Source Telecom Corporation.
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

AUTOMAKE_OPTIONS = no-dependencies dist-shar
RELEASE = -version-info @LT_VERSION@ -release @LT_RELEASE@
EXTRA_DIST = engine.h bayonne.h libexec.h libbayonne.pc*

ccxxincludedir=$(includedir)/cc++

if LIBEXEC
ccxxinclude_HEADERS = bayonne.h libexec.h
else
ccxxinclude_HEADERS = bayonne.h
endif

noinst_HEADERS = engine.h

lib_LTLIBRARIES = libbayonne.la 

libbayonne_la_SOURCES = bayonne.cpp session.cpp states.cpp span.cpp \
	driver.cpp msgport.cpp audio.cpp translators.cpp service.cpp \
	libexec.cpp posix.cpp sysexec.cpp binder.cpp config.cpp md5.cpp \
	rpc.cpp keydata.cpp buffer.cpp

libbayonne_la_LDFLAGS = @COMMON_LIBFLAGS@ $(RELEASE)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libbayonne.pc 
