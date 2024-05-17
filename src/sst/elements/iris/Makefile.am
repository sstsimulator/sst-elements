# -*- Makefile -*-
#
#

SUBDIRS = libfabric

AM_CPPFLAGS += \
	$(MPI_CPPFLAGS) \
        -I$(top_srcdir)/src/sst/elements/iris \
        -I$(top_srcdir)/src/sst/elements/mercury \
        -I$(top_srcdir)/src/sst/elements

# unpleasant hack to make vintage automake (e.g. 1.13.4) work
AM_LIBTOOLFLAGS = --tag=CXX

compdir = $(pkglibdir)
comp_LTLIBRARIES = libsumi.la
libsumi_la_SOURCES = \
  sumi/allgather.cc \
  sumi/allgatherv.cc  \
  sumi/allreduce.cc \
  sumi/alltoall.cc \
  sumi/alltoallv.cc \
  sumi/bcast.cc \
  sumi/collective.cc \
  sumi/collective_actor.cc \
  sumi/collective_message.cc \
  sumi/communicator.cc \
  sumi/dense_rank_map.cc \
  sumi/gather.cc \
  sumi/gatherv.cc \
  sumi/message.cc \
  sumi/monitor.cc \
  sumi/rank_mapper.cc \
  sumi/reduce.cc \
  sumi/reduce_scatter.cc \
  sumi/scan.cc \
  sumi/scatter.cc \
  sumi/scatterv.cc  \
  sumi/sim_transport.cc \
  sumi/sumi.cc \
  sumi/sumi_thread.cc

library_includedir=$(includedir)/sst/elements/iris

nobase_library_include_HEADERS = \
  sumi/allgather.h \
  sumi/allgatherv.h \
  sumi/allreduce.h \
  sumi/alltoall.h \
  sumi/alltoallv.h \
  sumi/bcast.h \
  sumi/collective.h \
  sumi/collective_actor.h \
  sumi/collective_actor_fwd.h \
  sumi/collective_message.h \
  sumi/collective_message_fwd.h \
  sumi/comm_functions.h \
  sumi/communicator.h \
  sumi/communicator_fwd.h \
  sumi/dense_rank_map.h \
  sumi/gather.h \
  sumi/gatherv.h \
  sumi/message.h \
  sumi/message_fwd.h \
  sumi/monitor.h \
  sumi/network_id.h \
  sumi/null_buffer.h \
  sumi/options.h \
  sumi/rank_mapper.h \
  sumi/reduce.h \
  sumi/reduce_scatter.h \
  sumi/scan.h \
  sumi/scatter.h \
  sumi/scatterv.h \
  sumi/sim_transport.h \
  sumi/sumi.h \
  sumi/sumi_thread.h \
  sumi/timeout.h \
  sumi/transport.h \
  sumi/transport_fwd.h

if !SST_ENABLE_PREVIEW_BUILD
libsumi_la_SOURCES += $(deprecated_libsumi_sources)
endif

EXTRA_DIST =
deprecated_EXTRA_DIST =

if !SST_ENABLE_PREVIEW_BUILD
EXTRA_DIST += $(deprecated_EXTRA_DIST)
endif

libsumi_la_LDFLAGS = -module -avoid-version

install-exec-hook:
	$(SST_REGISTER_TOOL) SST_ELEMENT_SOURCE     iris=$(abs_srcdir)

# This sed script converts 'od' output to a comma-separated list of byte-
# values, suitable for #include'ing into an array definition.
# This can be done much more simply with xxd or hexdump, but those tools
# are not installed by default on all supported platforms.
#
# od:   -v:             Print all data
#               -t x1:  Print as byte-values, in hex
# sed:  Script 1:  Remove base-address column from od output
#               Script 2:  Remove trailing blank line resulting from script 1
#               Script 3:  Add '0x' prefix, and ',' suffix to each value
%.inc: %.py
	od -v -t x1 < $< | sed -e 's/^[^ ]*[ ]*//g' -e '/^\s*$$/d' -e 's/\([0-9a-f]*\)[ $$]*/0x\1,/g' > $@

##########################################################################
##########################################################################
##########################################################################
