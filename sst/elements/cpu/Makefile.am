# -*- Makefile -*-
#
#

AM_CPPFLAGS = $(BOOST_CPPFLAGS)
CXX = $(MPICXX)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libcpu.la
noinst_HEADERS = myMemEvent.h
libcpu_la_SOURCES = cpu.h cpu.cc
libcpu_la_LDFLAGS = -module -avoid-version
