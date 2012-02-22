# -*- Makefile -*-
#
#

AM_CPPFLAGS = \
	$(BOOST_CPPFLAGS) \
	$(MPI_CPPFLAGS) \
	$(shell sstmacro-config --cppflags)


compdir = $(pkglibdir)
comp_LTLIBRARIES = libmacro_component.la
libmacro_component_la_CPPFLAGS = $(BOOST_CPPFLAGS) \
	$(MPI_CPPFLAGS) \
	$(shell sstmacro-config --cppflags)
libmacro_component_la_SOURCES = \
macro_component.cc \
	macro_processor.h \
	macro_processor.cc \
	macro_address.h \
	macro_address.cc \
	sstMessageEvent.h \
	macro_network.h \
	macro_network.cc \
	macro_parameters.h \
	macro_fakenic.h \
	macro_fakenic.cc \
	macro_fakeeventmanager.h

EXTRA_DIST = makeSDL.pl


	
libmacro_component_la_LDFLAGS = -module -avoid-version $(shell sstmacro-config --ldflags )

