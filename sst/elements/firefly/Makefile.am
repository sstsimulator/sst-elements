# -*- Makefile -*-
#
#

AM_CPPFLAGS = $(BOOST_CPPFLAGS) $(MPI_CPPFLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libfirefly.la

libfirefly_la_SOURCES = \
	longMsgProtocol.cc \
	longMsgProtocol.h \
	protocolAPI.h \
	ioVec.h \
	info.h \
	group.h \
	nodeInfo.h \
	ctrlMsg.cc \
	ctrlMsg.h \
	ctrlMsgXXX.cc \
	ctrlMsgXXX.h \
	ctrlMsgState.h \
	ctrlMsgSendState.h \
	ctrlMsgRecvState.h \
	ctrlMsgWaitAnyState.h \
	ctrlMsgFunctors.h \
	ctrlMsgReadState.h \
	ctrlMsgRegRegionState.h \
	ctrlMsgUnregRegionState.h \
	ctrlMsgProcessQueuesState.h \
	entry.h \
	libfirefly.cc \
	functionSM.cc \
	functionSM.h \
	funcSM/api.h \
	funcSM/fini.h \
	funcSM/event.h \
	funcSM/rank.h \
	funcSM/size.h \
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
	funcSM/init.h \
	hades.h \
	hades.h \
	hades.cc \
	hadesAPI.cc \
	testDriver.h \
	testDriver.cc \
	merlinEvent.h \
	nicTester.h \
	nicTester.cc \
	nic.h \
	nic.cc

libfirefly_la_LDFLAGS = -module -avoid-version
