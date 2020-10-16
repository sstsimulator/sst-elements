# -*- Makefile -*-
#
#

AM_CPPFLAGS = \
	$(MPI_CPPFLAGS) \
	-I$(top_srcdir)/src

compdir = $(pkglibdir)
comp_LTLIBRARIES = libcacheTracer.la
libcacheTracer_la_SOURCES = \
	cacheTracer.h \
	cacheTracer.cc

EXTRA_DIST = \
    README \
    tests/testsuite_default_cacheTracer.py \
    tests/test_cacheTracer_1.py \
    tests/test_cacheTracer_2.py \
    tests/refFiles/test_cacheTracer_1.out \
    tests/refFiles/test_cacheTracer_2_memRef.out

libcacheTracer_la_LDFLAGS = -module -avoid-version

install-exec-hook:
	$(SST_REGISTER_TOOL) SST_ELEMENT_SOURCE     cacheTracer=$(abs_srcdir)
	$(SST_REGISTER_TOOL) SST_ELEMENT_TESTS      cacheTracer=$(abs_srcdir)/tests

##########################################################################
##########################################################################
##########################################################################

