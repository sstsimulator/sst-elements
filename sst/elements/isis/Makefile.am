
AM_CPPFLAGS = \
	$(BOOST_CPPFLAGS) \
	$(MPI_CPPFLAGS) 

compdir = $(pkglibdir)
comp_LTLIBRARIES = libisisComponent.la
libisisComponent_la_SOURCES = \
	isisComponent.h \
	isisComponent.cc

libisisComponent_la_LDFLAGS = -module -avoid-version
