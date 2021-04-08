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
	cacheController.cc \
	cacheFactory.cc \
	replacementManager.h \
	bus.h \
	bus.cc \
	memoryController.h \
	memoryController.cc \
	memoryCacheController.h \
	memoryCacheController.cc \
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
	memNICBase.h \
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
	coherencemgr/MESI_L1.h \
	coherencemgr/MESI_L1.cc \
	coherencemgr/MESI_Inclusive.h \
	coherencemgr/MESI_Inclusive.cc \
	coherencemgr/MESI_Private_Noninclusive.h \
	coherencemgr/MESI_Private_Noninclusive.cc \
	coherencemgr/MESI_Shared_Noninclusive.h \
	coherencemgr/MESI_Shared_Noninclusive.cc \
	coherencemgr/Incoherent_L1.h \
	coherencemgr/Incoherent_L1.cc \
	coherencemgr/Incoherent.h \
	coherencemgr/Incoherent.cc \
	multithreadL1Shim.h \
	multithreadL1Shim.cc \
	lineTypes.h \
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
	Sieve/alloctrackev.h \
	Sieve/memmgr_sieve.cc \
	Sieve/memmgr_sieve.h \
	memNetBridge.h \
	memNetBridge.cc

EXTRA_DIST = \
	tests/testsuite_default_memHierarchy_hybridsim.py \
	tests/testsuite_default_memHierarchy_memHA.py \
	tests/testsuite_default_memHierarchy_sdl.py \
	tests/testsuite_default_memHierarchy_memHSieve.py \
	tests/testsuite_sweep_memHierarchy_dir3LevelSweep.py \
	tests/testsuite_sweep_memHierarchy_dirSweep.py \
	tests/testsuite_sweep_memHierarchy_dirSweepB.py \
	tests/testsuite_sweep_memHierarchy_dirSweepI.py \
	tests/testsuite_sweep_memHierarchy_openMP.py \
	tests/testsuite_openMP_memHierarchy_dirnoncacheable.py \
	tests/testsuite_openMP_memHierarchy_noncacheable.py \
	tests/testsuite_openMP_memHierarchy_diropenMP.py \
	tests/testsuite_openMP_memHierarchy_openMP.py \
	Sieve/tests/StatisticOutput.csv.gold \
    Sieve/tests/sieveprospero-0.trace \
    Sieve/tests/trace-text.py \
    Sieve/tests/Makefile \
    Sieve/tests/ompsievetest.c \
    Sieve/tests/sieve-test.py \
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
	tests/test_hybridsim.py \
	tests/sdl4-2-ramulator.py \
	tests/sdl5-1-ramulator.py \
	tests/testBackendChaining.py \
	tests/testBackendDelayBuffer.py \
	tests/testBackendDramsim3.py \
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
	tests/testMemoryCache.py \
	tests/testNoninclusive-1.py \
	tests/testNoninclusive-2.py \
	tests/testPrefetchParams.py \
	tests/testThroughputThrottling.py \
	tests/testScratchDirect.py \
	tests/testScratchNetwork.py \
	tests/DDR3_micron_32M_8B_x4_sg125.ini \
	tests/system.ini \
    	tests/DDR4_8Gb_x16_3200.ini \
	tests/ramulator-ddr3.cfg \
	tests/ddr_device.ini \
	tests/ddr_system.ini \
	tests/hbm_device.ini \
	tests/hbm_system.ini \
	tests/utils.py \
	tests/mhlib.py \
    tests/refFiles/test_hybridsim.out \
    tests/refFiles/test_memHA_BackendChaining.out \
    tests/refFiles/test_memHA_BackendChaining_MC.out \
    tests/refFiles/test_memHA_BackendDelayBuffer.out \
    tests/refFiles/test_memHA_BackendDelayBuffer_MC.out \
    tests/refFiles/test_memHA_BackendDramsim3.out \
    tests/refFiles/test_memHA_BackendGoblinHMC.out \
    tests/refFiles/test_memHA_BackendGoblinHMC_MC.out \
    tests/refFiles/test_memHA_BackendHBMDramsim.out \
    tests/refFiles/test_memHA_BackendHBMDramsim_MC.out \
    tests/refFiles/test_memHA_BackendHBMDramsim_MR.out \
    tests/refFiles/test_memHA_BackendHBMPagedMulti.out \
    tests/refFiles/test_memHA_BackendPagedMulti.out \
    tests/refFiles/test_memHA_BackendPagedMulti_MC.out \
    tests/refFiles/test_memHA_BackendReorderRow.out \
    tests/refFiles/test_memHA_BackendReorderRow_MC.out \
    tests/refFiles/test_memHA_BackendReorderSimple.out \
    tests/refFiles/test_memHA_BackendReorderSimple_MC.out \
    tests/refFiles/test_memHA_BackendSimpleDRAM_1.out \
    tests/refFiles/test_memHA_BackendSimpleDRAM_1_MC.out \
    tests/refFiles/test_memHA_BackendSimpleDRAM_2.out \
    tests/refFiles/test_memHA_BackendSimpleDRAM_2_MC.out \
    tests/refFiles/test_memHA_BackendTimingDRAM_1.out \
    tests/refFiles/test_memHA_BackendTimingDRAM_1_MC.out \
    tests/refFiles/test_memHA_BackendTimingDRAM_2.out \
    tests/refFiles/test_memHA_BackendTimingDRAM_2_MC.out \
    tests/refFiles/test_memHA_BackendTimingDRAM_3.out \
    tests/refFiles/test_memHA_BackendTimingDRAM_3_MC.out \
    tests/refFiles/test_memHA_BackendTimingDRAM_4.out \
    tests/refFiles/test_memHA_BackendTimingDRAM_4_MC.out \
    tests/refFiles/test_memHA_BackendVaultSim.out \
    tests/refFiles/test_memHA_BackendVaultSim_MC.out \
    tests/refFiles/test_memHA_CustomCmdGoblin_1.out \
    tests/refFiles/test_memHA_CustomCmdGoblin_1_MC.out \
    tests/refFiles/test_memHA_CustomCmdGoblin_2.out \
    tests/refFiles/test_memHA_CustomCmdGoblin_2_MC.out \
    tests/refFiles/test_memHA_CustomCmdGoblin_3.out \
    tests/refFiles/test_memHA_CustomCmdGoblin_3_MC.out \
    tests/refFiles/test_memHA_DistributedCaches.out \
    tests/refFiles/test_memHA_DistributedCaches_MC.out \
    tests/refFiles/test_memHA_Flushes.out \
    tests/refFiles/test_memHA_Flushes_2.out \
    tests/refFiles/test_memHA_Flushes_2_MC.out \
    tests/refFiles/test_memHA_Flushes_MC.out \
    tests/refFiles/test_memHA_HashXor.out \
    tests/refFiles/test_memHA_HashXor_MC.out \
    tests/refFiles/test_memHA_Incoherent.out \
    tests/refFiles/test_memHA_Incoherent_2.out \
    tests/refFiles/test_memHA_Incoherent_MC.out \
    tests/refFiles/test_memHA_Kingsley.out \
    tests/refFiles/test_memHA_MemoryCache.out \
    tests/refFiles/test_memHA_MemoryCache_MC.out \
    tests/refFiles/test_memHA_Noninclusive_1.out \
    tests/refFiles/test_memHA_Noninclusive_1_MC.out \
    tests/refFiles/test_memHA_Noninclusive_2.out \
    tests/refFiles/test_memHA_Noninclusive_2_MC.out \
    tests/refFiles/test_memHA_PrefetchParams.out \
    tests/refFiles/test_memHA_PrefetchParams_MC.out \
    tests/refFiles/test_memHA_ScratchCache_1.out \
    tests/refFiles/test_memHA_ScratchCache_2.out \
    tests/refFiles/test_memHA_ScratchCache_3.out \
    tests/refFiles/test_memHA_ScratchDirect.out \
    tests/refFiles/test_memHA_ScratchNetwork.out \
    tests/refFiles/test_memHA_ThroughputThrottling.out \
    tests/refFiles/test_memHA_ThroughputThrottling_MC.out \
    tests/refFiles/test_memHierarchy_sdl2_1.out \
    tests/refFiles/test_memHierarchy_sdl3_1.out \
    tests/refFiles/test_memHierarchy_sdl3_2.out \
    tests/refFiles/test_memHierarchy_sdl3_3.out \
    tests/refFiles/test_memHierarchy_sdl4_1.out \
    tests/refFiles/test_memHierarchy_sdl4_2.out \
    tests/refFiles/test_memHierarchy_sdl4_2_ramulator.out \
    tests/refFiles/test_memHierarchy_sdl5_1.out \
    tests/refFiles/test_memHierarchy_sdl5_1_MC.out \
    tests/refFiles/test_memHierarchy_sdl5_1_ramulator.out \
    tests/refFiles/test_memHierarchy_sdl5_1_ramulator_MC.out \
    tests/refFiles/test_memHierarchy_sdl8_1.out \
    tests/refFiles/test_memHierarchy_sdl8_3.out \
    tests/refFiles/test_memHierarchy_sdl8_4.out \
    tests/refFiles/test_memHierarchy_sdl9_1.out \
    tests/refFiles/test_memHierarchy_sdl9_1_MC.out \
    tests/refFiles/test_memHierarchy_sdl9_2.out \
    tests/refFiles/test_memHierarchy_sdl_1.out \
    tests/refFiles/test_memHierarchy_sdl_2.out \
    tests/refFiles/test_memHierarchy_sdl_3.out \
    tests/refFiles/test_OMP_noncacheable_ompapi.out \
    tests/refFiles/test_OMP_noncacheable_ompatomic.out \
    tests/refFiles/test_OMP_noncacheable_ompbarrier.out \
    tests/refFiles/test_OMP_noncacheable_ompcritical.out \
    tests/refFiles/test_OMP_noncacheable_ompdynamic.out \
    tests/refFiles/test_OMP_noncacheable_ompreduce.out \
    tests/refFiles/test_OMP_noncacheable_ompthrcount.out \
    tests/refFiles/test_OMP_noncacheable_omptriangle.out \
    tests/refFiles/test_OMP_ompapi.out \
    tests/refFiles/test_OMP_ompatomic.out \
    tests/refFiles/test_OMP_ompatomicShort.out \
    tests/refFiles/test_OMP_ompbarrier.out \
    tests/refFiles/test_OMP_ompcritical.out \
    tests/refFiles/test_OMP_ompdynamic.out \
    tests/refFiles/test_OMP_ompmybarrier.out \
    tests/refFiles/test_OMP_ompreduce.out \
    tests/refFiles/test_OMP_ompthrcount.out \
    tests/refFiles/test_OMP_omptriangle.out \
    tests/openMP/ompapi/ompapi \
    tests/openMP/ompatomic/ompatomic \
    tests/openMP/ompatomicShort/ompatomicShort \
    tests/openMP/ompbarrier/ompbarrier \
    tests/openMP/ompcritical/ompcritical \
    tests/openMP/ompdynamic/ompdynamic \
    tests/openMP/ompmybarrier/ompmybarrier \
    tests/openMP/ompreduce/ompreduce \
    tests/openMP/ompthrcount/ompthrcount \
    tests/openMP/omptriangle/omptriangle \
    tests/openMP/dir-non-cacheable-8cores-2nodes.py \
    tests/openMP/directory-8cores-2nodes.py \
    tests/openMP/noncacheable-openmp.py \
    tests/openMP/openmp.py \
    tests/openMP/sweepdirectory-8cores-2nodes.py \
    tests/openMP/sweepdirectory-exclusive.py \
    tests/openMP/sweepopenmp.py \
    tests/openMP/test-distributed-caches.py

