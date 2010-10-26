# -*- Makefile -*-
#
#

AM_CPPFLAGS = \
	$(BOOST_CPPFLAGS) \
	$(MPI_CPPFLAGS) 

compdir = $(pkglibdir)
comp_LTLIBRARIES = libsimpleComponent.la
libsimpleComponent_la_SOURCES = \
	simpleComponent.h \
	simpleComponent.cc \
	simpleEvent.h 

libsimpleComponent_la_LDFLAGS = -module -avoid-version
