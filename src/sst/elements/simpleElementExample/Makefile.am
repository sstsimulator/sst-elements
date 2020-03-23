# -*- Makefile -*-
#
#

AM_CPPFLAGS = \
	$(MPI_CPPFLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libsimpleElementExample.la
libsimpleElementExample_la_SOURCES = \
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
	simpleMessageGeneratorComponent.cc \
	simpleLookupTableComponent.h \
	simpleLookupTableComponent.cc \
	simpleSubComponent.h \
	simpleSubComponent.cc \
	simpleModule.h \
	simpleModule.cc \
	simpleParamComponent.h \
	simpleParamComponent.cc

deprecated_libsimpleElementExample_sources =

if !SST_ENABLE_PREVIEW_BUILD
libsimpleElementExample_la_SOURCES += $(deprecated_libsimpleElementExample_sources)
endif

EXTRA_DIST = \
    README \
    tests/test_simpleParamComponent.py \
    tests/test_simpleComponent.py \
    tests/test_simpleClockerComponent.py \
    tests/test_simpleDistribComponent_discrete.py \
    tests/test_simpleDistribComponent_expon.py \
    tests/test_simpleDistribComponent_gaussian.py \
    tests/test_simpleDistribComponent_poisson.py \
    tests/test_simpleLookupTable.py \
    tests/test_simpleRNGComponent_mersenne.py \
    tests/test_simpleRNGComponent_marsaglia.py \
    tests/test_simpleRNGComponent_xorshift.py \
    tests/test_simpleStatisticsComponent.py \
    tests/test_simpleMessageGeneratorComponent.py \
    tests/test_simpleSubComponent.py \
    tests/test_simpleSubComponent_2.py \
    tests/subcomponent_tests/test_sc_2a.py \
    tests/subcomponent_tests/test_sc_2u2u.py \
    tests/subcomponent_tests/test_sc_2u.py \
    tests/subcomponent_tests/test_sc_a.py \
    tests/subcomponent_tests/test_sc_u2u.py \
    tests/subcomponent_tests/test_sc_u.py \
    tests/subcomponent_tests/test_sc_2u2a.py \
    tests/subcomponent_tests/test_sc_2ua.py \
    tests/subcomponent_tests/test_sc_2uu.py \
    tests/subcomponent_tests/test_sc_u2a.py \
    tests/subcomponent_tests/test_sc_ua.py \
    tests/subcomponent_tests/test_sc_uu.py


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


