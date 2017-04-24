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
    tests/test_cacheTracer_1.py \
    tests/test_cacheTracer_2.py

libcacheTracer_la_LDFLAGS = -module -avoid-version

##########################################################################
##########################################################################
##########################################################################

