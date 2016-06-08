# -*- Makefile -*-
#
#

AM_CPPFLAGS = \
	$(BOOST_CPPFLAGS) \
	$(MPI_CPPFLAGS) \
	$(QSIM_CPPFLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libqsimComponent.la
libqsimComponent_la_SOURCES = \
	qsimComponent.h \
	qsimComponent.cc

EXTRA_DIST = gen_sdl.sh README test/Makefile test/runme.sh test/state.4.bz2 test/test-app.c test/test-app patches/qsim-0.2.1-fileOpenCheck.patch

libqsimComponent_la_LDFLAGS = -module -avoid-version $(QSIM_LDFLAGS)
libqsimComponent_la_LIBADD = -lqsim
