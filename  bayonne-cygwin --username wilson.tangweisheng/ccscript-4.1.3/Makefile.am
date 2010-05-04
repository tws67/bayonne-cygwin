# Copyright (C) 2008 David Sugar, Tycho Softworks.
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

AUTOMAKE_OPTIONS = no-dependencies dist-shar dist-zip
EXTRA_DIST = autogen.sh README *.pc.in *.spec.in *.spec *-config.in \
	cmodel.sh SUPPORT ccscript-config.1 CMakeLists.txt config.h.cmake

SUBDIRS = src inc test doc

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = ccscript.pc

scriptdir = $(bindir)
script_SCRIPTS = ccscript-config

man_MANS = ccscript-config.1

#doxy:
#	$(DOXYGEN) Doxyfile

docs:
	(cd doc ; make docs)

		
