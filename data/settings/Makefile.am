gsettings_SCHEMAS =
gsettings_SCHEMAS += org.perfkit.profiler.gschema.xml

$(builddir)/gschemas.compiled: $(gsettings_SCHEMAS)
	glib-compile-schemas $(srcdir) --targetdir=$(builddir)

@GSETTINGS_RULES@
