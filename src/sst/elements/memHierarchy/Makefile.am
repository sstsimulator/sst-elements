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
	coherentMemoryController.h \
	coherentMemoryController.cc \
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
	membackend/flagMemBackendConvertor.h \
	membackend/flagMemBackendConvertor.cc \
	membackend/extMemBackendConvertor.h \
	membackend/extMemBackendConvertor.cc \
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
	memEventBase.h \
	memEvent.h \
	moveEvent.h \
	memLinkBase.h \
	memLink.h \
	memLink.cc \
	memNIC.h \
	memNIC.cc \
	memNICFour.h \
	memNICFour.cc \
	customcmd/customCmdEvent.h \
	customcmd/customCmdMemory.h \
	customcmd/customOpCodeCmd.h \
	customcmd/amoCustomCmdHandler.cc \
	customcmd/amoCustomCmdHandler.h \
	directoryController.h \
	directoryController.cc \
	scratchpad.h \
	scratchpad.cc \
	coherencemgr/coherenceController.h \
	coherencemgr/coherenceController.cc \
	memHierarchyInterface.cc \
	memHierarchyInterface.h \
	memHierarchyScratchInterface.cc \
	memHierarchyScratchInterface.h \
	coherencemgr/MESIInternalDirectory.h \
	coherencemgr/MESIInternalDirectory.cc \
	coherencemgr/MESICoherenceController.h \
	coherencemgr/MESICoherenceController.cc \
	coherencemgr/IncoherentController.h \
	coherencemgr/IncoherentController.cc \
	coherencemgr/L1IncoherentController.h \
	coherencemgr/L1IncoherentController.cc \
	coherencemgr/L1CoherenceController.h \
	coherencemgr/L1CoherenceController.cc \
	multithreadL1Shim.h \
	multithreadL1Shim.cc \
	cacheArray.cc \
	cacheArray.h \
	mshr.h \
	mshr.cc \
	testcpu/trivialCPU.h \
	testcpu/trivialCPU.cc \
	testcpu/streamCPU.h \
	testcpu/streamCPU.cc \
	testcpu/scratchCPU.h \
	testcpu/scratchCPU.cc \
	util.h \
	memTypes.h \
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
	memNetBridge.cc

EXTRA_DIST = \
        Sieve/tests/StatisticOutput.csv.gold \
        Sieve/tests/sieveprospero-0.trace \
        Sieve/tests/trace-text.py \
        Sieve/tests/Makefile \
        Sieve/tests/ompsievetest.c \
        Sieve/tests/sieve-test.py \
	tests/example.py \
	tests/miranda.cfg \
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
	tests/sdl4-2-ramulator.py \
	tests/sdl5-1-ramulator.py \
	tests/testBackendChaining.py \
	tests/testBackendDelayBuffer.py \
	tests/testBackendGoblinHMC.py \
	tests/testBackendHBMDramsim.py \
	tests/testBackendHBMPagedMulti.py \
	tests/testBackendPagedMulti.py \
	tests/testBackendReorderRow.py \
	tests/testBackendReorderSimple.py \
	tests/testBackendSimpleDRAM-1.py \
	tests/testBackendSimpleDRAM-2.py \
	tests/testBackendTimingDRAM-1.py \
	tests/testBackendTimingDRAM-2.py \
	tests/testBackendTimingDRAM-3.py \
	tests/testBackendTimingDRAM-4.py \
	tests/testBackendVaultSim.py \
	tests/testCustomCmdGoblin-1.py \
	tests/testCustomCmdGoblin-2.py \
	tests/testCustomCmdGoblin-3.py \
	tests/testDistributedCaches.py \
	tests/testFlushes.py \
	tests/testFlushes-2.py \
	tests/testHashXor.py \
	tests/testIncoherent.py \
	tests/testKingsley.py \
	tests/testNoninclusive-1.py \
	tests/testNoninclusive-2.py \
	tests/testPrefetchParams.py \
	tests/testThroughputThrottling.py \
	tests/testScratchDirect.py \
	tests/testScratchNetwork.py \
	tests/DDR3_micron_32M_8B_x4_sg125.ini \
	tests/system.ini \
	tests/ramulator-ddr3.cfg \
	tests/ddr_device.ini \
	tests/ddr_system.ini \
	tests/hbm_device.ini \
	tests/hbm_system.ini \
	tests/utils.py

sstdir = $(includedir)/sst/elements/memHierarchy
nobase_sst_HEADERS = \
	memEventBase.h \
	memEvent.h \
	memNIC.h \
	memNICFour.h \
	memLink.h \
	memLinkBase.h \
	memHierarchyInterface.h \
	memHierarchyScratchInterface.h \
	customcmd/customCmdEvent.h \
	customcmd/customCmdMemory.h \
	customcmd/customOpCodeCmd.h \
	customcmd/amoCustomCmdHandler.h \
	membackend/backing.h \
	membackend/memBackend.h \
	membackend/vaultSimBackend.h \
	membackend/MessierBackend.h \
	membackend/simpleMemBackend.h \
	membackend/simpleDRAMBackend.h \
	membackend/requestReorderSimple.h \
	membackend/requestReorderByRow.h \
	membackend/delayBuffer.h \
	membackend/memBackendConvertor.h \
	membackend/extMemBackendConvertor.h \
	membackend/flagMemBackendConvertor.h \
	membackend/scratchBackendConvertor.h \
	membackend/simpleMemBackendConvertor.h \
	membackend/simpleMemScratchBackendConvertor.h \
	memoryController.h \
	coherentMemoryController.h \
	cacheListener.h \
	bus.h \
	util.h \
	memTypes.h

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

if HAVE_HBMDRAMSIM
libmemHierarchy_la_LDFLAGS += $(HBMDRAMSIM_LDFLAGS)
libmemHierarchy_la_LIBADD += $(HBMDRAMSIM_LIB)
libmemHierarchy_la_SOURCES += membackend/HBMdramSimBackend.cc \
	membackend/HBMdramSimBackend.h
nobase_sst_HEADERS += membackend/HBMdramSimBackend.h
AM_CPPFLAGS += $(HBMDRAMSIM_CPPFLAGS) -DHAVE_LIBHBMDRAMSIM

libmemHierarchy_la_SOURCES += membackend/HBMpagedMultiBackend.cc \
	membackend/HBMpagedMultiBackend.h
nobase_sst_HEADERS += membackend/HBMpagedMultiBackend.h

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
	$(SST_REGISTER_TOOL) HBMDRAMSIM LIBDIR=$(HBMDRAMSIM_LIBDIR)
	$(SST_REGISTER_TOOL) HYBRIDSIM LIBDIR=$(HYBRIDSIM_LIBDIR)
	$(SST_REGISTER_TOOL) NVDIMMSIM LIBDIR=$(NVDIMMSIM_LIBDIR)
	$(SST_REGISTER_TOOL) FDSIM LIBDIR=$(FDSIM_LIBDIR)
	$(SST_REGISTER_TOOL) GOBLIN_HMCSIM LIBDIR=$(GOBLIN_HMCSIM_LIBDIR)
	$(SST_REGISTER_TOOL) RAMULATOR LIBDIR=$(RAMULATOR_LIBDIR)

