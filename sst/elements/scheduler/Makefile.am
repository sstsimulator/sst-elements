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
    Job.h	Machine.h \
    Job.cc \
    linkBuilder.h		misc.h			nodeComponent.h			schedComponent.h \
    linkBuilder.cc		misc.cc			nodeComponent.cc	 	schedComponent.cc \
    Scheduler.h 	SimpleMachine.h		Statistics.h \
    schedLib.cc		SimpleMachine.cc	Statistics.cc \
    MachineMesh.cc		MachineMesh.h \
    MeshAllocInfo.cc \
    MeshAllocInfo.h \
    MBSAllocInfo.h \
    Factory.cc \
    Factory.h \
    faultInjectionComponent.cc \
    faultInjectionComponent.h \
    FST.cc \
    FST.h \
    InputParser.cc \
    InputParser.h \
    output.h \
    TaskCommInfo.cc \
    TaskCommInfo.h \
    TaskMapInfo.cc \
    TaskMapInfo.h \
    TaskMapper.h \
    allocators/BestFitAllocator.cc \
    allocators/BestFitAllocator.h \
    allocators/ConstraintAllocator.cc \
    allocators/ConstraintAllocator.h \
    allocators/EnergyAllocator.cc \
    allocators/EnergyAllocator.h \
    allocators/EnergyAllocClasses.cc \
    allocators/EnergyAllocClasses.h \
    allocators/FirstFitAllocator.cc \
    allocators/FirstFitAllocator.h \
    allocators/GranularMBSAllocator.cc \
    allocators/GranularMBSAllocator.h \
    allocators/LinearAllocator.cc \
    allocators/LinearAllocator.h \
    allocators/MBSAllocator.cc \
    allocators/MBSAllocator.h \
    allocators/MBSAllocClasses.h \
    allocators/NearestAllocator.cc \
    allocators/NearestAllocator.h \
    allocators/NearestAllocClasses.cc \
    allocators/NearestAllocClasses.h \
    allocators/OctetMBSAllocator.cc \
    allocators/OctetMBSAllocator.h \
    allocators/RandomAllocator.cc \
    allocators/RandomAllocator.h \
    allocators/RoundUpMBSAllocator.cc \
    allocators/RoundUpMBSAllocator.h \
    allocators/SimpleAllocator.cc \
    allocators/SimpleAllocator.h \
    allocators/SortedFreeListAllocator.cc \
    allocators/SortedFreeListAllocator.h \
    events/ArrivalEvent.cc \
    events/ArrivalEvent.h \
    events/CommunicationEvent.h \
    events/CompletionEvent.h \
    events/FaultEvent.h \
    events/FinalTimeEvent.h \
    events/JobKillEvent.h \
    events/JobStartEvent.h \
    events/ObjectRetrievalEvent.h \
    schedulers/EASYScheduler.cc \
    schedulers/EASYScheduler.h \
    schedulers/PQScheduler.cc \
    schedulers/PQScheduler.h \
    schedulers/StatefulScheduler.cc \
    schedulers/StatefulScheduler.h \
    taskMappers/SimpleTaskMapper.cc \
    taskMappers/SimpleTaskMapper.h

EXTRA_DIST = \
	simulations/makeSDL.pl \
	simulations/sdl-3.py

libscheduler_la_LDFLAGS = -module -avoid-version $(BOOST_LDFLAGS) $(BOOST_FILESYSTEM_LIB) $(BOOST_THREAD_LIB)
libscheduler_la_LIBADD =

if HAVE_GLPK
libscheduler_la_LDFLAGS += $(GLPK_LDFLAGS)
libscheduler_la_LIBADD += $(GLPK_LIB)
AM_CPPFLAGS += $(GLPK_CPPFLAGS)
endif
