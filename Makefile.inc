WARNINGS =
WARNINGS += -Wall
WARNINGS += -Werror
WARNINGS += -Wold-style-definition
WARNINGS += -Wdeclaration-after-statement
WARNINGS += -Wredundant-decls
WARNINGS += -Wmissing-noreturn
WARNINGS += -Wcast-align
WARNINGS += -Wwrite-strings
WARNINGS += -Winline
WARNINGS += -Wformat-nonliteral
WARNINGS += -Wformat-security
WARNINGS += -Wswitch-enum
WARNINGS += -Wswitch-default
WARNINGS += -Winit-self
WARNINGS += -Wmissing-include-dirs
WARNINGS += -Wundef
WARNINGS += -Waggregate-return
WARNINGS += -Wmissing-format-attribute
WARNINGS += -Wnested-externs
WARNINGS += -Wshadow

INCLUDE_CFLAGS =
INCLUDE_CFLAGS += -DGETTEXT_PACKAGE=\""perfkit"\"
if HAVE_AVAHI
INCLUDE_CFLAGS += -DHAVE_AVAHI
endif
INCLUDE_CFLAGS += -DPACKAGE_DATA_DIR=\""$(datadir)"\"
INCLUDE_CFLAGS += -DPACKAGE_LIB_DIR=\""$(libdir)"\"
INCLUDE_CFLAGS += -DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)s/locale"\"
INCLUDE_CFLAGS += -DPACKAGE_SRC_DIR=\""$(srcdir)"\"
INCLUDE_CFLAGS += -DPACKAGE_SYSCONFDIR=\""$(sysconfdir)"\"
INCLUDE_CFLAGS += -DPERFKIT_COMPILATION
INCLUDE_CFLAGS += -I$(top_srcdir)
INCLUDE_CFLAGS += -I$(top_srcdir)/cut-n-paste
INCLUDE_CFLAGS += -I$(top_builddir)/data
INCLUDE_CFLAGS += -I$(top_builddir)/perfkit
INCLUDE_CFLAGS += $(PERFKIT_DEBUG_CFLAGS)
INCLUDE_CFLAGS += $(WARNINGS)
