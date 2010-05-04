# Copyright (C) 2005 Open Source Telecom Corp.
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

AC_INIT(reconfig)

VERSION="2.3.2"
LT_RELEASE="2.3"
LT_VERSION="0:2"

cfg_options=""
if test -z "$*" ; then
        if test -f ~/.configure ; then
       		cfg_options=`grep ^bayonne2: ~/.configure | sed -e s/^bayonne2://`
        elif test -f /etc/configure.conf ; then
		cfg_options=`grep ^bayonne2: /etc/configure.conf | sed -e s/^bayonne2://`
        fi
fi

if test ! -z "$cfg_options" ; then
        echo "using ~/.configure defaults...$cfg_options"
        case "$SHELL" in
        */bash*)
                exec $SHELL $0 $cfg_options
                exit -1
                ;;
        esac
        exec $0 $cfg_options
        exit -1
fi

AC_CONFIG_AUX_DIR(autoconf)
AC_CANONICAL_SYSTEM
AC_PROG_CPP
AC_PROG_CXXCPP
AC_PROG_CXX
AM_PROG_LIBTOOL
AM_INIT_AUTOMAKE(bayonne2,[$VERSION])
AM_CONFIG_HEADER(engine/private.h)
AM_MAINTAINER_MODE      

AC_SUBST(LT_VERSION)
AC_SUBST(LT_RELEASE)

if test -z "$PKG_CONFIG_PATH" ; then       
        PKG_CONFIG_PATH="/usr/local/lib/pkgconfig:/usr/lib/pkgconfig"
fi

if test "$prefix" != "NONE" ; then
        PKG_CONFIG_PATH="$prefix/lib/pkgconfig:$PKG_CONFIG_PATH"
fi

export PKG_CONFIG_PATH

SANGOMA_DRIVER=""
SANGOMA_LIBS=""
VPB_DRIVER=""

case $target_os in
*linux*)
        CPPFLAGS="$CPPFLAGS -D__LINUX__"
        ;;
esac

AC_CHECK_HEADERS(sys/mount.h sys/statvfs.h sys/vfs.h)

AC_LANG_SAVE
AC_LANG_CPLUSPLUS

AC_CHECK_HEADER(libsangoma.h,[
	SANGOMA_DRIVER="sangoma"
	SANGOMA_LIBS="-lsangoma -lpri"
])	

AC_CHECK_HEADER(vpbapi.h,[
        VPB_DRIVER="voicetronix"
])
AC_LANG_RESTORE

STREAM_LIBS=""
AC_CHECK_HEADERS(stropts.h,[
	if test -d /usr/src/LiS/libc ; then
		STREAM_LIBS="-L/usr/src/LiS/libc -lLiS" ; fi
	if test -d /usr/src/LiS/include ; then
		CPPFLAGS="$CPPFLAGS -I/usr/src/LiS/include" ; fi
])

SUNRPC_MODULE=""
SUNRPC_LIBS=""

PERL_CCFLAGS=""
PERL_LDFLAGS=""
PYTHON_CCFLAGS=""
PYTHON_LDFLAGS=""

# some systems have broken rpcgen, etc.  Much simpler to disable
# by default and require manual enabling.

AC_ARG_ENABLE(sunrpc, [ --enable-sunrpc for sunrpc module],[

	AC_CHECK_LIB(c, xdr_string, [SUNRPC_MODULE="sunrpc"],[
		AC_CHECK_LIB(nsl, xdr_string, 
			[SUNRPC_LIBS='-lnsl'
			SUNRPC_MODULE="sunrpc"])
	])
])

