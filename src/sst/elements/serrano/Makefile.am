# -*- Makefile -*-
#
#

AM_CPPFLAGS = \
	$(MPI_CPPFLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libserrano.la
libserrano_la_SOURCES = \
	scircq.h \
	sercgunit.h \
	seriterunit.h \
	serprintunit.h \
	serrano.cc \
	serrano.h \
	serstdunit.h \
	smsg.h	

EXTRA_DIST = \
	tests/test_serrano.py \
	tests/graphs/sum.graph

libserrano_la_LDFLAGS = -module -avoid-version

install-exec-hook:
	$(SST_REGISTER_TOOL) SST_ELEMENT_SOURCE     serrano=$(abs_srcdir)
	$(SST_REGISTER_TOOL) SST_ELEMENT_TESTS      serrano=$(abs_srcdir)/tests

