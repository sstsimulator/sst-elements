AM_CPPFLAGS = \
	$(BOOST_CPPFLAGS) \
	$(MPI_CPPFLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libcpu_power.la
libcpu_power_la_SOURCES = \
	cpu_power.h \
	cpu_power.cc \
	simpleEvent.h \
	power_power.cc \
	power_HotSpot-interface.cc \
	power_IntSim-interface.cc

libcpu_power_la_CPPFLAGS = $(BOOST_CPPFLAGS) $(MCPAT_CPPFLAGS) $(HOTSPOT_CPPFLAGS) $(INTSIM_CPPFLAGS) $(ORION_CPPFLAGS) $(MPI_CPPFLAGS) -I$(top_srcdir)/sst/elements/power
libcpu_power_la_LDFLAGS = -module -avoid-version $(MCPAT_LDFLAGS) $(HOTSPOT_LDFLAGS) $(INTSIM_LDFLAGS) $(ORION_LDFLAGS)   
libcpu_power_la_LIBADD = $(MCPAT_LIB) $(HOTSPOT_LIB) $(INTSIM_LIB) $(ORION_LIB)

# Work around bad habit of including source files from another component.
power_power.cc: $(top_srcdir)/sst/elements/power/power.cc
	@cp $(top_srcdir)/sst/elements/power/power.cc $(srcdir)/power_power.cc
power_HotSpot-interface.cc: $(top_srcdir)/sst/elements/power/HotSpot-interface.cc
	@cp $(top_srcdir)/sst/elements/power/HotSpot-interface.cc $(srcdir)/power_HotSpot-interface.cc
power_IntSim-interface.cc: $(top_srcdir)/sst/elements/power/IntSim-interface.cc
	@cp $(top_srcdir)/sst/elements/power/IntSim-interface.cc $(srcdir)/power_IntSim-interface.cc
