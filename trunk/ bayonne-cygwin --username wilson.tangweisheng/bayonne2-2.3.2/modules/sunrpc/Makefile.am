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
EXTRA_DIST = bayonne_rpc.x bayonne_rpc.h
AM_CXXFLAGS = -fPIC -DPIC 
RPCGEN = rpcgen -C

rpcincludedir = $(includedir)/rpcsvc
moduledir = @module_libdir@
module_PROGRAMS = rpc.mon
sbin_PROGRAMS = bts_uptime bts_reload bts_cancel bts_start

%_rpc.h:        %_rpc.x
	$(RPCGEN) -h $< >$@

%_xdr.c:        %_rpc.x
	$(RPCGEN) -c $< >$@

%_svc.c:        %_rpc.x
	$(RPCGEN) -m $< >$@

%_cl.c: %_rpc.x
	$(RPCGEN) -l $< >$@

lib_LTLIBRARIES = libbayonnerpc.la
libbayonnerpc_la_SOURCES = bayonne_cl.c bayonne_xdr.c

rpc_mon_SOURCES = calls.cpp server.cpp
rpc_mon_LDFLAGS = @COMMON_MODFLAGS@
rpc_mon_LDADD = ../../engine/libbayonne.la @COMMON_LIBFLAGS@ @SUNRPC_LIBS@

bts_uptime_SOURCES = uptime.c
bts_uptime_LDADD = libbayonnerpc.la @SUNRPC_LIBS@

bts_reload_SOURCES = reload.c
bts_reload_LDADD = libbayonnerpc.la @SUNRPC_LIBS@

bts_cancel_SOURCES = cancel.c
bts_cancel_LDADD = libbayonnerpc.la @SUNRPC_LIBS@

bts_start_SOURCES = start.c
bts_start_LDADD = libbayonnerpc.la @SUNRPC_LIBS@

# bayonnerpc_SOURCES = bayonnerpc.cpp
# bayonnerpc_LDFLAGS = libbayonnerpc.la @SUNRPC_LIBS@

noinst_HEADERS = bayonne_svc.c bayonne_xdr.c

rpc:    bayonne_rpc.h bayonne_cl.c bayonne_svc.c bayonne_xdr.c

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(rpcincludedir)
	$(INSTALL_DATA) bayonne_rpc.x $(DESTDIR)$(rpcincludedir)/bayonne.x
	$(INSTALL_DATA) bayonne_rpc.h $(DESTDIR)$(rpcincludedir)/bayonne.h

