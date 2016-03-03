# -*- Makefile -*-
#
#

########################################################
## THIS SECTION ADDED TO BUILD PATTERNS INTO A SINGLE patterns.so FILE 
## WITH MULTIPLE COMPONENTS - ALEVINE
########################################################

bin_PROGRAMS =

AM_CPPFLAGS = $(BOOST_CPPFLAGS) $(MPI_CPPFLAGS)

compdir = $(pkglibdir)

comp_LTLIBRARIES = libpatterns.la

libpatterns_la_LDFLAGS = -module -avoid-version

libpatterns_la_SOURCES = \
	end_points/msgrate_pattern.h \
	end_points/msgrate_pattern.cc \
	end_points/ghost_pattern.h \
	end_points/ghost_pattern.cc \
	end_points/pingpong_pattern.h \
	end_points/pingpong_pattern.cc \
	end_points/allreduce_pattern.h \
	end_points/allreduce_pattern.cc \
	end_points/alltoall_pattern.h \
	end_points/alltoall_pattern.cc \
	end_points/fft_pattern.h \
	end_points/fft_pattern.cc \
	components/bit_bucket/bit_bucket.h \
	components/bit_bucket/bit_bucket.cc \
	components/routermodel/routermodel.h \
	components/routermodel/routermodel.cc \
	util/stats.h  \
	util/stats.cc \
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
	collective_patterns/collective_topology.cc \
	support/NIC_model.cc \
	support/NIC_model.h \
	support/comm_pattern.cc \
	support/comm_pattern.h \
	support/machine_info.cc \
	support/machine_info.h \
	support/NIC_stats.cc \
	support/NIC_stats.h \
	support/pattern_common.cc \
	support/pattern_common.h \
	support/routing.cc \
	support/routing.h \
	support/state_machine.cc \
	support/state_machine.h \
	cpunicEvent.cc \
	cpunicEvent.h \
	patterns.cc \
	patterns.h

EXTRA_DIST = \
	config_files/M_Shamrock.in \
	config_files/M_XE.in \
	config_files/ping.in \
	config_files/alltoall.in


if SST_USE_MPI
libpatterns_la_SOURCES += \
	benchmarks/src_ghost_bench/ghost.h \
	benchmarks/src_ghost_bench/memory.h \
	benchmarks/src_ghost_bench/memory.c \
	benchmarks/src_ghost_bench/ranks.h \
	benchmarks/src_ghost_bench/ranks.c \
	benchmarks/src_ghost_bench/neighbors.h \
	benchmarks/src_ghost_bench/neighbors.c \
	benchmarks/src_ghost_bench/work.h \
	benchmarks/src_ghost_bench/work.c
endif

### BUILD THE genPatterns application
include src_genPatterns/Makefile.inc

########################################################
## THIS SECTION REMOVED TO BUILD PATTERNS INTO A SINGLE patterns.so FILE 
## WITH MULTIPLE COMPONENTS - ALEVINE
########################################################
### AM_CPPFLAGS = $(BOOST_CPPFLAGS) $(MPI_CPPFLAGS)
###
### compdir = $(pkglibdir)
### comp_LTLIBRARIES = 
### bin_PROGRAMS =
### 
### include end_points/Makefile.inc
### include components/Makefile.inc
### include src_genPatterns/Makefile.inc
### include benchmarks/Makefile.inc
