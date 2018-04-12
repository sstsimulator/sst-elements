
AM_CPPFLAGS = \
	-I$(top_srcdir)/src \
	$(MPI_CPPFLAGS)


compdir = $(pkglibdir)
comp_LTLIBRARIES = libkingsley.la

libkingsley_la_SOURCES = \
	nocEvents.h \
	noc_mesh.h \
	noc_mesh.cc \
	lru_unit.h \
	linkControl.h \
	linkControl.cc 

EXTRA_DIST = 

libkingsley_la_LDFLAGS = -module -avoid-version
