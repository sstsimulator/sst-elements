# -*- Makefile -*-
#
#

AM_CPPFLAGS = 	-I$(top_srcdir)/src \
	$(MPI_CPPFLAGS) \
	$(PYTHON_CPPFLAGS) $(CPPFLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libmerlin.la

libmerlin_la_SOURCES = \
	merlin.h \
	merlin.cc \
	router.h \
	linkControl.h \
	linkControl.cc \
	portControl.h \
	portControl.cc \
	reorderLinkControl.h \
	reorderLinkControl.cc \
	bridge.h \
	bridge.cc \
	offeredload/offered_load.h \
	offeredload/offered_load.cc \
	target_generator/target_generator.h \
	target_generator/target_generator.cc \
	target_generator/bit_complement.h \
	target_generator/uniform.h \
	test/nic.h \
	test/nic.cc \
	test/route_test/route_test.h \
	test/route_test/route_test.cc \
	test/pt2pt/pt2pt_test.h \
	test/pt2pt/pt2pt_test.cc \
	test/bisection/bisection_test.h \
	test/bisection/bisection_test.cc \
	test/simple_patterns/shift.h \
	test/simple_patterns/shift.cc \
	topology/torus.h \
	topology/torus.cc \
	topology/mesh.h \
	topology/mesh.cc \
	topology/fattree.h \
	topology/fattree.cc \
	topology/dragonfly.h \
	topology/dragonfly.cc \
	topology/dragonfly_legacy.h \
	topology/dragonfly_legacy.cc \
	topology/dragonfly2.h \
	topology/dragonfly2.cc \
	topology/singlerouter.h \
	topology/singlerouter.cc \
	topology/hyperx.h \
	topology/hyperx.cc \
	hr_router/hr_router.h \
	hr_router/hr_router.cc \
	hr_router/xbar_arb_age.h \
	hr_router/xbar_arb_lru.h \
	hr_router/xbar_arb_lru_infx.h \
	hr_router/xbar_arb_rand.h \
	hr_router/xbar_arb_rr.h \
	trafficgen/trafficgen.h \
	trafficgen/trafficgen.cc \
	inspectors/circuitCounter.h \
	inspectors/circuitCounter.cc \
	inspectors/testInspector.cc \
	inspectors/testInspector.h \
	pymodule.h \
	pymodule.c \
	pymerlin.py

EXTRA_DIST = \
	tests/dragon_128_test.py \
	tests/dragon_72_test.py \
	tests/fattree_128_test.py \
	tests/fattree_256_test.py \
	tests/torus_128_test.py \
	tests/torus_5_trafficgen.py \
	tests/torus_64_test.py

sstdir = $(includedir)/sst/elements/merlin
nobase_sst_HEADERS = \
	linkControl.h \
	router.h

libmerlin_la_LDFLAGS = -module -avoid-version $(PYTHON_LDFLAGS)

BUILT_SOURCES = pymerlin.inc

# This sed script converts 'od' output to a comma-separated list of byte-
# values, suitable for #include'ing into an array definition.
# This can be done much more simply with xxd or hexdump, but those tools
# are not installed by default on all supported platforms.
#
# od:	-v:		Print all data
#		-t x1:	Print as byte-values, in hex
# sed:	Script 1:  Remove base-address column from od output
# 		Script 2:  Remove trailing blank line resulting from script 1
# 		Script 3:  Add '0x' prefix, and ',' suffix to each value
%.inc: %.py
	od -v -t x1 < $< | sed -e 's/^[^ ]*[ ]*//g' -e '/^\s*$$/d' -e 's/\([0-9a-f]*\)[ $$]*/0x\1,/g' > $@

clean-local: clean-local-check
.PHONY: clean-local-check
clean-local-check:
	-rm -rf pymerlin.inc
