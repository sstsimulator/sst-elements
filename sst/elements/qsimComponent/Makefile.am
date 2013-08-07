# -*- Makefile -*-
#
#

AM_CPPFLAGS = \
	$(BOOST_CPPFLAGS) \
	$(MPI_CPPFLAGS) 

compdir = $(pkglibdir)
comp_LTLIBRARIES = libqsimComponent.la
libqsimComponent_la_SOURCES = \
	qsimComponent.h \
	qsimComponent.cc

EXTRA_DIST = makeSDL.pl

libqsimComponent_la_LDFLAGS = -module -avoid-version
libqsimComponent_la_LIBADD = -lqsim
