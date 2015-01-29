# -*- Makefile -*-
#
#


compdir = $(pkglibdir)
comp_LTLIBRARIES = libgem5.la


libgem5_la_SOURCES = \
					 libgem5.cc \
					 gem5.h \
					 gem5.cc \
					 Gem5Connector.h \
					 Gem5Connector.cc


AM_CPPFLAGS = $(BOOST_CPPFLAGS) $(MPI_CPPFLAGS) $(M5_CPPFLAGS)
AM_CXXFLAGS = $(SST_CXX0X_FLAGS)
libgem5_la_LDFLAGS = -module -avoid-version $(M5_LDFLAGS)
libgem5_la_LIBADD = $(M5_LIB)

