#
# Perfkit Agent
#

SUBDIRS = . sources encoders listeners

noinst_LTLIBRARIES = libperfkit-agent.la
bin_PROGRAMS = perfkit-agent

headerdir = $(prefix)/include/perfkit-1.0/perfkit-agent
header_DATA = $(INST_H_FILES)

etcdir = $(sysconfdir)/perfkit
etc_DATA = agent.conf

WARNINGS =								\
	-Wall								\
	-Werror								\
	-Wold-style-definition						\
	-Wdeclaration-after-statement					\
	-Wredundant-decls						\
	-Wmissing-noreturn						\
	-Wshadow							\
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
	-Wnested-externs

AM_CPPFLAGS =								\
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)s/locale"\"	\
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\"				\
	-DPACKAGE_DATA_DIR=\""$(datadir)"\"				\
	-DPACKAGE_LIB_DIR=\""$(libdir)"\"				\
	-DPACKAGE_SYSCONFDIR=\""$(sysconfdir)"\"			\
	-DGETTEXT_PACKAGE=\""perfkit-agent"\"				\
	-DPERFKIT_COMPILATION						\
	-DG_LOG_DOMAIN=\"Agent\"					\
	-D_GNU_SOURCE							\
	-I$(top_srcdir)/cut-n-paste					\
	$(WARNINGS)							\
	$(PERFKIT_DEBUG_CFLAGS)						\
	$(GIO_CFLAGS)							\
	$(GMODULE_CFLAGS)						\
	$(GOBJECT_CFLAGS)						\
	$(NULL)

INST_H_FILES =								\
	pka-channel.h							\
	pka-config.h							\
	pka-context.h							\
	pka-encoder.h							\
	pka-listener.h							\
	pka-listener-lowlevel.h						\
	pka-log.h							\
	pka-manager.h							\
	pka-manifest.h							\
	pka-plugin.h							\
	pka-sample.h							\
	pka-source.h							\
	pka-source-simple.h						\
	pka-spawn-info.h						\
	pka-subscription.h						\
	pka-util.h							\
	pka-version.h							\
	perfkit-agent.h							\
	$(NULL)

NOINST_H_FILES =							\
	pka-listener-closures.h						\
	pka-private.h							\
	$(NULL)

BUILT_SOURCES =								\
	pka-marshal.h							\
	pka-marshal.c							\
	$(NULL)

##
# libperfkit-agent - this is statically linked against perfkit-agent
# and used in unit tests.
##

libperfkit_agent_la_SOURCES =
libperfkit_agent_la_SOURCES += $(BUILT_SOURCES)
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
perfkit_agent_LDADD += $(GIO_LIBS)
perfkit_agent_LDADD += $(GOBJECT_LIBS)
perfkit_agent_LDADD += $(GMODULE_LIBS)

perfkit_agent_LDFLAGS =							\
	-export-dynamic							\
	-export-symbols-regex "^pka_.*"					\
	$(NULL)
perfkit_agent_DEPENDENCIES = libperfkit-agent.la

pka-marshal.c pka-marshal.h: pka-marshal.list
	$(GLIB_GENMARSHAL) --header --prefix=pka_marshal $(srcdir)/pka-marshal.list > $(builddir)/pka-marshal.h
	$(GLIB_GENMARSHAL) --body --prefix=pka_marshal $(srcdir)/pka-marshal.list > $(builddir)/pka-marshal.c

run: perfkit-agent
	@rm -f $(builddir)/perfkit-agent.log
	@PERFKIT_PLUGINS_PATH=$(builddir)/sources/.libs:$(builddir)/listeners/.libs:$(builddir)/encoders/.libs \
	$(builddir)/perfkit-agent --stdout -c $(srcdir)/agent.conf --log=$(builddir)/perfkit-agent.log

debug:
	@PERFKIT_PLUGINS_PATH=$(builddir)/sources/.libs:$(builddir)/listeners/.libs:$(builddir)/encoders/.libs \
	gdb --args $(builddir)/perfkit-agent --stdout -c $(srcdir)/agent.conf

valgrind:
	@PERFKIT_PLUGINS_PATH=$(builddir)/sources/.libs:$(builddir)/listeners/.libs:$(builddir)/encoders/.libs \
	PERFKIT_VALGRIND=1						\
	G_SLICE=always-malloc G_DEBUG=gc-friendly			\
	libtool --mode=execute valgrind --tool=memcheck			\
	--leak-check=full --leak-resolution=high			\
	--log-file=perfkit-agent.valgrind --show-reachable=yes		\
	--suppressions=$(top_builddir)/cut-n-paste/valgrind.suppression	\
	$(builddir)/perfkit-agent --stdout -c $(srcdir)/agent.conf

