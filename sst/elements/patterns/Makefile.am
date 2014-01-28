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
	benchmarks/src_ghost_bench/ghost.h \
	benchmarks/src_ghost_bench/memory.h \
	benchmarks/src_ghost_bench/memory.c \
	benchmarks/src_ghost_bench/ranks.h \
	benchmarks/src_ghost_bench/ranks.c \
	benchmarks/src_ghost_bench/neighbors.h \
	benchmarks/src_ghost_bench/neighbors.c \
	benchmarks/src_ghost_bench/work.h \
	benchmarks/src_ghost_bench/work.c \
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
	patterns.cc \
	patterns.h

### ADD EXTRA FILES TO THE BUILD IF USING THE POWER COMPONENTS
if USE_MCPAT
if USE_ORION
if USE_HOTSPOT
libpatterns_la_SOURCES += \
	power_power.cc \
	power_HotSpot-interface.cc

libpatterns_la_CXXFLAGS = -DWITH_POWER -I$(top_srcdir)/sst/elements/power

libpatterns_la_LDFLAGS += $(MCPAT_LDFLAGS) $(HOTSPOT_LDFLAGS) $(ORION_LDFLAGS)
libpatterns_la_LIBADD = $(MCPAT_LIB) $(HOTSPOT_LIB) $(ORION_LIB)
endif
endif
endif

# Work around bad habit of including source files from another component.
power_power.cc: $(top_srcdir)/sst/elements/power/power.cc
	@cp $(top_srcdir)/sst/elements/power/power.cc $(srcdir)/power_power.cc
power_HotSpot-interface.cc: $(top_srcdir)/sst/elements/power/HotSpot-interface.cc
	@cp $(top_srcdir)/sst/elements/power/HotSpot-interface.cc $(srcdir)/power_HotSpot-interface.cc

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
