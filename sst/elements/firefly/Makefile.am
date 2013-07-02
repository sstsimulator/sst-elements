# -*- Makefile -*-
#
#

AM_CPPFLAGS = $(BOOST_CPPFLAGS) $(MPI_CPPFLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libfirefly.la

libfirefly_la_SOURCES = \
	entry.cc \
	entry.h \
	libfirefly.cc \
	functionCtx.cc \
	functionCtx.h \
	funcCtx/recv.cc \
	funcCtx/recv.h \
	funcCtx/send.cc \
	funcCtx/send.h \
	funcCtx/wait.cc \
	funcCtx/wait.h \
	funcCtx/rank.cc \
	funcCtx/rank.h \
	funcCtx/size.cc \
	funcCtx/size.h \
	hades.h \
	hades.cc \
	hadesAPI.cc \
	simpleIO.h \
	simpleIO.cc \
	testDriver.h \
	testDriver.cc \
	ioSwitch.h \
	ioSwitch.cc \
	nic.h \
	nic.cc

libfirefly_la_LDFLAGS = -module -avoid-version
