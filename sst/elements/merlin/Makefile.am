# -*- Makefile -*-
#
#

AM_CPPFLAGS = $(BOOST_CPPFLAGS) $(MPI_CPPFLAGS) $(PYTHON_CPPFLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libmerlin.la

libmerlin_la_SOURCES = \
	merlin.cc \
	router.h \
	linkControl.h \
	linkControl.cc \
	portControl.h \
	test/nic.h \
	test/nic.cc \
	test/pt2pt/pt2pt_test.h \
	test/pt2pt/pt2pt_test.cc \
	topology/torus.h \
	topology/torus.cc \
	topology/fattree.h \
	topology/fattree.cc \
	topology/dragonfly.h \
	topology/dragonfly.cc \
	topology/singlerouter.h \
	topology/singlerouter.cc \
	hr_router/hr_router.h \
	hr_router/hr_router.cc \
	hr_router/xbar_arb_rr.h \
	trafficgen/trafficgen.h \
	trafficgen/trafficgen.cc \
	pymodule.c \
	pymerlin.py


libmerlin_la_LDFLAGS = -module -avoid-version $(PYTHON_LDFLAGS)

%.inc: %.py
	$(XXD_PATH) -i < $< > $@

clean-local: clean-local-check
.PHONY: clean-local-check
clean-local-check:
	-rm -rf pymerlin.inc
