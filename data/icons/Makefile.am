all: $(ICONS)

%.h: %.png
	@$(GDK_PIXBUF_CSOURCE) --name=$*_pixbuf $*.png > tmp-$*.h
	@mv tmp-$*.h $*.h

ICONS =
ICONS += perfkit.h
ICONS += plugin.h
ICONS += pointer.h
ICONS += selection.h

EXTRA_DIST = $(ICONS)
CLEANFILES = $(ICONS)
GDK_PIXBUF_CSOURCE = gdk-pixbuf-csource
