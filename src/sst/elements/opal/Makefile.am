# -*- Makefile -*-
#
#

AM_CPPFLAGS += \
	$(MPI_CPPFLAGS) \
	-I$(top_srcdir)/src

AM_LDFLAGS = -lm
compdir = $(pkglibdir)
comp_LTLIBRARIES = libOpal.la

libOpal_la_SOURCES = \
	mempool.h \
	mempool.cc \
	opal.cc \
	opal.h \
        opal_event.h \
	arielmemmgr_opal.cc \
	arielmemmgr_opal.h \
	opalMemNIC.cc \
	opalMemNIC.h \
	page_fault_handler.cc \
	page_fault_handler.h

libOpal_la_LDFLAGS = \
	-avoid-version

libOpal_la_LIBADD = \
	$(SST_SYSTEMC_LIB)

install-exec-hook:
	$(SST_REGISTER_TOOL) SST_ELEMENT_SOURCE     Opal=$(abs_srcdir)
	$(SST_REGISTER_TOOL) SST_ELEMENT_TESTS      Opal=$(abs_srcdir)/tests
