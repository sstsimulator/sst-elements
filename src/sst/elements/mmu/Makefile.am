# -*- Makefile -*-
#
#

AM_CPPFLAGS += \
	$(MPI_CPPFLAGS)\
	-I$(top_srcdir)/src

compdir = $(pkglibdir)
comp_LTLIBRARIES = libmmu.la

libmmu_la_SOURCES = \
	mmu.cc \
	mmuEvents.h \
	mmu.h \
	mmuTypes.h \
	simpleMMU.cc \
	simpleMMU.h \
	simpleTLB.cc \
	simpleTLB.h \
	tlb.h \
	tlbWrapper.cc \
	tlbWrapper.h


libmmu_la_LDFLAGS = -module -avoid-version

EXTRA_DIST = 

install-exec-hook:
	$(SST_REGISTER_TOOL) SST_ELEMENT_SOURCE     mmu=$(abs_srcdir)
	$(SST_REGISTER_TOOL) SST_ELEMENT_TESTS      mmu=$(abs_srcdir)/tests
