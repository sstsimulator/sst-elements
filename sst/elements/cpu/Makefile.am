# -*- Makefile -*-
#
#

AM_CPPFLAGS = $(BOOST_CPPFLAGS) $(MPI_CPPFLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libcpu.la
libcpu_la_SOURCES = cpu.h cpu.cc myMemEvent.h
libcpu_la_LDFLAGS = -module -avoid-version
