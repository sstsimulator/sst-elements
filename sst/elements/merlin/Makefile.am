# -*- Makefile -*-
#
#

AM_CPPFLAGS = $(BOOST_CPPFLAGS) $(MPI_CPPFLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libmerlin.la

libmerlin_la_SOURCES = \
	merlin.cc \
	router.h \
	linkControl.h \
	portControl.h \
	test/nic.h \
	test/nic.cc \
	test/pt2pt/pt2pt_test.h \
	test/pt2pt/pt2pt_test.cc \
	topology/torus.h \
	topology/torus.cc \
	topology/fattree.h \
	topology/fattree.cc \
	topology/dragonfly.h \
	topology/dragonfly.cc \
	hr_router/hr_router.h \
	hr_router/hr_router.cc \
	hr_router/xbar_arb_rr.h \
	trafficgen/trafficgen.h \
	trafficgen/trafficgen.cc


libmerlin_la_LDFLAGS = -module -avoid-version

#bin_PROGRAMS = portals4_sm_sdlgen
#portals4_sm_sdlgen_SOURCES = portals4_sm_sdlgen.c portals_args.h
