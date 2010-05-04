# Copyright (C) 2005 David Sugar, Tycho Softworks
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

AUTOMAKE_OPTIONS = no-dependencies dist-shar
RELEASE = -version-info @LT_VERSION@ -release @LT_RELEASE@
EXTRA_DIST = *.scr *.pm *.py *.php *.java *.cs *.sh *.mac *.def 

tgidir=@tgi_libdir@
tgi_DATA = *.pm *.py *.php *.sh 

if MONO
monodir=$(libdir)/mono/gnu-telephony
mono_DATA = bayonne.dll

bayonne.dll:	libexec.cs
	mcs -out:bayonne.dll -t:library libexec.cs
endif

if JAVA
moduledir=@module_libdir@
javadir=$(datadir)/java
java_DATA = bayonne.jar
module_DATA = bayonne.jar

bayonne.jar:	Libexec.java
	$(JAVA_COMPILER) Libexec.java
	-mkdir -p org/gnutelephony/bayonne
	mv -f Libexec.class org/gnutelephony/bayonne
	$(JAR) cf bayonne.jar org
	rm -rf org
endif

scriptdir=@scriptfiles@
script_DATA = *.scr

macrodir=@macrofiles@
macro_DATA = *.mac *.def

