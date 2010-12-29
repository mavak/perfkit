include $(top_srcdir)/Makefile.decl

noinst_PROGRAMS =
noinst_PROGRAMS += test-pk-connection

TEST_PROGS += test-pk-connection

AM_CPPFLAGS =
AM_CPPFLAGS += $(GIO_CFLAGS)
AM_CPPFLAGS += -I$(top_srcdir)
AM_CPPFLAGS += -I$(top_srcdir)/cut-n-paste
AM_CPPFLAGS += -I$(top_builddir)/perfkit
AM_CPPFLAGS += -DPERFKIT_CONNECTIONS_DIR="\"$(top_builddir)/perfkit/.libs\""

AM_LDFLAGS =
AM_LDFLAGS += $(GIO_LIBS)
AM_LDFLAGS += $(top_builddir)/perfkit/libperfkit-1.0.la

test_pk_connection_SOURCES = test-pk-connection.c
