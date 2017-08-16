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
	ctrlMsgFunctors.h \
	ctrlMsgProcessQueuesState.h \
	ctrlMsgProcessQueuesState.cc \
	ctrlMsgCommReq.h \
	ctrlMsgWaitReq.h \
	ctrlMsgMemory.h \
	ctrlMsgMemoryBase.h \
	ctrlMsgTiming.h \
	ctrlMsgTimingBase.h \
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
	hadesSHMEM.cc \
	hadesSHMEM.h \
	shmem/barrier.cc \
	shmem/barrier.h \
	merlinEvent.h \
	virtNic.h \
	virtNic.cc \
	nic.cc \
	nic.h \
	nicArbitrateDMA.h \
	nicEntryBase.cc \
	nicEntryBase.h \
	nicEvents.h \
	nicMsgStream.cc \
	nicMsgStream.h \
	nicRdmaStream.cc \
	nicRdmaStream.h \
	nicRecvEntry.h \
	nicRecvMachine.cc \
	nicRecvMachine.h \
	nicSendEntry.h \
	nicSendMachine.cc \
	nicSendMachine.h \
	nicShmem.cc \
	nicShmem.h \
	nicShmemMove.h \
	nicShmemMove.cc \
	nicShmemRecvMachine.h \
	nicShmemSendEntry.h \
	nicShmemStream.cc \
	nicShmemStream.h \
	nicVirtNic.h \
	nodePerf.h

libfirefly_la_LDFLAGS = -module -avoid-version
