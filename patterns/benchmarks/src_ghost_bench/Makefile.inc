# -*- Makefile -*-

ghost_bench_CPPFLAGS = -DNOT_PART_OF_SIM -D_PATTERNS_H_ $(MPI_CPPFLAGS)
ghost_bench_LDADD = $(MPILIBS) -lm
ghost_bench_SOURCES = \
	benchmarks/src_ghost_bench/ghost_driver.cc \
	benchmarks/src_ghost_bench/ghost.cc \
	benchmarks/src_ghost_bench/ghost.h \
	benchmarks/src_ghost_bench/memory.c \
	benchmarks/src_ghost_bench/memory.h \
	benchmarks/src_ghost_bench/neighbors.c \
	benchmarks/src_ghost_bench/neighbors.h \
	benchmarks/src_ghost_bench/ranks.c \
	benchmarks/src_ghost_bench/ranks.h \
	benchmarks/src_ghost_bench/work.c \
	benchmarks/src_ghost_bench/work.h \
	benchmarks/util.c benchmarks/util.h \
	benchmarks/stat_p.c benchmarks/stat_p.h \
	benchmarks/Collectives/allreduce.cc \
	benchmarks/Collectives/allreduce.h \
	collective_patterns/collective_topology.cc \
	collective_patterns/collective_topology.h \
	util/stats.h \
	util/stats.cc

bin_PROGRAMS += ghost_bench

