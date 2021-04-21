# -*- Makefile -*-
#
#

AM_CPPFLAGS = $(MPI_CPPFLAGS) \
	-I$(top_srcdir)/src

compdir = $(pkglibdir)
comp_LTLIBRARIES = libsstSwm.la

libsstSwm_la_SOURCES = \
	event.h \
	swm.cc \
	swm.h \
	workload.cc \
	workload.h \
	convert.cc \
	convert.h

libsstSwm_la_LDFLAGS = -module -avoid-version

libsstSwm_la_LDFLAGS += $(SWM_LDFLAGS)
libsstSwm_la_LIBADD = $(SWM_LIB)
AM_CPPFLAGS += $(SWM_CPPFLAGS) #-DHAVE_LIBDRAMSIM

install-exec-hook:
	$(SST_REGISTER_TOOL) SST_ELEMENT_SOURCE     sstSwm=$(abs_srcdir)
