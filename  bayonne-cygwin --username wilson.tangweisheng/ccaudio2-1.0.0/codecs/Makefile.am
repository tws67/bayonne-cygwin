# Copyright (C) 1999-2005 Open Source Telecom Corporation.
#  
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without 
# modifications, as long as this notice is preserved.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

AUTOMAKE_OPTIONS = no-dependencies dist-shar
LDADD = ../src/libccaudio2.la

pkglibdir = $(codec_libdir)
noinst_HEADERS = codecs.h

if MODULES
pkglib_LTLIBRARIES = g711.la adpcm.la oki.la

if GSM_CODEC
pkglib_LTLIBRARIES += gsm.la
endif

if SPEEX_CODEC
pkglib_LTLIBRARIES += speex.la
endif

AM_CPPFLAGS = -I../src -I$(top_srcdir)/src -fPIC -DPIC

oki_la_SOURCES = oki.cpp
oki_la_LDFLAGS = -module -avoid-version @MODULE_FLAGS@ $(LDADD) -lm

adpcm_la_SOURCES = adpcm.cpp
adpcm_la_LDFLAGS = -module -avoid-version @MODULE_FLAGS@ $(LDADD) -lm

g711_la_SOURCES = g711.cpp
g711_la_LDFLAGS = -module -avoid-version @MODULE_FLAGS@ $(LDADD)

if GSM_CODEC
gsm_la_SOURCES = gsm.cpp
gsm_la_LDFLAGS = -module -avoid-version @MODULE_FLAGS@ $(LDADD) -lgsm -lm
endif

if SPEEX_CODEC
speex_la_SOURCES = speex.cpp
speex_la_LDFLAGS = -module -avoid-version @MODULE_FLAGS@ $(LDADD) -lspeex -lm
endif

endif


