SUBDIRS =
SUBDIRS += data
SUBDIRS += perfkit-agent
SUBDIRS += perfkit
SUBDIRS += tools
SUBDIRS += uber
SUBDIRS += perfkit-gui
SUBDIRS += perfkit-profiler
SUBDIRS += bindings
SUBDIRS += tests
SUBDIRS += doc

ACLOCAL_AMFLAGS = -I build/m4 ${ACLOCAL_FLAGS}
DISTCHECK_CONFIGURE_FLAGS = --enable-maintainer-flags --enable-gtk-doc --enable-introspection

perfkitdocdir = ${prefix}/doc/perfkit
perfkitdoc_DATA =
perfkitdoc_DATA += README
perfkitdoc_DATA += COPYING
perfkitdoc_DATA += AUTHORS
perfkitdoc_DATA += ChangeLog
perfkitdoc_DATA += INSTALL
perfkitdoc_DATA += NEWS

EXTRA_DIST = $(perfkitdoc_DATA)

dist-hook:
	@if test -d "$(srcdir)/.git"; then                              \
          (cd "$(srcdir)" &&                                            \
           $(top_srcdir)/missing --run git log --stat ) > ChangeLog.tmp \
           && mv -f ChangeLog.tmp $(top_distdir)/ChangeLog              \
           || (rm -f ChangeLog.tmp;                                     \
               echo Failed to generate ChangeLog >&2);                  \
	else                                                            \
	  echo A git checkout is required to generate a ChangeLog >&2;  \
	fi

uninstall-local:
	-rm -r $(perfkitdocdir)
