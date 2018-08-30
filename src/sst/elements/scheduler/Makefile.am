#
#

AM_CPPFLAGS = \
    $(MPI_CPPFLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libscheduler.la
libscheduler_la_SOURCES = \
    Allocator.h \
    AllocInfo.cc \
    AllocInfo.h \
    AllocMapper.cc \
    AllocMapper.h \
    DragonflyMachine.cc \
    DragonflyMachine.h \
    exceptions.h \
    Factory.cc \
    Factory.h \
    faultInjectionComponent.cc \
    faultInjectionComponent.h \
    FST.cc \
    FST.h \
    InputParser.cc \
    InputParser.h \
    Job.cc \
    Job.h \
    linkBuilder.cc \
    linkBuilder.h \
    Machine.cc \
    Machine.h \
    Mesh3DMachine.cc \
    Mesh3DMachine.h \
    misc.h \
    nodeComponent.cc \
    nodeComponent.h \
    output.h \
    schedComponent.cc \
    schedComponent.h \
    schedLib.cc \
    Scheduler.h \
    SimpleMachine.cc \
    SimpleMachine.h \
    Snapshot.h \
    Snapshot.cc \
    Statistics.cc \
    Statistics.h \
    StencilMachine.cc \
    StencilMachine.h \
    TaskCommInfo.cc \
    TaskCommInfo.h \
    TaskMapInfo.cc \
    TaskMapInfo.h \
    TaskMapper.h \
    Torus3DMachine.cc \
    Torus3DMachine.h \
	util.h \
	util.cc \
    allocators/BestFitAllocator.cc \
    allocators/BestFitAllocator.h \
    allocators/ConstraintAllocator.cc \
    allocators/ConstraintAllocator.h \
    allocators/DragonflyAllocator.h \
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
    allocators/MBSAllocInfo.h \
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
    allocators/SimpleSpreadAllocator.cc \
    allocators/SimpleSpreadAllocator.h \
    allocators/SortedFreeListAllocator.cc \
    allocators/SortedFreeListAllocator.h \
    allocMappers/FibonacciHeap.cc \
    allocMappers/FibonacciHeap.h \
    allocMappers/NearestAllocMapper.cc \
    allocMappers/NearestAllocMapper.h \
    allocMappers/SpectralAllocMapper.cc \
    allocMappers/SpectralAllocMapper.h \
    events/ArrivalEvent.cc \
    events/ArrivalEvent.h \
    events/CommunicationEvent.h \
    events/CompletionEvent.h \
    events/FaultEvent.h \
    events/FinalTimeEvent.h \
    events/JobKillEvent.h \
    events/JobStartEvent.h \
    events/ObjectRetrievalEvent.h \
    events/SnapshotEvent.h \
    schedulers/EASYScheduler.cc \
    schedulers/EASYScheduler.h \
    schedulers/PQScheduler.cc \
    schedulers/PQScheduler.h \
    schedulers/StatefulScheduler.cc \
    schedulers/StatefulScheduler.h \
    taskMappers/RandomTaskMapper.cc \
    taskMappers/RandomTaskMapper.h \
    taskMappers/RCBTaskMapper.cc \
    taskMappers/RCBTaskMapper.h \
    taskMappers/SimpleTaskMapper.cc \
    taskMappers/SimpleTaskMapper.h \
    taskMappers/TopoMapper.cc \
    taskMappers/TopoMapper.h \
    taskMappers/Rcm.cc \
    taskMappers/Rcm.h

EXTRA_DIST = \
    DetailedNetworkSim_HOWTO \
    simulations/DMatrix4_5_2 \
    simulations/makeSDL.pl \
    simulations/test_scheduler_Atlas.sim \
    simulations/sphere3.mtx \
    simulations/sphere3_coord.mtx \
    simulations/CTH_108.phase \
    simulations/CTH_128.phase \
    simulations/CTH_16.phase \
    simulations/CTH_256.phase \
    simulations/CTH_288.phase \
    simulations/CTH_32.phase \
    simulations/CTH_64.phase \
    simulations/emberLoad.py \
    simulations/run_DetailedNetworkSim.py \
    simulations/snapshotParser_ember.py \
    simulations/snapshotParser_sched.py \
    simulations/test_DetailedNetwork.py \
    simulations/test_DetailedNetwork.sim

libscheduler_la_LDFLAGS = -module -avoid-version
libscheduler_la_LIBADD = 

if HAVE_GLPK
libscheduler_la_LDFLAGS += $(GLPK_LDFLAGS)
libscheduler_la_LIBADD += $(GLPK_LIB)
AM_CPPFLAGS += $(GLPK_CPPFLAGS)
endif

if HAVE_METIS
libscheduler_la_LDFLAGS += $(METIS_LDFLAGS)
libscheduler_la_LIBADD += $(METIS_LIB)
AM_CPPFLAGS += $(METIS_CPPFLAGS)
endif

install-exec-hook:
	$(SST_REGISTER_TOOL) GLPK LIBDIR=$(GLPK_LIBDIR)
	$(SST_REGISTER_TOOL) METIS LIBDIR=$(METIS_LIBDIR)

