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
libprofiler_la_SOURCES += ppg-color.c
libprofiler_la_SOURCES += ppg-configure-instrument-dialog.c
libprofiler_la_SOURCES += ppg-cpu-instrument.c
libprofiler_la_SOURCES += ppg-discover-dialog.c
libprofiler_la_SOURCES += ppg-edit-channel-task.c
libprofiler_la_SOURCES += ppg-gtk-instrument.c
libprofiler_la_SOURCES += ppg-header.c
libprofiler_la_SOURCES += ppg-instrument.c
libprofiler_la_SOURCES += ppg-instruments.c
libprofiler_la_SOURCES += ppg-line-visualizer.c
libprofiler_la_SOURCES += ppg-log.c
libprofiler_la_SOURCES += ppg-memory-instrument.c
libprofiler_la_SOURCES += ppg-menu-tool-item.c
libprofiler_la_SOURCES += ppg-model.c
libprofiler_la_SOURCES += ppg-monitor.c
libprofiler_la_SOURCES += ppg-paths.c
libprofiler_la_SOURCES += ppg-prefs.c
libprofiler_la_SOURCES += ppg-prefs-dialog.c
libprofiler_la_SOURCES += ppg-process-menu.c
libprofiler_la_SOURCES += ppg-restart-task.c
libprofiler_la_SOURCES += ppg-row.c
libprofiler_la_SOURCES += ppg-ruler.c
libprofiler_la_SOURCES += ppg-runtime.c
libprofiler_la_SOURCES += ppg-session.c
libprofiler_la_SOURCES += ppg-session-task.c
libprofiler_la_SOURCES += ppg-sessions-store.c
libprofiler_la_SOURCES += ppg-settings-dialog.c
libprofiler_la_SOURCES += ppg-spawn-process-dialog.c
libprofiler_la_SOURCES += ppg-status-actor.c
libprofiler_la_SOURCES += ppg-task.c
libprofiler_la_SOURCES += ppg-time-visualizer.c
libprofiler_la_SOURCES += ppg-time-span-visualizer.c
libprofiler_la_SOURCES += ppg-timer-tool-item.c
libprofiler_la_SOURCES += ppg-util.c
libprofiler_la_SOURCES += ppg-visualizer.c
libprofiler_la_SOURCES += ppg-visualizer-menu.c
libprofiler_la_SOURCES += ppg-welcome-dialog.c
libprofiler_la_SOURCES += ppg-window.c

libprofiler_la_CPPFLAGS =
libprofiler_la_CPPFLAGS += $(AVAHI_CFLAGS)
libprofiler_la_CPPFLAGS += $(CLUTTER_CFLAGS)
libprofiler_la_CPPFLAGS += $(GIO_CFLAGS)
libprofiler_la_CPPFLAGS += $(GTK_CFLAGS)
libprofiler_la_CPPFLAGS += $(INCLUDE_CFLAGS)
libprofiler_la_CPPFLAGS += -I$(top_srcdir)/uber
libprofiler_la_CPPFLAGS += -I$(top_srcdir)/cut-n-paste
libprofiler_la_CPPFLAGS += -DPRODUCT_NAME="\"Perfkit Profiler\""
libprofiler_la_CPPFLAGS += -DG_LOG_DOMAIN=\""Profiler"\"

libprofiler_la_LIBADD =
libprofiler_la_LIBADD += $(AVAHI_LIBS)
libprofiler_la_LIBADD += $(GIO_LIBS)
libprofiler_la_LIBADD += $(GTK_LIBS)
libprofiler_la_LIBADD += $(CLUTTER_LIBS)
libprofiler_la_LIBADD += $(top_builddir)/perfkit/libperfkit-1.0.la
libprofiler_la_LIBADD += $(top_builddir)/uber/libuber.la

INST_H_FILES =
INST_H_FILES += perfkit-profiler.h
INST_H_FILES += ppg-about-dialog.h
INST_H_FILES += ppg-actions.h
INST_H_FILES += ppg-add-instrument-dialog.h
INST_H_FILES += ppg-color.h
INST_H_FILES += ppg-configure-instrument-dialog.h
INST_H_FILES += ppg-cpu-instrument.h
INST_H_FILES += ppg-discover-dialog.h
INST_H_FILES += ppg-edit-channel-task.h
INST_H_FILES += ppg-gtk-instrument.h
INST_H_FILES += ppg-header.h
INST_H_FILES += ppg-instrument.h
INST_H_FILES += ppg-instruments.h
INST_H_FILES += ppg-line-visualizer.h
INST_H_FILES += ppg-log.h
INST_H_FILES += ppg-memory-instrument.h
INST_H_FILES += ppg-menu-tool-item.h
INST_H_FILES += ppg-model.h
INST_H_FILES += ppg-monitor.h
INST_H_FILES += ppg-paths.h
INST_H_FILES += ppg-prefs.h
INST_H_FILES += ppg-prefs-dialog.h
INST_H_FILES += ppg-process-menu.h
INST_H_FILES += ppg-restart-task.h
INST_H_FILES += ppg-row.h
INST_H_FILES += ppg-ruler.h
INST_H_FILES += ppg-runtime.h
INST_H_FILES += ppg-session.h
INST_H_FILES += ppg-session-task.h
INST_H_FILES += ppg-sessions-store.h
INST_H_FILES += ppg-settings-dialog.h
INST_H_FILES += ppg-spawn-process-dialog.h
INST_H_FILES += ppg-status-actor.h
INST_H_FILES += ppg-task.h
INST_H_FILES += ppg-time-visualizer.h
INST_H_FILES += ppg-time-span-visualizer.h
INST_H_FILES += ppg-timer-tool-item.h
INST_H_FILES += ppg-util.h
INST_H_FILES += ppg-visualizer.h
INST_H_FILES += ppg-visualizer-menu.h
INST_H_FILES += ppg-welcome-dialog.h
INST_H_FILES += ppg-window.h
INST_H_FILES += ppg-version.h

NOINST_H_FILES =
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
# development helpers
#

run: perfkit-profiler
	@rm -f perfkit-profiler.log
	@PPG_DATA_DIR=$(top_builddir)/data				\
	PERFKIT_CONNECTIONS_DIR=$(top_builddir)/perfkit/.libs		\
	GSETTINGS_SCHEMA_DIR=$(top_builddir)/data/settings		\
	libtool --mode=execute $(builddir)/perfkit-profiler		\
	--log=perfkit-profiler.log					\
	$(PERFKIT_OPTIONS)

debug: perfkit-profiler
	@rm -f perfkit-profiler.log
	@PPG_DATA_DIR=$(top_builddir)/data				\
	PERFKIT_CONNECTIONS_DIR=$(top_builddir)/perfkit/.libs		\
	GSETTINGS_SCHEMA_DIR=$(top_builddir)/data/settings		\
	libtool --mode=execute gdb --args $(builddir)/perfkit-profiler	\
	--log=perfkit-profiler.log					\
	$(PERFKIT_OPTIONS)
