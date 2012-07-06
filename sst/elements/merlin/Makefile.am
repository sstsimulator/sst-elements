# -*- Makefile -*-
#
#

AM_CPPFLAGS = $(BOOST_CPPFLAGS) $(MPI_CPPFLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libmerlin.la

libmerlin_la_SOURCES = \
	merlin.cc \
	router.h \
	topology/torus.cc \
	hr_router/hr_router.h \
	hr_router/hr_router.cc


libmerlin_la_LDFLAGS = -module -avoid-version

#bin_PROGRAMS = portals4_sm_sdlgen
#portals4_sm_sdlgen_SOURCES = portals4_sm_sdlgen.c portals_args.h
