# -*- Makefile -*-
#
#

AM_CPPFLAGS = \
	$(BOOST_CPPFLAGS) \
	$(MPI_CPPFLAGS) 

compdir = $(pkglibdir)
comp_LTLIBRARIES = liboberon.la
liboberon_la_SOURCES = \
  	oberonexprstack.h \
  	oberonexprstack.cc \
	obboolval.h \
	obboolval.cc \
	obintval.h \
	obintval.cc \
	obfpstkval.h \
	obfpstkval.cc \
	oberon.h \
	oberon.cc \
	oberonengine.h \
	oberonengine.cc \
	oberonstack.h \
	oberonstack.cc

liboberon_la_LDFLAGS = -module -avoid-version
