# -*- Makefile -*-
#
#

AM_CPPFLAGS = $(MPI_CPPFLAGS) \
	-I$(top_srcdir)/src

compdir = $(pkglibdir)
comp_LTLIBRARIES = libfirefly.la

libfirefly_la_SOURCES = \
	protocolAPI.h \
	ioVec.h \
	info.h \
	group.h \
	ctrlMsg.cc \
	ctrlMsg.h \
	ctrlMsgXXX.cc \
	ctrlMsgXXX.h \
	ctrlMsgFunctors.h \
	ctrlMsgProcessQueuesState.h \
	heapAddrs.h \
	ioapi.h \
	nicTester.h \
	mem.h \
	latencyMod.h \
	rangeLatMod.h \
	scaleLatMod.h \
	loopBack.h \
	loopBack.cc \
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
	funcSM/commSplit.cc \
	funcSM/commSplit.h \
	funcSM/commCreate.cc \
	funcSM/commCreate.h \
	funcSM/commDestroy.h \
	funcSM/init.h \
	funcSM/makeProgress.h \
	hades.h \
	hades.h \
	hades.cc \
	hadesMP.cc \
	hadesMP.h \
	merlinEvent.h \
	virtNic.h \
	virtNic.cc \
	nic.h \
	nic.cc \
	nicVirtNic.h \
	nicArbitrateDMA.h \
	nicSendMachine.cc \
	nicSendMachine.h \
	nicRecvMachine.cc \
	nicRecvMachine.h \
	nodePerf.h

libfirefly_la_LDFLAGS = -module -avoid-version