sstdir = $(includedir)/sst/elements/memHierarchy
nobase_sst_HEADERS = \
	memEventBase.h \
	memEvent.h \
	memNICBase.h \
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

if HAVE_DRAMSIM3
libmemHierarchy_la_LDFLAGS += $(DRAMSIM3_LDFLAGS)
libmemHierarchy_la_LIBADD += $(DRAMSIM3_LIB)
libmemHierarchy_la_SOURCES += membackend/dramSim3Backend.cc \
	membackend/dramSim3Backend.h
nobase_sst_HEADERS += membackend/dramSim3Backend.h
AM_CPPFLAGS += $(DRAMSIM3_CPPFLAGS) -DHAVE_LIBDRAMSIM3

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
	$(SST_REGISTER_TOOL) DRAMSIM3 LIBDIR=$(DRAMSIM3_LIBDIR)
	$(SST_REGISTER_TOOL) HBMDRAMSIM LIBDIR=$(HBMDRAMSIM_LIBDIR)
	$(SST_REGISTER_TOOL) HYBRIDSIM LIBDIR=$(HYBRIDSIM_LIBDIR)
	$(SST_REGISTER_TOOL) NVDIMMSIM LIBDIR=$(NVDIMMSIM_LIBDIR)
	$(SST_REGISTER_TOOL) FDSIM LIBDIR=$(FDSIM_LIBDIR)
	$(SST_REGISTER_TOOL) GOBLIN_HMCSIM LIBDIR=$(GOBLIN_HMCSIM_LIBDIR)
	$(SST_REGISTER_TOOL) RAMULATOR LIBDIR=$(RAMULATOR_LIBDIR)
	$(SST_REGISTER_TOOL) SST_ELEMENT_SOURCE     memHierarchy=$(abs_srcdir)
	$(SST_REGISTER_TOOL) SST_ELEMENT_TESTS      memHierarchy=$(abs_srcdir)/tests

