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
EXTRA_DIST=ttstool audiosync btsmusic

INCLUDES = -I$(top_srcdir)/engine

bin_PROGRAMS = btsexec btsencode
bin_SCRIPTS = ttstool audiosync btsmusic
noinst_PROGRAMS = testexec

testexec_SOURCES = testexec.c

btsexec_SOURCES = btsexec.c

btsencode_SOURCES = btsencode.c

install-exec-local:
	cd $(DESTDIR)$(bindir) ; ln -sf btsexec btsexec-perl
	cd $(DESTDIR)$(bindir) ; ln -sf btsexec btsexec-python
	cd $(DESTDIR)$(bindir) ; ln -sf btsexec btsexec-php

