# -*- Makefile -*-
#
#

AM_CPPFLAGS += \
	$(MPI_CPPFLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libsimpleSimulation.la
libsimpleSimulation_la_SOURCES = \
	simpleCarWash/simpleCarWash.h \
	simpleCarWash/simpleCarWash.cc \
	distCarWash/distCarEvent.h \
	distCarWash/distCarGenerator.h \
	distCarWash/distCarGenerator.cc \
	distCarWash/distCarWash.h \
	distCarWash/distCarWash.cc

EXTRA_DIST = \
    README \
    tests/test_simpleCarWash.py \
    tests/test_distCarWash.py \
    tests/testsuite_default_simpleSimulation.py \
    tests/refFiles/test_simpleCarWash.out \
    tests/refFiles/test_distCarWash.out

libsimpleSimulation_la_LDFLAGS = -module -avoid-version

install-exec-hook:
	$(SST_REGISTER_TOOL) SST_ELEMENT_SOURCE     simpleSimulation=$(abs_srcdir)
	$(SST_REGISTER_TOOL) SST_ELEMENT_TESTS      simpleSimulation=$(abs_srcdir)/tests

