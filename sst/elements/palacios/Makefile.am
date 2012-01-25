# -*- Makefile -*-
#
#

AM_CPPFLAGS = \
	$(BOOST_CPPFLAGS) \
	$(MPI_CPPFLAGS) \
	-I/home/mjleven/sst_palacios/palacios/linux_module \
	-I/home/mjleven/sst_palacios/palacios/linux_usr \
	-I../../../../sst/elements

compdir = $(pkglibdir)
comp_LTLIBRARIES = libpalacios.la
libpalacios_la_SOURCES = \
	lib.cc \
	palacios.h \
	palacios.cc \
	ptlNicMMIF.h \
	ptlNicMMIF.cc

libpalacios_la_LDFLAGS = -module -avoid-version -lpthread -lv3hostdev -L/home/mjleven/sst_palacios/palacios/linux_usr
