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
	nbprefetch.cc \
	nbprefetch.h 

libcassini_la_LDFLAGS = -module -avoid-version
