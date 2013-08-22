# -*- Makefile -*-
#
#

AM_CPPFLAGS = \
	$(BOOST_CPPFLAGS) \
	$(MPI_CPPFLAGS) 

compdir = $(pkglibdir)
comp_LTLIBRARIES = liboberon.la
liboberon_la_SOURCES = \
	oberon.cc \
	oberonengine.cc \
	oberonengine.h \
	oberon.h \
	oberoninst.h \
	oberonmodel.cc \
	oberonmodel.h \
	obexprstack.cc \
	obexprstack.h \
	obexprval.h \
	obi64val.cc \
	obi64val.h \
	oberonev.h \
	oberonev.cc

liboberon_la_LDFLAGS = -module -avoid-version
