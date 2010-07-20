# -*- Makefile -*-
#
#

AM_CPPFLAGS = $(BOOST_CPPFLAGS)

CXX = $(MPICXX)
compdir = $(pkglibdir)
comp_LTLIBRARIES = libevent_test.la

libevent_test_la_SOURCES = \
	event_test.cc \
	event_test.h \
	myEvent.h

libevent_test_la_LDFLAGS = -module -avoid-version
