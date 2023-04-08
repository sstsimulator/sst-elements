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
	balar_event.h \
	cuda_runtime_api.h \
	util.cc \
	util.h \
	balarMMIO.cc \
	balarMMIO.h \
	dmaEngine.cc \
	dmaEngine.h \
	testcpu/balarTestCPU.h \
	testcpu/balarTestCPU.cc

EXTRA_DIST = \
	tests/gpu-v100-mem.cfg \
	tests/testBalar-vanadis.py \
	tests/testBalar-testcpu.py \
	tests/gpgpusim.config \
	tests/testsuite_default_balar.py \
	tests/utils.py \
	tests/vectorAdd/vecAdd.cu \
	tests/vectorAdd/Makefile \
	tests/vanadisHandshake/vanadisHandshake.c \
	tests/vanadisHandshake/cuda_runtime_api.c \
	tests/vanadisHandshake/cuda_runtime_api.h \
	tests/vanadisHandshake/Makefile \
	tests/refFiles/test_gpgpu_vectorAdd.out

libbalar_la_LDFLAGS = \
	-lm \
	-avoid-version \
	$(AM_LDFLAGS)

libbalar_la_LIBADD = \
	-L$(GPGPUSIM_LIBDIR) \
	$(GPGPUSIM_LIB)

install-exec-hook:
	$(SST_REGISTER_TOOL) SST_ELEMENT_SOURCE balar=$(abs_srcdir)
	$(SST_REGISTER_TOOL) SST_ELEMENT_TESTS  balar=$(abs_srcdir)/tests
