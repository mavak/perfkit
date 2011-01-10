include $(top_srcdir)/Makefile.inc

bin_PROGRAMS =
bin_PROGRAMS += perfkit-shell

bin_SCRIPTS =
bin_SCRIPTS += perfkit-support

#
# perfkit-shell
#

perfkit_shell_SOURCES =
perfkit_shell_SOURCES += perfkit-shell.c
perfkit_shell_SOURCES += $(top_srcdir)/cut-n-paste/egg-fmt.c
perfkit_shell_SOURCES += $(top_srcdir)/cut-n-paste/egg-line.c

perfkit_shell_CPPFLAGS =
perfkit_shell_CPPFLAGS += $(INCLUDE_CFLAGS)
perfkit_shell_CPPFLAGS += $(GOBJECT_CFLAGS)
perfkit_shell_CPPFLAGS += -I$(top_srcdir)/cut-n-paste
perfkit_shell_CPPFLAGS += -I$(top_srcdir)
perfkit_shell_CPPFLAGS += -DG_LOG_DOMAIN=\"Shell\"

perfkit_shell_LDFLAGS =
perfkit_shell_LDFLAGS += $(PERFKIT_TOOLS_LIBS)
perfkit_shell_LDFLAGS += -lreadline

perfkit_shell_LDADD = $(top_builddir)/perfkit/libperfkit-1.0.la

#
# development helpers

debug-shell:
	@PERFKIT_CONNECTIONS_DIR=$(top_builddir)/perfkit/.libs			\
	libtool --mode=execute gdb $(builddir)/perfkit-shell

run-shell:
	@PERFKIT_CONNECTIONS_DIR=$(top_builddir)/perfkit/.libs			\
	libtool --mode=execute $(builddir)/perfkit-shell

