# -*- Makefile -*-
#
#

AM_CPPFLAGS = $(BOOST_CPPFLAGS) $(MPI_CPPFLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libghost_pattern.la libmsgrate_pattern.la libpingpong_pattern.la \
			liballreduce_pattern.la

libghost_pattern_la_SOURCES = pattern_common.h pattern_common.cc \
                              ghost_pattern.h ghost_pattern.cc
libghost_pattern_la_LDFLAGS = -module -avoid-version

libmsgrate_pattern_la_SOURCES = msgrate_pattern.h msgrate_pattern.cc \
			    comm_pattern.h comm_pattern.cc \
			    state_machine.h state_machine.cc \
			    pattern_common.h pattern_common.cc \
			    allreduce_op.h allreduce_op.cc \
			    collective_topology.h collective_topology.cc
libmsgrate_pattern_la_LDFLAGS = -module -avoid-version

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

CC = $(MPICC)
CXX = $(MPICXX)
CXXLD = $(MPICXX)
CCLD = $(MPICC)
bin_PROGRAMS = 
include src_genPatterns/Makefile.inc
include src_ghost_bench/Makefile.inc

# allreduce_bench_LDFLAGS = -lmpi -lmpi_cxx
allreduce_bench_SOURCES = allreduce_bench.cc
# msgrate_bench_LDFLAGS = -lmpi -lmpi_cxx
msgrate_bench_SOURCES = msgrate_bench.cc 
pingpong_bench_SOURCES = pingpong_bench.c
bin_PROGRAMS += allreduce_bench msgrate_bench pingpong_bench

