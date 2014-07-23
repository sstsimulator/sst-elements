# -*- Makefile -*-
#
#

AM_CPPFLAGS = \
	$(BOOST_CPPFLAGS) \
	$(MPI_CPPFLAGS) 

compdir = $(pkglibdir)
comp_LTLIBRARIES = libmessageGeneratorComponent.la
libmessageGeneratorComponent_la_SOURCES = \
	messageGeneratorComponent.h \
	messageGeneratorComponent.cc \
	simpleMessage.h 

EXTRA_DIST = \
	test.xml \
	test.py

libmessageGeneratorComponent_la_LDFLAGS = -module -avoid-version
