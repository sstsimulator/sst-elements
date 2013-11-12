# -*- Makefile -*-
#
#

AM_CPPFLAGS = \
	$(BOOST_CPPFLAGS) $(MPI_CPPFLAGS) \
	-g


compdir = $(pkglibdir)
comp_LTLIBRARIES = libmemHierarchy.la
libmemHierarchy_la_SOURCES = \
	cacheListener.h \
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
	dmaEngine.h \
	dmaEngine.cc \
	libmemHierarchy.cc


libmemHierarchy_la_LDFLAGS = -module -avoid-version
libmemHierarchy_la_LIBADD = 

if HAVE_DRAMSIM
libmemHierarchy_la_LDFLAGS += $(DRAMSIM_LDFLAGS)
libmemHierarchy_la_LIBADD += $(DRAMSIM_LIB)
AM_CPPFLAGS += $(DRAMSIM_CPPFLAGS) -DHAVE_LIBDRAMSIM
endif

if HAVE_HYBRIDSIM
libmemHierarchy_la_LDFLAGS += $(HYBRIDSIM_LDFLAGS)
libmemHierarchy_la_LIBADD += $(HYBRIDSIM_LIB)
AM_CPPFLAGS += $(HYBRIDSIM_CPPFLAGS) -DHAVE_LIBHYBRIDSIM
endif

if USE_LIBZ
libmemHierarchy_la_LDFLAGS += $(LIBZ_LDFLAGS)
libmemHierarchy_la_LIBADD += $(LIBZ_LIB)
AM_CPPFLAGS += $(LIBZ_CPPFLAGS)
endif
