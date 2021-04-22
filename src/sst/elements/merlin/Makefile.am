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
	bridge.h \
	bridge.cc \
	background_traffic/background_traffic.h \
	background_traffic/background_traffic.cc \
	offeredload/offered_load.h \
	offeredload/offered_load.cc \
	target_generator/target_generator.h \
	target_generator/target_generator.cc \
	target_generator/bit_complement.h \
	target_generator/shift.h \
	target_generator/uniform.h \
	test/nic.h \
	test/nic.cc \
	test/route_test/route_test.h \
	test/route_test/route_test.cc \
	test/pt2pt/pt2pt_test.h \
	test/pt2pt/pt2pt_test.cc \
	test/bisection/bisection_test.h \
	test/bisection/bisection_test.cc \
	test/simple_patterns/empty.h \
	test/simple_patterns/empty.cc \
	test/simple_patterns/shift.h \
	test/simple_patterns/shift.cc \
	test/simple_patterns/incast.h \
	test/simple_patterns/incast.cc \
	topology/torus.h \
	topology/torus.cc \
	topology/mesh.h \
	topology/mesh.cc \
	topology/fattree.h \
	topology/fattree.cc \
	topology/dragonfly.h \
	topology/dragonfly.cc \
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
	interfaces/linkControl.h \
	interfaces/linkControl.cc \
	interfaces/portControl.h \
	interfaces/portControl.cc \
	interfaces/reorderLinkControl.h \
	interfaces/reorderLinkControl.cc \
	interfaces/output_arb_basic.h \
	interfaces/output_arb_qos_multi.h \
	arbitration/single_arb.h \
	arbitration/single_arb_lru.h \
	arbitration/single_arb_rr.h \
	pymodule.h \
	pymodule.c \
	pymerlin.py \
	pymerlin-base.py \
	pymerlin-endpoint.py \
	pymerlin-router.py \
	interfaces/pymerlin-interface.py \
	target_generator/pymerlin-targetgen.py \
	topology/pymerlin-topo-dragonfly.py \
	topology/pymerlin-topo-hyperx.py \
	topology/pymerlin-topo-fattree.py \
	topology/pymerlin-topo-mesh.py

EXTRA_DIST = \
	tests/testsuite_default_merlin.py \
	tests/hyperx_128_test.py \
	tests/dragon_128_test.py \
	tests/dragon_72_test.py \
	tests/fattree_128_test.py \
	tests/fattree_256_test.py \
	tests/torus_128_test.py \
	tests/torus_5_trafficgen.py \
	tests/torus_64_test.py \
	tests/dragon_128_test_fl.py \
	tests/dragon_128_platform_test.py \
	tests/dragon_128_platform_test_cm.py \
	tests/platform_file_dragon_128.py \
	tests/refFiles/test_merlin_dragon_128_platform_test.out \
	tests/refFiles/test_merlin_dragon_128_platform_test_cm.out \
	tests/refFiles/test_merlin_dragon_128_test.out \
	tests/refFiles/test_merlin_dragon_128_test_fl.out \
	tests/refFiles/test_merlin_dragon_72_test.out \
	tests/refFiles/test_merlin_fattree_128_test.out \
	tests/refFiles/test_merlin_fattree_256_test.out \
	tests/refFiles/test_merlin_hyperx_128_test.out \
	tests/refFiles/test_merlin_torus_128_test.out \
	tests/refFiles/test_merlin_torus_5_trafficgen.out \
	tests/refFiles/test_merlin_torus_64_test.out

sstdir = $(includedir)/sst/elements/merlin
nobase_sst_HEADERS = \
	router.h

libmerlin_la_LDFLAGS = -module -avoid-version $(PYTHON_LDFLAGS)

BUILT_SOURCES = \
	pymerlin.inc \
	pymerlin-base.inc \
	pymerlin-endpoint.inc \
	pymerlin-router.inc \
	interfaces/pymerlin-interface.inc \
	target_generator/pymerlin-targetgen.inc \
	topology/pymerlin-topo-dragonfly.inc \
	topology/pymerlin-topo-hyperx.inc \
	topology/pymerlin-topo-fattree.inc \
	topology/pymerlin-topo-mesh.inc

install-exec-hook:
	$(SST_REGISTER_TOOL) SST_ELEMENT_SOURCE     merlin=$(abs_srcdir)
	$(SST_REGISTER_TOOL) SST_ELEMENT_TESTS      merlin=$(abs_srcdir)/tests

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
	-rm -rf $(BUILT_SOURCES)
