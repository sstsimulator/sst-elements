# -*- Makefile -*-
#
#

AM_CPPFLAGS = \
	$(BOOST_CPPFLAGS) \
	$(MPI_CPPFLAGS) 

compdir = $(pkglibdir)
comp_LTLIBRARIES = libtracerComponent.la
libtracerComponent_la_SOURCES = \
	tracerComponent.h \
	tracerComponent.cc

EXTRA_DIST = \
    README \
    tests/test_tracerComponent_1.py \
    tests/test_tracerComponent_2.py

libtracerComponent_la_LDFLAGS = -module -avoid-version

##########################################################################
##########################################################################
##########################################################################

