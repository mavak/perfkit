all: $(builddir)/gschemas.compiled

SCHEMAS =					\
	org.perfkit.profiler.gschema.xml	\
	$(NULL)

$(builddir)/gschemas.compiled: $(SCHEMAS)
	glib-compile-schemas $(top_srcdir)/data/settings --targetdir=$(builddir)

# TODO: Make this shiz install
