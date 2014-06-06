# -*- Makefile -*-
#
#

AM_CPPFLAGS = \
	$(BOOST_CPPFLAGS) \
	$(MPI_CPPFLAGS) \
	$(CHDL_CPPFLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libchdlComponent.la
libchdlComponent_la_SOURCES = \
	chdlComponent.h \
	chdlComponent.cc \
	ldnetl.h \
	ldnetl.cc

EXTRA_DIST = gen_sdl.sh README test/acpu.nand

libchdlComponent_la_LDFLAGS = -module -avoid-version $(CHDL_LDFLAGS)
libchdlComponent_la_LIBADD = -lchdl