CAPI20_DRIVER=""
AC_LANG_SAVE
AC_LANG_CPLUSPLUS
AC_CHECK_LIB(capi20, capi20_release, [
        AC_CHECK_HEADERS(capi20.h)
        AC_DEFINE(HAVE_CAPI20_H, [1], [has capi 2.0 support])
	CAPI20_DRIVER="capi20"
        AC_MSG_CHECKING([for capi20 ALERT with sending complete])
        AC_TRY_COMPILE([#include <capiutils.h>],
		[_cmsg cm; ALERT_REQ(&cm, 1, 1, 1, NULL, NULL, NULL, NULL, NULL);],     $
                [AC_DEFINE([HAVE_ALERT_SENDING_COMPLETE],1,
                        [we have SENDING_COMPLETE in ALERT_REQ])
                AC_MSG_RESULT(yes)
                ],
                AC_MSG_RESULT(no)
        )
])
AC_LANG_RESTORE

ZEROCONF_MODULE=""
ZEROCONF_LIBFLAGS=""
if test -z "$ZEROCONF_MODULE" ; then
	PKG_CHECK_MODULES(ZEROCONF, avahi-client > 0.3, [
		AC_DEFINE(ZEROCONF_AVAHI, 1, [zeroconf avahi client])
		ZEROCONF_MODULE="zeroconf"
		ZEROCONF_LIBFLAGS="$ZEROCONF_LIBS"
	],[AC_MSG_RESULT(no avahi)])
fi

AC_SUBST(ZEROCONF_MODULE)
AC_SUBST(ZEROCONF_LIBFLAGS)

SIP_DRIVER=""
AC_CHECK_HEADER(eXosip2/eXosip.h,[
        PKG_CHECK_MODULES(RTPSTACK, libccrtp1 >= 1.5.0,[
                SIP_DRIVER="sip"
     		RTPSTACK_LIBPATH=`pkg-config --variable=libdir libccrtp1`
		RTPSTACK_LIBFLAGS="-losipparser2 -losip2 -L$RTPSTACK_LIBPATH $RTPSTACK_LIBS"
                AC_SUBST(RTPSTACK_LIBFLAGS)
        ])
	AC_EGREP_HEADER(EXOSIP_OPT_DONT_SEND_101, eXosip2/eX_setup.h,[
		AC_DEFINE(EXOSIP2_OPTION_SEND_101, 1, [new option])
	])
	PKG_CHECK_MODULES(libosip2, libosip2 >= 3.0.0, [
		AC_DEFINE(OSIP2_LIST_PTR, [&], [define pointer mode])],[
		echo "<3.0.0"
		AC_DEFINE(OSIP2_LIST_PTR, [])
	])
])

FOX_FLAGS=""
FOX_LIBS=""
AC_PATH_PROG(PHP_PROGRAM, php, /usr/local/bin/php)
AC_PATH_PROG(PNET_PROGRAM, ilrun, /usr/local/bin/ilrun)
AC_PATH_PROG(PERL_PROGRAM, perl, /usr/local/bin/perl)
AC_PATH_PROG(JAVA_PROGRAM, java, /usr/bin/java)
AC_PATH_PROG(PYTHON_PROGRAM, python, /usr/local/bin/python)
AC_PATH_PROG(FOXCONFIG, fox-config, no)
AC_PATH_PROG(JAR, fastjar, jar)
AC_PATH_PROG(JIKES_PROGRAM, jikes, no)
AC_PATH_PROG(JAVAC_PROGRAM, javac, no)
AC_PATH_PROG(MONO_COMPILER, mcs, none)

JAVA_COMPILER="none"
if test "$JAVAC_PROGRAM" != "no" ; then
	JAVA_COMPILER="javac" ; fi

if test "$JIKES_PROGRAM" != "no" -a -d /usr/share/java ; then
	bootclass=`echo /usr/share/java/libgcj*`
	if test -f "$bootclass" ; then
		JAVA_COMPILER="jikes --bootclasspath $bootclass" ; fi
fi

AC_SUBST(JAR)
AC_SUBST(JAVA_COMPILER)
AC_SUBST(MONO_COMPILER)
AM_CONDITIONAL(JAVA, test "$JAVA_COMPILER" != "none")
AM_CONDITIONAL(MONO, test "$MONO_COMPILER" != "none")

if test "$FOXCONFIG" != "no" ; then
	AC_DEFINE(HAVE_FOX, [1], [have fox toolkit for gui])
	FOX_FLAGS=`$FOXCONFIG --cflags`
	FOX_LIBS=`$FOXCONFIG --libs`
fi
AC_SUBST(FOX_FLAGS)
AC_SUBST(FOX_LIBS)

H323_DRIVER=""
OPENH323_FLAGS=""
OPENH323_LIBS=""

AC_ARG_ENABLE(openh323, [ --enable-openh323 for h323 support],
[

AC_ARG_ENABLE(openh323-codecs, ,
	[ AC_DEFINE(H323_CODECS_DISABLED, [1], [some distros do not pre-build openh323 with gsm or speex])
])

AC_PATH_PROG(PTCONFIG, ptlib-config, no)
AC_MSG_CHECKING([for openh323 libraries])
if test -z "$OPENH323DIR" ; then
	if test -d /usr/openh323 ; then
                export OPENH323DIR=/usr/openh323   
        elif test -d /usr/include/openh323 ; then
                export OPENH323DIR=/usr/include/openh323
        elif test -d /usr/local/include/openh323 ; then
                export OPENH323DIR=/usr/local/include/openh323
        else
                export OPENH323DIR=~/openh323
        fi
fi
if test "$PTCONFIG" == "no" ; then
        AC_ARG_WITH(pwlibdir,
                [  --with-pwlibdir=DIR     where openh323 pwlib is],
                [PWLIBDIR="$with_pwlibdir"],
                [if test -d /sw/lib/pwlib ; then
                        PWLIBDIR=/sw/lib/pwlib; fi])

        AC_ARG_WITH(openh323dir,
                [  --with-openh323dir=DIR  where openh323 is],
                [export OPENH323DIR="$with_openh323dir"],
                [if test -d /sw/lib/openh323 ; then
                        OPENH323DIR=/sw/lib/openh323; fi])
fi
if test "$PTCONFIG" != "no" ; then
        ptlib_prefix=`$PTCONFIG --prefix`
	OPENH323_FLAGS="-I$ptlib_prefix/include/ptlib "`$PTCONFIG --ccflags`
	OPENH323_LIBS="-lopenh323 -lpt "`$PTCONFIG --libs`
        H323_DRIVER="h323"
elif test -z "$PWLIBDIR" ; then
        if test -d /usr/share/pwlib ; then
                export PWLIBDIR=~/pwlib
        else
                export PWLIBDIR=~/pwlib
        fi
        if test -d $PWLIBDIR/include ; then
                OPENH323_FLAGS="-I$PWLIBDIR/include"
        elif test -d /usr/include/ptlib ; then
                OPENH323_FLAGS="-I/usr/include/ptlib"
        fi
elif test -d "$PWLIBDIR/include/unix" ; then
        OPENH323_FLAGS="-I$PWLIBDIR/include"
elif test -d "$PWLIBDIR/include/ptlib" ; then
        OPENH323_FLAGS="-I$PWLIBDIR/include"
fi
if test -d "$OPENH323DIR" ; then
		H323_DRIVER="h323"
        if test -d /usr/include/openh323 ; then
        	OPENH323_FLAGS="$OPENH323_FLAGS -I/usr/include/openh323"
        elif test -d $OPENH323DIR/include ; then
        	OPENH323_FLAGS="$OPENH323_FLAGS -I$OPENH323DIR/include"
        else
                OPENH323_FLAGS="$OPENH323_FLAGS -I$OPENH323DIR"
        fi
#       OPENH323_FLAGS="$OPENH323_FLAGS"
        if test -f $OPENH323DIR/lib/libopenh323.so ; then
                libh323="-L$OPENH323DIR/lib -lopenh323"
        elif test -d $OPENH323DIR/lib ; then
                libh323=`ls $OPENH323DIR/lib/*.a`
        else
                if test -f /usr/lib/libopenh323.so ; then
                        libh323="-lopenh323"
                elif test -f /usr/local/lib/libopenh323.so ; then
                        libh323="-lopenh323"
                else
			libh323="-lh323_"`ls /usr/lib/libh323_* | sed -e "s/.*libh323_//" -e "s/[.]so.*$//" | uniq`
                fi
        fi
        if test "$PTCONFIG" != "no" ; then
                OPENH323_LIBS="-lopenh323 -lpt "`$PTCONFIG --libs`
        else
                if test -d $PWLIBDIR/lib/obj* ; then
        		OPENH323_LIBS="$OPENH323_LIBS -L"`echo $PWLIBDIR/lib/obj*`
                fi
                if test -d $PWLIBDIR/lib ; then
                        libpt=`ls $PWLIBDIR/lib/*.a`
                else
                        libpt="-lpt"
                fi
                OPENH323_LIBS="$OPENH323_LIBS $libh323 $libpt"
                if test -f $PWLIBDIR/make/ptlib-config ; then
			OPENH323_FLAGS="$OPENH323_FLAGS "`sh $PWLIBDIR/make/ptlib-config --ccflags`
     			OPENH323_LIBS="$OPENH323_LIBS "`sh $PWLIBDIR/make/ptlib-config --ldflags`" "`sh $PWLIBDIR/make/ptlib-config --libs`
                fi
        fi
fi
if test ! -z "$H323_DRIVER" ; then
        AC_MSG_RESULT(found)
else
        AC_MSG_RESULT(missing)
fi

])

AC_SUBST(OPENH323_FLAGS)
AC_SUBST(OPENH323_LIBS)
AC_SUBST(ACULAB_LDADD)
AC_SUBST(ACULAB_LDFLAGS)
AC_SUBST(ACULAB_FLAGS)

AC_SUBST(SUNRPC_MODULE)
AC_SUBST(H323_DRIVER)
AC_SUBST(CAPI20_DRIVER)
AC_SUBST(SIP_DRIVER)
AC_SUBST(VPB_DRIVER)
AC_SUBST(SANGOMA_DRIVER)
AC_SUBST(SANGOMA_LIBS)
AC_SUBST(STREAM_LIBS)
AC_SUBST(SUNRPC_LIBS)

PKG_CHECK_MODULES(EXTRAS, libccext2 >= 1.5.0 libccscript3 >= 1.1.2 libccaudio2 >= 0.9.5)
PKG_CHECK_MODULES(COMMON, libccscript3 >= 1.1.2 libccaudio2 >= 0.9.5 libccgnu2 >= 1.5.0)
CODECS_LIBPATH=`pkg-config --variable=codecdir libccaudio2`
COMMON_LIBPATH=`pkg-config --variable=libdir libccgnu2`
COMMON_LIBFLAGS="-L$COMMON_LIBPATH $COMMON_LIBS"
COMMON_MODFLAGS=`pkg-config --variable=modflags libccgnu2`
EXTRAS_LIBFLAGS="-L$COMMON_LIBPATH $EXTRAS_LIBS"
CXXFLAGS="$CXXFLAGS $COMMON_CFLAGS"
AC_SUBST(COMMON_MODFLAGS)
AC_SUBST(COMMON_LIBFLAGS)
AC_SUBST(EXTRAS_LIBFLAGS)

gsm_format="gsm"
check=`pkg-config --variable=gsm_format libccaudio2`
if test "$check" = "false" ; then
	gsm_format="" ; fi

adpcm_format=""
check=`pkg-config --variable=adpcm_format libccaudio2`
if test "$check" = "true" ; then
	adpcm_format="adpcm" ; fi

AC_SUBST(gsm_format)
AC_SUBST(adpcm_format)

AC_DEFUN([OST_DEBUG],[
        AC_MSG_CHECKING(for debugging)
        AC_ARG_ENABLE(debug, [  --enable-debug          compile for debugging])
        if test -z "$enable_debug" ; then
                enable_debug="no"
        elif test $enable_debug = "yes" ; then
                CXXFLAGS="${CXXFLAGS} -g -DDEBUG"
#               CXXFLAGS=`echo $CFLAGS | sed 's/-O.//'`
        fi
        AC_MSG_RESULT([$enable_debug])
        AC_MSG_CHECKING(for profiling)
	AC_ARG_ENABLE(profiling, [  --enable-profiling      compile for profiling])
        if test -z "$enable_profiling" ; then
                enable_profiling="no"
        elif test "$enable_profiling" = "yes" ; then
                CXXFLAGS="${CXXFLAGS} -p"
        fi
        AC_MSG_RESULT([$enable_profiling])
])

AC_DEFUN([AC_SUBST_DIR], [
        ifelse($2,,,$1="[$]$2")
        result="***"
        prior="A"
        while test "$prior" != "$result" ; do
                prior=`(echo "[$]$1")`
                $1=`(
                test "x$prefix" = xNONE && prefix="$ac_default_prefix"
                test "x$exec_prefix" = xNONE && exec_prefix="${prefix}"
                eval echo \""[$]$1"\"
                )`
                result=`(echo "[$]$1")`
        done
        AC_SUBST($1)
])

cfgprefix='/etc/sysconfig'
sysconfig='/etc/sysconfig/bayonne'
sysprefix='${sysconfdir}/bayonne'
tgi_libdir='${datadir}/bayonne/libexec'
bayonnefiles='${datadir}/bayonne'
execfiles='${datadir}/bayonne/libexec'
scriptfiles='${datadir}/bayonne/scripts'
sharedfiles='${datadir}/bayonne'
macrofiles='${datadir}/bayonne/macros'
promptfiles='${datadir}/phrasebook'
varfiles='${localstatedir}/lib/bayonne'
logfiles='${localstatedir}/log/bayonne'
runfiles='${localstatedir}/run/bayonne'
module_libdir='${libdir}/bayonne-${LT_RELEASE}'
mono_libdir='${prefix}/lib/mono'
sys_libdir='${prefix}/libexec/bayonne'

if test ! -d /etc/sysconfig ; then
	if test -d /etc/conf.d ; then
		sysconfig="/etc/conf.d/bayonne"
		cfgprefix="/etc/conf.d"
	fi
	if test -d /etc/site-start.d ; then
		sysconfig="/etc/site-start.d/bayonne"
		cfgprefix="/etc/site-start.d"
	fi
	if test -d /etc/defaults ; then
		sysconfig="/etc/defaults/bayonne.conf"
		cfgprefix="/etc/defaults"
	fi
fi 

case ${localstatedir} in
*/bayonne*)
        libfiles='${localstatedir}/lib'
	logfiles='${localstatedir}/log'
	runfiles='${localstatedir}/run'
        ;;
