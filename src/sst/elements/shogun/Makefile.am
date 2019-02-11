# -*- Makefile -*-
#
#

AM_CPPFLAGS = \
	$(MPI_CPPFLAGS) 

compdir = $(pkglibdir)
comp_LTLIBRARIES = libshogun.la
libshogun_la_SOURCES = \
	shogun.cc \
	shogun.h \
	shogun_event.h \
	shogun_init_event.h \
	shogun_nic.cc \
	shogun_nic.h \
	arb/shogunrrarb.cc \
	arb/shogunrrarb.h \
	arb/shogunarb.h

EXTRA_DIST =

libshogun_la_LDFLAGS = -module -avoid-version


