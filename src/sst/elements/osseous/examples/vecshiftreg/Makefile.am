# -*- Makefile -*-
#
#
INCLUDE = -I$(top_srcdir)/src/sst/elements/osseous/eras_datatypes/ -I$(top_srcdir)/src/sst/elements/ariel/
FLAGS = -fpermissive -std=c++11

AM_CPPFLAGS = \
	$(MPI_CPPFLAGS) \
	$(INCLUDE) \
	$(FLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libvecshiftreg.la
libvecshiftreg_la_SOURCES = \
	vecshiftreg.h \
	vecshiftreg.cc \
	rtlevent.h \
	rtlevent.cc \
	rtlmemmgr.h \
	rtlmemmgr_cache.h \
	rtlmemmgr_simple.h \
	rtlmemmgr_simple.cc \
	rtlreadev.h \
	rtlwriteev.h 

libvecshiftreg_la_LDFLAGS = -module -avoid-version

install-exec-hook:
	$(SST_REGISTER_TOOL) SST_ELEMENT_SOURCE  vecshiftreg=$(abs_srcdir)

##########################################################################
##########################################################################
##########################################################################