esac

case ${sysconfdir} in
*/bayonne)
	sysprefix=${sysconfdir}
	;;
esac

case "$prefix" in
/opt/*)
        if test "$exec_prefix" == "NONE" -o "$exec_prefix" == '${prefix}'; then
                exec_prefix=${prefix} ; fi

        if test "$localstatedir" == '${prefix}/var' ; then
                localstatedir=/var${prefix}
                varfiles='${localstatedir}/lib'
                logfiles='${localstatedir}/log'
                runfiles='${localstatedir}/run'
	fi

        if test "$datadir" == '${prefix}/share' ; then
                if test "$mandir" == '${datadir}/man' ; then
                        mandir='${prefix}/man' ; fi
                if test "$infodir" == '${datadir}/info' ; then
                        infodir='${prefix}/info' ; fi
                datadir='${prefix}'
        fi
        if test "$sysconfdir" == '${prefix}/etc' ; then
                sysconfdir="/etc$prefix" ; fi

        AC_SUBST(mandir)
        AC_SUBST(infodir)
        AC_SUBST(datadir)
        AC_SUBST(localstatedir)
	AC_SUBST(sysconfdir)
	AC_SUBST(exec_prefix)
        ;;
esac

if test "$localstatedir" == '${prefix}/var' ; then
        localstatedir="/var" ; fi
AC_SUBST(localstatedir)

AC_SUBST(STAGE2)
AC_SUBST(mono_libdir)
AC_SUBST(module_libdir)
AC_SUBST_DIR(module_libpath, module_libdir)
AC_SUBST_DIR(codecs_libdir, CODECS_LIBPATH)


AC_DEFINE_UNQUOTED(MODULE_LIBPATH, "$module_libpath", [define module  path])

SHARED_FLAGS=""
STAGE2=""
case "$target_os" in
cygwin*)
        SHARED_FLAGS="-no-undefined"
        ;;
darwin6*)
        STAGE2="macosx"
        ;;
esac

if test "$sysconfdir" == '${prefix}/etc' ; then
	sysprefix="/etc/bayonne"
	sysconfdir="/etc"
	AC_SUBST(sysconfdir)
fi


VOICES="none"
AC_ARG_WITH(voices,
	[  --with-voices=langs    install libraries for languages],
	[
		case "$with_voices" in
		*en*)
			VOICES="$VOICES en"
			;;
		*fr*)
			VOICES="$VOICES fr"
			;;
		*ru*)
			VOICES="$VOICES ru"
			;;
		esac
	])

AC_MSG_CHECKING(for voice libraries enabled)
AC_ARG_ENABLE(voices, [ --enable-voices for full multi-lingual support],
        VOICES="none en ru fr") 

AC_MSG_RESULT([$VOICES])

AC_SUBST(VOICES)

AC_MSG_CHECKING(for libexec)
AC_ARG_ENABLE(libexec, [  --disable-libexec           remove libexec support])
if test -z "$enable_libexec" ; then
	enable_libexec="yes" ; fi
AC_MSG_RESULT([$enable_libexec])

libexec_runtime="x"	

if test "$enable_libexec" == "yes" ; then
	AC_DEFINE(HAVE_LIBEXEC, [1], [libexec enabled])
#	pltest=`perl -MExtUtils::Embed -e ccopts 2>/dev/null`
#	if test ! -z "$pltest" ; then
#		libexec_runtime="$libexec_runtime perl" ; fi
#	pytest=`${PYTHON_PROGRAM} -c "import sys;print sys.exec_prefix" 2>/dev/null`
#	if test ! -z "$pytest" ; then
#		libexec_runtime="$libexec_runtime python" ; fi
	
fi

libexec_python=false

for libexec_engine in $libexec_runtime ; do
case "$libexec_engine" in
*perl*)
	AC_MSG_NOTICE([enabling perl embedded configuration...])
	perl -MExtUtils::Embed -e xsinit -- -o server/perlxsi.c
	PERL_CCFLAGS="-DLIBEXEC_USES_PERL "`perl -MExtUtils::Embed -e ccopts`
	PERL_LDFLAGS=`perl -MExtUtils::Embed -e ldopts`
	;;
*python*)
	AC_MSG_NOTICE([enabling python embedded configuration...])
	python_prefix=`${PYTHON_PROGRAM} -c "import sys; print sys.prefix"`
	python_execprefix=`${PYTHON_PROGRAM} -c "import sys; print sys.exec_prefix"`
	python_version=`${PYTHON_PROGRAM} -c "import sys;print sys.version"`
	python_include="-I${python_prefix}/include/python${python_version}"
#	if test x"$python_prefix" != x"$python_execprefix" ; then
#		python_include="$python_include -I${python_execprefix}/include/python${python_version}"
#	fi
#	python_shared=`${PYTHON_PROGRAM} -c "import distutils.sysconfig; print distutils.sysconfig.get_config_var('CFLAGSFORSHARED')"`
#	PYTHON_CCFLAGS="${python_shared} ${python_include}"
	libexec_python=true
	;;
esac
done
	
AC_PATH_PROG(LATEX2HTML, latex2html, no)
AC_PATH_PROG(PDFLATEX, pdflatex, no)
AC_PATH_PROG(DOXYGEN, doxygen, no)
AC_SUBST(DOXYGEN)
AM_CONDITIONAL(DOXY, test "$DOXYGEN" != "no")
AM_CONDITIONAL(MAKE_HTML, test "$LATEX2HTML" != "no")
AM_CONDITIONAL(MAKE_PDF, test "$PDFLATEX" != "no")
AM_CONDITIONAL(LIBEXEC, test "$enable_libexec" == "yes")

runner='${module_libdir}/libexec.bin'
AC_SUBST_DIR(config_sbindir, sbindir)
AC_SUBST_DIR(config_libexec, tgi_libdir)
AC_SUBST_DIR(config_libdir, module_libdir)
AC_SUBST_DIR(config_runner, runner)
AC_SUBST_DIR(config_statedir, localstatedir)
AC_SUBST_DIR(config_files, sysprefix)
AC_SUBST_DIR(config_system, sysconfig)
AC_SUBST_DIR(config_scripts, scriptfiles)
AC_SUBST_DIR(config_shared, sharedfiles)
AC_SUBST_DIR(config_bayonne, bayonnefiles)
AC_SUBST_DIR(config_macros, macrofiles)
AC_SUBST_DIR(config_prompts, promptfiles)
AC_SUBST_DIR(config_var, varfiles)
AC_SUBST_DIR(config_run, runfiles)
AC_SUBST_DIR(config_log, logfiles)
AC_SUBST_DIR(config_bin, bindir)

AC_SUBST(sysprefix)
AC_SUBST(cfgprefix)
AC_SUBST(sysconfig)
AC_SUBST(promptfiles)
AC_SUBST(scriptfiles)
AC_SUBST(sharedfiles)
AC_SUBST(execfiles)
AC_SUBST(tgi_libdir)
AC_SUBST(sys_libdir)
AC_SUBST(bayonnefiles)
AC_SUBST(macrofiles)
AC_SUBST(PERL_CCFLAGS)
AC_SUBST(PERL_LDFLAGS)

config_dir=`dirname $0` 
config_srcdir=`cd ${config_dir} ; pwd`

AC_DEFINE_UNQUOTED(JAVA_PROGRAM, "$JAVA_PROGRAM", [java binary])
AC_DEFINE_UNQUOTED(PHP_PROGRAM, "$PHP_PROGRAM", [php binary])
AC_DEFINE_UNQUOTED(PNET_PROGRAM, "$PNET_PROGRAM", [ilrun for pnet])
AC_DEFINE_UNQUOTED(PERL_PROGRAM, "$PERL_PROGRAM", [perl binary])
AC_DEFINE_UNQUOTED(PYTHON_PROGRAM, "$PYTHON_PROGRAM", [python binary])
AC_DEFINE_UNQUOTED(LIBEXEC_PROGRAM, "$config_runner", [libexec runner])
AC_DEFINE_UNQUOTED(LIBEXEC_FILES, "$config_libexec", [libexec path])
AC_DEFINE_UNQUOTED(LIBDIR_FILES, "$config_libdir", [libdir path])
AC_DEFINE_UNQUOTED(CONFIG_FILES, "$config_files", [config files])
AC_DEFINE_UNQUOTED(SCRIPT_FILES, "$config_scripts", [script files])
AC_DEFINE_UNQUOTED(SHARED_FILES, "$config_shared", [shared files])
AC_DEFINE_UNQUOTED(BAYONNE_FILES, "$config_bayonne", [bayonne files])
AC_DEFINE_UNQUOTED(MACRO_FILES, "$config_macros", [macro files])
AC_DEFINE_UNQUOTED(PROMPT_FILES, "$config_prompts", [prompt files])
AC_DEFINE_UNQUOTED(SOURCE_FILES, "$config_srcdir", [source files])
AC_DEFINE_UNQUOTED(VAR_FILES, "$config_var", [var files])
AC_DEFINE_UNQUOTED(RUN_FILES, "$config_run", [run files])
AC_DEFINE_UNQUOTED(LOG_FILES, "$config_log", [log files])
AC_DEFINE_UNQUOTED(BIN_FILES, "$config_bin", [bin files])

OST_DEBUG

AC_OUTPUT( scripts/Makefile config/Makefile audio/Makefile 
audio/none/Makefile audio/en/Makefile audio/fr/Makefile audio/ru/Makefile
audio/none/prompts/Makefile audio/en/deborah/Makefile 
audio/fr/nicole/Makefile
audio/ru/natalia/Makefile audio/ru/dmitry/Makefile
docs/Makefile engine/Makefile server/Makefile modules/Makefile Makefile
modules/sunrpc/Makefile modules/tcpmon/Makefile modules/languages/Makefile
modules/soundcard/Makefile modules/voicetronix/Makefile modules/h323/Makefile
modules/sangoma/Makefile modules/sip/Makefile modules/capi20/Makefile 
modules/snapshot/Makefile modules/webservice/Makefile
modules/zeroconf/Makefile modules/xmlrpc/Makefile
utils/Makefile binders/Makefile 
binders/ivrscript/Makefile 
binders/bayonnexml/Makefile
config/bayonne.init docs/Doxyfile engine/libbayonne.pc )

if test ! -z "$SUNRPC_MODULE" ; then
	if test ! -f modules/sunrpc/bayonne_rpc.h ; then
		cd modules/sunrpc ; make rpc
	fi
fi
