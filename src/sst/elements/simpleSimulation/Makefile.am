# -*- Makefile -*-
#
#

AM_CPPFLAGS = \
	$(MPI_CPPFLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libsimpleSimulation.la
libsimpleSimulation_la_SOURCES = \
	simpleCarWash.h \
	simpleCarWash.cc

EXTRA_DIST = \
    README \
    tests/test_simpleSimulationCarWash.py \
    tests/testsuite_default_simpleSimulation.py

libsimpleSimulation_la_LDFLAGS = -module -avoid-version

install-exec-hook:
	$(SST_REGISTER_TOOL) SST_ELEMENT_SOURCE     simpleSimulation=$(abs_srcdir)
	$(SST_REGISTER_TOOL) SST_ELEMENT_TESTS      simpleSimulation=$(abs_srcdir)/tests

##########################################################################
##########################################################################
##########################################################################


