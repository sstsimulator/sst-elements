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
	chdlComponent.cc

EXTRA_DIST = README test/acpu.nand test/test.py

libchdlComponent_la_LDFLAGS = -module -avoid-version $(CHDL_LDFLAGS)
libchdlComponent_la_LIBADD = -lchdl -lchdl-module
