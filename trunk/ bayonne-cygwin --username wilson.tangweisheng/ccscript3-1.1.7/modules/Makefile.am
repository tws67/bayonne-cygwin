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
INCLUDES = -I$(srcdir)/../src

moduledir = @script_libdir@
module_PROGRAMS = math.dso time.dso sort.dso string.dso check.dso \
	csv.dso property.dso locker.dso userauth.dso url.dso

string_dso_SOURCES = string.cpp
string_dso_LDFLAGS = ../src/libccscript3.la @COMMON_MODFLAGS@

math_dso_SOURCES = math.cpp
math_dso_LDFLAGS = ../src/libccscript3.la @COMMON_MODFLAGS@
math_dso_LDADD = -lm

time_dso_SOURCES = time.cpp
time_dso_LDFLAGS = ../src/libccscript3.la @COMMON_MODFLAGS@

sort_dso_SOURCES = sort.cpp
sort_dso_LDFLAGS = ../src/libccscript3.la @COMMON_MODFLAGS@

check_dso_SOURCES = check.cpp
check_dso_LDFLAGS = ../src/libccscript3.la @COMMON_MODFLAGS@
                       
csv_dso_SOURCES = csv.cpp
csv_dso_LDFLAGS = ../src/libccscript3.la @COMMON_MODFLAGS@

property_dso_SOURCES = property.cpp
property_dso_LDFLAGS = ../src/libccscript3.la @COMMON_MODFLAGS@

locker_dso_SOURCES = locker.cpp
locker_dso_LDFLAGS = ../src/libccscript3.la @COMMON_MODFLAGS@

url_dso_SOURCES = url.cpp
url_dso_LDFLAGS = ../src/libccscript3.la @COMMON_MODFLAGS@

userauth_dso_SOURCES = userauth.cpp
userauth_dso_LDFLAGS = ../src/libccscript3.la @COMMON_MODFLAGS@ 
userauth_dso_LDADD = @CRYPT_LIBS@


