
AM_CPPFLAGS = \
        $(BOOST_CPPFLAGS) \
        $(MPI_CPPFLAGS) \
	-std=c++0x

compdir = $(pkglibdir)
comp_LTLIBRARIES = libprospero.la
libprospero_la_SOURCES = \
        prospero.h \
        prospero.cc

libprospero_la_LDFLAGS = -module -avoid-version
