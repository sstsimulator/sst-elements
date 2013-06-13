# -*- Makefile -*-
#
#

AM_CPPFLAGS = \
	$(BOOST_CPPFLAGS) \
	$(MPI_CPPFLAGS)

libzodiac_la_CPPFLAGS = \
	$(DUMPI_CPPFLAGS) \
	$(BOOST_CPPFLAGS) \
	$(MPI_CPPFLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libzodiac.la
libzodiac_la_SOURCES = \
	zodiac.cc \
	ztrace.cc \
	ztrace.h \
	zdumpi.cc \
	zdumpi.h \
	dumpireader.h \
	dumpireader.cc \
	dumpifunc.h

libzodiac_la_LDFLAGS = -module -avoid-version $(DUMPI_LDFLAGS) -lundumpi
