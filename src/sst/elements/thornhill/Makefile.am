# -*- Makefile -*-
#
#

AM_CPPFLAGS = $(MPI_CPPFLAGS) \
	-I$(top_srcdir)/src


compdir = $(pkglibdir)
comp_LTLIBRARIES = libthornhill.la

sstdir = $(includedir)/sst/elements/thornhill

libthornhill_la_SOURCES = \
	libthornhill.cc\
	memoryHeap.h\
	memoryHeap.cc\
	singleThread.h\
	singleThread.cc

nobase_sst_HEADERS = \
    detailedCompute.h \
	memoryHeapEvent.h\
	memoryHeapLink.h \
	types.h

libthornhill_la_LDFLAGS = -module -avoid-version

install-exec-hook:
	$(SST_REGISTER_TOOL) SST_ELEMENT_SOURCE     thornhill=$(abs_srcdir)
