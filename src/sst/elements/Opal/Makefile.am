# -*- Makefile -*-
#
#

compdir = $(pkglibdir)
comp_LTLIBRARIES = libOpal.la

libOpal_la_SOURCES = \
	mempool.h \
	mempool.cc \
	Opal.cc \
	Opal.h \
        Opal_Event.h \
	arielmemmgr_opal.cc \
	arielmemmgr_opal.h \
	opalMemNIC.cc \
	opalMemNIC.h \
	PageFaultHandler.cc \
	PageFaultHandler.h


libOpal_la_CPPFLAGS = \
	$(MPI_CPPFLAGS) \
	-I$(top_srcdir)/src \
	-fPIC -Wall

libOpal_la_LDFLAGS = \
	-lm \
	-avoid-version

libOpal_la_LIBADD = \
	$(SST_SYSTEMC_LIB)

install-exec-hook:
	$(SST_REGISTER_TOOL) SST_ELEMENT_SOURCE     Opal=$(abs_srcdir)
	$(SST_REGISTER_TOOL) SST_ELEMENT_TESTS      Opal=$(abs_srcdir)/tests
