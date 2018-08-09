# -*- Makefile -*-
#
#

AM_CPPFLAGS = \
	$(MPI_CPPFLAGS) 

compdir = $(pkglibdir)
comp_LTLIBRARIES = libSTPU.la
libSTPU_la_SOURCES = \
	stpu_lib.h \
	neuron.h \
	sts.h \
	STPU.cc \
	STPU.h

EXTRA_DIST = \
    README 


libSTPU_la_LDFLAGS = -module -avoid-version

##########################################################################
##########################################################################
##########################################################################


