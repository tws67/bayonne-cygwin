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
module_PROGRAMS = en.phrases fr.phrases it.phrases es.phrases de.phrases \
	ru.phrases

de_phrases_SOURCES = de.cpp
de_phrases_LDFLAGS = @COMMON_MODFLAGS@
de_phrases_LDADD = ../../engine/libbayonne.la @COMMON_LIBFLAGS@  

en_phrases_SOURCES = en.cpp
en_phrases_LDFLAGS = @COMMON_MODFLAGS@
en_phrases_LDADD = ../../engine/libbayonne.la @COMMON_LIBFLAGS@

es_phrases_SOURCES = es.cpp
es_phrases_LDFLAGS = @COMMON_MODFLAGS@
es_phrases_LDADD = ../../engine/libbayonne.la @COMMON_LIBFLAGS@
                                                                    
fr_phrases_SOURCES = fr.cpp
fr_phrases_LDFLAGS = @COMMON_MODFLAGS@
fr_phrases_LDADD = ../../engine/libbayonne.la @COMMON_LIBFLAGS@

it_phrases_SOURCES = it.cpp
it_phrases_LDFLAGS = @COMMON_MODFLAGS@
it_phrases_LDADD = ../../engine/libbayonne.la @COMMON_LIBFLAGS@
 
ru_phrases_SOURCES = ru.cpp
ru_phrases_LDFLAGS = @COMMON_MODFLAGS@
ru_phrases_LDADD = ../../engine/libbayonne.la @COMMON_LIBFLAGS@                                                                 
