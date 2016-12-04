# -*- Makefile -*-
#
#

AM_CPPFLAGS = \
	$(BOOST_CPPFLAGS) \
	$(PYTHON_CPPFLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libpyproto.la
libpyproto_la_SOURCES = \
						eli.cc \
						pyproto.h \
						pyproto.cc \
						pymodule.h \
						pymodule.cc \
						pyarchive.h \
						pyarchive.cc


libpyproto_la_LDFLAGS = -module -avoid-version \
						  $(PYTHON_LDFLAGS)
#						  $(BOOST_LDFLAGS)
#libpyproto_la_LIBADD = -lboost_serialization$(BOOST_LIB_SUFFIX)

