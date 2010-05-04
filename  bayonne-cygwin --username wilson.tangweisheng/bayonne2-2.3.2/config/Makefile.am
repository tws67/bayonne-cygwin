# Copyright (C) 2005 Open Source Telecom Corporation.
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

AUTOMAKE_OPTIONS = no-dependencies dist-shar
EXTRA_DIST = *.conf bayonne.init* instconfig bayonne.log
RELEASE = -version-info @LT_VERSION@ -release @LT_RELEASE@
 
#ETC = $(DESTDIR)/$(sysconfdir)
#CONF = $(DESTDIR)/$(sysconfdir)/bayonne
#INIT = $(DESTDIR)/$(sysconfdir)/init.d
#LOGD = $(DESTDIR)/$(sysconfdir)/logrotate.d
ETC = $(sysconfdir)
CONF = $(sysconfdir)/bayonne
INIT = $(sysconfdir)/init.d
LOGD = $(sysconfdir)/logrotate.d

all:

install-exec-local:
	/bin/sh instconfig $(ETC) $(CONF) $(INIT) $(LOGD)

