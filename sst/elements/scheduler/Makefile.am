# -*- Makefile -*-
#
#

AM_CPPFLAGS = \
	$(BOOST_CPPFLAGS) \
	$(MPI_CPPFLAGS) 

compdir = $(pkglibdir)
comp_LTLIBRARIES = libscheduler.la
libscheduler_la_SOURCES = \
Allocator.h \
AllocInfo.h		ArrivalEvent.h		CompletionEvent.h	exceptions.h \
AllocInfo.cc		ArrivalEvent.cc \
Job.h			JobStartEvent.h		Machine.h \
Job.cc \
misc.h			nodeComponent.h		PQScheduler.h		schedComponent.h \
misc.cc			nodeComponent.cc	PQScheduler.cc		schedComponent.cc \
Scheduler.h 		SimpleAllocator.h	SimpleMachine.h		Statistics.h \
schedLib.cc		SimpleAllocator.cc	SimpleMachine.cc	Statistics.cc

EXTRA_DIST = makeSDL.pl test.xml test30.xml

libscheduler_la_LDFLAGS = -module -avoid-version
