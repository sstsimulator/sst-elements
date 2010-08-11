# -*- Makefile -*-
#
#

AM_CPPFLAGS = $(BOOST_CPPFLAGS) $(MPI_CPPFLAGS)

introdir = $(pkglibdir)
intro_LTLIBRARIES = libintrospector_dram.la
libintrospector_dram_la_SOURCES = introspector_dram.h introspector_dram.cc
libintrospector_dram_la_LDFLAGS = -module -avoid-version
