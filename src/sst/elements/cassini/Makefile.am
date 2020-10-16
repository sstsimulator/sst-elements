# -*- Makefile -*-
#
#

AM_CPPFLAGS = \
	$(MPI_CPPFLAGS) \
	-I$(top_srcdir)/src

compdir = $(pkglibdir)
comp_LTLIBRARIES = libcassini.la
libcassini_la_SOURCES = \
	strideprefetch.cc \
	strideprefetch.h \
	palaprefetch.h \
	palaprefetch.cc \
	nbprefetch.cc \
	nbprefetch.h \
	pageentry.h \
	pageentry.cc \
	addrHistogrammer.cc \
	addrHistogrammer.h \
	cacheLineTrack.cc \
	cacheLineTrack.h

EXTRA_DIST = \
	tests/testsuite_default_cassini_prefetch.py \
	tests/streamcpu-nbp.py \
	tests/streamcpu-nopf.py \
	tests/streamcpu-sp.py \
    tests/refFiles/test_cassini_prefetch.out \
    tests/refFiles/test_cassini_prefetch_nbp.out \
    tests/refFiles/test_cassini_prefetch_nopf.out \
    tests/refFiles/test_cassini_prefetch_pp.out \
    tests/refFiles/test_cassini_prefetch_sp.out \
    tests/refFiles/test_cassini_stride_prefetch.out

libcassini_la_LDFLAGS = -module -avoid-version

install-exec-hook:
	$(SST_REGISTER_TOOL) SST_ELEMENT_SOURCE     cassini=$(abs_srcdir)
	$(SST_REGISTER_TOOL) SST_ELEMENT_TESTS      cassini=$(abs_srcdir)/tests
