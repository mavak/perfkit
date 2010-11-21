dnl ************************************************************************
dnl Versioning Macros
dnl ************************************************************************
m4_define([perfkit_major_version], [0])
m4_define([perfkit_minor_version], [1])
m4_define([perfkit_micro_version], [1])
m4_define([perfkit_version],
          [perfkit_major_version.perfkit_minor_version.perfkit_micro_version])
m4_define([perfkit_binary_age], [m4_eval(100 * perfkit_minor_version +
           perfkit_micro_version)])
m4_define([lt_current], [m4_eval(100 * perfkit_minor_version +
           perfkit_micro_version - perfkit_interface_age)])
m4_define([lt_revision], [perfkit_interface_age])
m4_define([lt_age], [m4_eval(perfkit_binary_age - perfkit_interface_age)])


dnl ************************************************************************
dnl Initialize Automake
dnl ************************************************************************
AC_PREREQ([2.64])
AC_INIT([perfkit],[perfkit_version])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([build/m4])

AM_INIT_AUTOMAKE([1.10])
AM_MAINTAINER_MODE([enable])

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES])
AS_AM_REALLY_SILENT

# Check for programs
AC_PROG_CC
AM_PROG_CC_C_O

AM_PATH_GLIB_2_0

# Initialize libtool
LT_PREREQ([2.2])
LT_INIT


dnl ************************************************************************
dnl Versioning Definitions
dnl ************************************************************************
PK_MAJOR_VERSION=perfkit_major_version
PK_MINOR_VERSION=perfkit_minor_version
PK_MICRO_VERSION=perfkit_micro_version
PK_API_VERSION=1.0
PK_VERSION=perfkit_version
AC_SUBST(PK_MAJOR_VERSION)
AC_SUBST(PK_MINOR_VERSION)
AC_SUBST(PK_MICRO_VERSION)
AC_SUBST(PK_API_VERSION)
AC_SUBST(PK_VERSION)


dnl ************************************************************************
dnl Check for Required Modules
dnl ************************************************************************
PKG_CHECK_MODULES(AVAHI,    [avahi-client     >= 0.6
                             avahi-gobject    >= 0.6])
PKG_CHECK_MODULES(CLUTTER,  [clutter-gtk-0.10 >= 0.10.4])
PKG_CHECK_MODULES(DBUS,     [dbus-glib-1      >= 0.80])
PKG_CHECK_MODULES(GIO,      [gio-2.0          >= 2.26
                             gio-unix-2.0     >= 2.26])
PKG_CHECK_MODULES(GMODULE,  [gmodule-2.0      >= 2.26])
PKG_CHECK_MODULES(GOBJECT,  [gobject-2.0      >= 2.26
                             gthread-2.0      >= 2.26])
PKG_CHECK_MODULES(GTK,      [gtk+-2.0         >= 2.20])


dnl ************************************************************************
dnl Enable extra debugging options
dnl ************************************************************************
m4_define([debug_default],
	  [m4_if(m4_eval(perfkit_minor_version % 2), [1],
	  [yes], [minimum])])
AC_ARG_ENABLE([debug],
	      [AS_HELP_STRING([--enable-debug=@<:@no/minimum/yes@:>@],
	      		      [turn on debugging @<:@default=debug_default@:>@])],
	      		      [],
	      		      [enable_debug=debug_default])
AS_CASE([$enable_debug],
	[yes], [
		test "$cflags_set" = set || CFLAGS="$CFLAGS -g"
		PERFKIT_DEBUG_CFLAGS="-DPERFKIT_TRACE -DPERFKIT_DEBUG"
	],
	[minimum], [
		PERFKIT_DEBUG_CFLAGS="-DG_DISABLE_CAST_CHECKS"
	],
	[no], [
		PERFKIT_DEBUG_CFLAGS="-DG_DISABLE_ASSERT -DG_DISABLE_CHECKS -DG_DISABLE_CAST_CHECKS"
	],
	[AC_MSG_ERROR([Unknown argument to --enable-debug])]
)
AC_SUBST(PERFKIT_DEBUG_CFLAGS)


dnl ************************************************************************
dnl Additional Headers
dnl ************************************************************************
AC_CHECK_HEADERS([readline/readline.h readline/history.h],
		 [],
		 [AC_MSG_ERROR([Readline headers were not found.])])


dnl ************************************************************************
dnl Unit Tests
dnl ************************************************************************
AC_PATH_PROG([GTESTER], [gtester])
AC_PATH_PROG([GTESTER_REPORT], [gtester-report])
AM_CONDITIONAL(ENABLE_GLIB_TEST, test "x$enable_glibtest" = "xyes")


dnl ************************************************************************
dnl Miscellaneous
dnl ************************************************************************
GTK_DOC_CHECK([1.11])
GOBJECT_INTROSPECTION_CHECK([0.6.7])
SHAMROCK_EXPAND_LIBDIR
SHAMROCK_EXPAND_BINDIR
SHAMROCK_EXPAND_DATADIR


dnl ************************************************************************
dnl Output
dnl ************************************************************************
AC_CONFIG_FILES([
	Makefile

	data/Makefile
	data/ui/Makefile
	data/icons/Makefile
	data/settings/Makefile

	doc/Makefile
	doc/reference/Makefile
	doc/reference/perfkit/Makefile
	doc/reference/perfkit/version.xml
	doc/reference/perfkit-agent/Makefile
	doc/reference/perfkit-agent/version.xml
	doc/reference/perfkit-gui/Makefile
	doc/reference/perfkit-gui/version.xml
	doc/reference/perfkit-profiler/Makefile
	doc/reference/perfkit-profiler/version.xml

	tools/Makefile

	perfkit/Makefile
	perfkit/connections/Makefile
	perfkit/pk-version.h
	perfkit/perfkit-1.0.pc

	perfkit-agent/Makefile
	perfkit-agent/sources/Makefile
	perfkit-agent/encoders/Makefile
	perfkit-agent/listeners/Makefile
	perfkit-agent/pka-version.h

	perfkit-gui/Makefile
	perfkit-gui/pkg-version.h

	uber/Makefile

	perfkit-profiler/Makefile
	perfkit-profiler/ppg-version.h

	bindings/Makefile
	bindings/vala/Makefile

	tests/Makefile
	tests/perfkit/Makefile
	tests/perfkit-agent/Makefile
	tests/test-suite/Makefile
])
AC_OUTPUT

echo ""
echo "Perfkit $VERSION"
echo ""
echo "  Prefix.....................: ${prefix}"
echo "  Debug Level................: ${enable_debug}"
echo "  Compiler Flags.............: ${CFLAGS}"
echo "  Enable API Reference.......: ${enable_gtk_doc}"
echo "  Enable Test Suite..........: ${enable_glibtest}"
echo ""
