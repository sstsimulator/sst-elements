# -*- Makefile -*-
#
#
AM_CPPFLAGS += \
	$(MPI_CPPFLAGS) \
	-I$(top_srcdir)/src

compdir = $(pkglibdir)
comp_LTLIBRARIES = libllyr.la
libllyr_la_SOURCES = \
	llyr.h \
	llyr.cc \
	llyrTypes.h \
	llyrHelpers.h \
	lsQueue.h \
	graph/graph.h \
	graph/edge.h \
	graph/vertex.h \
	mappers/csvParser.h \
	mappers/mapperList.h \
	mappers/llyrMapper.h \
	mappers/simpleMapper.h \
	mappers/pyMapper.h \
	pes/peList.h \
	pes/processingElement.h \
	pes/dummyPE.h \
	pes/fpPE.h \
	pes/intPE.h \
	pes/logicPE.h \
	pes/loadPE.h \
	pes/storePE.h \
	pes/complexPE.h \
	pes/controlPE.h

deprecated_libllyr_sources =

libllyr_la_CFLAGS = \
	$(AM_CFLAGS) \
	$(LLVM_CFLAGS)

libllyr_la_CXXFLAGS = \
	$(AM_CXXFLAGS) \
	$(LLVM_CXXFLAGS) -fexceptions

libllyr_la_CPPFLAGS = \
	-I$(top_srcdir)/src \
	$(AM_CPPFLAGS) -fPIC -Wall \
	$(MPI_CPPFLAGS) \
	$(LLVM_CPPFLAGS)

libllyr_la_LDFLAGS = \
	-module \
	-avoid-version \
	$(AM_LDFLAGS) \
	$(LLVM_LDFLAGS)

if !SST_ENABLE_PREVIEW_BUILD
libllyr_la_SOURCES += $(deprecated_libsimpleElementExample_sources)
endif

EXTRA_DIST = \
	tests/simple_test.py

deprecated_EXTRA_DIST =

if !SST_ENABLE_PREVIEW_BUILD
EXTRA_DIST += $(deprecated_EXTRA_DIST)
endif

install-exec-hook:
	$(SST_REGISTER_TOOL) SST_ELEMENT_SOURCE     llyr=$(abs_srcdir)
	$(SST_REGISTER_TOOL) SST_ELEMENT_TESTS      llyr=$(abs_srcdir)/tests


