# -*- Makefile -*-
#
#

AM_CPPFLAGS += -fmax-errors=1 \
	-I$(top_srcdir)/src \
	$(MPI_CPPFLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libgolem.la
libgolem_la_SOURCES = \
	golem.cc \
	array/computeArray.h \
	array/mvmComputeArray.h \
	array/mvmFloatArray.h \
	array/mvmIntArray.h \
	array/crossSimComputeArray.h \
	array/crossSimFloatArray.h \
	array/crossSimIntArray.h \
	rocc/roccAnalog.h \
	rocc/roccAnalogFloat.h \
	rocc/roccAnalogInt.h


libgolem_la_LDFLAGS = -module -avoid-version

install-exec-hook:
	$(SST_REGISTER_TOOL) SST_ELEMENT_SOURCE     golem=$(abs_srcdir)
	$(SST_REGISTER_TOOL) SST_ELEMENT_TESTS      golem=$(abs_srcdir)/tests

##########################################################################
##########################################################################
##########################################################################
