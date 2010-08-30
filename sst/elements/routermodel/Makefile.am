# -*- Makefile -*-
#
#

AM_CPPFLAGS = $(BOOST_CPPFLAGS) $(MPI_CPPFLAGS) $(MCPAT_CPPFLAGS) $(HOTSPOT_CPPFLAGS)  $(ORION_CPPFLAGS) 

compdir = $(pkglibdir)
comp_LTLIBRARIES = libroutermodel.la
libroutermodel_la_SOURCES = \
		routermodel.h \
		routermodel.cc \
		../power/power.cc \
		../power/power.h \
		../power/interface.h \
		../power/HotSpot-interface.cc \
		../power/HotSpot-interface.h 
libroutermodel_la_LDFLAGS = -module -avoid-version $(MCPAT_LDFLAGS) $(HOTSPOT_LDFLAGS) $(ORION_LDFLAGS)   
libroutermodel_la_LIBADD = $(MCPAT_LIB) $(HOTSPOT_LIB) $(ORION_LIB)

#EXTRA_DIST = 2Dmesh4x4.xml 2Dmesh4x4.README

include src_genTopo/Makefile.inc
