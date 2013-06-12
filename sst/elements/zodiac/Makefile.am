# -*- Makefile -*-
#
#

AM_CPPFLAGS = \
	$(BOOST_CPPFLAGS) \
	$(MPI_CPPFLAGS) 

compdir = $(pkglibdir)
comp_LTLIBRARIES = libzodiac.la
libzodiac_la_SOURCES = \
	zodiac.cc \
	ztrace.cc \
	ztrace.h

libzodiac_la_LDFLAGS = -module -avoid-version
