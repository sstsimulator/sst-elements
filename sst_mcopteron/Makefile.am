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
	mcopteron/McOpteron.h \
	mcopteron/ConfigVars.cc \
	mcopteron/ConfigVars.h \
	mcopteron/InstructionInfo.cc \
	mcopteron/InstructionInfo.h \
	mcopteron/ReorderBuffer.cc \
	mcopteron/ReorderBuffer.h \
	mcopteron/Dependency.cc \
	mcopteron/Dependency.h \
	mcopteron/InstructionQueue.cc \
	mcopteron/InstructionQueue.h \
	mcopteron/MemoryModel.cc \
	mcopteron/MemoryModel.h \
	mcopteron/Token.cc \
	mcopteron/Token.h \
	mcopteron/FunctionalUnit.cc \
	mcopteron/FunctionalUnit.h \
	mcopteron/LoadStoreUnit.cc \
	mcopteron/LoadStoreUnit.h \
	mcopteron/Random.cc \
	mcopteron/MarkovModel.cc \
	mcopteron/MarkovModel.h \
	mcopteron/OpteronDefs.cc \
	mcopteron/OpteronDefs.h \
	mcopteron/Listener.h \
	mcopteron/mersenne.h

EXTRA_DIST = \
	test1.py \
        test2.py \
        cpuconfig.ini \
        appconfig.ini \
        mcopteron/agner-insn.txt \
        mcopteron/instrSizeDistr.txt \
        mcopteron/instrMix.txt \
        mcopteron/instrSizeDistr.txt \
        mcopteron/transition_prob.txt \
        mcopteron/trace.txt \
        mcopteron/usedist_new.all

libsst_mcopteron_la_LDFLAGS = -module -avoid-version
