# -*- Makefile -*-
#
#

AM_CPPFLAGS = $(BOOST_CPPFLAGS) $(MPI_CPPFLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libSS_router.la
libSS_router_la_SOURCES = \
	SS_router.cc \
	SS_router/RtrIF.h \
	SS_router/SS_network.cpp \
	SS_router/SS_network.h \
	SS_router/SS_router-inline.h \
	SS_router/SS_router.h \
	SS_router/SS_router.cpp

sstdir = $(includedir)/sst/elements/SS_router
nobase_sst_HEADERS = \
	SS_router/RtrIF.h \
	SS_router/SS_network.h

EXTRA_DIST = \
	SS_router/SS_routerInternals.cpp \
	SS_router/SS_routerEvent.cpp

libSS_router_la_LDFLAGS = -module -avoid-version

noinst_PROGRAMS = SS_router_test_sdlgen
SS_router_test_sdlgen_SOURCES = SS_router_test_sdlgen.c
