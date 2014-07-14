
AM_CPPFLAGS = \
        $(BOOST_CPPFLAGS) \
        $(MPI_CPPFLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libprospero.la
libprospero_la_SOURCES = \
        prospero.h \
        prospero.cc

EXTRA_DIST = \
	xml/trace.xml \
	xml/trace-withdramsim.xml

libprospero_la_LDFLAGS = -module -avoid-version

if USE_LIBZ
libprospero_la_LIBADD = -lz
endif
