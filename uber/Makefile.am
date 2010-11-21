include $(top_srcdir)/Makefile.inc

noinst_LTLIBRARIES = libuber.la

#
# libuber graphing library
#

NOINST_H_FILES =
NOINST_H_FILES += g-ring.h
NOINST_H_FILES += uber.h
NOINST_H_FILES += uber-frame-source.h
NOINST_H_FILES += uber-graph.h
NOINST_H_FILES += uber-heat-map.h
NOINST_H_FILES += uber-label.h
NOINST_H_FILES += uber-line-graph.h
NOINST_H_FILES += uber-range.h
NOINST_H_FILES += uber-scale.h
NOINST_H_FILES += uber-scatter.h
NOINST_H_FILES += uber-timeout-interval.h

libuber_la_SOURCES =
libuber_la_SOURCES += $(NOINST_H_FILES)
libuber_la_SOURCES += g-ring.c
libuber_la_SOURCES += uber-frame-source.c
libuber_la_SOURCES += uber-graph.c
libuber_la_SOURCES += uber-heat-map.c
libuber_la_SOURCES += uber-label.c
libuber_la_SOURCES += uber-line-graph.c
libuber_la_SOURCES += uber-range.c
libuber_la_SOURCES += uber-scale.c
libuber_la_SOURCES += uber-scatter.c
libuber_la_SOURCES += uber-timeout-interval.c

libuber_la_CPPFLAGS =
libuber_la_CPPFLAGS += $(INCLUDE_CFLAGS)
libuber_la_CPPFLAGS += $(GTK_CFLAGS)
libuber_la_CPPFLAGS += -DG_LOG_DOMAIN=\"Uber\"

libuber_la_LIBADD =
libuber_la_LIBADD += $(GTK_LIBS)
