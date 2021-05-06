# -*- Makefile -*-
#
#

AM_CPPFLAGS = \
	$(MPI_CPPFLAGS)

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

if !SST_ENABLE_PREVIEW_BUILD
libllyr_la_SOURCES += $(deprecated_libsimpleElementExample_sources)
endif

EXTRA_DIST = \
	tests/llyr_test.py

deprecated_EXTRA_DIST = 

if !SST_ENABLE_PREVIEW_BUILD
EXTRA_DIST += $(deprecated_EXTRA_DIST)
endif


libllyr_la_LDFLAGS = -module -avoid-version

install-exec-hook:
	$(SST_REGISTER_TOOL) SST_ELEMENT_SOURCE     llyr=$(abs_srcdir)
	$(SST_REGISTER_TOOL) SST_ELEMENT_TESTS      llyr=$(abs_srcdir)/tests

##########################################################################
##########################################################################
##########################################################################


