include $(top_srcdir)/Makefile.inc

lib_LTLIBRARIES = libperfkit-1.0.la
connections_LTLIBRARIES = libdbus.la
connectionsdir = $(libdir)/perfkit/connections

headerdir = $(prefix)/include/perfkit-1.0/perfkit
header_DATA = $(INST_H_FILES)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = perfkit-1.0.pc

#
# libperfkit-1.0
#

INST_H_FILES =
INST_H_FILES += perfkit.h
INST_H_FILES += pk-connection.h
INST_H_FILES += pk-connection-lowlevel.h
INST_H_FILES += pk-manifest.h
INST_H_FILES += pk-model.h
INST_H_FILES += pk-model-memory.h
INST_H_FILES += pk-sample.h
INST_H_FILES += pk-version.h

NOINST_H_FILES =
NOINST_H_FILES += pk-log.h
NOINST_H_FILES += pk-util.h

libperfkit_1_0_la_SOURCES =
libperfkit_1_0_la_SOURCES += $(INST_H_FILES)
libperfkit_1_0_la_SOURCES += $(NOINST_H_FILES)
libperfkit_1_0_la_SOURCES += pk-connection.c
libperfkit_1_0_la_SOURCES += pk-manifest.c
libperfkit_1_0_la_SOURCES += pk-model.c
libperfkit_1_0_la_SOURCES += pk-model-memory.c
libperfkit_1_0_la_SOURCES += pk-sample.c
libperfkit_1_0_la_SOURCES += $(top_srcdir)/cut-n-paste/egg-buffer.c

libperfkit_1_0_la_CPPFLAGS =
libperfkit_1_0_la_CPPFLAGS += -DG_LOG_DOMAIN=\"Perfkit\"
libperfkit_1_0_la_CPPFLAGS += -DPERFKIT_COMPILATION
libperfkit_1_0_la_CPPFLAGS += -I$(top_srcdir)/cut-n-paste
libperfkit_1_0_la_CPPFLAGS += $(INCLUDE_CFLAGS)
libperfkit_1_0_la_CPPFLAGS += $(GIO_CFLAGS)
libperfkit_1_0_la_CPPFLAGS += $(GMODULE_CFLAGS)
libperfkit_1_0_la_CPPFLAGS += $(GOBJECT_CFLAGS)

libperfkit_1_0_la_LIBADD =
libperfkit_1_0_la_LIBADD += $(GIO_LIBS)
libperfkit_1_0_la_LIBADD += $(GMODULE_LIBS)
libperfkit_1_0_la_LIBADD += $(GOBJECT_LIBS)

#
# DBus connection
#

libdbus_la_SOURCES =
libdbus_la_SOURCES += connections/pk-connection-dbus.c
libdbus_la_SOURCES += connections/pk-connection-dbus.h

libdbus_la_CPPFLAGS =
libdbus_la_CPPFLAGS += $(INCLUDE_CFLAGS)
libdbus_la_CPPFLAGS += $(DBUS_CFLAGS)

libdbus_la_LIBADD =
libdbus_la_LIBADD += $(DBUS_LIBS)
libdbus_la_LIBADD += libperfkit-1.0.la

libdbus_la_DEPENDENCIES =
libdbus_la_DEPENDENCIES += libperfkit-1.0.la

libdbus_la_LDFLAGS =
libdbus_la_LDFLAGS += -export-dynamic
libdbus_la_LDFLAGS += -export-symbols-regex "^pk_.*"
libdbus_la_LDFLAGS += -module
