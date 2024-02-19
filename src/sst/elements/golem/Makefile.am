# -*- Makefile -*-
#
#

AM_CPPFLAGS += /usr/local/lib/python3.10/dist-packages/numpy/core/include \
	-I$(top_srcdir)/src \
	$(MPI_CPPFLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libgolem.la
libgolem_la_SOURCES = \
	computeArray.h \
	arrayEvent.h \
	golemUtil.h \
	mmioTile.h \
	mmioTile.cc \
	vroccanalog.h \
	crossSimComputeArray.h \
	testComputeArray.h



EXTRA_DIST = \
	tests/mmioArrayTest.py

libgolem_la_LDFLAGS = -module -avoid-version

install-exec-hook:
	$(SST_REGISTER_TOOL) SST_ELEMENT_SOURCE     golem=$(abs_srcdir)
	$(SST_REGISTER_TOOL) SST_ELEMENT_TESTS      golem=$(abs_srcdir)/tests

##########################################################################
##########################################################################
##########################################################################
