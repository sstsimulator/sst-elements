# -*- Makefile -*-
#
#

AM_CPPFLAGS += \
	$(MPI_CPPFLAGS) \
	-I$(top_srcdir)/src

compdir = $(pkglibdir)
comp_LTLIBRARIES = libcarcosa.la
libcarcosa_la_SOURCES =                     \
    components/hali.cc                      \
    components/hali.h                       \
    components/eccGuard.cc                  \
    components/eccGuard.h                   \
    components/eccModelMath.h               \
    components/criticalActionWatcher.cc     \
    components/criticalActionWatcher.h      \
    components/eccScheme.h                  \
    components/eccPolicy.h                  \
    components/actionScorer.cc              \
    components/actionScorer.h               \
    components/faultInjManager.cc           \
    components/faultInjManager.h            \
    injectors/faultInjectorBase.cc          \
    injectors/faultInjectorBase.h           \
    injectors/stuckAtFaultInjector.cc       \
    injectors/stuckAtFaultInjector.h        \
    injectors/corruptMemFaultInjector.cc    \
    injectors/corruptMemFaultInjector.h     \
    injectors/randomDropFaultInjector.cc    \
    injectors/randomDropFaultInjector.h     \
    injectors/randomFlipFaultInjector.cc    \
    injectors/randomFlipFaultInjector.h     \
    injectors/dropFlipFaultInjector.cc      \
    injectors/dropFlipFaultInjector.h       \
    injectors/portModuleStateGate.cc        \
    injectors/portModuleStateGate.h         \
    injectors/faultInjectorMemH.cc          \
    injectors/faultInjectorMemH.h           \
    faultlogic/faultBase.cc                 \
    faultlogic/faultBase.h                  \
    faultlogic/stuckAtFault.cc              \
    faultlogic/stuckAtFault.h               \
    faultlogic/corruptMemFault.cc           \
    faultlogic/corruptMemFault.h            \
    faultlogic/randomDropFault.cc           \
    faultlogic/randomDropFault.h            \
    faultlogic/randomFlipFault.cc           \
    faultlogic/randomFlipFault.h            \
    faultlogic/randomFlipMemHFault.cc       \
    faultlogic/randomFlipMemHFault.h        \
    components/pmDataRegistry.h             \
    components/carcosaCPUBase.cc            \
    components/carcosaCPUBase.h             \
    components/carcosaCPU.cc                \
    components/carcosaCPU.h                 \
    components/faultInjCPU.cc               \
    components/faultInjCPU.h                \
    components/cpuEvent.h                   \
    components/faultInjEvent.h              \
    components/faultInjManagerAPI.h         \
    components/interceptionAgentAPI.h       \
    components/hyadesProtocol.h             \
    components/pipelineStateRegistry.h      \
    components/pingPongAgent.cc             \
    components/pingPongAgent.h              \
    components/fourStateAgent.cc            \
    components/fourStateAgent.h             \
    components/framePipelineDriver.cc       \
    components/framePipelineDriver.h        \
    components/regressionTestComponents.cc  \
    components/regressionTestComponents.h   \
    components/vlaRegions.h                 \
    components/carcosaMemCtrl.cc            \
    components/carcosaMemCtrl.h             \
    components/sensorComponent.cc           \
    components/sensorComponent.h            \
    components/sensorEvent.h                \
    components/haliEvent.h                  \
    components/ringProtocol.h               \
    components/carcosaHash.h                \
    examples/SimplePipeline/simplePipelineExample.cc \
    examples/SimplePipeline/simplePipelineExample.h \
    examples/MmioControl/mmioControlExample.cc \
    examples/MmioControl/mmioControlExample.h

# Optional balar ring bridge: compiled into libcarcosa only when GPGPU-Sim/balar
# is available (SST_CARCOSA_HAVE_BALAR, set by configure.m4 -> HAVE_BALAR_BRIDGE).
# Header-only dependency on balar's packet ABI, so only the CUDA include path is
# added here (no GPGPU-Sim libs). A checkout with no balar/ builds libcarcosa
# unchanged with the bridge compiled out.
if SST_CARCOSA_HAVE_BALAR
libcarcosa_la_SOURCES += \
    components/balarRingBridge.cc \
    components/balarRingBridge.h
# HAVE_BALAR_BRIDGE is passed on the command line (like balar's own -DHAVE_CUDA=1
# in CUDA_CPPFLAGS) rather than via sst_element_config.h, which element sources do
# not include. CUDA_CPPFLAGS carries the CUDA include path for balar's packet headers.
AM_CPPFLAGS += $(CUDA_CPPFLAGS) -DHAVE_BALAR_BRIDGE=1
endif

