# -*- Makefile -*-
#
#

AM_CPPFLAGS = $(BOOST_CPPFLAGS) $(MPI_CPPFLAGS) $(MCPAT_CPPFLAGS) $(HOTSPOT_CPPFLAGS)  $(ORION_CPPFLAGS) 

compdir = $(pkglibdir)
comp_LTLIBRARIES = libroutermodel.la libroutermodel_power.la
libroutermodel_la_SOURCES = \
		routermodel.h \
		routermodel.cc
libroutermodel_power_la_SOURCES = \
		routermodel.h \
		routermodel.cc \
		../power/power.cc \
		../power/power.h \
		../power/interface.h \
		../power/HotSpot-interface.cc \
		../power/HotSpot-interface.h 
libroutermodel_la_LDFLAGS = -module -avoid-version
libroutermodel_power_la_LDFLAGS = -module -avoid-version $(MCPAT_LDFLAGS) $(HOTSPOT_LDFLAGS) $(ORION_LDFLAGS)   
libroutermodel_power_la_LIBADD = $(MCPAT_LIB) $(HOTSPOT_LIB) $(ORION_LIB)

include src_genTopo/Makefile.inc
