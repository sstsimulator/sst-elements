# -*- Makefile -*-

ws_bench_CPPFLAGS = -DNOT_PART_OF_SIM -D_PATTERNS_H_ $(MPI_CPPFLAGS)
ws_bench_LDADD = $(MPILIBS) -lm
ws_bench_SOURCES = \
	benchmarks/src_work_stealing_bench/driver.cc \
	benchmarks/src_work_stealing_bench/ws.h \
	benchmarks/src_work_stealing_bench/ws.cc \
	benchmarks/util.h \
	benchmarks/util.c \
	benchmarks/stat_p.h \
	benchmarks/stat_p.c \
	util/stats.h \
	util/stats.cc \
	benchmarks/Collectives/allreduce.h \
	benchmarks/Collectives/allreduce.cc \
	collective_patterns/collective_topology.h \
	collective_patterns/collective_topology.cc \
	util/msg_counter.h \
	util/msg_counter.cc

bin_PROGRAMS += ws_bench

