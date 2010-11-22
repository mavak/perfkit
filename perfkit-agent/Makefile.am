include $(top_srcdir)/Makefile.inc

noinst_LTLIBRARIES = libperfkit-agent.la

bin_PROGRAMS = perfkit-agent

listeners_LTLIBRARIES =
listeners_LTLIBRARIES += dbus.la
listenersdir = $(libdir)/perfkit-agent/listeners

sources_LTLIBRARIES =
sources_LTLIBRARIES += cpu.la
sources_LTLIBRARIES += gdkevent.la
sources_LTLIBRARIES += memory.la
sources_LTLIBRARIES += netdev.la
sources_LTLIBRARIES += sched.la
sourcesdir = $(libdir)/perfkit-agent/plugins

modules_LTLIBRARIES = libgdkevent-module.la
modulesdir = $(libdir)/gtk-2.0/modules

headerdir = $(prefix)/include/perfkit-1.0/perfkit-agent
header_DATA = $(INST_H_FILES)

etcdir = $(sysconfdir)/perfkit
etc_DATA = agent.conf

#
# libperfkit-agent - this is statically linked against perfkit-agent
# and used in unit tests.
#

INST_H_FILES =
INST_H_FILES += pka-channel.h
INST_H_FILES += pka-config.h
INST_H_FILES += pka-context.h
INST_H_FILES += pka-encoder.h
INST_H_FILES += pka-listener.h
INST_H_FILES += pka-listener-lowlevel.h
INST_H_FILES += pka-log.h
INST_H_FILES += pka-manager.h
INST_H_FILES += pka-manifest.h
INST_H_FILES += pka-plugin.h
INST_H_FILES += pka-sample.h
INST_H_FILES += pka-source.h
INST_H_FILES += pka-source-simple.h
INST_H_FILES += pka-spawn-info.h
INST_H_FILES += pka-subscription.h
INST_H_FILES += pka-util.h
INST_H_FILES += pka-version.h
INST_H_FILES += perfkit-agent.h

NOINST_H_FILES =
NOINST_H_FILES += pka-listener-closures.h
NOINST_H_FILES += pka-private.h

libperfkit_agent_la_SOURCES =
libperfkit_agent_la_SOURCES += pka-marshal.c
libperfkit_agent_la_SOURCES += pka-marshal.h
libperfkit_agent_la_SOURCES += $(INST_H_FILES)
libperfkit_agent_la_SOURCES += $(NOINST_H_FILES)
libperfkit_agent_la_SOURCES += pka-channel.c
libperfkit_agent_la_SOURCES += pka-config.c
libperfkit_agent_la_SOURCES += pka-context.c
libperfkit_agent_la_SOURCES += pka-encoder.c
libperfkit_agent_la_SOURCES += pka-listener.c
libperfkit_agent_la_SOURCES += pka-log.c
libperfkit_agent_la_SOURCES += pka-manager.c
libperfkit_agent_la_SOURCES += pka-manifest.c
libperfkit_agent_la_SOURCES += pka-plugin.c
libperfkit_agent_la_SOURCES += pka-sample.c
libperfkit_agent_la_SOURCES += pka-source.c
libperfkit_agent_la_SOURCES += pka-source-simple.c
libperfkit_agent_la_SOURCES += pka-spawn-info.c
libperfkit_agent_la_SOURCES += pka-subscription.c
libperfkit_agent_la_SOURCES += pka-util.c
libperfkit_agent_la_SOURCES += $(top_srcdir)/cut-n-paste/egg-buffer.c

libperfkit_agent_la_CPPFLAGS =
libperfkit_agent_la_CPPFLAGS += -DG_LOG_DOMAIN=\"Agent\"
libperfkit_agent_la_CPPFLAGS += -D_GNU_SOURCE
libperfkit_agent_la_CPPFLAGS += $(INCLUDE_CFLAGS)
libperfkit_agent_la_CPPFLAGS += $(GIO_CFLAGS)
libperfkit_agent_la_CPPFLAGS += $(GMODULE_CFLAGS)
libperfkit_agent_la_CPPFLAGS += $(GOBJECT_CFLAGS)

libperfkit_agent_la_LIBADD =
libperfkit_agent_la_LIBADD += $(GIO_LIBS)
libperfkit_agent_la_LIBADD += $(GOBJECT_LIBS)
libperfkit_agent_la_LIBADD += $(GMODULE_LIBS)

libperfkit_agent_la_LDFLAGS =
libperfkit_agent_la_LDFLAGS += --export-all-symbols

##
# perfkit-agent executable
##

perfkit_agent_SOURCES = main.c

perfkit_agent_LDADD =
perfkit_agent_LDADD += libperfkit-agent.la

perfkit_agent_CPPFLAGS =
perfkit_agent_CPPFLAGS += $(INCLUDE_CFLAGS)
perfkit_agent_CPPFLAGS += $(GOBJECT_CFLAGS)

perfkit_agent_LDFLAGS =
perfkit_agent_LDFLAGS += -export-dynamic
perfkit_agent_LDFLAGS += -export-symbols-regex "^pka_.*"

perfkit_agent_DEPENDENCIES = libperfkit-agent.la

pka-marshal.c pka-marshal.h: pka-marshal.list
	$(GLIB_GENMARSHAL) --header --prefix=pka_marshal		\
	$(srcdir)/pka-marshal.list > $(builddir)/pka-marshal.h
	$(GLIB_GENMARSHAL) --body --prefix=pka_marshal			\
	$(srcdir)/pka-marshal.list > $(builddir)/pka-marshal.c

