# Copyright (C) 2005 Open Source Telecom Corp.
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

AUTOMAKE_OPTIONS = no-dependencies dist-shar dist-zip
EXTRA_DIST = reconfig autoconf/* w32/*.dsp w32/*.dsw TODO \
	tests/*.scr tests/*it tests/*.bat tests/*.sh tests/*.pl \
	tests/*.py tests/*.php tests/*.java tests/*.cs FAQ \
	w32/*.sln w32/*.vcproj
  
SUBDIRS = engine server binders modules config scripts utils audio docs
DIST_SUBDIRS = ${SUBDIRS}

update:
	cd engine ; make install
	cd server ; make install
	cd modules ; make install
	cd binders ; make install
	if test -x /sbin/ldconfig ; then /sbin/ldconfig ; fi

java:
	cd scripts ; make java
	cd tests ; javac -classpath ../scripts/bayonne.jar libjava.java

pnet:
	cd scripts ; make pnet
	cd tests ; cscc -L ../scripts -lbayonne.dll -o libpnet.exe libpnet.cs

	

