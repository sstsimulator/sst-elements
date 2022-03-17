# -*- Makefile -*-
#
#

compdir = $(pkglibdir)
comp_LTLIBRARIES = libSamba.la

libSamba_la_SOURCES = \
	libSamba.cc \
	utils.h \
	Samba_Event.h \
	Samba.cc \
	Samba.h \
	TLBUnit.cc \
	TLBUnit.h \
	TLBentry.h \
	TLBhierarchy.h \
	TLBhierarchy.cc \
	PageTableWalker.h \
	PageTableWalker.cc \
	PageFaultHandler.h \
	SimpleTLB.cc \
	SimpleTLB.h 


libSamba_la_CPPFLAGS = \
	-I$(top_srcdir)/src \
	$(SST_CXX0X_FLAGS) -fPIC -Wall \
	$(MPI_CPPFLAGS)

libSamba_la_LDFLAGS = \
	-lm \
	-avoid-version

libSamba_la_LIBADD = \
	$(SST_SYSTEMC_LIB)

EXTRA_DIST = \
	tests/testsuite_default_Samba.py \
	tests/gupsgen_mmu_4KB.py \
	tests/gupsgen_mmu.py \
	tests/gupsgen_mmu_three_levels.py \
	tests/stencil3dbench_mmu.py \
	tests/streambench_mmu.py \
	tests/refFiles/test_Samba_gupsgen_mmu.out \
	tests/refFiles/test_Samba_gupsgen_mmu_4KB.out \
	tests/refFiles/test_Samba_gupsgen_mmu_three_levels.out \
	tests/refFiles/test_Samba_stencil3dbench_mmu.out \
	tests/refFiles/test_Samba_streambench_mmu.out

install-exec-hook:
	$(SST_REGISTER_TOOL) SST_ELEMENT_SOURCE     Samba=$(abs_srcdir)
	$(SST_REGISTER_TOOL) SST_ELEMENT_TESTS      Samba=$(abs_srcdir)/tests
