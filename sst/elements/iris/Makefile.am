# -*- Makefile -*-
#
#

AM_CPPFLAGS = \
	$(BOOST_CPPFLAGS) \
	$(MPI_CPPFLAGS) 

compdir = $(pkglibdir)
comp_LTLIBRARIES = libiris.la
libiris_la_SOURCES = \
	interfaces/genericHeader.h \
	interfaces/irisEvent.h \
	\
	data_types/flit.h \
	data_types/flit.cc \
	\
	components/router_params.h \
	components/genericBuffer.h \
	components/genericBuffer.cc \
	components/genericRC.cc \
	components/genericRC.h	\
	components/genericSwa.h	\
	components/genericSwa.cc \
	components/genericVca.h	\
	components/genericVca.cc \
	components/router.cc	\
	components/router.h


EXTRA_DIST = makeSDL.pl

libiris_la_LDFLAGS = -module -avoid-version

