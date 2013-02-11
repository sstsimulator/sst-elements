# -*- Makefile -*-
#
#

#if HAVE_STDCXX_0X

AM_CPPFLAGS = -Wall \
	$(BOOST_CPPFLAGS) \
	$(MPI_CPPFLAGS) 

compdir = $(pkglibdir)
comp_LTLIBRARIES = libsst_mcniagara.la
libsst_mcniagara_la_SOURCES = \
	sst_mcniagara.h \
	sst_mcniagara.cpp \
	mcniagara/CycleTracker.cc \
	mcniagara/MemoryModel.cc \
	mcniagara/Dependency.cc \
	mcniagara/McNiagara.cc \
	mcniagara/stringutil.cc \
	mcniagara/McNiagara-sim.cc  
#	mcniagara/McNiagara-mem.cc \
#	mcniagara/readdefs.cc 

libsst_mcniagara_la_LDFLAGS = -module -avoid-version

#endif
