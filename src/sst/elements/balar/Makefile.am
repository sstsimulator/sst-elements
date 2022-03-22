AM_CPPFLAGS = \
	$(MPIT_CPPFLAGS)

AM_LDFLAGS = -lm
compdir = $(pkglibdir)
comp_LTLIBRARIES = libbalar.la

libbalar_la_SOURCES = \
	mempool.h \
	mempool.cpp \
	balar_event.h \
	cuda_runtime_api.h \
	balar.cc \
	balar.h \
	util.cc \
	util.h \
	balarMMIO.cc \
	balarMMIO.h \
	testcpu/balarTestCPU.h \
	testcpu/balarTestCPU.cc

EXTRA_DIST = \
	tests/ariel-gpu-v100.cfg \
	tests/cuda-test.py \
	tests/testBalar-simple.py \
	tests/gpgpusim.config \
	tests/testsuite_default_balar.py \
	tests/utils.py \
	tests/vectorAdd/vecAdd.cu \
	tests/vectorAdd/Makefile \
	tests/refFiles/test_gpgpu_vectorAdd.out

libbalar_la_CPPFLAGS = \
	-I$(top_srcdir)/src \
	$(SST_CXX0X_FLAGS) -fPIC -Wall \
	$(MPI_CPPFLAGS) \
	$(CUDA_CPPFLAGS) \
	-fdiagnostics-color=always

libbalar_la_LDFLAGS = \
	-avoid-version

libbalar_la_LIBADD = \
	$(SST_SYSTEMC_LIB) \
	-L$(GPGPUSIM_LIBDIR) \
	$(GPGPUSIM_LIB)

install-exec-hook:
	$(SST_REGISTER_TOOL) SST_ELEMENT_SOURCE balar=$(abs_srcdir)
	$(SST_REGISTER_TOOL) SST_ELEMENT_TESTS  balar=$(abs_srcdir)/tests
