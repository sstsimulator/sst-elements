# -*- Makefile -*-
#
#

AM_CPPFLAGS = \
	$(BOOST_CPPFLAGS) \
	$(MPI_CPPFLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libsimpleStatistics.la
libsimpleStatistics_la_SOURCES = \
	simpleStatistics.h \
	simpleStatistics.cc

EXTRA_DIST = test.py

libsimpleStatistics_la_LDFLAGS = -module -avoid-version
