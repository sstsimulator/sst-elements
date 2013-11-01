# -*- Makefile -*-
#
#

AM_CPPFLAGS = \
	$(BOOST_CPPFLAGS) \
	$(MPI_CPPFLAGS) -DNOTSTANDALONE

compdir = $(pkglibdir)
comp_LTLIBRARIES = libsst_mcopteron.la
libsst_mcopteron_la_SOURCES = \
	sst_mcopteron.h \
	sst_mcopteron.cpp \
	mcopteron/McOpteron.cc \
	mcopteron/ConfigVars.cc \
	mcopteron/InstructionInfo.cc \
	mcopteron/ReorderBuffer.cc \
	mcopteron/Dependency.cc \
	mcopteron/InstructionQueue.cc \
	mcopteron/MemoryModel.cc \
	mcopteron/Token.cc \
	mcopteron/FunctionalUnit.cc \
	mcopteron/LoadStoreUnit.cc \
	mcopteron/Random.cc \
	mcopteron/MarkovModel.cc \
	mcopteron/OpteronDefs.cc

libsst_mcopteron_la_LDFLAGS = -module -avoid-version
