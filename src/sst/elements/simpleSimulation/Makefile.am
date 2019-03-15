# -*- Makefile -*-
#
#

AM_CPPFLAGS = \
	$(MPI_CPPFLAGS) 

compdir = $(pkglibdir)
comp_LTLIBRARIES = libsimpleSimulation.la
libsimpleSimulation_la_SOURCES = \
	simpleCarWash.h \
	simpleCarWash.cc
	
EXTRA_DIST = \
    README \
    tests/test_simpleCarWash.py 

libsimpleSimulation_la_LDFLAGS = -module -avoid-version

##########################################################################
##########################################################################
##########################################################################


