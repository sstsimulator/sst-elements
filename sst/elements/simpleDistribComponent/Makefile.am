# -*- Makefile -*-
#
#

AM_CPPFLAGS = \
	$(BOOST_CPPFLAGS) \
	$(MPI_CPPFLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libsimpleDistribComponent.la
libsimpleDistribComponent_la_SOURCES = \
	simpleDistribComponent.h \
	simpleDistribComponent.cc

libsimpleDistribComponent_la_LDFLAGS = -module -avoid-version

EXTRA_DIST = \
	simpleexpon.py \
	simplegaussian.py
