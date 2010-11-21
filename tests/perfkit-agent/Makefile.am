include $(top_srcdir)/Makefile.decl

noinst_PROGRAMS =							\
	test-pka-sample							\
	test-pka-manifest						\
	test-pka-encoder						\
	test-pka-source-simple						\
	$(NULL)

TEST_PROGS +=								\
	test-pka-sample							\
	test-pka-manifest						\
	test-pka-encoder						\
	test-pka-source-simple						\
	$(NULL)

AM_CPPFLAGS =								\
	$(GIO_CFLAGS)							\
	$(GMODULE_CFLAGS)						\
	$(GOBJECT_CFLAGS)						\
	-I$(top_srcdir)							\
	-I$(top_srcdir)/cut-n-paste					\
	$(WARNINGS)							\
	$(NULL)

AM_LDFLAGS =								\
	$(top_builddir)/perfkit-agent/libperfkit-agent.la		\
	$(GIO_LIBS)							\
	$(GMODULE_LIBS)							\
	$(GOBJECT_LIBS)							\
	$(NULL)

test_pka_sample_SOURCES = test-pka-sample.c $(top_srcdir)/cut-n-paste/egg-buffer.c
test_pka_manifest_SOURCES = test-pka-manifest.c
test_pka_encoder_SOURCES = test-pka-encoder.c
test_pka_source_simple_SOURCES = test-pka-source-simple.c
