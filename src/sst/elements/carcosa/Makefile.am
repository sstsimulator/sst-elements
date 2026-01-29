# -*- Makefile -*-
#
#

AM_CPPFLAGS += \
	$(MPI_CPPFLAGS) \
	-I$(top_srcdir)/src

compdir = $(pkglibdir)
comp_LTLIBRARIES = libcarcosa.la
libcarcosa_la_SOURCES =                     \
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
    injectors/dropFlipFaultInjector.cc        \
    injectors/dropFlipFaultInjector.h         \
    faultlogic/faultBase.cc                 \
    faultlogic/faultBase.h                  \
    faultlogic/stuckAtFault.cc              \
    faultlogic/stuckAtFault.h               \
    faultlogic/corruptMemFault.cc           \
    faultlogic/corruptMemFault.h            \
    faultlogic/randomDropFault.cc           \
    faultlogic/randomDropFault.h            \
    faultlogic/randomFlipFault.cc           \
    faultlogic/randomFlipFault.h

EXTRA_DIST = \
    tests/testCorruptMemBasic.py                  \
    tests/testCorruptMemDouble.py                 \
    tests/testCorruptMemDoubleOverlap.py          \
    tests/testRandomDrop.py                       \
    tests/testRandomFlip.py                       \
    tests/testStuckAtBasic.py                     \
    tests/testStuckAtMultiple.py                  \
    tests/testStuckAtOverlap.py                   \
    tests/testStuckAtSameByte.py

sstdir = $(includedir)/sst/elements/carcosa
nobase_sst_HEADERS =                        \
    injectors/faultInjectorBase.h           \
    injectors/stuckAtFaultInjector.h        \
    injectors/corruptMemFaultInjector.h     \
    injectors/randomDropFaultInjector.h     \
    injectors/randomFlipFaultInjector.h     \
    injectors/dropFlipFaultInjector.h       \
    faultlogic/faultBase.h                  \
    faultlogic/stuckAtFault.h               \
    faultlogic/corruptMemFault.h            \
    faultlogic/randomDropFault.h            \
    faultlogic/randomFlipFault.h

libcarcosa_la_LDFLAGS = -module -avoid-version
libcarcosa_la_LIBADD =

AM_CPPFLAGS += $(HMC_FLAG)
install-exec-hook:
	$(SST_REGISTER_TOOL) SST_ELEMENT_SOURCE     carcosa=$(abs_srcdir)
	$(SST_REGISTER_TOOL) SST_ELEMENT_TESTS      carcosa=$(abs_srcdir)/tests
