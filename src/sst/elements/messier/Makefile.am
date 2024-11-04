# -*- Makefile -*-
#
#

AM_CPPFLAGS += \
	$(MPI_CPPFLAGS) \
	-I$(top_srcdir)/src

AM_LDFLAGS = -lm
compdir = $(pkglibdir)
comp_LTLIBRARIES = libMessier.la

libMessier_la_SOURCES = \
	cache.h \
	libMessier.cc \
	memReqEvent.h \
	messier.cc \
	messier.h \
	bank.h \
	rank.h \
        messier_event.h \
	writeBuffer.h \
	writeBuffer.cc \
	nvm_request.h \
	nvm_dimm.h \
	nvm_dimm.cc \
	nvm_params.h

libMessier_la_LDFLAGS = \
	-avoid-version

libMessier_la_LIBADD = \
	$(SST_SYSTEMC_LIB)

EXTRA_DIST = \
	tests/testsuite_default_Messier.py \
	tests/gupsgen_2RANKS.py \
	tests/gupsgen_fastNVM.py \
	tests/gupsgen.py \
	tests/stencil3dbench_messier.py \
	tests/streambench_messier.py \
	tests/refFiles/test_Messier_gupsgen.out \
	tests/refFiles/test_Messier_gupsgen_2RANKS.out \
	tests/refFiles/test_Messier_gupsgen_fastNVM.out \
	tests/refFiles/test_Messier_stencil3dbench_messier.out \
	tests/refFiles/test_Messier_streambench_messier.out

install-exec-hook:
	$(SST_REGISTER_TOOL) SST_ELEMENT_SOURCE     Messier=$(abs_srcdir)
	$(SST_REGISTER_TOOL) SST_ELEMENT_TESTS      Messier=$(abs_srcdir)/tests
