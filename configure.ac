m4_define(LIBCEC_MAJOR, [0])
m4_define(LIBCEC_MINOR, [4])
m4_define(LIBCEC_MICRO, [1])
m4_define(LIBCEC_NANO, [10009])

AC_INIT([libcec], LIBCEC_MAJOR.LIBCEC_MINOR.LIBCEC_MICRO, [cecd-devel@lists.sourceforge.net], [libcec], [http://sourceforge.net/apps/trac/cecd/wiki])

AC_SUBST([LIBCEC_VERSION_MAJOR], [LIBCEC_MAJOR])
AC_SUBST([LIBCEC_VERSION_MINOR], [LIBCEC_MINOR])
AC_SUBST([LIBCEC_VERSION_MICRO], [LIBCEC_MICRO])
AC_SUBST([LIBCEC_VERSION_NANO], [LIBCEC_NANO])

# Library versioning
# These numbers should be tweaked on every release. Read carefully:
# http://www.gnu.org/software/libtool/manual/html_node/Updating-version-info.html
# http://sourceware.org/autobook/autobook/autobook_91.html
lt_current="1"
lt_revision="0"
lt_age="0"
LTLDFLAGS="-version-info ${lt_current}:${lt_revision}:${lt_age}"

AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE

AC_CONFIG_SRCDIR([libcec/libcec.c])
AC_CONFIG_MACRO_DIR(m4)
AM_CONFIG_HEADER([config.h])
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AC_PREREQ([2.50])
AC_PROG_CC
AC_PROG_LIBTOOL
AC_C_INLINE
AM_PROG_CC_C_O

AC_MSG_CHECKING([operating system])
case $host in
*-linux*)
	os="linux"
	AM_CFLAGS="-std=gnu99"
	;;
*)
	AC_MSG_ERROR([unsupported Operating System])
esac
AC_MSG_RESULT([$os][$suffix])

LTLDFLAGS="${LTLDFLAGS} -no-undefined"

case $os in
Linux)
	AC_DEFINE(OS_LINUX, 1, [Linux OS])
	;;
esac
AC_SUBST(PC_LIBS_PRIVATE)
LIBS="${LIBS} ${PC_LIBS_PRIVATE}"

AM_CONDITIONAL([OS_LINUX], [test "x$os" = "xlinux"])

# Build daemon?
AC_ARG_ENABLE([daemon], [AS_HELP_STRING([--enable-daemon],
	[Build cec daemon (cecd, default y)])],
	[enable_daemon=$enableval],
	[enable_daemon='yes'])
AM_CONDITIONAL([BUILD_CECD], [test "x$enable_daemon" != "xno"])

# Implementation backend
AC_ARG_ENABLE([realtek], [AS_HELP_STRING([--enable-realtek],
	[enable Realtek SoC CEC driver support (default y)])],
	[enable_realtek=$enableval],
	[enable_realtek='yes'])
if test "x$enable_realtek" != "xno"; then
	AC_DEFINE([LINUX_REALTEK_SOC], 1, [Realtek SoC CEC driver support])
fi
AM_CONDITIONAL([LINUX_REALTEK_SOC], [test "x$enable_realtek" != "xno"])

# Logging
AC_ARG_ENABLE([log], [AS_HELP_STRING([--disable-log],
	[disable all logging (default is enabled)])],
	[enable_log=$enableval],
	[enable_log='yes'])
if test "x$enable_log" != "xno"; then
	AC_DEFINE([ENABLE_LOGGING], 1, [Message logging])
fi

AC_ARG_ENABLE([debug-log], [AS_HELP_STRING([--enable-debug-log],
	[enable debug logging (default n)])],
	[enable_debug_log=$enableval],
	[enable_debug_log='no'])
if test "x$enable_log" != "xno"; then
	if test "x$enable_debug_log" != "xno"; then
		AC_DEFINE([ENABLE_DEBUG_LOGGING], 1, [Debug message logging])
	fi
fi

# Check for -fvisibility=hidden compiler support (GCC >= 3.4)
saved_cflags="$CFLAGS"
CFLAGS="$CFLAGS -Werror -fvisibility=hidden"
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([])],
	[VISIBILITY_CFLAGS="-fvisibility=hidden"
	 AC_DEFINE([DEFAULT_VISIBILITY], [__attribute__((visibility("default")))], [Default visibility]) ],
	[ VISIBILITY_CFLAGS=""
	 AC_DEFINE([DEFAULT_VISIBILITY], [], [Default visibility]) ],
	])
CFLAGS="$saved_cflags"

AM_CFLAGS="$AM_CFLAGS -Wall -Wundef -Wunused -Wstrict-prototypes -Werror-implicit-function-declaration -Wshadow"

AC_SUBST(VISIBILITY_CFLAGS)
AC_SUBST(AM_CFLAGS)
AC_SUBST(LTLDFLAGS)

AC_CONFIG_FILES([Makefile libcec/Makefile cecd/Makefile libcec/libcec_version.h libcec/libcec.pc])
AC_OUTPUT