EXTRA_DIST = \
    hyades.h                                      \
    tests/pingpong.c                              \
    tests/fourstate.c                             \
    tests/testCarcosaPingPong.py                  \
    tests/testFourStateRegistry.py                \
    tests/testFourStateRegistryGated.py           \
    examples/SimplePipeline/tests/testSimplePipeline.py \
    examples/SimplePipeline/tests/testPortModuleStateGate.py \
    examples/MmioControl/tests/testMmioControl.py \
    tests/testCorruptMemBasic.py                  \
    tests/testCorruptMemDouble.py                 \
    tests/testCorruptMemDoubleOverlap.py          \
    tests/testRandomDrop.py                       \
    tests/testRandomFlip.py                       \
    tests/testStuckAtBasic.py                     \
    tests/testStuckAtMultiple.py                  \
    tests/testStuckAtOverlap.py                   \
    tests/testStuckAtSameByte.py                  \
    tests/mhlib.py                                \
    tests/testDynamicPM.py                        \
    tests/testEccGuardJedecMix.py                 \
    tests/testEccGuardRegionPolicy.py             \
    tests/testEccGuardResident.py                 \
    tests/testEccGuardSmoke.py                    \
    tests/testEccModelDeterministic.py            \
    tests/eccRuntimeCommon.py                     \
    tests/testEccRuntimeCorrectable.py            \
    tests/testEccRuntimeDueDrop.py                \
    tests/testEccRuntimeEscape.py                 \
    tests/testEccCampaignReentry.py               \
    tests/testEccPoissonDistribution.py           \
    tests/testEccJedecDistribution.py             \
    tests/testEccResidentDistribution.py          \
    tests/framePipelineCommon.py                  \
    tests/testFramePipelineClean.py               \
    tests/testFramePipelineCorrupt.py             \
    tests/testFramePipelineFallback.py            \
    tests/testFramePipelineMissingGolden.py       \
    tests/testStateGateRegionNoOverlap.py         \
    tests/testStateGateRegionOverlap.py           \
    tests/testActionScorerTaxonomy.py             \
    tests/testActionScorerTokenFallback.py        \
    tests/testActionScorerDropDivergence.py       \
    tests/haliEdgeCommon.py                       \
    tests/testHaliPayloadlessGetX.py              \
    tests/testHaliDoubleDeferred.py               \
    tests/testHaliDeferredComplete.py             \
    tests/testHaliPayloadGetX.py                  \
    tests/testHaliBacking.py                      \
    tests/testHaliMemH.py                         \
    tests/testHaliPM.py                           \
    tests/testManagerLogic.py

sstdir = $(includedir)/sst/elements/carcosa
nobase_sst_HEADERS =                        \
    injectors/faultInjectorBase.h           \
    injectors/stuckAtFaultInjector.h        \
    injectors/corruptMemFaultInjector.h     \
    injectors/randomDropFaultInjector.h     \
    injectors/randomFlipFaultInjector.h     \
    injectors/dropFlipFaultInjector.h       \
    injectors/portModuleStateGate.h         \
    injectors/faultInjectorMemH.h           \
    faultlogic/faultBase.h                  \
    faultlogic/stuckAtFault.h               \
    faultlogic/corruptMemFault.h            \
    faultlogic/randomDropFault.h            \
    faultlogic/randomFlipFault.h            \
    faultlogic/randomFlipMemHFault.h        \
    components/pmDataRegistry.h             \
    components/eccScheme.h                  \
    components/eccPolicy.h                  \
    components/eccGuard.h                   \
    components/eccModelMath.h               \
    components/criticalActionWatcher.h      \
    components/actionScorer.h               \
    components/faultInjManagerAPI.h         \
    components/interceptionAgentAPI.h       \
    components/ringProtocol.h               \
    components/hyadesProtocol.h             \
    components/carcosaHash.h                \
    components/pipelineStateRegistry.h      \
    components/pingPongAgent.h              \
    components/fourStateAgent.h             \
    components/haliEvent.h                  \
    examples/SimplePipeline/simplePipelineExample.h \
    examples/MmioControl/mmioControlExample.h \
    components/vlaRegions.h

libcarcosa_la_LDFLAGS = -module -avoid-version

AM_CPPFLAGS += $(HMC_FLAG)
install-exec-hook:
	$(SST_REGISTER_TOOL) SST_ELEMENT_SOURCE     carcosa=$(abs_srcdir)
	$(SST_REGISTER_TOOL) SST_ELEMENT_TESTS      carcosa=$(abs_srcdir)/tests
