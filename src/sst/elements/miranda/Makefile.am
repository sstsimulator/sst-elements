
AM_CPPFLAGS = \
	$(MPI_CPPFLAGS) \
	-I$(top_srcdir)/src

compdir = $(pkglibdir)
comp_LTLIBRARIES = libmiranda.la

libmiranda_la_SOURCES = \
	mirandaEvent.h \
	mirandaGenerator.h \
	mirandaCPU.cc \
	mirandaCPU.h	\
	mirandaMemMgr.h \
	mirandaIncGen.cc \
	generators/singlestream.h \
	generators/singlestream.cc \
	generators/randomgen.h \
	generators/randomgen.cc \
	generators/streambench.h \
	generators/streambench.cc \
	generators/revsinglestream.h \
	generators/inorderstreambench.h \
	generators/revsinglestream.cc \
	generators/stencil3dbench.h \
	generators/stencil3dbench.cc \
	generators/gupsgen.h \
	generators/gupsgen.cc \
	generators/nullgen.h \
	generators/spmvgen.h \
	generators/copygen.h \
	generators/streambench_customcmd.h \
	generators/streambench_customcmd.cc

EXTRA_DIST = \
	tests/testsuite_default_miranda.py \
	tests/randomgen.py \
	tests/singlestream.py \
	tests/revsinglestream.py \
	tests/stencil3dbench.py \
	tests/streambench.py \
	tests/inorderstream.py \
	tests/copybench.py \
	tests/gupsgen.py \
    tests/refFiles/test_miranda_copybench.out \
    tests/refFiles/test_miranda_gupsgen.out \
    tests/refFiles/test_miranda_inorderstream.out \
    tests/refFiles/test_miranda_randomgen.out \
    tests/refFiles/test_miranda_revsinglestream.out \
    tests/refFiles/test_miranda_singlestream.out \
    tests/refFiles/test_miranda_spmvgen.out \
    tests/refFiles/test_miranda_stencil3dbench.out \
    tests/refFiles/test_miranda_streambench.out

libmiranda_la_LDFLAGS = -module -avoid-version

if USE_STAKE
libmiranda_la_SOURCES += \
	generators/stake.cc \
	generators/stake.h
libmiranda_la_LDFLAGS += $(STAKE_LDFLAGS) $(STAKE_LIB)
AM_CPPFLAGS += $(STAKE_CPPFLAGS) -DHAVE_STAKE
endif

install-exec-hook:
	$(SST_REGISTER_TOOL) SST_ELEMENT_SOURCE     miranda=$(abs_srcdir)
	$(SST_REGISTER_TOOL) SST_ELEMENT_TESTS      miranda=$(abs_srcdir)/tests
