-include $(INTROSPECTION_MAKEFILE)

SUBDIRS = . connections

lib_LTLIBRARIES = libperfkit-1.0.la

headerdir = $(prefix)/include/perfkit-1.0/perfkit
header_DATA = $(INST_H_FILES)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = perfkit-1.0.pc

WARNINGS =								\
	-Wall								\
	-Werror								\
	-Wold-style-definition						\
	-Wdeclaration-after-statement					\
	-Wredundant-decls						\
	-Wmissing-noreturn						\
	-Wcast-align							\
	-Wwrite-strings							\
	-Winline							\
	-Wformat-nonliteral						\
	-Wformat-security						\
	-Wswitch-enum							\
	-Wswitch-default						\
	-Winit-self							\
	-Wmissing-include-dirs						\
	-Wundef								\
	-Waggregate-return						\
	-Wmissing-format-attribute					\
	-Wnested-externs						\
	-Wshadow

BUILD_SOURCES = pk-version.h

INST_H_FILES =								\
	$(BUILD_SOURCES)						\
	perfkit.h							\
	pk-connection.h							\
	pk-connection-lowlevel.h					\
	pk-manifest.h							\
	pk-sample.h							\
	$(NULL)

NOINST_H_FILES =							\
	pk-log.h							\
	pk-util.h							\
	$(NULL)

libperfkit_1_0_la_SOURCES =						\
	$(INST_H_FILES)							\
	$(NOINST_H_FILES)						\
	pk-connection.c							\
	pk-manifest.c							\
	pk-sample.c							\
	$(top_srcdir)/cut-n-paste/egg-buffer.c				\
	$(NULL)

libperfkit_1_0_la_CPPFLAGS =						\
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)s/locale"\"	\
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\"				\
	-DPACKAGE_DATA_DIR=\""$(datadir)"\"				\
	-DPACKAGE_LIB_DIR=\""$(libdir)"\"				\
	-DGETTEXT_PACKAGE=\""perfkit"\"					\
	-DG_LOG_DOMAIN=\"Perfkit\"					\
	-DPERFKIT_COMPILATION						\
	-I$(top_srcdir)/cut-n-paste					\
	$(PERFKIT_DEBUG_CFLAGS)						\
	$(WARNINGS)							\
	$(GIO_CFLAGS)							\
	$(GMODULE_CFLAGS)						\
	$(GOBJECT_CFLAGS)						\
	$(NULL)

libperfkit_1_0_la_LIBADD =
libperfkit_1_0_la_LIBADD += $(GIO_LIBS)
libperfkit_1_0_la_LIBADD += $(GMODULE_LIBS)
libperfkit_1_0_la_LIBADD += $(GOBJECT_LIBS)
