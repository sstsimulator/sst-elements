# -*- Makefile -*-
#
#

AM_CPPFLAGS = \
	$(BOOST_CPPFLAGS) \
	$(MPI_CPPFLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libsimpleRNGComponent.la
libsimpleRNGComponent_la_SOURCES = \
	simpleRNGComponent.h \
	simpleRNGComponent.cc
EXTRA_DIST = \
	mersenne.xml \
	marsaglia.xml    

libsimpleRNGComponent_la_LDFLAGS = -module -avoid-version
