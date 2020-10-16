AM_CPPFLAGS = \
	$(MPIT_CPPFLAGS)

AM_LDFLAGS = -lm
compdir = $(pkglibdir)
comp_LTLIBRARIES = libMessier.la

libMessier_la_SOURCES = \
	Cache.h \
	libMessier.cpp \
	memReqEvent.h \
	Messier.cc \
	Messier.h \
	Bank.h \
	Rank.h \
        Messier_Event.h \
	WriteBuffer.h \
	WriteBuffer.cc \
	NVM_Request.h \
	NVM_DIMM.h \
	NVM_DIMM.cc \
	NVM_Params.h



libMessier_la_CPPFLAGS = \
	-I$(top_srcdir)/src \
	$(SST_CXX0X_FLAGS) -fPIC -Wall \
	$(MPI_CPPFLAGS)

libMessier_la_LDFLAGS = \
	-avoid-version

libMessier_la_LIBADD = \
	$(SST_SYSTEMC_LIB)


EXTRA_DIST = \
	tests/testsuite_default_Messier.py \
	tests/gupsgen_2RANKS.py \
	tests/gupsgen_fastNVM.py \
	tests/gupsgen.py \
	tests/stencil3dbench_messier.py \
	tests/streambench_messier.py \
    tests/refFiles/test_Messier_gupsgen.out \
    tests/refFiles/test_Messier_gupsgen_2RANKS.out \
    tests/refFiles/test_Messier_gupsgen_fastNVM.out \
    tests/refFiles/test_Messier_stencil3dbench_messier.out \
    tests/refFiles/test_Messier_streambench_messier.out

#noinst_PROGRAMS = infogather
#infogather_SOURCES = infogather.cc
#infogather_CPPFLAGS = -I/usr/local/systemc-2.3/include \
#											-std=c++11 -fPIC -Wall \
#											$(BOOST_CPPFLAGS) \
#											$(MPI_CPPFLAGS)
#infogather_LDFLAGS =  -L/usr/local/systemc-2.3/lib-linux64/ -lsystemc \
#											-lhmcsim \
#										 	-avoid-version
#infogather_LDADD = libhmcsim.so

install-exec-hook:
	$(SST_REGISTER_TOOL) SST_ELEMENT_SOURCE     Messier=$(abs_srcdir)
	$(SST_REGISTER_TOOL) SST_ELEMENT_TESTS      Messier=$(abs_srcdir)/tests
