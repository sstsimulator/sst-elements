#
#

AM_CPPFLAGS = \
	$(BOOST_CPPFLAGS) \
	$(MPI_CPPFLAGS)


compdir = $(pkglibdir)
comp_LTLIBRARIES = libscheduler.la
libscheduler_la_SOURCES = \
    Allocator.h \
    AllocInfo.h		exceptions.h \
    AllocInfo.cc \
    Job.h			JobStartEvent.h		Machine.h \
    Job.cc \
    linkBuilder.h		misc.h			nodeComponent.h			schedComponent.h \
    linkBuilder.cc		misc.cc			nodeComponent.cc	 	schedComponent.cc \
    Scheduler.h 	SimpleMachine.h		Statistics.h \
    schedLib.cc		SimpleMachine.cc	Statistics.cc \
    MachineMesh.cc		MachineMesh.h \
    MeshAllocInfo.cc \
    MBSAllocInfo.h \
    Factory.h		Factory.cc \
    FST.cc		FST.h \
    output.h \
    faultInjectionComponent.cc	faultInjectionComponent.h \
    BestFitAllocator.cc \
    BestFitAllocator.h \
    ConstraintAllocator.cc \
    ConstraintAllocator.h \
    EnergyAllocator.cc \
    EnergyAllocator.h \
    EnergyAllocClasses.cc \
    EnergyAllocClasses.h \
    FirstFitAllocator.cc \
    FirstFitAllocator.h \
    GranularMBSAllocator.cc \
    GranularMBSAllocator.h \
    LinearAllocator.cc \
    LinearAllocator.h \
    MBSAllocator.cc \
    MBSAllocator.h \
    MBSAllocClasses.h \
    NearestAllocator.cc \
    NearestAllocator.h \
    NearestAllocClasses.cc \
    NearestAllocClasses.h \
    OctetMBSAllocator.cc \
    OctetMBSAllocator.h \
    RandomAllocator.cc \
    RandomAllocator.h \
    RoundUpMBSAllocator.cc \
    RoundUpMBSAllocator.h \
    SimpleAllocator.cc \
    SimpleAllocator.h \
    SortedFreeListAllocator.cc \
    SortedFreeListAllocator.h \
    ArrivalEvent.cc \
    ArrivalEvent.h \
    CommunicationEvent.h \
    CompletionEvent.h \
    FaultEvent.h \
    FinalTimeEvent.h \
    JobKillEvent.h \
    ObjectRetrievalEvent.h \
    EASYScheduler.cc \
    EASYScheduler.h \
    PQScheduler.cc \
    PQScheduler.h \
    StatefulScheduler.cc \
    StatefulScheduler.h

EXTRA_DIST = \
	simulations/makeSDL.pl

libscheduler_la_LDFLAGS = -module -avoid-version $(BOOST_LDFLAGS) $(BOOST_FILESYSTEM_LIB) $(BOOST_THREAD_LIB)
libscheduler_la_LIBADD =

if HAVE_GLPK
libscheduler_la_LDFLAGS += $(GLPK_LDFLAGS)
libscheduler_la_LIBADD += $(GLPK_LIB)
AM_CPPFLAGS += $(GLPK_CPPFLAGS)
endif
