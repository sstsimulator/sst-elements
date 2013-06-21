# -*- Makefile -*-
#
#

AM_CPPFLAGS = \
	$(BOOST_CPPFLAGS) \
	$(MPI_CPPFLAGS)

libzodiac_la_CPPFLAGS = \
	$(DUMPI_CPPFLAGS) \
	$(BOOST_CPPFLAGS) \
	$(MPI_CPPFLAGS) \
	$(shell $(OTF_CONFIG_TOOL) --includes)

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
	dumpifunc.h \
	otfreader.h \
	otfreader.cc \
	zotf.h \
	zotf.cc \
	zevent.h \
	zevent.cc

libzodiac_la_LDFLAGS = -module -avoid-version \
	$(shell $(OTF_CONFIG_TOOL) --libs)
