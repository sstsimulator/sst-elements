# -*- Makefile -*-
#
#

AM_CPPFLAGS = $(BOOST_CPPFLAGS)

CXX = $(MPICXX)
compdir = $(pkglibdir)
comp_LTLIBRARIES = libghost_pattern.la
libghost_pattern_la_SOURCES = ghost.h ghost.cc common.h
libghost_pattern_la_LDFLAGS = -module -avoid-version

include src_genPatterns/Makefile.inc
