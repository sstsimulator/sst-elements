# -*- Makefile -*-
#
#

AM_CPPFLAGS = \
	$(BOOST_CPPFLAGS) \
	$(MPI_CPPFLAGS) 

compdir = $(pkglibdir)
comp_LTLIBRARIES = libcpu_data.la
libcpu_data_la_SOURCES = \
	cpu_data.h \
	cpu_data.cc \
	simpleEvent.h 
libcpu_data_la_LDFLAGS = -module -avoid-version
