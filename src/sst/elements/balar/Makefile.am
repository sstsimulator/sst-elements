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
	balar_consts.h \
	balar_event.h \
	balar_packet.h \
	balarMMIO.cc \
	balarMMIO.h \
	cuda_runtime_api.h \
	dmaEngine.cc \
	dmaEngine.h \
	util.cc \
	util.h \
	testcpu/balarTestCPU.cc \
	testcpu/balarTestCPU.h

EXTRA_DIST = \
	tests/refFiles/test_gpgpu_helloworld.out \
	tests/refFiles/test_gpgpu_rodinia-2.0-backprop-1024.out \
	tests/refFiles/test_gpgpu_rodinia-2.0-backprop-2048.out \
	tests/refFiles/test_gpgpu_rodinia-2.0-backprop-short.out \
	tests/refFiles/test_gpgpu_rodinia-2.0-bfs-graph4096.out \
	tests/refFiles/test_gpgpu_rodinia-2.0-bfs-SampleGraph.out \
	tests/refFiles/test_gpgpu_rodinia-2.0-heartwall-1.out \
	tests/refFiles/test_gpgpu_rodinia-2.0-hotspot-30-6-40.out \
	tests/refFiles/test_gpgpu_rodinia-2.0-lud-64.out \
	tests/refFiles/test_gpgpu_rodinia-2.0-lud-256.out \
	tests/refFiles/test_gpgpu_rodinia-2.0-nn-4-3-30-90.out \
	tests/refFiles/test_gpgpu_rodinia-2.0-nw-128-10.out \
	tests/refFiles/test_gpgpu_rodinia-2.0-pathfinder-1000-20-5.out \
	tests/refFiles/test_gpgpu_rodinia-2.0-srad_v2-128x128.out \
	tests/refFiles/test_gpgpu_rodinia-2.0-streamcluster-3_6_16_1024_1024_100_none_1.out \
	tests/refFiles/test_gpgpu_simpleStreams.out \
	tests/refFiles/test_gpgpu_vanadisHandshake.out \
	tests/refFiles/test_gpgpu_vecadd.out \
	tests/refFiles/test_gpgpu_vectorAdd.out \
	tests/vanadisHandshake/vanadisHandshake.c \
	tests/vanadisHandshake/cuda_runtime_api.c \
	tests/vanadisHandshake/cuda_runtime_api.h \
	tests/vanadisHandshake/Makefile \
	tests/vanadisLLVMRISCV/cudaSamplesCommon/exception.h \
	tests/vanadisLLVMRISCV/cudaSamplesCommon/helper_cuda.h \
	tests/vanadisLLVMRISCV/cudaSamplesCommon/helper_functions.h \
	tests/vanadisLLVMRISCV/cudaSamplesCommon/helper_image.h \
	tests/vanadisLLVMRISCV/cudaSamplesCommon/helper_string.h \
	tests/vanadisLLVMRISCV/cudaSamplesCommon/helper_timer.h \
	tests/vanadisLLVMRISCV/balar_vanadis.h \
	tests/vanadisLLVMRISCV/cuda_runtime_api_vanadis.cc \
	tests/vanadisLLVMRISCV/helloworld.c \
	tests/vanadisLLVMRISCV/Makefile \
	tests/vanadisLLVMRISCV/simpleStreams.cu \
	tests/vanadisLLVMRISCV/vecadd.cu \
	tests/vectorAdd/cuda_calls.trace \
	tests/vectorAdd/cuMemcpyD2H-0-524288.data \
	tests/vectorAdd/cuMemcpyH2D-0-524288.data \
	tests/vectorAdd/cuMemcpyH2D-1-524288.data \
	tests/vectorAdd/Makefile \
	tests/vectorAdd/vecAdd.cu \
	tests/balarBlock.py \
	tests/gpgpusim.config \
	tests/gpu-v100-mem.cfg \
	tests/memory.py \
	tests/testBalar_testsuite_util.py \
	tests/testBalar-testcpu.py \
	tests/testBalar-vanadis.py \
	tests/testsuite_default_balar_long.py \
	tests/testsuite_default_balar_medium.py \
	tests/testsuite_default_balar_simple.py \
	tests/utils.py \
	tests/vanadisBlock.py \
	tests/vanadisOS.py

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
