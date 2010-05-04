# Copyright (C) 2006-2007 David Sugar, Tycho Softworks.
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

AC_INIT(autogen.sh)

VERSION="4.1.3"
LT_VERSION="4:7:0"

UCOMMON_REQUIRES="2.0.0"

AC_CONFIG_AUX_DIR(autoconf)
AC_CANONICAL_SYSTEM
AC_PROG_CPP
AC_PROG_CC
AC_PROG_CXXCPP
AC_PROG_CXX
AC_LIBTOOL_WIN32_DLL
AM_PROG_LIBTOOL
AM_INIT_AUTOMAKE(ccscript, [$VERSION])
AM_CONFIG_HEADER(config.h)

AC_C_RESTRICT
AC_C_VOLATILE
AC_C_INLINE

AC_SUBST(LT_VERSION)

if test -z "$PKG_CONFIG_PATH" ; then
	case "$prefix" in
	NONE|/usr/local|/usr)
		PKG_CONFIG_PATH="/usr/local/lib/pkgconfig:/usr/lib/pkgconfig"
		;;
	*)
		PKG_CONFIG_PATH="$prefix/lib/pkgconfig"
		;;
	esac
else
	case "$prefix" in
	NONE|/usr/local|/usr)
		PKG_CONFIG_PATH="$PKG_CONFIG_PATH:/usr/local/lib/pkgconfig:/usr/lib/pkgconfig"
		;;
	*)
		PKG_CONFIG_PATH="$PKG_CONFIG_PATH:$prefix/lib/pkgconfig"
		;;
	esac
fi

export PKG_CONFIG_PATH

EXPORT_LIBS=""
EXPORT_FLAGS=""
SCRIPT_LIBS=""
SCRIPT_FLAGS=""

AC_ARG_ENABLE(debug, 
	AC_HELP_STRING([--enable-debug],[compile for debugging]))
if test -z "$enable_debug" ; then
	enable_debug="no"
elif test $enable_debug = "yes" ; then
	CXXFLAGS="${CXXFLAGS} -g -DDEBUG"
fi

AC_ARG_WITH(pkg-config,
	AC_HELP_STRING([--with-pkg-config],[enable support for pkg-config]),[
	PKG_CHECK_MODULES(UCOMMON, ucommon >= $UCOMMON_REQUIRES)
	UCOMMON_CLINK=`pkg-config --variable=clink ucommon`
	UCOMMON_MODEL=`pkg-config --variable=model ucommon`
],[
	AC_PATH_PROG([UCOMMON],[ucommon-config],[none])
	if test $UCOMMON = "none" ; then
		AC_ERROR("required ucommon library missing")
	fi
	UCOMMON_CLINK=`$UCOMMON --clink`
	UCOMMON_MODEL=`$UCOMMON --model`
	UCOMMON_CFLAGS=`$UCOMMON --cflags`
	UCOMMON_LIBS=`$UCOMMON --libs`

])

if test "$UCOMMON_MODEL" = "CC" ; then
	LIBTOOL='$(SHELL) $(top_srcdir)/cmodel.sh'" ${LIBTOOL}" ; fi

AC_LANG_CPLUSPLUS
COMPILER_FLAGS=""
for flag in $CXXFLAGS ; do
	case "$flag" in
	-f*exceptions|-f*rtti|-f*check-new|-f*enforce-eh-specs|-finline|-f*except)
		if test "$UCOMMON_MODEL" = "CXX" ; then
			COMPILER_FLAGS="$COMPILER_FLAGS $flag" ; fi
		;;
	-fvisibility=*)
		;;
	*)
		COMPILER_FLAGS="$COMPILER_FLAGS $flag"
		;;
	esac
done
export CXXFLAGS="$COMPILER_FLAGS"

AC_CHECK_HEADERS(regex.h)

SCRIPT_FLAGS="$EXPORT_FLAGS $UCOMMON_CFLAGS"
SCRIPT_LIBS="$EXPORT_LIBS $UCOMMON_LIBS $ac_with_malloc $UCOMMON_CLINK"

case "$SCRIPT_FLAGS -I/usr/include" in
*-I${default_incpath}*)
	;;
*)
	EXPORT_FLAGS="$EXPORT_FLAGS -I$default_incpath"
	;;
esac

AC_SUBST(UCOMMON_REQUIRES)
AC_SUBST(UCOMMON_MODEL)
AC_SUBST(DAEMON_LIBS)
AC_SUBST(EXPORT_FLAGS)
AC_SUBST(EXPORT_LIBS)
AC_SUBST(SCRIPT_FLAGS)
AC_SUBST(SCRIPT_LIBS)
AC_SUBST(CXXFLAGS)
AC_OUTPUT(Makefile inc/Makefile src/Makefile test/Makefile doc/Makefile
ccscript-config ccscript.pc ccscript.spec)

