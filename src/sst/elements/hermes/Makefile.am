# -*- Makefile -*-
#
#

AM_CPPFLAGS = \
	$(MPI_CPPFLAGS) \
	-I$(top_srcdir)/src

compdir = $(pkglibdir)
comp_LTLIBRARIES = libhermes.la
libhermes_la_SOURCES = \
	msgapi.h \
	miscapi.h \
	hermes.h \
	functor.h \
	shmemapi.h

libhermes_la_LDFLAGS = -module -avoid-version
