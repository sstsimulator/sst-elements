# -*- Makefile -*-
#
#

comp_LTLIBRARIES += \
	libmsgrate_pattern.la \
	libpingpong_pattern.la \
	liballreduce_pattern.la \
	liballtoall_pattern.la \
	libghost_pattern.la \
	libfft_pattern.la

support = \
	support/NIC_model.cc \
	support/comm_pattern.cc \
	support/machine_info.cc \
	support/NIC_stats.cc \
	support/pattern_common.cc \
	support/routing.cc \
	support/state_machine.cc

collectives = \
	collective_patterns/barrier_op.h \
	collective_patterns/barrier_op.cc \
	collective_patterns/allreduce_op.h \
	collective_patterns/allreduce_op.cc \
	collective_patterns/alltoall_op.h \
	collective_patterns/alltoall_op.cc \
	collective_patterns/scatter_op.h \
	collective_patterns/scatter_op.cc \
	collective_patterns/gather_op.h \
	collective_patterns/gather_op.cc \
	collective_patterns/collective_topology.h \
	collective_patterns/collective_topology.cc

libmsgrate_pattern_la_LDFLAGS = -module -avoid-version
libmsgrate_pattern_la_SOURCES = \
	end_points/msgrate_pattern.h \
	end_points/msgrate_pattern.cc \
	patterns.h \
	$(collectives) \
	$(support)

libghost_pattern_la_LDFLAGS = -module -avoid-version
libghost_pattern_la_SOURCES = \
	end_points/ghost_pattern.h \
	end_points/ghost_pattern.cc \
	patterns.h \
	benchmarks/src_ghost_bench/memory.h \
	benchmarks/src_ghost_bench/memory.c \
	benchmarks/src_ghost_bench/ranks.h \
	benchmarks/src_ghost_bench/ranks.c \
	benchmarks/src_ghost_bench/neighbors.h \
	benchmarks/src_ghost_bench/neighbors.c \
	benchmarks/src_ghost_bench/work.h \
	benchmarks/src_ghost_bench/work.c \
	$(collectives) \
	$(support)

libpingpong_pattern_la_LDFLAGS = -module -avoid-version
libpingpong_pattern_la_SOURCES = \
	end_points/pingpong_pattern.h \
	end_points/pingpong_pattern.cc \
	patterns.h \
	$(collectives) \
	$(support)

liballreduce_pattern_la_LDFLAGS = -module -avoid-version
liballreduce_pattern_la_SOURCES = \
	end_points/allreduce_pattern.h \
	end_points/allreduce_pattern.cc \
	patterns.h \
	util/stats.h util/stats.cc \
	$(collectives) \
	$(support)

liballtoall_pattern_la_LDFLAGS = -module -avoid-version
liballtoall_pattern_la_SOURCES = \
	end_points/alltoall_pattern.h \
	end_points/alltoall_pattern.cc \
	patterns.h \
	util/stats.h util/stats.cc \
	$(collectives) \
	$(support)

libfft_pattern_la_LDFLAGS = -module -avoid-version
libfft_pattern_la_SOURCES = \
	end_points/fft_pattern.h \
	end_points/fft_pattern.cc \
	patterns.h \
	util/stats.h util/stats.cc \
	$(collectives) \
	$(support)
