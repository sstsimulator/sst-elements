# -*- Makefile -*-
#
#

AM_CPPFLAGS = $(BOOST_CPPFLAGS) $(MPI_CPPFLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libghost_pattern.la libmsgrate_pattern.la
libghost_pattern_la_SOURCES = pattern_common.h pattern_common.cc \
                              ghost_pattern.h ghost_pattern.cc
libghost_pattern_la_LDFLAGS = -module -avoid-version

libmsgrate_pattern_la_SOURCES = pattern_common.h pattern_common.cc \
                              msgrate_pattern.h msgrate_pattern.cc
libmsgrate_pattern_la_LDFLAGS = -module -avoid-version

include src_genPatterns/Makefile.inc
