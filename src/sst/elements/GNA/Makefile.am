# -*- Makefile -*-
#
#

AM_CPPFLAGS += \
	$(MPI_CPPFLAGS) \
	-I$(top_srcdir)/src

compdir = $(pkglibdir)
comp_LTLIBRARIES = libGNA.la
libGNA_la_SOURCES = \
	neuron.h \
	neuron.cc \
	GNA.h \
	GNA.cc \
	OutputHolder.h

EXTRA_DIST = \
	README \
	tests/testsuite_default_GNA.py \
	tests/test_GNA_1.py \
	tests/model \
	tests/OutputParser.py

libGNA_la_LDFLAGS = -module -avoid-version

install-exec-hook:
	$(SST_REGISTER_TOOL) SST_ELEMENT_SOURCE     GNA=$(abs_srcdir)
	$(SST_REGISTER_TOOL) SST_ELEMENT_TESTS      GNA=$(abs_srcdir)/tests

