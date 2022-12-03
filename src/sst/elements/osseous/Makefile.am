# -*- Makefile -*-
#
#
INCLUDE = -I$(top_srcdir)/src/sst/elements/osseous/eras_datatypes/ -I$(top_srcdir)/src/sst/elements/ariel/
FLAGS = -fpermissive -std=c++11

AM_CPPFLAGS += \
	$(MPI_CPPFLAGS) \
	$(INCLUDE) \
	$(FLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = librtlcomponent.la
librtlcomponent_la_SOURCES = \
	rtlcomponent.h \
	rtlcomponent.cc \
	rtlevent.h \
	rtlevent.cc \
	rtlmemmgr.h \
	rtlmemmgr_cache.h \
	rtlmemmgr_simple.h \
	rtlmemmgr_simple.cc \
	rtl_header.h \
	rtlreadev.h \
	AXI_port.h \
	rtlwriteev.h 

EXTRA_DIST = \
			 eras_datatypes/sint.h \
			 eras_datatypes/uint.h \
			 tests/Makefile \
			 tests/tb_header.h \
			 tests/tb_header.c \
			 tests/testbench.c \
			 tests/runstream.py


librtlcomponent_la_LDFLAGS = -module -avoid-version

install-exec-hook:
	$(SST_REGISTER_TOOL) rtlcomponent  rtlcomponent_LIBDIR=$(abs_srcdir)
	$(SST_REGISTER_TOOL) SST_ELEMENT_SOURCE  rtlcomponent=$(abs_srcdir)


##########################################################################
##########################################################################
##########################################################################
