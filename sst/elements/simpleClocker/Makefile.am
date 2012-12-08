# -*- Makefile -*-
#
#

AM_CPPFLAGS = \
	$(BOOST_CPPFLAGS) \
	$(MPI_CPPFLAGS) 

compdir = $(pkglibdir)
comp_LTLIBRARIES = libsimpleClockerComponent.la
libsimpleClockerComponent_la_SOURCES = \
	simpleClocker.h \
	simpleClocker.cc

libsimpleClockerComponent_la_LDFLAGS = -module -avoid-version
