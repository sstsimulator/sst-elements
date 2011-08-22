# -*- Makefile -*-
#
#

AM_CPPFLAGS = $(BOOST_CPPFLAGS) $(MPI_CPPFLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libghost_pattern.la libmsgrate_pattern.la libpingpong_pattern.la

libghost_pattern_la_SOURCES = pattern_common.h pattern_common.cc \
                              ghost_pattern.h ghost_pattern.cc
libghost_pattern_la_LDFLAGS = -module -avoid-version

libmsgrate_pattern_la_SOURCES = msgrate_pattern.h msgrate_pattern.cc \
			    comm_pattern.h comm_pattern.cc \
			    state_machine.h state_machine.cc \
			    pattern_common.h pattern_common.cc \
			    allreduce.h allreduce.cc \
			    collective_topology.h collective_topology.cc
libmsgrate_pattern_la_LDFLAGS = -module -avoid-version

libpingpong_pattern_la_SOURCES = ping_pong.h ping_pong.cc \
			    comm_pattern.h comm_pattern.cc \
			    state_machine.h state_machine.cc \
			    pattern_common.h pattern_common.cc \
			    barrier.h barrier.cc \
			    allreduce.h allreduce.cc \
			    collective_topology.h collective_topology.cc
libpingpong_pattern_la_LDFLAGS = -module -avoid-version

include src_genPatterns/Makefile.inc

CXXLD = $(MPICXX)
allreduce_bench_LDFLAGS = -lmpi
allreduce_bench_SOURCES = allreduce_bench.cc 
msgrate_bench_LDFLAGS = -lmpi
msgrate_bench_SOURCES = msgrate_bench.cc 
bin_PROGRAMS += allreduce_bench msgrate_bench

