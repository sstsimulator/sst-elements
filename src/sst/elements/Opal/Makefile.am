AM_CPPFLAGS = \
	$(MPIT_CPPFLAGS) 

AM_LDFLAGS = -lm 
compdir = $(pkglibdir)
comp_LTLIBRARIES = libOpal.la 

libOpal_la_SOURCES = \
	mempool.h \
	mempool.cpp \
	Opal.cc \
	Opal.h \
        Opal_Event.h 	


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
