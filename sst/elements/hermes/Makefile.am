# -*- Makefile -*-
#
#

AM_CPPFLAGS = \
	$(BOOST_CPPFLAGS) \
	$(MPI_CPPFLAGS) 

compdir = $(pkglibdir)
comp_LTLIBRARIES = libhermes.la
libhermes_la_SOURCES = \
	msgapi.h \
	hermes.cc \
	hermes.h \
	functor.h \
	shmemapi.h

libhermes_la_LDFLAGS = -module -avoid-version
