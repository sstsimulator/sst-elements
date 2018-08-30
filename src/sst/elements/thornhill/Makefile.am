# -*- Makefile -*-
#
#

AM_CPPFLAGS = $(MPI_CPPFLAGS) \
	-I$(top_srcdir)/src

compdir = $(pkglibdir)
comp_LTLIBRARIES = libthornhill.la

libthornhill_la_SOURCES = \
	libthornhill.cc\
	detailedCompute.h\
	memoryHeap.h\
	memoryHeap.cc\
	memoryHeapLink.h\
	singleThread.h\
	memoryHeapEvent.h\
	memoryHeapLink.h\
	singleThread.cc\
	types.h

libthornhill_la_LDFLAGS = -module -avoid-version
