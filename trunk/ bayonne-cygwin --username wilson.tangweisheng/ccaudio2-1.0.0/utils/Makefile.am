# Copyright (C) 1999-2005 Open Source Telecom Corporation.
# Copyright (C) 2006-2008 David Sugar, Tycho Softworks.
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

MAINTAINERCLEANFILES = Makefile.in Makefile
EXTRA_DIST = tones.conf audiotool.1 tonetool.1

INCLUDES = -I$(top_srcdir)/src
LDADD = ../src/libccaudio2.la $(DYN_LOADER) $(AUDIO_LIBS) -lm

bin_PROGRAMS = audiotool tonetool

noinst_HEADERS = audiotool.h tonetool.h
man1_MANS = audiotool.1 tonetool.1
man_MANS = audiotool.1 tonetool.1

audiotool_SOURCES = audiotool.cpp info.cpp notation.cpp rewrite.cpp \
	chart.cpp build.cpp play.cpp strip.cpp trim.cpp packetdump.cpp size.cpp

tonetool_SOURCES = tonetool.cpp tonedetect.cpp tonelist.cpp tonewrite.cpp
