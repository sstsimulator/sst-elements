AM_YFLAGS = -d
if ENABLE_CPSP

INCLUDES = -I$(top_builddir)

CPROPSLIB = $(top_builddir)/libcprops.la

cpspdir = $(bindir)
bin_PROGRAMS = cpsp
cpsp_LDADD = @LEXLIB@ $(CPROPSLIB)
cpsp_SOURCES = parser.y cpsp.l
bin_SCRIPTS = cpsp-gen.sh

EXTRA_DIST = $(cpsp_SCRIPTS) \
			 Makefile.vc \
			 Makefile.cpsp.vc \
			 bison.simple \
			 cpsp-gen.bat

clean-local:
	$(RM) cpsp.c parser.c parser.h
maintainer-clean-local:
	$(RM) cpsp.h

endif
