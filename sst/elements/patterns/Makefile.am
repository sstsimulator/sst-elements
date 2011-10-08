# -*- Makefile -*-
#
#

AM_CPPFLAGS = $(BOOST_CPPFLAGS) $(MPI_CPPFLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libmsgrate_pattern.la libpingpong_pattern.la \
			liballreduce_pattern.la liballtoall_pattern.la \
			libghost_pattern.la libfft_pattern.la

libmsgrate_pattern_la_SOURCES = msgrate_pattern.h msgrate_pattern.cc \
			    comm_pattern.h comm_pattern.cc \
			    state_machine.h state_machine.cc \
			    pattern_common.h pattern_common.cc \
			    allreduce_op.h allreduce_op.cc \
			    collective_topology.h collective_topology.cc
libmsgrate_pattern_la_LDFLAGS = -module -avoid-version

libghost_pattern_la_SOURCES = ghost_pattern.h ghost_pattern.cc \
			    comm_pattern.h comm_pattern.cc \
			    state_machine.h state_machine.cc \
			    pattern_common.h pattern_common.cc \
			    allreduce_op.h allreduce_op.cc \
			    collective_topology.h collective_topology.cc \
			    Benchmarks/src_ghost_bench/memory.h Benchmarks/src_ghost_bench/memory.c \
			    Benchmarks/src_ghost_bench/ranks.h Benchmarks/src_ghost_bench/ranks.c \
			    Benchmarks/src_ghost_bench/neighbors.h Benchmarks/src_ghost_bench/neighbors.c \
			    Benchmarks/src_ghost_bench/work.h Benchmarks/src_ghost_bench/work.c
libghost_pattern_la_LDFLAGS = -module -avoid-version

libpingpong_pattern_la_SOURCES = pingpong_pattern.h pingpong_pattern.cc \
			    comm_pattern.h comm_pattern.cc \
			    state_machine.h state_machine.cc \
			    pattern_common.h pattern_common.cc \
			    barrier_op.h barrier_op.cc \
			    allreduce_op.h allreduce_op.cc \
			    collective_topology.h collective_topology.cc
libpingpong_pattern_la_LDFLAGS = -module -avoid-version

liballreduce_pattern_la_SOURCES = allreduce_pattern.h allreduce_pattern.cc \
			    comm_pattern.h comm_pattern.cc \
			    state_machine.h state_machine.cc \
			    pattern_common.h pattern_common.cc \
			    barrier_op.h barrier_op.cc \
			    allreduce_op.h allreduce_op.cc \
			    stats.h stats.cc \
			    collective_topology.h collective_topology.cc
liballreduce_pattern_la_LDFLAGS = -module -avoid-version

liballtoall_pattern_la_SOURCES = alltoall_pattern.h alltoall_pattern.cc \
			    comm_pattern.h comm_pattern.cc \
			    state_machine.h state_machine.cc \
			    pattern_common.h pattern_common.cc \
			    barrier_op.h barrier_op.cc \
			    allreduce_op.h allreduce_op.cc \
			    collective_topology.h collective_topology.cc \
			    alltoall_op.h alltoall_op.cc \
			    stats.h stats.cc
liballtoall_pattern_la_LDFLAGS = -module -avoid-version

libfft_pattern_la_SOURCES = fft_pattern.h fft_pattern.cc \
			    comm_pattern.h comm_pattern.cc \
			    state_machine.h state_machine.cc \
			    pattern_common.h pattern_common.cc \
			    scatter_op.h scatter_op.cc \
			    gather_op.h gather_op.cc \
			    collective_topology.h collective_topology.cc \
			    stats.h stats.cc
libfft_pattern_la_LDFLAGS = -module -avoid-version

bin_PROGRAMS =
include src_genPatterns/Makefile.inc
