# -*- Makefile -*-
#
#

AM_CPPFLAGS = \
	$(BOOST_CPPFLAGS) $(MPI_CPPFLAGS) \
	-g


compdir = $(pkglibdir)
comp_LTLIBRARIES = libmemHierarchy.la
libmemHierarchy_la_SOURCES = \
	hash.h \
	cacheListener.h \
	cacheController.h \
	cacheEventProcessing.cc \
	cacheController.cc \
	cacheFactory.cc \
	ccLine.h \
	replacementManager.h \
	bus.h \
	bus.cc \
	memoryController.h \
	memoryController.cc \
	memNIC.h \
	memNIC.cc \
	directoryController.h \
	directoryController.cc \
	coherenceControllers.h \
	MESIBottomCoherenceController.h \
	MESIBottomCoherenceController.cc \
	MESITopCoherenceController.h \
	MESITopCoherenceController.cc \
	cacheArray.cc \
	cacheArray.h \
	mshr.h \
	trivialCPU.h \
	trivialCPU.cc \
	streamCPU.h \
	streamCPU.cc \
	util.h \
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
