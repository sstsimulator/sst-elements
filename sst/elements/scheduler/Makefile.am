# -*- Makefile -*-
#
#

AM_CPPFLAGS = \
	$(BOOST_CPPFLAGS) \
	$(MPI_CPPFLAGS) 

compdir = $(pkglibdir)
comp_LTLIBRARIES = libscheduler.la
libscheduler_la_SOURCES = \
SWFEvent.h              nodeComponent.h         schedComponent.h \
nodeComponent.cc        schedComponent.cc	schedLib.cc


EXTRA_DIST = makeSDL.pl test.xml test30.xml

libscheduler_la_LDFLAGS = -module -avoid-version
