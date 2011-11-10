# -*- Makefile -*-
#
#

comp_LTLIBRARIES += libroutermodel.la libroutermodel_power.la
libroutermodel_la_SOURCES = \
		components/routermodel/routermodel.h \
		components/routermodel/routermodel.cc
libroutermodel_la_LDFLAGS = -module -avoid-version

libroutermodel_power_la_SOURCES = \
		components/routermodel/routermodel.h \
		components/routermodel/routermodel.cc \
		../power/power.cc \
		../power/power.h \
		../power/interface.h \
		../power/HotSpot-interface.cc \
		../power/HotSpot-interface.h 

libroutermodel_power_la_CXXFLAGS = -DWITH_POWER

libroutermodel_power_la_LDFLAGS = -module -avoid-version $(MCPAT_LDFLAGS) $(HOTSPOT_LDFLAGS) $(ORION_LDFLAGS)
libroutermodel_power_la_LIBADD = $(MCPAT_LIB) $(HOTSPOT_LIB) $(ORION_LIB)
