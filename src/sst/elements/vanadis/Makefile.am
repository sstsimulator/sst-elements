# -*- Makefile -*-
#
#

AM_CPPFLAGS = \
	$(MPI_CPPFLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libvanadis.la
libvanadis_la_SOURCES = \
	vanadis.cc \
	vanadis.h \
	os/vcpuos.h \
	os/vmipscpuos.h \
	os/vosresp.h	

EXTRA_DIST = \
	tests/basic_miranda.py \
	tests/hierarchy_test.py

libvanadis_la_LDFLAGS = -module -avoid-version

install-exec-hook:
	$(SST_REGISTER_TOOL) SST_ELEMENT_SOURCE     vanadis=$(abs_srcdir)
	$(SST_REGISTER_TOOL) SST_ELEMENT_TESTS      vanadis=$(abs_srcdir)/tests
