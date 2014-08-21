# Makefile

compdir = $(pkglibdir)
comp_LTLIBRARIES = libsimpleTracer.la

libsimpleTracer_la_SOURCES = \
	libsimpleTracerGen.cc \
	simpleTracer.h \
	simpleTracer.cc

libsimpleTracer_la_CPPFLAGS = $(BOOST_CPPFLAGS) $(MPI_CPPFLAGS) -g
libsimpleTracer_la_LDFLAGS = -module -avoid-version

EXTRA_DIST = \
        tests/test-1.py \
        tests/test-2.py \
        README 

