# -*- Makefile -*-
#
#

AM_CPPFLAGS = \
	$(BOOST_CPPFLAGS) \
	$(MPI_CPPFLAGS) 

compdir = $(pkglibdir)
comp_LTLIBRARIES = libcassini.la
libcassini_la_SOURCES = \
	cassini.cc \
	strideprefetch.cc \
	strideprefetch.h \
	nbprefetch.cc \
	nbprefetch.h \
	simpletlb.h \
	simpletlb.cc \
	pageentry.h \
	pageentry.cc

EXTRA_DIST = \
	tests/streamcpu.xml \
	tests/streamcpu-sp.xml

libcassini_la_LDFLAGS = -module -avoid-version
