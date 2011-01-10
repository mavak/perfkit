include $(top_srcdir)/Makefile.inc

bin_PROGRAMS = perfkit-profiler
noinst_LTLIBRARIES = libprofiler.la

headerdir = $(prefix)/include/perfkit-1.0/perfkit-profiler
header_DATA = $(INST_H_FILES)

#
# libprofiler static library
#

libprofiler_la_SOURCES =
libprofiler_la_SOURCES += $(INST_H_FILES)
libprofiler_la_SOURCES += $(NOINST_H_FILES)
libprofiler_la_SOURCES += ppg-about-dialog.c
libprofiler_la_SOURCES += ppg-actions.c
libprofiler_la_SOURCES += ppg-add-instrument-dialog.c
libprofiler_la_SOURCES += ppg-animation.c
libprofiler_la_SOURCES += ppg-clock-source.c
libprofiler_la_SOURCES += ppg-color.c
libprofiler_la_SOURCES += ppg-configure-instrument-dialog.c
libprofiler_la_SOURCES += ppg-cpu-instrument.c
if HAVE_AVAHI
libprofiler_la_SOURCES += ppg-discover-dialog.c
endif
libprofiler_la_SOURCES += ppg-frame-source.c
libprofiler_la_SOURCES += ppg-header.c
libprofiler_la_SOURCES += ppg-hud-window.c
libprofiler_la_SOURCES += ppg-instrument.c
libprofiler_la_SOURCES += ppg-instrument-menu.c
libprofiler_la_SOURCES += ppg-instrument-view.c
libprofiler_la_SOURCES += ppg-instruments.c
libprofiler_la_SOURCES += ppg-log.c
libprofiler_la_SOURCES += $(builddir)/ppg-marshal.c
libprofiler_la_SOURCES += ppg-memory-instrument.c
libprofiler_la_SOURCES += ppg-menu-tool-item.c
libprofiler_la_SOURCES += ppg-monitor.c
libprofiler_la_SOURCES += ppg-paths.c
libprofiler_la_SOURCES += ppg-prefs.c
libprofiler_la_SOURCES += ppg-prefs-dialog.c
libprofiler_la_SOURCES += ppg-process-menu.c
libprofiler_la_SOURCES += ppg-renderer.c
libprofiler_la_SOURCES += ppg-renderer-line.c
libprofiler_la_SOURCES += ppg-restart-task.c
libprofiler_la_SOURCES += ppg-rt-graph.c
libprofiler_la_SOURCES += ppg-ruler.c
libprofiler_la_SOURCES += ppg-runtime.c
libprofiler_la_SOURCES += ppg-session.c
libprofiler_la_SOURCES += ppg-session-view.c
libprofiler_la_SOURCES += ppg-session-task.c
libprofiler_la_SOURCES += ppg-sessions-store.c
libprofiler_la_SOURCES += ppg-settings-dialog.c
libprofiler_la_SOURCES += ppg-spawn-process-dialog.c
libprofiler_la_SOURCES += ppg-status-actor.c
libprofiler_la_SOURCES += ppg-task.c
libprofiler_la_SOURCES += ppg-task-render.c
libprofiler_la_SOURCES += ppg-timeout-interval.c
libprofiler_la_SOURCES += ppg-timer-tool-item.c
libprofiler_la_SOURCES += ppg-util.c
libprofiler_la_SOURCES += ppg-visualizer.c
libprofiler_la_SOURCES += ppg-visualizer-simple.c
libprofiler_la_SOURCES += ppg-visualizer-menu.c
libprofiler_la_SOURCES += ppg-welcome-dialog.c
libprofiler_la_SOURCES += ppg-window.c

libprofiler_la_CPPFLAGS =
libprofiler_la_CPPFLAGS += $(AVAHI_CFLAGS)
libprofiler_la_CPPFLAGS += $(GOOCANVAS_CFLAGS)
libprofiler_la_CPPFLAGS += $(GIO_CFLAGS)
libprofiler_la_CPPFLAGS += $(GTK_CFLAGS)
libprofiler_la_CPPFLAGS += $(INCLUDE_CFLAGS)
libprofiler_la_CPPFLAGS += -include config.h
libprofiler_la_CPPFLAGS += -I$(top_srcdir)/cut-n-paste
libprofiler_la_CPPFLAGS += -DPRODUCT_NAME="\"Perfkit Profiler\""
libprofiler_la_CPPFLAGS += -DG_LOG_DOMAIN=\""Profiler"\"

libprofiler_la_DEPENDENCIES =
libprofiler_la_DEPENDENCIES += ppg-marshal.c
libprofiler_la_DEPENDENCIES += ppg-marshal.h

libprofiler_la_LIBADD =
if HAVE_AVAHI
libprofiler_la_LIBADD += $(AVAHI_LIBS)
endif
libprofiler_la_LIBADD += $(GIO_LIBS)
libprofiler_la_LIBADD += $(GTK_LIBS)
libprofiler_la_LIBADD += $(GOOCANVAS_LIBS)
libprofiler_la_LIBADD += $(top_builddir)/perfkit/libperfkit-1.0.la

