# -*- Makefile -*-
#
#

AM_CPPFLAGS = \
	$(MPI_CPPFLAGS) 

compdir = $(pkglibdir)
comp_LTLIBRARIES = libGNA.la
libGNA_la_SOURCES = \
	gna_lib.h \
	neuron.h \
	sts.h \
	sts.cc \
	GNA.cc \
	GNA.h

EXTRA_DIST = \
    README 


libGNA_la_LDFLAGS = -module -avoid-version

##########################################################################
##########################################################################
##########################################################################


