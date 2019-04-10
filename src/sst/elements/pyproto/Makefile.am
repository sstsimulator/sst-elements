# -*- Makefile -*-
#
#

AM_CPPFLAGS = \
	$(PYTHON_CPPFLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libpyproto.la
libpyproto_la_SOURCES = \
						pyproto.h \
						pyproto.cc \
						pymodule.h \
						pymodule.cc \
						pyarchive.h \
						pyarchive.cc


libpyproto_la_LDFLAGS = -module -avoid-version \
						  $(PYTHON_LDFLAGS)

