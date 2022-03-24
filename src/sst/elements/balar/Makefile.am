# -*- Makefile -*-
#
#

AM_CPPFLAGS += \
	$(MPI_CPPFLAGS) \
	-I$(top_srcdir)/src \
	$(CUDA_CPPFLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libbalar.la

libbalar_la_SOURCES = \
	mempool.h \
	mempool.cpp \
	balar_event.h \
	cuda_runtime_api.h \
	balar.cc \
	balar.h

EXTRA_DIST = \
	tests/ariel-gpu-v100.cfg \
	tests/cuda-test.py \
	tests/gpgpusim.config \
	tests/testsuite_default_balar.py \
	tests/utils.py \
	tests/vectorAdd/vecAdd.cu \
	tests/vectorAdd/Makefile \
	tests/refFiles/test_gpgpu_vectorAdd.out


libbalar_la_LDFLAGS = \
	-lm \
	-avoid-version

libbalar_la_LIBADD = \
	$(SST_SYSTEMC_LIB) \
	-L$(GPGPUSIM_LIBDIR) \
	$(GPGPUSIM_LIB)

install-exec-hook:
	$(SST_REGISTER_TOOL) SST_ELEMENT_SOURCE balar=$(abs_srcdir)
	$(SST_REGISTER_TOOL) SST_ELEMENT_TESTS  balar=$(abs_srcdir)/tests
