# -*- Makefile -*-
#
#

AM_CPPFLAGS = $(BOOST_CPPFLAGS) $(MPI_CPPFLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libiris.la

libiris_la_SOURCES = \
	iris.cc \
	RtrIF.h \
	genericBuffer.cc \
	genericBuffer.h \
	genericHeader.cc \
	genericHeader.h \
	SST_interface.h \
	genericRC.cc \
	genericRC.h \
	genericSwa.cc \
	genericSwa.h \
	genericVca.cc \
	genericVca.h \
	stat.h \
	router.cc \
	router.h \
	router_params.h \
	irisTerminal.h \
	ninterface.h \
	ninterface.cc \
	macsim_terminal.h \
	macsim_terminal.cc \
	trig_nic/trig_nic.cc \
	trig_nic/trig_nic.h \
	ex_macsim_component.h

EXTRA_DIST = \
	test_v2.py

sstdir = $(includedir)/sst/elements/iris
nobase_sst_HEADERS = \
	RtrIF.h 

libiris_la_LDFLAGS = -module -avoid-version

noinst_PROGRAMS = iris_sdlgen
iris_sdlgen_SOURCES = iris_sdlgen.c
