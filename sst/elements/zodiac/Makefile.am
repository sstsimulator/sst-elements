# -*- Makefile -*-
#
#

AM_CPPFLAGS = \
	$(BOOST_CPPFLAGS) \
	$(MPI_CPPFLAGS) 

compdir = $(pkglibdir)
comp_LTLIBRARIES = libzodiac.la
libzodiac_la_SOURCES = \
	zodiac.cc

libzodiac_la_LDFLAGS = -module -avoid-version
