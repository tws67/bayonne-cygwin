# Copyright (C) 2004 Open Source Telecom Corporation.
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

AC_INIT(autogen.sh)

VERSION="1.1.7"
LT_RELEASE="1.1"
LT_VERSION="0:7"

AC_CONFIG_AUX_DIR(autoconf)
AC_CANONICAL_SYSTEM
AC_PROG_CPP
AC_PROG_CXXCPP
AC_PROG_CXX
AM_PROG_LIBTOOL
AM_INIT_AUTOMAKE(ccscript3,[$VERSION])
AM_CONFIG_HEADER(src/private.h)
AM_MAINTAINER_MODE

AC_ARG_WITH(odbc, [--without-odbc          Disable odbc support],,[
        ost_cv_odbc_database=false
        AC_CHECK_HEADER(odbc/sql.h,[   
                AC_DEFINE(HAVE_ODBC_SQL_H, [1], [offset odbc header])
                ost_cv_odbc_database=true
        ],[
                AC_CHECK_HEADER(sql.h,[
                        AC_DEFINE(HAVE_SQL_H, [1], [local odbc header])
                        ost_cv_odbc_database=true
                ])
        ])
        if "$ost_cv_odbc_database" = "true" ; then
                ODBC_LIB="-liodbc"
                AC_CHECK_LIB(odbc, SQLConnect, ODBC_LIB="-liodbc")
                AC_SUBST(ODBC_LIB)
        fi
])

CRYPT_LIBS=""
AC_CHECK_LIB(crypt, crypt, CRYPT_LIBS="-lcrypt")
AC_SUBST(CRYPT_LIBS)

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

PKG_CHECK_MODULES(COMMON, libccgnu2 >= 1.3.5)
CXXFLAGS="$CXXFLAGS $COMMON_CFLAGS"
COMMON_LIBPATH=`pkg-config --variable=libdir libccgnu2`
LIBS="$LIBS -L$COMMON_LIBPATH $COMMON_LIBS"
COMMON_MODFLAGS=`pkg-config --variable=modflags libccgnu2`
AC_SUBST(COMMON_MODFLAGS)
AC_CHECK_HEADERS(regex.h)



AC_DEFUN([OST_DEBUG],[
	AC_MSG_CHECKING(for debugging)
	AC_ARG_ENABLE(debug, [  --enable-debug          compile for debugging])	
	if test -z "$enable_debug" ; then
		enable_debug="no"
	elif test $enable_debug = "yes" ; then
		CXXFLAGS="${CXXFLAGS} -g -DDEBUG"
#		CXXFLAGS=`echo $CFLAGS | sed 's/-O.//'`
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

SQL_MODULE=""
ODBC_MODULE=""
if test "$ost_cv_odbc_database" = true ; then
	SQL_MODULE=odbc ; ODBC_MODULE=odbc ; fi

AC_SUBST(ODBC_MODULE)

OST_DEBUG

case "$prefix" in
/opt/*)
        if test "$localstatedir" == '${prefix}/var' ; then
                localstatedir=/var${prefix} ; fi

        if test "$datadir" == '${prefix}/share' ; then
                if test "$mandir" == '${datadir}/man' ; then
                        mandir='${prefix}/man' ; fi
                if test "$infodir" == '${datadir}/info' ; then
                        infodir='${prefix}/info' ; fi
                datadir='${prefix}'
        fi
        AC_SUBST(mandir)
        AC_SUBST(infodir)
        AC_SUBST(datadir)
        AC_SUBST(localstatedir)
        ;;
esac

if test "$localstatedir" == '${prefix}/var' ; then
        localstatedir="/var" ; fi
AC_SUBST(localstatedir)

script_libdir='${libdir}/ccscript3-'${LT_RELEASE}
eprefix=${exec_prefix}
if test "$eprefix" == "NONE" ; then
        eprefix=${prefix} ; fi

case "$eprefix" in
/opt/*)
        if test "$libdir" == '${exec_prefix}/lib' ; then
                script_libdir='${exec_prefix}/lib/ccscript3'
        fi
        ;;
esac

AC_SUBST(STAGE2)
AC_SUBST(script_libdir)
AC_SUBST_DIR(script_libpath, script_libdir)
AC_SUBST(LT_VERSION)
AC_SUBST(LT_RELEASE)

if test ! -z "$SQL_MODULE" ; then
	AC_DEFINE_UNQUOTED(SQL_MODULE, "$SQL_MODULE", [default sql module]) 
fi

AC_DEFINE_UNQUOTED(SCRIPT_LIBPATH, "$script_libpath", [define codec path])

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

AC_PATH_PROG(DOXYGEN, doxygen, no)
AC_SUBST(DOXYGEN)
AM_CONDITIONAL(DOXY, test "$DOXYGEN" != "no")

AC_OUTPUT(Makefile src/Makefile tests/Makefile modules/Makefile
optional/Makefile optional/odbc/Makefile
libccscript3.pc ccscript3.spec ccscript3.list)


