# -*- Makefile -*-
#
#

AM_CPPFLAGS += \
	$(MPI_CPPFLAGS) \
	-I$(top_srcdir)/src

compdir = $(pkglibdir)
comp_LTLIBRARIES = libcarcosa.la
libcarcosa_la_SOURCES =             \
    faultInjectorBase.cc            \
    faultInjectorBase.h             \
    faultlogic/stuckAtFault.cc      \
    faultlogic/stuckAtFault.h       \
    faultlogic/corruptMemFault.cc  \
    faultlogic/corruptMemFault.h

EXTRA_DIST = \
    dummyTest.py

sstdir = $(includedir)/sst/elements/carcosa
nobase_sst_HEADERS =                \
    faultInjectorBase.h             \
    faultlogic/stuckAtFault.h       \
    faultlogic/corruptMemFault.h

libcarcosa_la_LDFLAGS = -module -avoid-version
libcarcosa_la_LIBADD =

AM_CPPFLAGS += $(HMC_FLAG)
install-exec-hook:
	$(SST_REGISTER_TOOL) SST_ELEMENT_SOURCE     carcosa=$(abs_srcdir)
	$(SST_REGISTER_TOOL) SST_ELEMENT_TESTS      carcosa=$(abs_srcdir)/tests
