# -*- Makefile -*-
#
#

#AM_CPPFLAGS = $(BOOST_CPPFLAGS) $(MPI_CPPFLAGS)

#compdir = $(pkglibdir)
#comp_LTLIBRARIES = libmesmthi.la
#libmesmthi_la_SOURCES = mesmthi.cc mesmthi_model.cc mesmthi_model.h mesmthi.h \
#                        aux/des.h aux/cont_proc.h
#libmesmthi_la_LDFLAGS = -module -avoid-version -ldl -lqsim




AM_CPPFLAGS = $(BOOST_CPPFLAGS) $(MPI_CPPFLAGS) $(MCPAT_CPPFLAGS) $(HOTSPOT_CPPFLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libmesmthi.la libmesmthi_power.la
libmesmthi_la_SOURCES = mesmthi.cc mesmthi_model.cc mesmthi_model.h mesmthi.h \
			aux/des.h aux/cont_proc.h
libmesmthi_power_la_SOURCES = \
		mesmthi.h \
		mesmthi.cc \
		mesmthi_model.cc \
		mesmthi_model.h \
		aux/des.h \
		aux/cont_proc.h \
		../power/power.cc \
		../power/power.h \
		../power/interface.h \
		../power/HotSpot-interface.cc \
		../power/HotSpot-interface.h \
		../power/reliability.cc \
		../power/reliability.h 
libmesmthi_la_LDFLAGS = -module -avoid-version -ldl -lqsim
libmesmthi_power_la_LDFLAGS = -module -avoid-version -ldl -lqsim $(MCPAT_LDFLAGS) $(HOTSPOT_LDFLAGS) $(ORION_LDFLAGS)  
libmesmthi_power_la_LIBADD = $(MCPAT_LIB) $(HOTSPOT_LIB) $(ORION_LIB)
