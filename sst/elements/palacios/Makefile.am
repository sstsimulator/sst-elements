# -*- Makefile -*-
#
#

EXTRA_DIST = config.xml

AM_CPPFLAGS = \
    $(BOOST_CPPFLAGS)\
    $(MPI_CPPFLAGS)\
    $(PALACIOS_CPPFLAGS)\
    -I$(top_srcdir)/sst/elements

compdir = $(pkglibdir)
comp_LTLIBRARIES = libpalacios.la
libpalacios_la_SOURCES = \
	lib.cc \
	palacios.h \
	palacios.cc \
	ptlNicMMIF.h \
	ptlNicMMIF.cc

libpalacios_la_LDFLAGS = -module -avoid-version $(PALACIOS_LDFLAGS)
libpalacios_la_LIBADD = $(PALACIOS_LIBS) -lpthread
