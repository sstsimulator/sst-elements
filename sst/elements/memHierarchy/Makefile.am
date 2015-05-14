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
        membackend/memBackend.h \
        membackend/memBackend.cc \
	membackend/simpleMemBackend.cc \
	membackend/simpleMemBackend.h \
	membackend/vaultSimBackend.h \
	membackend/vaultSimBackend.cc \
	memEvent.h \
	memNIC.h \
	memNIC.cc \
	directoryController.h \
	directoryController.cc \
	coherenceControllers.h \
        memHierarchyInterface.cc \
        memHierarchyInterface.h \
	MESIBottomCoherenceController.h \
	MESIBottomCoherenceController.cc \
	MESITopCoherenceController.h \
	MESITopCoherenceController.cc \
	MOESIBottomCoherenceController.h \
	MOESIBottomCoherenceController.cc \
	MOESITopCoherenceController.h \
	MOESITopCoherenceController.cc \
	cacheArray.cc \
	cacheArray.h \
	mshr.h \
	mshr.cc \
	trivialCPU.h \
	trivialCPU.cc \
	streamCPU.h \
	streamCPU.cc \
	util.h \
	dmaEngine.h \
	dmaEngine.cc \
	networkMemInspector.h \
	networkMemInspector.cc \
	libmemHierarchy.cc

EXTRA_DIST = \
	tests/example.py \
	tests/exampleM5.xml \
	tests/sdl-1.py \
	tests/sdl2-1.py \
	tests/sdl-2.py \
	tests/sdl3-1.py \
	tests/sdl3-2.py \
	tests/sdl3-3.py \
	tests/sdl-3.py \
	tests/sdl4-1.py \
	tests/sdl4-2.py \
	tests/sdl5-1.py \
	tests/sdl8-1.py \
	tests/sdl8-3.py \
	tests/sdl8-4.py \
	tests/sdl9-1.py \
	tests/sdl9-2.py \
	tests/DDR3_micron_32M_8B_x4_sg125.ini \
	tests/system.ini

sstdir = $(includedir)/sst/elements/memHierarchy
nobase_sst_HEADERS = \
	memEvent.h \
	memNIC.h \
	membackend/memBackend.h \
	membackend/vaultSimBackend.h \
	membackend/simpleMemBackend.h \
	memoryController.h \
	cacheListener.h \
	bus.h \
	util.h

libmemHierarchy_la_LDFLAGS = -module -avoid-version
libmemHierarchy_la_LIBADD = 

if HAVE_DRAMSIM
libmemHierarchy_la_LDFLAGS += $(DRAMSIM_LDFLAGS)
libmemHierarchy_la_LIBADD += $(DRAMSIM_LIB)
libmemHierarchy_la_SOURCES += membackend/dramSimBackend.cc \
	membackend/dramSimBackend.h
nobase_sst_HEADERS += membackend/dramSimBackend.h
AM_CPPFLAGS += $(DRAMSIM_CPPFLAGS) -DHAVE_LIBDRAMSIM
endif

if HAVE_HYBRIDSIM
libmemHierarchy_la_LDFLAGS += $(HYBRIDSIM_LDFLAGS)
libmemHierarchy_la_LIBADD += $(HYBRIDSIM_LIB)
libmemHierarchy_la_SOURCES += membackend/hybridSimBackend.cc \
	membackend/hybridSimBackend.h
nobase_sst_HEADERS += membackend/hybridSimBackend.h
AM_CPPFLAGS += $(HYBRIDSIM_CPPFLAGS) -DHAVE_LIBHYBRIDSIM
endif

if USE_LIBZ
libmemHierarchy_la_LDFLAGS += $(LIBZ_LDFLAGS)
libmemHierarchy_la_LIBADD += $(LIBZ_LIB)
AM_CPPFLAGS += $(LIBZ_CPPFLAGS)
endif

if USE_GOBLIN_HMCSIM
libmemHierarchy_la_SOURCES += \
	membackend/goblinHMCBackend.cc \
	membackend/goblinHMCBackend.h
nobase_sst_HEADERS += membackend/goblinHMCBackend.h
libmemHierarchy_la_LDFLAGS += $(GOBLIN_HMCSIM_LDFLAGS)
libmemHierarchy_la_LIBADD += $(GOBLIN_HMCSIM_LIB)
AM_CPPFLAGS += $(GOBLIN_HMCSIM_CPPFLAGS) -DHAVE_GOBLIN_HMCSIM
endif

if HAVE_FDSIM
libmemHierarchy_la_SOURCES += \
	membackend/flashSimBackend.cc \
	membackend/flashSimBackend.h
libmemHierarchy_la_LDFLAGS += $(FDSIM_LDFLAGS)
libmemHierarchy_la_LIBADD += $(FDSIM_LIB)
AM_CPPFLAGS += $(FDSIM_CPPFLAGS) $(FDSIM_CXXFLAGS)
endif
