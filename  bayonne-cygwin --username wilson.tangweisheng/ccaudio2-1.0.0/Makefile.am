# Copyright (C) 2000-2005 Open Source Telecom Corporation.
# Copyright (C) 2006-2008 David Sugar, Tycho Softworks.
#  
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without 
# modifications, as long as this notice is preserved.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

AUTOMAKE_OPTIONS = no-dependencies dist-shar dist-zip
ACLOCAL_AMFLAGS = -I m4
EXTRA_DIST = autogen.sh ccaudio2.spec ccaudio2.spec.in autoconf/* TODO \
	libccaudio2.pc libccaudio2.pc.in ccaudio2.list ccaudio2.list.in COPYING.LESSER

SUBDIRS = src codecs utils doc
DIST_SUBDIRS = m4 src codecs utils doc w32 

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libccaudio2.pc

