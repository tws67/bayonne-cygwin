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

AUTOMAKE_OPTIONS = no-dependencies dist-shar
KDOC_DIR = @KDOC_DIR@
DYN_LOADER = @DYN_LOADER@
RELEASE = -version-info @LT_VERSION@ -release @LT_RELEASE@
ccxxincludedir=$(includedir)/cc++
ccxxinclude_HEADERS = audio2.h

lib_LTLIBRARIES = libccaudio2.la

libccaudio2_la_SOURCES = audiofile.cpp friends.cpp codec.cpp tone.cpp \
	fileio.cpp buffer.cpp stream.cpp oss.cpp w32.cpp osx.cpp \
	mapper.cpp dialers.cpp teltones.cpp detect.cpp audiobase.cpp \
	resample.cpp

libccaudio2_la_LDFLAGS = $(DYN_LOADER) $(AUDIO_LIBS) -lm $(RELEASE)

noinst_DATA = @STAGE2@

kdoc_headers = audio2.h

kdoc:	
	kdoc -f html -d $(KDOC_DIR) -L $(KDOC_DIR) -n ostaudio  $(kdoc_headers)

view:
	lynx ../doc/index.html

macosx: libccaudio2.la
	rm -rf .libs/*.dylib
	@LD@ -r -o ccaudio2.lo $(libccaudio2_la_OBJECTS)
	../libtool --mode=link c++ -dynamic -dynamiclib -o libccaudio2.la \
		-rpath $(libdir) ccaudio2.lo $(LDFLAGS) -lpthread
	touch macosx
                                                                                

