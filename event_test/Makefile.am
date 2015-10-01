# -*- Makefile -*-
#
#

AM_CPPFLAGS = $(BOOST_CPPFLAGS) $(MPI_CPPFLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libevent_test.la

libevent_test_la_SOURCES = \
	event_test.cc \
	event_test.h

libevent_test_la_LDFLAGS = -module -avoid-version
