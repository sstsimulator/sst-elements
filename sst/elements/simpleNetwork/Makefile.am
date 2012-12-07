# -*- Makefile -*-
#
#

AM_CPPFLAGS = \
	$(BOOST_CPPFLAGS) \
	$(MPI_CPPFLAGS) 

compdir = $(pkglibdir)
comp_LTLIBRARIES = libmessageGeneratorComponent.la
libsimpleComponent_la_SOURCES = \
	messageGeneratorComponent.h \
	messageGeneratorComponent.cc \
	simpleMessage.h 

libmessageGeneratorComponent_la_LDFLAGS = -module -avoid-version
