
AM_CPPFLAGS = \
        $(MPI_CPPFLAGS) \
	-I$(top_srcdir)/src

compdir = $(pkglibdir)
comp_LTLIBRARIES = libsavannah.la

libsavannah_la_SOURCES = \
	libsavannah.cpp \
	savarb.h \
	savcomp.cc \
	savcomp.h \
	savevent.h \
	arbitrator/savfifoarb.h

EXTRA_DIST =

libsavannah_la_LDFLAGS = -module -avoid-version
