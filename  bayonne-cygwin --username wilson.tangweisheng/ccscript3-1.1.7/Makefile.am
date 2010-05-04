# Copyright (C) 2004 Open Source Telecom Corporation.
#  
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without 
# modifications, as long as this notice is preserved.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

AUTOMAKE_OPTIONS = no-dependencies dist-shar dist-zip
EXTRA_DIST = autogen.sh autoconf/* libccscript3.pc* \
	w32/*.dsp w32/*.dsw ccscript3.spec* ccscript3.list* \
	w32/*.sln w32/*.vcproj TODO

SUBDIRS = src tests modules optional
DIST_SUBDIRS = src tests modules optional

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libccscript3.pc

