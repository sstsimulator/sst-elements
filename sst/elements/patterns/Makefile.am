# -*- Makefile -*-
#
#

AM_CPPFLAGS = $(BOOST_CPPFLAGS) $(MPI_CPPFLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libghost_pattern.la
libghost_pattern_la_SOURCES = ghost.h ghost.cc common.h common.cc
libghost_pattern_la_LDFLAGS = -module -avoid-version

include src_genPatterns/Makefile.inc
