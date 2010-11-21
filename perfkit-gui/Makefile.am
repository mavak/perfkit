include $(top_srcdir)/Makefile.inc

bin_PROGRAMS = perfkit-gui
noinst_LTLIBRARIES = libperfkit-gui.la

headerdir = $(prefix)/include/perfkit-1.0/perfkit-gui
header_DATA = $(INST_H_FILES)

AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)s/locale"\"	\
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\"				\
	-DPACKAGE_DATA_DIR=\""$(datadir)"\"				\
	-DPACKAGE_LIB_DIR=\""$(libdir)"\"				\
	-DGETTEXT_PACKAGE=\""pkg"\"					\
	-DHAVE_CONFIG_H							\
	-DG_LOG_DOMAIN=\"PerfkitGui\"					\
	-DPERFKIT_GUI_COMPILATION					\
	-I$(top_srcdir)/cut-n-paste					\
	$(WARNINGS)							\
	$(PERFKIT_DEBUG_CFLAGS)						\
	$(GTK_CFLAGS)							\
	$(NULL)

AM_LDFLAGS = $(PERFKIT_GUI_LIBS)

#
# libperfkit-gui static library
#

INST_H_FILES =
INST_H_FILES += perfkit-gui.h
INST_H_FILES += pkg-channel-page.h
INST_H_FILES += pkg-channels-page.h
INST_H_FILES += pkg-closures.h
INST_H_FILES += pkg-page.h
INST_H_FILES += pkg-path.h
INST_H_FILES += pkg-plugin-page.h
INST_H_FILES += pkg-source-page.h
INST_H_FILES += pkg-sources-page.h
INST_H_FILES += pkg-subscription-page.h
INST_H_FILES += pkg-version.h

NOINST_H_FILES =
NOINST_H_FILES += pkg-log.h
NOINST_H_FILES += pkg-util.h

libperfkit_gui_la_SOURCES =
libperfkit_gui_la_SOURCES += $(INST_H_FILES)
libperfkit_gui_la_SOURCES += $(NOINST_H_FILES)
libperfkit_gui_la_SOURCES += pkg-channel-page.c
libperfkit_gui_la_SOURCES += pkg-channels-page.c
libperfkit_gui_la_SOURCES += pkg-closures.c
libperfkit_gui_la_SOURCES += pkg-log.c
libperfkit_gui_la_SOURCES += pkg-page.c
libperfkit_gui_la_SOURCES += pkg-path.c
libperfkit_gui_la_SOURCES += pkg-plugin-page.c
libperfkit_gui_la_SOURCES += pkg-source-page.c
libperfkit_gui_la_SOURCES += pkg-sources-page.c
libperfkit_gui_la_SOURCES += pkg-subscription-page.c
libperfkit_gui_la_SOURCES += pkg-util.c	
libperfkit_gui_la_SOURCES += pkg-window.c

#
# perfkit-gui executable
#

perfkit_gui_SOURCES =
perfkit_gui_SOURCES += main.c

perfkit_gui_LDADD =
perfkit_gui_LDADD += libperfkit-gui.la
perfkit_gui_LDADD += $(top_builddir)/perfkit/libperfkit-1.0.la
perfkit_gui_LDADD += $(GTK_LIBS)

perfkit_gui_LDFLAGS =
perfkit_gui_LDFLAGS += -export-dynamic
perfkit_gui_LDFLAGS += -no-undefined
perfkit_gui_LDFLAGS += -export-symbols-regex "^pkg_.*"

#
# development helpers
#

run:
	@PERFKIT_GUI_DATA_PATH=$(top_srcdir)/data		\
	PERFKIT_CONNECTIONS_DIR=$(top_srcdir)/perfkit/.libs	\
	libtool --mode=execute $(builddir)/perfkit-gui

debug:
	@PERFKIT_GUI_DATA_PATH=$(top_srcdir)/data		\
	PERFKIT_CONNECTIONS_DIR=$(top_srcdir)/perfkit/.libs	\
	libtool --mode=execute gdb $(builddir)/perfkit-gui
