# -*- Makefile -*-
#
#

AM_CPPFLAGS = $(BOOST_CPPFLAGS) $(MPI_CPPFLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libghost_pattern.la
libghost_pattern_la_SOURCES = pattern_common.h pattern_common.cc \
                              pattern_component.h pattern_component.cc \
			      ghost.cc ghost.h
libghost_pattern_la_LDFLAGS = -module -avoid-version

include src_genPatterns/Makefile.inc
