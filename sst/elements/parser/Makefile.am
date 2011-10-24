# -*- Makefile -*-
#
#

AM_CPPFLAGS = \
	$(BOOST_CPPFLAGS) \
	$(MPI_CPPFLAGS) \
	$(MCPAT_CPPFLAGS) \
	$(HOTSPOT_CPPFLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libparser.la
libparser_la_SOURCES = \
	parser.h \
	parser.cc \
	../power/power.cc \
	../power/power.h \
	../power/interface.h \
	../power/HotSpot-interface.cc \
	../power/HotSpot-interface.h \
	../power/reliability.cc \
	../power/reliability.h 


libparser_la_LDFLAGS = -module -avoid-version $(MCPAT_LDFLAGS) $(HOTSPOT_LDFLAGS)
libparser_la_LIBADD = $(MCPAT_LIB) $(HOTSPOT_LIB)
