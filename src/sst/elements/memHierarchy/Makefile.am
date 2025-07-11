# -*- Makefile -*-
#
#

AM_CPPFLAGS += \
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
	memEventCustom.h \
	moveEvent.h \
	memLinkBase.h \
	memNICBase.h \
	memLink.h \
	memLink.cc \
	memNIC.h \
	memNIC.cc \
	memNICFour.h \
	memNICFour.cc \
	customcmd/customCmdMemory.h \
	customcmd/defCustomCmdHandler.cc \
	customcmd/defCustomCmdHandler.h \
	directoryController.h \
	directoryController.cc \
	scratchpad.h \
	scratchpad.cc \
	coherencemgr/coherenceController.h \
	coherencemgr/coherenceController.cc \
	standardInterface.cc \
	standardInterface.h \
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
	testcpu/standardCPU.h \
	testcpu/standardCPU.cc \
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
	memNetBridge.cc \
	testcpu/standardMMIO.h \
	testcpu/standardMMIO.cc

EXTRA_DIST = \
	membackend/ramulator2/Instructions.md \
	membackend/ramulator2/sst_frontend.cpp \
	tests/testsuite_default_memHierarchy_hybridsim.py \
	tests/testsuite_default_memHierarchy_memHA.py \
	tests/testsuite_default_memHierarchy_sdl.py \
	tests/testsuite_default_memHierarchy_memory.py \
	tests/testsuite_default_memHierarchy_coherence.py \
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
	tests/testsuite_weekly_memHierarchy_examples.py \
	Sieve/tests/StatisticOutput.csv.gold \
	Sieve/tests/sieveprospero-0.trace \
	Sieve/tests/trace-text.py \
	Sieve/tests/Makefile \
	Sieve/tests/ompsievetest.c \
	Sieve/tests/sieve-test.py \
	Sieve/tests/refFiles/test_memHSieve.out \
	tests/miranda.cfg \
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
	tests/test_coherence_1core.py \
	tests/test_coherence_2core_3level.py \
	tests/test_coherence_4core_5level.py \
	tests/test_coherence_none.py \
	tests/test_backing.py \
	tests/testBackendChaining.py \
	tests/testBackendDelayBuffer.py \
	tests/testBackendDramsim3.py \
	tests/testBackendGoblinHMC.py \
	tests/testBackendHBMDramsim.py \
	tests/testBackendHBMPagedMulti.py \
	tests/testBackendPagedMulti.py \
	tests/testBackendRamulator-1.py \
	tests/testBackendRamulator-2.py \
	tests/testBackendRamulator2.py \
	tests/testBackendReorderRow.py \
	tests/testBackendReorderSimple.py \
	tests/testBackendSimpleDRAM-1.py \
	tests/testBackendSimpleDRAM-2.py \
	tests/testBackendTimingDRAM-1.py \
	tests/testBackendTimingDRAM-2.py \
	tests/testBackendTimingDRAM-3.py \
	tests/testBackendTimingDRAM-4.py \
	tests/testBackendVaultSim.py \
	tests/testCoherenceDomains.py \
	tests/testCustomCmdGoblin-1.py \
	tests/testCustomCmdGoblin-2.py \
	tests/testCustomCmdGoblin-3.py \
	tests/testDistributedCaches.py \
	tests/testFlushes.py \
	tests/testFlushes-2.py \
	tests/testHashXor.py \
	tests/testKingsley.py \
	tests/testMemoryCache.py \
	tests/testNoninclusive-1.py \
	tests/testNoninclusive-2.py \
	tests/testPrefetchParams.py \
	tests/testThroughputThrottling.py \
	tests/testRangeCheck.py \
	tests/testScratchCache-1.py \
	tests/testScratchCache-2.py \
	tests/testScratchCache-3.py \
	tests/testScratchCache-4.py \
	tests/testScratchDirect.py \
	tests/testScratchNetwork.py \
	tests/testStdMem.py \
	tests/testStdMem-noninclusive.py \
	tests/testStdMem-nic.py \
	tests/testStdMem-flush.py \
	tests/testStdMem-mmio.py \
	tests/testStdMem-mmio2.py \
	tests/testStdMem-mmio3.py \
	tests/DDR3_micron_32M_8B_x4_sg125.ini \
	tests/system.ini \
	tests/DDR4_8Gb_x16_3200.ini \
	tests/ramulator-ddr3.cfg \
	tests/ramulator2-ddr4.cfg \
	tests/ddr_device.ini \
	tests/ddr_system.ini \
	tests/hbm_device.ini \
	tests/hbm_system.ini \
	tests/utils.py \
	tests/mhlib.py \
	tests/refFiles/test_hybridsim.out \
	tests/refFiles/test_memHA_BackendChaining.out \
	tests/refFiles/test_memHA_BackendDelayBuffer.out \
	tests/refFiles/test_memHA_BackendDramsim3.out \
	tests/refFiles/test_memHA_BackendGoblinHMC.out \
	tests/refFiles/test_memHA_BackendHBMDramsim.out \
	tests/refFiles/test_memHA_BackendHBMPagedMulti.out \
	tests/refFiles/test_memHA_BackendPagedMulti.out \
	tests/refFiles/test_memHA_BackendRamulator_1.out \
	tests/refFiles/test_memHA_BackendRamulator_2.out \
	tests/refFiles/test_memHA_BackendRamulator2.out \
	tests/refFiles/test_memHA_BackendReorderRow.out \
	tests/refFiles/test_memHA_BackendReorderSimple.out \
	tests/refFiles/test_memHA_BackendSimpleDRAM_1.out \
	tests/refFiles/test_memHA_BackendSimpleDRAM_2.out \
	tests/refFiles/test_memHA_BackendTimingDRAM_1.out \
	tests/refFiles/test_memHA_BackendTimingDRAM_2.out \
	tests/refFiles/test_memHA_BackendTimingDRAM_3.out \
	tests/refFiles/test_memHA_BackendTimingDRAM_4.out \
	tests/refFiles/test_memHA_BackendVaultSim.out \
	tests/refFiles/test_memHA_CoherenceDomains.out \
	tests/refFiles/test_memHA_CustomCmdGoblin_1.out \
	tests/refFiles/test_memHA_CustomCmdGoblin_2.out \
	tests/refFiles/test_memHA_CustomCmdGoblin_3.out \
	tests/refFiles/test_memHA_DistributedCaches.out \
	tests/refFiles/test_memHA_Flushes.out \
	tests/refFiles/test_memHA_Flushes_2.out \
	tests/refFiles/test_memHA_HashXor.out \
	tests/refFiles/test_memHA_Kingsley.out \
	tests/refFiles/test_memHA_MemoryCache.out \
	tests/refFiles/test_memHA_Noninclusive_1.out \
	tests/refFiles/test_memHA_Noninclusive_2.out \
	tests/refFiles/test_memHA_PrefetchParams.out \
	tests/refFiles/test_memHA_RangeCheck.out \
	tests/refFiles/test_memHA_ScratchCache_1.out \
	tests/refFiles/test_memHA_ScratchCache_2.out \
	tests/refFiles/test_memHA_ScratchCache_3.out \
	tests/refFiles/test_memHA_ScratchCache_4.out \
	tests/refFiles/test_memHA_ScratchDirect.out \
	tests/refFiles/test_memHA_ScratchNetwork.out \
	tests/refFiles/test_memHA_StdMem_flush.out \
	tests/refFiles/test_memHA_StdMem_mmio3.out \
	tests/refFiles/test_memHA_StdMem_nic.out \
	tests/refFiles/test_memHA_StdMem.out \
	tests/refFiles/test_memHA_StdMem_mmio2.out \
	tests/refFiles/test_memHA_StdMem_mmio.out \
	tests/refFiles/test_memHA_StdMem_noninclusive.out \
	tests/refFiles/test_memHA_ThroughputThrottling.out \
	tests/refFiles/test_memHierarchy_coherence_1core_case0_mesi.out \
	tests/refFiles/test_memHierarchy_coherence_1core_case0_mesi.malloc.mem \
	tests/refFiles/test_memHierarchy_coherence_1core_case1_mesi.out \
	tests/refFiles/test_memHierarchy_coherence_1core_case1_mesi.malloc.mem \
	tests/refFiles/test_memHierarchy_coherence_1core_case2_mesi.out \
	tests/refFiles/test_memHierarchy_coherence_1core_case2_mesi.malloc.mem \
	tests/refFiles/test_memHierarchy_coherence_1core_case3_mesi.out \
	tests/refFiles/test_memHierarchy_coherence_1core_case3_mesi.malloc.mem \
	tests/refFiles/test_memHierarchy_coherence_1core_case4_mesi.out \
	tests/refFiles/test_memHierarchy_coherence_1core_case4_mesi.malloc.mem \
	tests/refFiles/test_memHierarchy_coherence_1core_case5_mesi.out \
	tests/refFiles/test_memHierarchy_coherence_1core_case5_mesi.malloc.mem \
	tests/refFiles/test_memHierarchy_coherence_1core_case6_mesi.out \
	tests/refFiles/test_memHierarchy_coherence_1core_case6_mesi.malloc.mem \
	tests/refFiles/test_memHierarchy_coherence_2core_3level_case0_mesi.out \
	tests/refFiles/test_memHierarchy_coherence_2core_3level_case0_mesi.malloc.mem \
	tests/refFiles/test_memHierarchy_coherence_2core_3level_case1_mesi.out \
	tests/refFiles/test_memHierarchy_coherence_2core_3level_case1_mesi.malloc.mem \
	tests/refFiles/test_memHierarchy_coherence_2core_3level_case2_mesi.out \
	tests/refFiles/test_memHierarchy_coherence_2core_3level_case2_mesi.malloc.mem \
	tests/refFiles/test_memHierarchy_coherence_2core_3level_case3_mesi.out \
	tests/refFiles/test_memHierarchy_coherence_2core_3level_case3_mesi.malloc.mem \
	tests/refFiles/test_memHierarchy_coherence_2core_3level_case4_mesi.out \
	tests/refFiles/test_memHierarchy_coherence_2core_3level_case4_mesi.malloc.mem \
	tests/refFiles/test_memHierarchy_coherence_2core_3level_case5_mesi.out \
	tests/refFiles/test_memHierarchy_coherence_2core_3level_case5_mesi.malloc.mem \
	tests/refFiles/test_memHierarchy_coherence_2core_3level_case6_mesi.out \
	tests/refFiles/test_memHierarchy_coherence_2core_3level_case6_mesi.malloc.mem \
	tests/refFiles/test_memHierarchy_coherence_2core_3level_case7_mesi.out \
	tests/refFiles/test_memHierarchy_coherence_2core_3level_case7_mesi.malloc.mem \
	tests/refFiles/test_memHierarchy_coherence_2core_3level_case8_mesi.out \
	tests/refFiles/test_memHierarchy_coherence_2core_3level_case8_mesi.malloc.mem \
	tests/refFiles/test_memHierarchy_coherence_2core_3level_case9_mesi.out \
	tests/refFiles/test_memHierarchy_coherence_2core_3level_case9_mesi.malloc.mem \
	tests/refFiles/test_memHierarchy_coherence_2core_3level_case10_mesi.out \
	tests/refFiles/test_memHierarchy_coherence_2core_3level_case10_mesi.malloc.mem \
	tests/refFiles/test_memHierarchy_coherence_2core_3level_case11_mesi.out \
	tests/refFiles/test_memHierarchy_coherence_2core_3level_case11_mesi.malloc.mem \
	tests/refFiles/test_memHierarchy_coherence_4core_5level_case0_mesi.out \
	tests/refFiles/test_memHierarchy_coherence_4core_5level_case0_mesi.malloc.mem \
	tests/refFiles/test_memHierarchy_coherence_4core_5level_case1_mesi.out \
	tests/refFiles/test_memHierarchy_coherence_4core_5level_case1_mesi.malloc.mem \
	tests/refFiles/test_memHierarchy_coherence_4core_5level_case2_mesi.out \
	tests/refFiles/test_memHierarchy_coherence_4core_5level_case2_mesi.malloc.mem \
	tests/refFiles/test_memHierarchy_coherence_4core_5level_case3_mesi.out \
	tests/refFiles/test_memHierarchy_coherence_4core_5level_case3_mesi.malloc.mem \
	tests/refFiles/test_memHierarchy_coherence_none_case0_none.out \
	tests/refFiles/test_memHierarchy_coherence_none_case0_none.malloc.mem \
	tests/refFiles/test_memHierarchy_coherence_none_case1_none.out \
	tests/refFiles/test_memHierarchy_coherence_none_case1_none.malloc.mem \
	tests/refFiles/test_memHierarchy_coherence_none_case2_none.out \
	tests/refFiles/test_memHierarchy_coherence_none_case2_none.malloc.mem \
	tests/refFiles/test_memHierarchy_coherence_none_case3_none.out \
	tests/refFiles/test_memHierarchy_coherence_none_case3_none.malloc.mem \
	tests/refFiles/test_memHierarchy_coherence_none_case4_none.out \
	tests/refFiles/test_memHierarchy_coherence_none_case4_none.malloc.mem \
	tests/refFiles/test_memHierarchy_memory_backing_out.malloc.mem \
	tests/refFiles/test_memHierarchy_memory_backing_out.mmap.mem \
	tests/refFiles/test_memHierarchy_memory_backing_2_mmap_inout.mmap.mem \
	tests/refFiles/test_memHierarchy_memory_backing_5_init.malloc.mem \
	tests/refFiles/test_memHierarchy_sdl2_1.out \
	tests/refFiles/test_memHierarchy_sdl3_1.out \
	tests/refFiles/test_memHierarchy_sdl3_2.out \
	tests/refFiles/test_memHierarchy_sdl3_3.out \
	tests/refFiles/test_memHierarchy_sdl4_1.out \
	tests/refFiles/test_memHierarchy_sdl4_2.out \
	tests/refFiles/test_memHierarchy_sdl5_1.out \
	tests/refFiles/test_memHierarchy_sdl8_1.out \
	tests/refFiles/test_memHierarchy_sdl8_3.out \
	tests/refFiles/test_memHierarchy_sdl8_4.out \
	tests/refFiles/test_memHierarchy_sdl9_1.out \
	tests/refFiles/test_memHierarchy_sdl9_2.out \
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
	tests/openMP/test-distributed-caches.py \
	examples/README.md \
	examples/example-1core-nocache.py \
	examples/example-1core-1level.py \
	examples/example-1core-3level.py \
	examples/example-4core-shared-llc.py \
	examples/example-4core-shared-distributed-llc.py

sstdir = $(includedir)/sst/elements/memHierarchy
nobase_sst_HEADERS = \
	memEventBase.h \
	memEvent.h \
	memNICBase.h \
	memNIC.h \
	memNICFour.h \
	memLink.h \
	memLinkBase.h \
	customcmd/customCmdMemory.h \
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

if HAVE_RAMULATOR2
libmemHierarchy_la_LDFLAGS += $(RAMULATOR2_LDFLAGS)
libmemHierarchy_la_LIBADD += $(RAMULATOR2_LIB)
libmemHierarchy_la_SOURCES += membackend/ramulator2Backend.cc \
	membackend/ramulator2Backend.h
nobase_sst_HEADERS += membackend/ramulator2Backend.h
AM_CPPFLAGS += $(RAMULATOR2_CPPFLAGS) -DHAVE_LIBRAMULATOR2
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
	$(SST_REGISTER_TOOL) RAMULATOR2 LIBDIR=$(RAMULATOR2_LIBDIR)
	$(SST_REGISTER_TOOL) SST_ELEMENT_SOURCE     memHierarchy=$(abs_srcdir)
	$(SST_REGISTER_TOOL) SST_ELEMENT_TESTS      memHierarchy=$(abs_srcdir)/tests

