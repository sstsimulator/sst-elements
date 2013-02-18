# -*- Makefile -*-
#
#

AM_CPPFLAGS = \
	$(BOOST_CPPFLAGS) \
	$(MPI_CPPFLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libmacro_component.la
libmacro_component_la_CPPFLAGS = $(BOOST_CPPFLAGS) \
	$(shell $(SSTMACRO_CONFIG_TOOL) --cppflags) -O0 -g \
	$(SSTMACRO_CPPFLAGS) \
	$(MPI_CPPFLAGS)

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
	macro_fakeeventmanager.h

EXTRA_DIST = makeSDL.pl

libmacro_component_la_LDFLAGS = -module -avoid-version $(shell $(SSTMACRO_CONFIG_TOOL) --ldflags) $(SSTMACRO_LDFLAGS)