INST_H_FILES =
INST_H_FILES += perfkit-profiler.h
INST_H_FILES += ppg-about-dialog.h
INST_H_FILES += ppg-actions.h
INST_H_FILES += ppg-add-instrument-dialog.h
INST_H_FILES += ppg-animation.h
INST_H_FILES += ppg-clock-source.h
INST_H_FILES += ppg-color.h
INST_H_FILES += ppg-configure-instrument-dialog.h
INST_H_FILES += ppg-cpu-instrument.h
if HAVE_AVAHI
INST_H_FILES += ppg-discover-dialog.h
endif
INST_H_FILES += ppg-frame-source.h
INST_H_FILES += ppg-header.h
INST_H_FILES += ppg-hud-window.h
INST_H_FILES += ppg-instrument.h
INST_H_FILES += ppg-instrument-menu.h
INST_H_FILES += ppg-instrument-view.h
INST_H_FILES += ppg-instruments.h
INST_H_FILES += ppg-log.h
INST_H_FILES += ppg-memory-instrument.h
INST_H_FILES += ppg-menu-tool-item.h
INST_H_FILES += ppg-paths.h
INST_H_FILES += ppg-prefs.h
INST_H_FILES += ppg-prefs-dialog.h
INST_H_FILES += ppg-process-menu.h
INST_H_FILES += ppg-restart-task.h
INST_H_FILES += ppg-renderer.h
INST_H_FILES += ppg-renderer-line.h
INST_H_FILES += ppg-rt-graph.h
INST_H_FILES += ppg-ruler.h
INST_H_FILES += ppg-runtime.h
INST_H_FILES += ppg-session.h
INST_H_FILES += ppg-session-view.h
INST_H_FILES += ppg-session-task.h
INST_H_FILES += ppg-sessions-store.h
INST_H_FILES += ppg-settings-dialog.h
INST_H_FILES += ppg-spawn-process-dialog.h
INST_H_FILES += ppg-status-actor.h
INST_H_FILES += ppg-task.h
INST_H_FILES += ppg-task-render.h
INST_H_FILES += ppg-timeout-interval.h
INST_H_FILES += ppg-timer-tool-item.h
INST_H_FILES += ppg-util.h
INST_H_FILES += ppg-visualizer.h
INST_H_FILES += ppg-visualizer-simple.h
INST_H_FILES += ppg-visualizer-menu.h
INST_H_FILES += ppg-welcome-dialog.h
INST_H_FILES += ppg-window.h
INST_H_FILES += ppg-version.h

NOINST_H_FILES =
NOINST_H_FILES += $(builddir)/ppg-marshal.h
NOINST_H_FILES += ppg-private.h
NOINST_H_FILES += ppg-window-actions.h
NOINST_H_FILES += ppg-window-ui.h

#
# perfkit-profiler
#

perfkit_profiler_SOURCES =
perfkit_profiler_SOURCES += main.c

perfkit_profiler_CPPFLAGS =
perfkit_profiler_CPPFLAGS += $(INCLUDE_CFLAGS)
perfkit_profiler_CPPFLAGS += $(GTK_CFLAGS)
perfkit_profiler_CPPFLAGS += -DPRODUCT_NAME="\"Perfkit Profiler\""
perfkit_profiler_CPPFLAGS += -DG_LOG_DOMAIN=\""Profiler"\"

perfkit_profiler_LDADD =
perfkit_profiler_LDADD += libprofiler.la

perfkit_profiler_DEPENDENCIES =
perfkit_profiler_DEPENDENCIES += libprofiler.la

#
# Marshalers
#

$(builddir)/ppg-marshal.c $(builddir)/ppg-marshal.h: ppg-marshal.list
	@$(GLIB_GENMARSHAL) --header --prefix=ppg_cclosure_marshal	\
	$(srcdir)/ppg-marshal.list > $(builddir)/ppg-marshal.h
	@$(GLIB_GENMARSHAL) --body --prefix=ppg_cclosure_marshal	\
	$(srcdir)/ppg-marshal.list > $(builddir)/ppg-marshal.c

#
# development helpers
#

run: perfkit-profiler
	@rm -f perfkit-profiler.log
	@PPG_DATA_DIR=$(top_srcdir)/data				\
	PERFKIT_CONNECTIONS_DIR=$(top_builddir)/perfkit/.libs		\
	GSETTINGS_SCHEMA_DIR=$(top_builddir)/data/settings		\
	libtool --mode=execute $(builddir)/perfkit-profiler		\
	--log=perfkit-profiler.log					\
	$(PERFKIT_OPTIONS)

debug: perfkit-profiler
	@rm -f perfkit-profiler.log
	@PPG_DATA_DIR=$(top_srcdir)/data				\
	PERFKIT_CONNECTIONS_DIR=$(top_builddir)/perfkit/.libs		\
	GSETTINGS_SCHEMA_DIR=$(top_builddir)/data/settings		\
	libtool --mode=execute gdb --args $(builddir)/perfkit-profiler	\
	--log=perfkit-profiler.log					\
	$(PERFKIT_OPTIONS)
