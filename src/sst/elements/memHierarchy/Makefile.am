# -*- Makefile -*-
#
#

AM_CPPFLAGS = \
	$(MPI_CPPFLAGS) \
	-I$(top_srcdir)/src

compdir = $(pkglibdir)
comp_LTLIBRARIES = libmemHierarchy.la
libmemHierarchy_la_SOURCES = \
	hash.h \
	cacheListener.h \
	cacheController.h \
	cacheEventProcessing.cc \
	cacheController.cc \
	cacheFactory.cc \
	replacementManager.h \
	bus.h \
	bus.cc \
	memoryController.h \
	memoryController.cc \
	membackend/timingDRAMBackend.cc \
	membackend/timingDRAMBackend.h \
	membackend/timingAddrMapper.h \
	membackend/timingPagePolicy.h \
	membackend/timingTransaction.h \
	membackend/backing.h \
	membackend/memBackend.h \
	membackend/memBackendConvertor.h \
	membackend/memBackendConvertor.cc \
	membackend/simpleMemBackendConvertor.h \
	membackend/simpleMemBackendConvertor.cc \
	membackend/hmcMemBackendConvertor.h \
	membackend/hmcMemBackendConvertor.cc \
	membackend/delayBuffer.h \
	membackend/delayBuffer.cc \
	membackend/simpleMemBackend.h \
	membackend/simpleMemBackend.cc \
	membackend/simpleDRAMBackend.h \
	membackend/simpleDRAMBackend.cc \
	membackend/requestReorderSimple.h \
	membackend/requestReorderSimple.cc \
	membackend/requestReorderByRow.h \
	membackend/requestReorderByRow.cc \
	membackend/vaultSimBackend.h \
	membackend/vaultSimBackend.cc \
	membackend/MessierBackend.h \
	membackend/MessierBackend.cc \
	membackend/scratchBackendConvertor.h \
	membackend/scratchBackendConvertor.cc \
	membackend/simpleMemScratchBackendConvertor.h \
	membackend/simpleMemScratchBackendConvertor.cc \
	membackend/cramSimBackend.h \
	membackend/cramSimBackend.cc \
	memEvent.h \
	scratchEvent.h \
	memNIC.h \
	memNIC.cc \
	directoryController.h \
	directoryController.cc \
	scratchpad.h \
	scratchpad.cc \
	coherenceController.h \
	coherenceController.cc \
	memHierarchyInterface.cc \
	memHierarchyInterface.h \
	memHierarchyScratchInterface.cc \
	memHierarchyScratchInterface.h \
	MESIInternalDirectory.h \
	MESIInternalDirectory.cc \
	MESICoherenceController.h \
	MESICoherenceController.cc \
	IncoherentController.h \
	IncoherentController.cc \
	L1IncoherentController.h \
	L1IncoherentController.cc \
	L1CoherenceController.h \
	L1CoherenceController.cc \
	multithreadL1Shim.h \
	multithreadL1Shim.cc \
	cacheArray.cc \
	cacheArray.h \
	mshr.h \
	mshr.cc \
	trivialCPU.h \
	trivialCPU.cc \
	streamCPU.h \
	streamCPU.cc \
	scratchCPU.h \
	scratchCPU.cc \
	util.h \
	dmaEngine.h \
	dmaEngine.cc \
	networkMemInspector.h \
	networkMemInspector.cc \
	memResponseHandler.h \
	Sieve/sieveController.h \
	Sieve/sieveController.cc \
	Sieve/sieveFactory.cc \
	Sieve/broadcastShim.h \
	Sieve/broadcastShim.cc \
	memNetBridge.h \
	memNetBridge.cc \
	libmemHierarchy.cc

