# -*- Makefile -*-
#
#


AM_CPPFLAGS = -Wall \
	$(BOOST_CPPFLAGS) \
	$(MPI_CPPFLAGS) 

compdir = $(pkglibdir)
comp_LTLIBRARIES = libsst_mcniagara.la
libsst_mcniagara_la_SOURCES = \
	sst_mcniagara.h \
	sst_mcniagara.cpp \
	mcniagara/CycleTracker.cc \
	mcniagara/CycleTracker.h \
	mcniagara/MemoryModel.cc \
	mcniagara/MemoryModel.h \
	mcniagara/Dependency.cc \
	mcniagara/Dependency.h \
	mcniagara/McNiagara.cc \
	mcniagara/McNiagara.h \
	mcniagara/stringutil.cc \
	mcniagara/stringutil.h \
	mcniagara/McNiagara-sim.cc \
	mcniagara/McNiagara.h \
	mcniagara/mersenne.h \
	mcniagara/McSimDefs.h \
	mcniagara/OffCpuIF.h
#	mcniagara/McNiagara-mem.cc \
#	mcniagara/readdefs.cc 

EXTRA_DIST = \
	test1.xml \
        test2.xml \
	test1.py \
        test2.py \
	mcniagara/inst_prob.data \
	mcniagara/perf_cnt.data \
	mcniagara/INPUT \
	mcniagara/sample_trace.txt

libsst_mcniagara_la_LDFLAGS = -module -avoid-version

