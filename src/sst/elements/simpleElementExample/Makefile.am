# -*- Makefile -*-
#
#

AM_CPPFLAGS = \
	$(MPI_CPPFLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libsimpleElementExample.la
libsimpleElementExample_la_SOURCES = \
	basicEvent.h \
	example0.h \
	example0.cc \
	example1.h \
	example1.cc \
	basicLinks.h \
	basicLinks.cc \
 	basicStatistics.h \
     	basicStatistics.cc \
     	basicClocks.h \
     	basicClocks.cc \
	basicParams.h \
	basicParams.cc

deprecated_libsimpleElementExample_sources =

if !SST_ENABLE_PREVIEW_BUILD
libsimpleElementExample_la_SOURCES += $(deprecated_libsimpleElementExample_sources)
endif

EXTRA_DIST = \
    README \
    tests/example0.py \
    tests/example1.py \
    tests/basicClocks.py \
    tests/basicLinks.py \
    tests/basicParams.py \
    tests/basicStatistics0.py \
    tests/basicStatistics1.py \
    tests/basicStatistics2.py

deprecated_EXTRA_DIST =

if !SST_ENABLE_PREVIEW_BUILD
EXTRA_DIST += $(deprecated_EXTRA_DIST)
endif


libsimpleElementExample_la_LDFLAGS = -module -avoid-version

install-exec-hook:
	$(SST_REGISTER_TOOL) SST_ELEMENT_SOURCE     simpleElementExample=$(abs_srcdir)
	$(SST_REGISTER_TOOL) SST_ELEMENT_TESTS      simpleElementExample=$(abs_srcdir)/tests

##########################################################################
##########################################################################
##########################################################################


