# -*- Makefile -*-
#
#

AM_CPPFLAGS = \
	$(BOOST_CPPFLAGS) \
	$(MPI_CPPFLAGS) 

compdir = $(pkglibdir)
comp_LTLIBRARIES = libsimpleElementExample.la
libsimpleElementExample_la_SOURCES = \
	simpleElementExample.cc \
	simpleComponent.h \
	simpleComponent.cc \
	simpleComponentEvent.h \
	simpleClockerComponent.h \
	simpleClockerComponent.cc \
	simpleDistribComponent.h \
	simpleDistribComponent.cc \
	simpleRNGComponent.h \
	simpleRNGComponent.cc \
	simpleStatisticsComponent.h \
	simpleStatisticsComponent.cc \
	simpleMessage.h \
	simpleMessageGeneratorComponent.h \
	simpleMessageGeneratorComponent.cc 

EXTRA_DIST = \
    README \
    tests/test_simpleComponent.py \
    tests/test_simpleClockerComponent.py \
    tests/test_simpleDistribComponent_discrete.py \
    tests/test_simpleDistribComponent_expon.py \
    tests/test_simpleDistribComponent_gaussian.py \
    tests/test_simpleDistribComponent_poisson.py \
    tests/test_simpleRNGComponent_mersenne.py \
    tests/test_simpleRNGComponent_marsaglia.py \
    tests/test_simpleRNGComponent_xorshift.py \
    tests/test_simpleStatisticsComponent.py \
    tests/test_simpleMessageGeneratorComponent.py

libsimpleElementExample_la_LDFLAGS = -module -avoid-version

##########################################################################
##########################################################################
##########################################################################


