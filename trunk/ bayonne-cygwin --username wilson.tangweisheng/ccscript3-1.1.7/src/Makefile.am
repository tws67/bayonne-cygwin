# Copyright (C) 1999-2001 Open Source Telecom Corporation.
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
ccxxincludedir=$(includedir)/cc++
ccxxinclude_HEADERS = script3.h
noinst_HEADERS = engine.h

lib_LTLIBRARIES = libccscript3.la 

libccscript3_la_SOURCES = compiler.cpp expression.cpp checks.cpp \
	methods.cpp runtime.cpp script.cpp symbols.cpp command.cpp \
	image.cpp interp.cpp thread.cpp property.cpp binder.cpp \
	ripple.cpp

libccscript3_la_LDFLAGS = $(RELEASE)
