
AM_CPPFLAGS = \
	-I$(top_srcdir)/src \
	$(MPI_CPPFLAGS)


compdir = $(pkglibdir)
comp_LTLIBRARIES = libkingsley.la

libkingsley_la_SOURCES = \
	nocEvents.h \
	noc_mesh.h \
	noc_mesh.cc \
	lru_unit.h \
	linkControl.h \
	linkControl.cc

EXTRA_DIST = \
	tests/testsuite_default_kingsley.py \
	tests/noc_mesh_32_test.py \
	tests/refFiles/test_kingsley_noc_mesh_32_test.out

libkingsley_la_LDFLAGS = -module -avoid-version

install-exec-hook:
	$(SST_REGISTER_TOOL) SST_ELEMENT_SOURCE     kingsley=$(abs_srcdir)
	$(SST_REGISTER_TOOL) SST_ELEMENT_TESTS      kingsley=$(abs_srcdir)/tests
