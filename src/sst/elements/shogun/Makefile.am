# -*- Makefile -*-
#
#

AM_CPPFLAGS = \
	$(MPI_CPPFLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libshogun.la
libshogun_la_SOURCES = \
	shogun.cc \
	shogun.h \
	shogun_credit_event.h \
	shogun_event.h \
	shogun_init_event.h \
	shogun_nic.cc \
	shogun_nic.h \
	shogun_q.h \
	shogun_stat_bundle.h \
	arb/shogunrrarb.cc \
	arb/shogunrrarb.h \
	arb/shogunarb.h

EXTRA_DIST = \
	tests/testsuite_default_shogun.py \
	tests/basic_miranda.py \
	tests/hierarchy_test.py \
    tests/refFiles/test_shogun_basic_miranda.out \
    tests/refFiles/test_shogun_hierarchy_test.out \
    tests/refFiles/test_shogun_hierarchy_test_multi.out

libshogun_la_LDFLAGS = -module -avoid-version

install-exec-hook:
	$(SST_REGISTER_TOOL) SST_ELEMENT_SOURCE     shogun=$(abs_srcdir)
	$(SST_REGISTER_TOOL) SST_ELEMENT_TESTS      shogun=$(abs_srcdir)/tests

