# -*- Makefile -*-
#
#

AM_CPPFLAGS = $(BOOST_CPPFLAGS) $(MPI_CPPFLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libfirefly.la

libfirefly_la_SOURCES = \
	longMsgProtocol.cc \
	longMsgProtocol.h \
	ctrlMsg.cc \
	ctrlMsg.h \
	entry.h \
	libfirefly.cc \
	functionSM.cc \
	functionSM.h \
	funcSM/fini.h \
	funcSM/alltoallv.h \
	funcSM/alltoallv.cc \
	funcSM/gatherv.h \
	funcSM/gatherv.cc \
	funcSM/allgather.cc \
	funcSM/allgather.h \
	funcSM/allreduce.h \
	funcSM/collectiveOps.h \
	funcSM/collectiveTree.cc \
	funcSM/collectiveTree.h \
	funcSM/barrier.h \
	funcSM/recv.cc \
	funcSM/recv.h \
	funcSM/send.cc \
	funcSM/send.h \
	funcSM/wait.cc \
	funcSM/wait.h \
	funcSM/waitAny.cc \
	funcSM/waitAny.h \
	funcSM/waitAll.cc \
	funcSM/waitAll.h \
	hades.h \
	hades.h \
	hades.cc \
	hadesAPI.cc \
	merlinIO.h \
	merlinIO.cc \
	simpleIO.h \
	simpleIO.cc \
	testDriver.h \
	testDriver.cc \
	ioSwitch.h \
	ioSwitch.cc \
	nicTester.h \
	nicTester.cc \
	nic.h \
	nic.cc \
	protocolAPI.h \
	ioapi.h \
	ioVec.h \
	info.h \
	group.h \
	ioEvent.h \
	funcSM/init.h \
	funcSM/api.h \
	funcSM/event.h \
	funcSM/rank.h \
	funcSM/size.h \
	merlinEvent.h \
	nodeInfo.h

libfirefly_la_LDFLAGS = -module -avoid-version
