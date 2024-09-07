# -*- Makefile -*-
#
#

comp_LTLIBRARIES = libmask_mpi.la sendrecv.la reduce.la alltoall.la allgather.la

compdir = $(pkglibdir)

AM_CPPFLAGS += -I$(top_srcdir)/src/sst/elements

libmask_mpi_la_SOURCES = \
  mpi_comm/mpi_comm.cc \
  mpi_comm/mpi_group.cc \
  mpi_comm/keyval.cc \
  mpi_comm/mpi_comm_factory.cc \
  mpi_comm/mpi_comm_cart.cc \
  mpi_queue/mpi_queue_probe_request.cc \
  mpi_queue/mpi_queue_recv_request.cc \
  mpi_queue/mpi_queue.cc \
  mpi_protocol/mpi_protocol.cc \
  mpi_protocol/eager1.cc \
  mpi_protocol/eager0.cc \
  mpi_protocol/rendezvous_rdma.cc \
  mpi_types/mpi_type.cc \
  otf2_output_stat.cc \
  mask_mpi.cc \
  mpi_api.cc \
  mpi_api_win.cc \
  mpi_api_collectives.cc \
  mpi_api_comm.cc \
  mpi_api_group.cc \
  mpi_api_probe.cc \
  mpi_api_send_recv.cc \
  mpi_api_test.cc \
  mpi_api_type.cc \
  mpi_api_vcollectives.cc \
  mpi_api_wait.cc \
  mpi_debug.cc \
  mpi_delay_stats.cc \
  mpi_message.cc \
  mpi_request.cc

library_includedir=$(includedir)/sst/elements/mask-mpi

library_include_HEADERS = \
  mpi_comm/keyval.h \
  mpi_comm/keyval_fwd.h \
  mpi_comm/mpi_comm.h \
  mpi_comm/mpi_comm_fwd.h \
  mpi_comm/mpi_comm_factory.h \
  mpi_comm/mpi_comm_cart.h \
  mpi_comm/mpi_group.h \
  mpi_queue/mpi_queue_probe_request_fwd.h \
  mpi_queue/mpi_queue_recv_request_fwd.h \
  mpi_queue/mpi_queue_probe_request.h \
  mpi_queue/mpi_queue_recv_request.h \
  mpi_queue/mpi_queue.h \
  mpi_queue/mpi_queue_fwd.h \
  mpi_protocol/mpi_protocol.h \
  mpi_protocol/mpi_protocol_fwd.h \
  mpi_types/mpi_type.h \
  mpi_types/mpi_type_fwd.h \
  mpi_api.h \
  mpi_api_fwd.h \
  mpi_debug.h \
  mpi_delay_stats.h \
  mpi_message.h \
  mpi_request.h \
  mpi_request_fwd.h \
  mpi_status.h \
  mpi_status_fwd.h \
  mpi_types.h \
  mpi_wrapper.h \
  mpi_integers.h \
  mpi_call.h \
  otf2_output_stat.h \
  otf2_output_stat_fwd.h \
  mask_mpi.h \
  mask_mpi_macro.h \
  unusedvariablemacro.h

sendrecv_la_SOURCES = tests/sendrecv.cc
reduce_la_SOURCES = tests/reduce.cc
alltoall_la_SOURCES = tests/alltoall.cc
allgather_la_SOURCES = tests/allgather.cc

EXTRA_DIST = \
 tests/testsuite_default_mask_mpi.py \
 tests/platform_file_mask_mpi_test.py \
 tests/test_reduce.py \
 tests/test_sendrecv.py \
 tests/test_alltoall.py \
 tests/test_allgather.py \
 tests/refFiles/test_reduce.out \
 tests/refFiles/test_sendrecv.out \
 tests/refFiles/test_alltoall.out \
 tests/refFiles/test_allgather.out

libmask_mpi_la_LDFLAGS = -module -avoid-version
sendrecv_la_LDFLAGS = -module -avoid-version
reduce_la_LDFLAGS = -module -avoid-version
alltoall_la_LDFLAGS = -module -avoid-version
allgather_la_LDFLAGS = -module -avoid-version

install-exec-hook: 
	$(SST_REGISTER_TOOL) SST_ELEMENT_SOURCE     mask-mpi=$(abs_srcdir)
	$(SST_REGISTER_TOOL) SST_ELEMENT_TESTS      mask-mpi=$(abs_srcdir)/tests
