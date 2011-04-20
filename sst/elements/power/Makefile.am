# -*- Makefile -*-
#
#

AM_CPPFLAGS = $(BOOST_CPPFLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libpower.la
libpower_la_SOURCES = interface.h power.cc power.h HotSpot-interface.cc HotSpot-interface.h IntSim-interface.cc IntSim-interface.h reliability.cc reliability.h
libpower_la_LDFLAGS = -module -avoid-version

EXTRA_DIST = README SSTrequiredPowerParams.pdf SSTrequiredStatistics.pdf
