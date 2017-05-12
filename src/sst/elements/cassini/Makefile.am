# -*- Makefile -*-
#
#

AM_CPPFLAGS = \
	$(MPI_CPPFLAGS) \
	-I$(top_srcdir)/src

compdir = $(pkglibdir)
comp_LTLIBRARIES = libcassini.la
libcassini_la_SOURCES = \
	cassini.cc \
	strideprefetch.cc \
	strideprefetch.h \
	nbprefetch.cc \
	nbprefetch.h \
	simpletlb.h \
	simpletlb.cc \
	pageentry.h \
	pageentry.cc \
	addrHistogrammer.cc \
	addrHistogrammer.h

EXTRA_DIST = \
	tests/streamcpu-nbp.py \
	tests/streamcpu-nopf.py \
	tests/streamcpu-sp.py

libcassini_la_LDFLAGS = -module -avoid-version
