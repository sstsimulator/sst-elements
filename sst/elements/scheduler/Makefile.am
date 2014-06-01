#
#

AM_CPPFLAGS = \
	$(BOOST_CPPFLAGS) \
	$(MPI_CPPFLAGS)
	$(BOOST_FILESYSTEM_LIB) \
	$(BOOST_THREAD_LIB) 


compdir = $(pkglibdir)
comp_LTLIBRARIES = libscheduler.la
libscheduler_la_SOURCES = \
Allocator.h \
AllocInfo.h		ArrivalEvent.h		CompletionEvent.h	exceptions.h \
AllocInfo.cc		ArrivalEvent.cc \
Job.h			JobStartEvent.h		Machine.h \
Job.cc \
linkBuilder.h		misc.h			nodeComponent.h		EASYScheduler.h 	PQScheduler.h		StatefulScheduler.h 	schedComponent.h \
linkBuilder.cc		misc.cc			nodeComponent.cc	EASYScheduler.cc 	PQScheduler.cc		StatefulScheduler.cc 	schedComponent.cc \
Scheduler.h 		SimpleAllocator.h	SimpleMachine.h		Statistics.h 	NearestAllocator.h 	NearestAllocClasses.h 	NearestAllocator.cc \
schedLib.cc		SimpleAllocator.cc	SimpleMachine.cc	Statistics.cc		RandomAllocator.cc 	RandomAllocator.h ConstraintAllocator.h\
MachineMesh.cc		MachineMesh.h		NearestAllocClasses.cc		MBSAllocator.cc		MBSAllocator.h		MBSAllocInfo.h \
GranularMBSAllocator.cc		GranularMBSAllocator.h		OctetMBSAllocator.h		OctetMBSAllocator.cc		RoundUpMBSAllocator.h \
RoundUpMBSAllocator.cc		Factory.h		Factory.cc \
LinearAllocator.cc		LinearAllocator.h		FirstFitAllocator.cc		FirstFitAllocator.h		BestFitAllocator.cc \
BestFitAllocator.h		SortedFreeListAllocator.h		SortedFreeListAllocator.cc ConstraintAllocator.cc 	MeshAllocInfo.cc \
	MeshAllocInfo.h		FST.cc		FST.h \
	output.h \
	FaultEvent.h \
	JobKillEvent.h \
	CommunicationEvent.h \
	ObjectRetrievalEvent.h \
	MBSAllocClasses.h \
	FinalTimeEvent.h \
faultInjectionComponent.cc	faultInjectionComponent.h \
EnergyAllocator.cc		EnergyAllocator.h \
EnergyAllocClasses.cc		EnergyAllocClasses.h 

EXTRA_DIST = \
	simulations/makeSDL.pl

libscheduler_la_LDFLAGS = -module -avoid-version $(BOOST_LDFLAGS) $(BOOST_FILESYSTEM_LIB) $(BOOST_THREAD_LIB)
if HAVE_GLPK
libscheduler_la_LDFLAGS += $(GLPK_LDFLAGS) $(GLPK_LIBDIR)  
libscheduler_la_LIBADD = $(GLPK_LIB)
AM_CPPFLAGS += $(GLPK_CPPFLAGS) $(GLPK_LIB)
endif
