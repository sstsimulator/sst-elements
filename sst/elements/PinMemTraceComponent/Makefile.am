
AM_CPPFLAGS = \
        $(BOOST_CPPFLAGS) \
        $(MPI_CPPFLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libPinMemTraceComponent.la
libPinMemTraceComponent_la_SOURCES = \
        pinmemtrace.h \
        pinmemtrace.cc

libPinMemTraceComponent_la_LDFLAGS = -module -avoid-version
