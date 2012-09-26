

if USE_MCPAT
if USE_ORION
if USE_INTSIM
if USE_HOTSPOT

AM_CPPFLAGS = \
	$(BOOST_CPPFLAGS) \
	$(MPI_CPPFLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libcpu_power.la
libcpu_power_la_SOURCES = \
	cpu_power.h \
	cpu_power.cc \
	simpleEvent.h \
	../power/power.cc \
	../power/power.h \
	../power/interface.h \
	../power/HotSpot-interface.cc \
	../power/HotSpot-interface.h \
	../power/IntSim-interface.cc \
	../power/IntSim-interface.h 

libcpu_power_la_CPPFLAGS = $(BOOST_CPPFLAGS) $(MCPAT_CPPFLAGS) $(HOTSPOT_CPPFLAGS) $(INTSIM_CPPFLAGS) $(ORION_CPPFLAGS) 
libcpu_power_la_LDFLAGS = -module -avoid-version $(MCPAT_LDFLAGS) $(HOTSPOT_LDFLAGS) $(INTSIM_LDFLAGS) $(ORION_LDFLAGS)   
libcpu_power_la_LIBADD = $(MCPAT_LIB) $(HOTSPOT_LIB) $(INTSIM_LIB) $(ORION_LIB)

endif
endif
endif
endif