#
# dbus listener
#

dbus_la_SOURCES =
dbus_la_SOURCES += listeners/pka-listener-dbus.c
dbus_la_SOURCES += listeners/pka-listener-dbus.h

dbus_la_LIBADD =
dbus_la_LIBADD += $(DBUS_LIBS)

dbus_la_LDFLAGS =
dbus_la_LDFLAGS += -export-dynamic
dbus_la_LDFLAGS += -export-symbols-regex "^pka_.*"
dbus_la_LDFLAGS += -module

dbus_la_CPPFLAGS =
dbus_la_CPPFLAGS += $(DBUS_CFLAGS)
dbus_la_CPPFLAGS += $(INCLUDE_CFLAGS)
dbus_la_CPPFLAGS += $(GIO_CFLAGS)
dbus_la_CPPFLAGS += $(GOBJECT_CFLAGS)

#
# memory source
#

memory_la_CPPFLAGS =
memory_la_CPPFLAGS += $(INCLUDE_CFLAGS)
memory_la_CPPFLAGS += $(GOBJECT_CFLAGS)

memory_la_SOURCES =
memory_la_SOURCES += sources/memory.c

memory_la_LDFLAGS =
memory_la_LDFLAGS += -module

#
# scheduler source
#

sched_la_CPPFLAGS =
sched_la_CPPFLAGS += $(INCLUDE_CFLAGS)
sched_la_CPPFLAGS += $(GOBJECT_CFLAGS)

sched_la_SOURCES =
sched_la_SOURCES += sources/sched.c

sched_la_LDFLAGS =
sched_la_LDFLAGS += -module

#
# network device source
#

netdev_la_CPPFLAGS =
netdev_la_CPPFLAGS += $(INCLUDE_CFLAGS)
netdev_la_CPPFLAGS += $(GOBJECT_CFLAGS)

netdev_la_SOURCES =
netdev_la_SOURCES += sources/netdev.c
netdev_la_SOURCES += sources/src-utils.c
netdev_la_SOURCES += sources/src-utils.h

netdev_la_LDFLAGS =
netdev_la_LDFLAGS += -module

#
# cpu source
#

cpu_la_CPPFLAGS =
cpu_la_CPPFLAGS += $(INCLUDE_CFLAGS)
cpu_la_CPPFLAGS += $(GOBJECT_CFLAGS)

cpu_la_SOURCES =
cpu_la_SOURCES += sources/cpu.c
cpu_la_SOURCES += sources/src-utils.c
cpu_la_SOURCES += sources/src-utils.h

cpu_la_LDFLAGS =
cpu_la_LDFLAGS += -module

#
# gdkevent source
#

gdkevent_la_CPPFLAGS =
gdkevent_la_CPPFLAGS += $(INCLUDE_CFLAGS)
gdkevent_la_CPPFLAGS += $(GOBJECT_CFLAGS)
gdkevent_la_CPPFLAGS += $(GIO_CFLAGS)
gdkevent_la_CPPFLAGS += $(GTK_CFLAGS)

gdkevent_la_SOURCES =
gdkevent_la_SOURCES += sources/gdkevent.c

gdkevent_la_LDFLAGS =
gdkevent_la_LDFLAGS += -module
gdkevent_la_LDFLAGS += $(GIO_LIBS)
gdkevent_la_LDFLAGS += $(GTK_LIBS)

libgdkevent_module_la_SOURCES =
libgdkevent_module_la_SOURCES += sources/gdkevent-module.c

libgdkevent_module_la_CPPFLAGS =
libgdkevent_module_la_CPPFLAGS += $(INCLUDE_CFLAGS)
libgdkevent_module_la_CPPFLAGS += $(GIO_CFLAGS)
libgdkevent_module_la_CPPFLAGS += $(GTK_CFLAGS)

libgdkevent_module_la_LIBADD =
libgdkevent_module_la_LIBADD += $(INCLUDE_LIBS)
libgdkevent_module_la_LIBADD += $(GIO_LIBS)
libgdkevent_module_la_LIBADD += $(GTK_LIBS)

#
# developement helpers
#

run: perfkit-agent
	@rm -f $(builddir)/perfkit-agent.log
	@PERFKIT_PLUGINS_PATH=$(builddir)/.libs				\
	$(builddir)/perfkit-agent --stdout -c $(srcdir)/agent.conf	\
	--log=$(builddir)/perfkit-agent.log

debug: perfkit-agent
	@PERFKIT_PLUGINS_PATH=$(builddir)/.libs				\
	gdb --args $(builddir)/perfkit-agent --stdout 			\
	-c $(srcdir)/agent.conf

valgrind: perfkit-agent
	@PERFKIT_PLUGINS_PATH=$(builddir)/.libs				\
	PERFKIT_VALGRIND=1						\
	G_SLICE=always-malloc G_DEBUG=gc-friendly			\
	libtool --mode=execute valgrind --tool=memcheck			\
	--leak-check=full --leak-resolution=high			\
	--log-file=perfkit-agent.valgrind --show-reachable=yes		\
	--suppressions=$(top_builddir)/cut-n-paste/valgrind.suppression	\
	$(builddir)/perfkit-agent --stdout -c $(srcdir)/agent.conf

