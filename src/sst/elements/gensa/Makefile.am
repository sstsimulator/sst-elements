# -*- Makefile -*-
#
#

AM_CPPFLAGS += \
	$(MPI_CPPFLAGS) \
	-I$(top_srcdir)/src

compdir = $(pkglibdir)
comp_LTLIBRARIES = libgensa.la
libgensa_la_SOURCES = \
	neuron.h \
	neuron.cc \
	gensa.h \
	gensa.cc \
	OutputHolder.h

EXTRA_DIST = \
	README \
	tests/testsuite_default_gensa.py \
	tests/test_gensa_1.py \
	tests/model \
	tests/OutputParser.py

libgensa_la_LDFLAGS = -module -avoid-version

install-exec-hook:
	$(SST_REGISTER_TOOL) SST_ELEMENT_SOURCE     gensa=$(abs_srcdir)
	$(SST_REGISTER_TOOL) SST_ELEMENT_TESTS      gensa=$(abs_srcdir)/tests

