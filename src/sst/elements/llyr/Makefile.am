# -*- Makefile -*-
#
#

compdir = $(pkglibdir)
comp_LTLIBRARIES = libllyr.la
libllyr_la_SOURCES = \
	llyrTypes.h \
	llyr.h \
	llyr.cc \
	graph/graph.h \
	graph/edge.h \
	graph/vertex.h \
	parser/parser.h \
	parser/parser.cc \
	mappers/llyrMapper.h \
	mappers/simpleMapper.h \
	mappers/pyMapper.h \
	mappers/gemmMapper.h \
	processingElement.h \
	pes/dummyPE.h \
	pes/fpPE.h \
	pes/intPE.h \
	pes/logicPE.h \
	pes/loadPE.h \
	pes/storePE.h \
	pes/complexPE.h \
	lsQueue.h

deprecated_libllyr_sources =

libllyr_la_CFLAGS = \
	$(AM_CLFAGS) \
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
	tests/llyr_test.py

deprecated_EXTRA_DIST = 

if !SST_ENABLE_PREVIEW_BUILD
EXTRA_DIST += $(deprecated_EXTRA_DIST)
endif

install-exec-hook:
	$(SST_REGISTER_TOOL) SST_ELEMENT_SOURCE     llyr=$(abs_srcdir)
	$(SST_REGISTER_TOOL) SST_ELEMENT_TESTS      llyr=$(abs_srcdir)/tests


