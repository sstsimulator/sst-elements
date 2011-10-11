# -*- Makefile -*-
#
#

AM_CPPFLAGS = $(BOOST_CPPFLAGS) $(MPI_CPPFLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libiris.la

libiris_la_SOURCES = \
	iris.cc \
	RtrIF.h \
	genericBuffer.cc \
	genericBuffer.h \
	genericHeader.cc \
	genericHeader.h \
	SST_interface.h \
	genericRC.cc \
	genericRC.h \
	genericSwa.cc \
	genericSwa.h \
	genericVca.cc \
	genericVca.h \
	router.cc \
	router.h \
	router_params.h 

libiris_la_LDFLAGS = -module -avoid-version

