# -*- Makefile -*-
#
#

AM_CPPFLAGS = \
	$(BOOST_CPPFLAGS) $(MPI_CPPFLAGS) \
	-g

compdir = $(pkglibdir)
comp_LTLIBRARIES = libmemHierarchy.la
libmemHierarchy_la_SOURCES = \
	bus.h \
	bus.cc \
	cache.h \
	cache.cc \
	memEvent.h \
	memTypes.h \
	trivialCPU.h \
	trivialCPU.cc \
	streamCPU.h \
	streamCPU.cc \
	memController.h \
	memController.cc \
	dircontroller.h \
	dircontroller.cc \
	memNIC.h \
	memNIC.cc \
	libmemHierarchy.cc


libmemHierarchy_la_LDFLAGS = -module -avoid-version

if HAVE_DRAMSIM
libmemHierarchy_la_LDFLAGS += $(DRAMSIM_LDFLAGS)
libmemHierarchy_la_LIBADD = $(DRAMSIM_LIB)
AM_CPPFLAGS += $(DRAMSIM_CPPFLAGS) -DHAVE_LIBDRAMSIM
endif
