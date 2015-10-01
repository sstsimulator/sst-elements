# -*- Makefile -*-
#
#

AM_CPPFLAGS = $(BOOST_CPPFLAGS) $(MPI_CPPFLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libportals4_sm.la

libportals4_sm_la_SOURCES = \
	portals4_sm.cc \
	portals_args.h \
	part.h \
	part.cc \
	trig_cpu/application.h \
	trig_cpu/portals.h \
	trig_cpu/portals.cc \
	trig_cpu/portals_types.h \
	trig_cpu/trig_cpu.cc \
	trig_cpu/trig_cpu.h \
	trig_cpu/trig_cpu_event.h \
	trig_cpu/apps/allreduce_recdbl.h \
	trig_cpu/apps/allreduce_recdbl_trig.h \
	trig_cpu/apps/allreduce_tree.h \
	trig_cpu/apps/allreduce_tree_trig.h \
	trig_cpu/apps/bcast.h \
	trig_cpu/apps/bcast_trig.h \
	trig_cpu/apps/barrier_tree.h \
	trig_cpu/apps/barrier_tree_trig.h \
	trig_cpu/apps/barrier_recdbl.h \
	trig_cpu/apps/barrier_recdbl_trig.h \
	trig_cpu/apps/barrier_dissem.h \
	trig_cpu/apps/barrier_dissem_trig.h \
	trig_cpu/apps/pingpong-bw-eager.h \
	trig_cpu/apps/test_portals.h \
	trig_cpu/apps/test_mpi.h \
	trig_cpu/apps/test_atomics.h \
	trig_cpu/barrier_action.h \
	trig_cpu/apps/ping_pong.h \
	trig_cpu/apps/bandwidth.h \
	trig_cpu/apps/eq_test.h \
	trig_nic/trig_nic.cc \
	trig_nic/trig_nic.h \
	trig_nic/trig_nic_event.h

libportals4_sm_la_LDFLAGS = -module -avoid-version

bin_PROGRAMS = portals4_sm_sdlgen
portals4_sm_sdlgen_SOURCES = portals4_sm_sdlgen.c portals_args.h
