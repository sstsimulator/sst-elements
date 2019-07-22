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
	simpleSubComponentLegacy.h \
	simpleSubComponentLegacy.cc \
	simpleModule.h \
	simpleModule.cc \
	simpleParamComponent.h \
	simpleParamComponent.cc

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
    tests/subcomponent_tests/test_sc_uu.py \
    tests/subcomponent_tests/legacy/test_sc_legacy_2nl.py \
    tests/subcomponent_tests/legacy/test_sc_legacy_n2l.py \
    tests/subcomponent_tests/legacy/test_sc_legacy_n.py \
    tests/subcomponent_tests/legacy/test_sc_legacy_2l.py \
    tests/subcomponent_tests/legacy/test_sc_legacy_2nn.py \
    tests/subcomponent_tests/legacy/test_sc_legacy_n2n.py \
    tests/subcomponent_tests/legacy/test_sc_legacy_2n2l.py \
    tests/subcomponent_tests/legacy/test_sc_legacy_2n.py \
    tests/subcomponent_tests/legacy/test_sc_legacy_nl.py \
    tests/subcomponent_tests/legacy/test_sc_legacy_2n2n.py \
    tests/subcomponent_tests/legacy/test_sc_legacy_l.py \
    tests/subcomponent_tests/legacy/test_sc_legacy_nn.py

libsimpleElementExample_la_LDFLAGS = -module -avoid-version

##########################################################################
##########################################################################
##########################################################################


