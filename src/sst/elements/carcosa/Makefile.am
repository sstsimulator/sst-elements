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
    components/pingPongAgent.cc             \
    components/pingPongAgent.h              \
    components/carcosaMemCtrl.cc            \
    components/carcosaMemCtrl.h             \
    components/sensorComponent.cc           \
    components/sensorComponent.h           \
    components/sensorEvent.h                \
    components/haliEvent.h

EXTRA_DIST = \
    hyades.h                                      \
    tests/pingpong.c                              \
    tests/testcarcosaPingPong.py                  \
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
    tests/testdynamicPM.py                        \
    tests/testhaliBacking.py                      \
    tests/testhaliMemH.py                         \
    tests/testhaliPM.py                           \
    tests/testmanagerLogic.py

sstdir = $(includedir)/sst/elements/carcosa
nobase_sst_HEADERS =                        \
    injectors/faultInjectorBase.h           \
    injectors/stuckAtFaultInjector.h        \
    injectors/corruptMemFaultInjector.h     \
    injectors/randomDropFaultInjector.h     \
    injectors/randomFlipFaultInjector.h     \
    injectors/dropFlipFaultInjector.h       \
    injectors/faultInjectorMemH.h           \
    faultlogic/faultBase.h                  \
    faultlogic/stuckAtFault.h               \
    faultlogic/corruptMemFault.h            \
    faultlogic/randomDropFault.h            \
    faultlogic/randomFlipFault.h            \
    faultlogic/randomFlipMemHFault.h        \
    components/pmDataRegistry.h             \
    components/faultInjManagerAPI.h        \
    components/interceptionAgentAPI.h      \
    components/pingPongAgent.h

libcarcosa_la_LDFLAGS = -module -avoid-version
libcarcosa_la_LIBADD =

AM_CPPFLAGS += $(HMC_FLAG)
install-exec-hook:
	$(SST_REGISTER_TOOL) SST_ELEMENT_SOURCE     carcosa=$(abs_srcdir)
	$(SST_REGISTER_TOOL) SST_ELEMENT_TESTS      carcosa=$(abs_srcdir)/tests
