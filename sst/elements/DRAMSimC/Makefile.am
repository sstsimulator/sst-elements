# -*- Makefile -*-
#
#

EXTRA_DIST = dram.xml

compdir = $(pkglibdir)
comp_LTLIBRARIES = libDRAMSimC.la
libDRAMSimC_la_SOURCES = \
	DRAMSimC.cpp \
	DRAMSimC.h 
libDRAMSimC_la_CPPFLAGS = $(BOOST_CPPFLAGS) $(MPI_CPPFLAGS) $(DRAMSIM_CPPFLAGS) \
            -I$(top_srcdir)/sst/elements/include
libDRAMSimC_la_LDFLAGS = -module -avoid-version $(DRAMSIM_LDFLAGS)
libDRAMSimC_la_LIBADD = $(DRAMSIM_LIB)
