# -*- Makefile -*-
#
#

AM_CPPFLAGS = \
	$(MPI_CPPFLAGS) \
	-I$(top_srcdir)/src

compdir = $(pkglibdir)
comp_LTLIBRARIES = libhermes.la

sstdir = $(includedir)/sst/elements/hermes

libhermes_la_SOURCES = \
	libhermes.c

nobase_sst_HEADERS = \
	msgapi.h \
	miscapi.h \
	hermes.h \
	functor.h \
	shmemapi.h

libhermes_la_LDFLAGS = -module -avoid-version

install-exec-hook:
	$(SST_REGISTER_TOOL) SST_ELEMENT_SOURCE     hermes=$(abs_srcdir)
