AM_CPPFLAGS = \
	$(MPIT_CPPFLAGS)

AM_LDFLAGS = -lm
compdir = $(pkglibdir)
comp_LTLIBRARIES = libOpal.la

libOpal_la_SOURCES = \
	mempool.h \
	mempool.cc \
	Opal.cc \
	Opal.h \
        Opal_Event.h \
	arielmemmgr_opal.cc \
	arielmemmgr_opal.h \
	opalMemNIC.cc \
	opalMemNIC.h \
	PageFaultHandler.cc \
	PageFaultHandler.h


libOpal_la_CPPFLAGS = \
	-I$(top_srcdir)/src \
	$(SST_CXX0X_FLAGS) -fPIC -Wall \
	$(MPI_CPPFLAGS)

libOpal_la_LDFLAGS = \
	-avoid-version

libOpal_la_LIBADD = \
	$(SST_SYSTEMC_LIB)

#
# EXTRA_DIST = \
# 	tests/gupsgen_2RANKS.py \
# 	tests/gupsgen_fastNVM.py \
# 	tests/gupsgen.py \
# 	tests/stencil3dbench_messier.py \
#	tests/streambench_messier.py

#noinst_PROGRAMS = infogather
#infogather_SOURCES = infogather.cc
#infogather_CPPFLAGS = -I/usr/local/systemc-2.3/include \
#											-std=c++11 -fPIC -Wall \
#											#											$(BOOST_CPPFLAGS) \
#											#											$(MPI_CPPFLAGS)
#											#infogather_LDFLAGS =  -L/usr/local/systemc-2.3/lib-linux64/ -lsystemc \
#											#											-lhmcsim \
#											#										 	-avoid-version
#											#infogather_LDADD = libhmcsim.so
#

install-exec-hook:
	$(SST_REGISTER_TOOL) SST_ELEMENT_SOURCE     Opal=$(abs_srcdir)
	$(SST_REGISTER_TOOL) SST_ELEMENT_TESTS      Opal=$(abs_srcdir)/tests
