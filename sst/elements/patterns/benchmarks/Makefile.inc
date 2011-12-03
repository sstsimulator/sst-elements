# -*- Makefile -*-
CC = $(MPICC)
CXX = $(MPICXX)

# Allreduce_bench
allreduce_bench_CPPFLAGS = -DNOT_PART_OF_SIM -D_PATTERNS_H_ $(MPI_CPPFLAGS)
allreduce_bench_LDADD = $(MPILIBS) -lm
allreduce_bench_SOURCES = \
	benchmarks/allreduce_bench.cc \
	util/stats.cc \
	benchmarks/stat_p.c \
	collective_patterns/collective_topology.cc \
	benchmarks/util.c \
	benchmarks/Collectives/allreduce.cc

bin_PROGRAMS += allreduce_bench



# Alltoall_bench
alltoall_bench_CPPFLAGS = -DNOT_PART_OF_SIM -D_PATTERNS_H_ $(MPI_CPPFLAGS)
alltoall_bench_LDADD = $(MPILIBS) -lm
alltoall_bench_SOURCES = \
	benchmarks/alltoall_bench.cc \
	util/stats.cc \
	benchmarks/stat_p.c \
	collective_patterns/collective_topology.cc \
	benchmarks/util.c \
	benchmarks/Collectives/alltoall.cc

bin_PROGRAMS += alltoall_bench



# memcpy benchmark
memcpy_CPPFLAGS = -DNOT_PART_OF_SIM -D_PATTERNS_H_ $(MPI_CPPFLAGS)
memcpy_LDADD = $(MPILIBS) -lm
memcpy_SOURCES = \
	benchmarks/memcpy.cc \
	util/stats.cc \
	benchmarks/stat_p.c \
	benchmarks/util.c

bin_PROGRAMS += memcpy



# vector_add benchmark
vector_add_CPPFLAGS = -DNOT_PART_OF_SIM -D_PATTERNS_H_ $(MPI_CPPFLAGS)
vector_add_LDADD = $(MPILIBS) -lm
vector_add_SOURCES = \
	benchmarks/vector_add.cc \
	util/stats.cc \
	benchmarks/stat_p.c \
	benchmarks/util.c

bin_PROGRAMS += vector_add



# ping-pong and ping-ping benchmark
pingpong_bench_CPPFLAGS = -DNOT_PART_OF_SIM -D_PATTERNS_H_ $(MPI_CPPFLAGS)
pingpong_bench_LDADD = $(MPILIBS) -lm
pingpong_bench_SOURCES = \
	benchmarks/pingpong_bench.c \
	benchmarks/stat_p.c \
	benchmarks/util.c

bin_PROGRAMS += pingpong_bench



# router characteristic benchmark
router_bench_CPPFLAGS = -DNOT_PART_OF_SIM -D_PATTERNS_H_ $(MPI_CPPFLAGS)
router_bench_LDADD = $(MPILIBS) -lm
router_bench_SOURCES = \
	benchmarks/router_bench.c \
	benchmarks/stat_p.c \
	benchmarks/util.c

bin_PROGRAMS += router_bench



# message rate benchmark
msgrate_bench_CPPFLAGS = -DNOT_PART_OF_SIM -D_PATTERNS_H_ $(MPI_CPPFLAGS)
msgrate_bench_LDADD = $(MPILIBS) -lm
msgrate_bench_SOURCES = \
	benchmarks/msgrate_bench.c \
	benchmarks/stat_p.c \
	benchmarks/util.c

bin_PROGRAMS += msgrate_bench



include benchmarks/src_ghost_bench/Makefile.inc
include benchmarks/src_work_stealing_bench/Makefile.inc
include benchmarks/src_fft_bench/Makefile.inc