EXTRA_DIST = \
        Sieve/tests/StatisticOutput.csv.gold \
        Sieve/tests/sieveprospero-0.trace \
        Sieve/tests/trace-text.py \
        Sieve/tests/Makefile \
        Sieve/tests/ompsievetest.c \
        Sieve/tests/sieve-test.py \
	tests/example.py \
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
	tests/testBackendChaining.py \
	tests/testBackendDelayBuffer.py \
	tests/testBackendPagedMulti.py \
	tests/testBackendReorderRow.py \
	tests/testBackendReorderSimple.py \
	tests/testBackendSimpleDRAM-1.py \
	tests/testBackendSimpleDRAM-2.py \
	tests/testBackendVaultSim.py \
	tests/testDistributedCaches.py \
	tests/testFlushes.py \
	tests/testFlushes-2.py \
	tests/testHashXor.py \
	tests/testIncoherent.py \
	tests/testNoninclusive-1.py \
	tests/testNoninclusive-2.py \
	tests/testPrefetchParams.py \
	tests/testThroughputThrottling.py \
	tests/testScratchDirect.py \
	tests/testScratchNetwork.py \
	tests/DDR3_micron_32M_8B_x4_sg125.ini \
	tests/system.ini

sstdir = $(includedir)/sst/elements/memHierarchy
nobase_sst_HEADERS = \
	memEvent.h \
	memNIC.h \
	membackend/memBackend.h \
	membackend/vaultSimBackend.h \
	membackend/MessierBackend.h \
	membackend/simpleMemBackend.h \
	membackend/simpleDRAMBackend.h \
	membackend/requestReorderSimple.h \
	membackend/requestReorderByRow.h \
	membackend/delayBuffer.h \
	memoryController.h \
	cacheListener.h \
	bus.h \
	util.h

libmemHierarchy_la_LDFLAGS = -module -avoid-version
libmemHierarchy_la_LIBADD = 

if HAVE_RAMULATOR
libmemHierarchy_la_LDFLAGS += $(RAMULATOR_LDFLAGS)
libmemHierarchy_la_LIBADD += $(RAMULATOR_LIB)
libmemHierarchy_la_SOURCES += membackend/ramulatorBackend.cc \
	membackend/ramulatorBackend.h
nobase_sst_HEADERS += membackend/ramulatorBackend.h
AM_CPPFLAGS += $(RAMULATOR_CPPFLAGS) -DHAVE_LIBRAMULATOR
endif

if HAVE_DRAMSIM
libmemHierarchy_la_LDFLAGS += $(DRAMSIM_LDFLAGS)
libmemHierarchy_la_LIBADD += $(DRAMSIM_LIB)
libmemHierarchy_la_SOURCES += membackend/dramSimBackend.cc \
	membackend/dramSimBackend.h
nobase_sst_HEADERS += membackend/dramSimBackend.h
AM_CPPFLAGS += $(DRAMSIM_CPPFLAGS) -DHAVE_LIBDRAMSIM

libmemHierarchy_la_SOURCES += membackend/pagedMultiBackend.cc \
	membackend/pagedMultiBackend.h
nobase_sst_HEADERS += membackend/pagedMultiBackend.h

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
AM_CPPFLAGS += $(FDSIM_CPPFLAGS) $(FDSIM_CXXFLAGS) -DHAVE_LIBFDSIM
endif

AM_CPPFLAGS += $(HMC_FLAG)

install-exec-hook:
	$(SST_REGISTER_TOOL) DRAMSIM LIBDIR=$(DRAMSIM_LIBDIR)
	$(SST_REGISTER_TOOL) HYBRIDSIM LIBDIR=$(HYBRIDSIM_LIBDIR)
	$(SST_REGISTER_TOOL) NVDIMMSIM LIBDIR=$(NVDIMMSIM_LIBDIR)
	$(SST_REGISTER_TOOL) FDSIM LIBDIR=$(FDSIM_LIBDIR)
	$(SST_REGISTER_TOOL) GOBLIN_HMCSIM LIBDIR=$(GOBLIN_HMCSIM_LIBDIR)

