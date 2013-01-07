# -*- Makefile -*-
#
#

AM_CPPFLAGS = \
	$(BOOST_CPPFLAGS) \
	$(MPI_CPPFLAGS)  -g

compdir = $(pkglibdir)
comp_LTLIBRARIES = libmemHierarchy.la
libmemHierarchy_la_SOURCES = \
	bus.h \
	bus.cc \
	cache.h \
	cache.cc \
	memEvent.h \
	memEvent.cc \
	memTypes.h \
	trivialCPU.h \
	trivialCPU.cc \
	trivialMemory.h \
	trivialMemory.cc \
	memController.h \
	memController.cc \
	libmemHierarchy.cc

EXTRA_DIST = makeSDL.pl

libmemHierarchy_la_LDFLAGS = -module -avoid-version
if HAVE_DRAMSIM
libmemHierarchy_la_LDFLAGS += $(DRAMSIM_LDFLAGS)
libmemHierarchy_la_LIBADD = $(DRAMSIM_LIB)
AM_CPPFLAGS += $(DRAMSIM_CPPFLAGS)
endif
