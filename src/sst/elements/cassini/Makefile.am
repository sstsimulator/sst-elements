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
	tests/streamcpu-nbp.py \
	tests/streamcpu-nopf.py \
	tests/streamcpu-sp.py

libcassini_la_LDFLAGS = -module -avoid-version
