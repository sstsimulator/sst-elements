# -*- Makefile -*-
#
#

AM_CPPFLAGS = $(BOOST_CPPFLAGS) $(MPI_CPPFLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libfirefly.la

libfirefly_la_SOURCES = \
	ctrlMsg.cc \
	ctrlMsg.h \
	dataMovement.cc \
	dataMovement.h \
	entry.h \
	libfirefly.cc \
	functionSM.cc \
	functionSM.h \
	funcSM/fini.cc \
	funcSM/fini.h \
	funcSM/allreduce.cc \
	funcSM/allreduce.h \
	funcSM/collectiveOps.cc \
	funcSM/collectiveOps.h \
	funcSM/collectiveTree.cc \
	funcSM/collectiveTree.h \
	funcSM/barrier.cc \
	funcSM/barrier.h \
	funcSM/recv.cc \
	funcSM/recv.h \
	funcSM/send.cc \
	funcSM/send.h \
	funcSM/wait.cc \
	funcSM/wait.h \
	hades.h \
	hades.cc \
	hadesAPI.cc \
	simpleIO.h \
	simpleIO.cc \
	testDriver.h \
	testDriver.cc \
	ioSwitch.h \
	ioSwitch.cc \
	nic.h \
	nic.cc

libfirefly_la_LDFLAGS = -module -avoid-version
