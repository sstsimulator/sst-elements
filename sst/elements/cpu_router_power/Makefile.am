# -*- Makefile -*-
#
#

AM_CPPFLAGS = $(BOOST_CPPFLAGS) $(MPI_CPPFLAGS)  $(SIMPANALYZER_CPPFLAGS) $(MCPAT_CPPFLAGS) $(ORION_CPPFLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libcpu_router_power.la
libcpu_router_power_la_SOURCES = \
	cpu_router_power.h \
	cpu_router_power.cc \
	../power/power.cc \
	../power/power.h
libcpu_router_power_la_LDFLAGS = -module -avoid-version $(SIMPANALYZER_LDFLAGS) $(MCPAT_LDFLAGS) $(ORION_LDFLAGS)
libcpu_router_power_la_LIBADD = $(SIMPANALYZER_LIB) $(MCPAT_LIB) $(ORION_LIB)

