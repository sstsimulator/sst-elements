AM_CPPFLAGS = \
	$(MPIT_CPPFLAGS) 

AM_LDFLAGS = -lm 
compdir = $(pkglibdir)
comp_LTLIBRARIES = libSamba.la 

libSamba_la_SOURCES = \
	libSamba.cc \
	Samba_Event.h \
	Samba.cc \
	Samba.h \
	TLBUnit.cc \
	TLBUnit.h \
	TLBentry.h \
	TLBhierarchy.h \
	TLBhierarchy.cc \
	PageTableWalker.h \
	PageTableWalker.cc


libSamba_la_CPPFLAGS = \
	-I$(top_srcdir)/src \
	$(SST_CXX0X_FLAGS) -fPIC -Wall \
	$(MPI_CPPFLAGS)

libSamba_la_LDFLAGS = \
	-avoid-version

libSamba_la_LIBADD = \
	$(SST_SYSTEMC_LIB)

EXTRA_DIST = \
	tests/gupsgen_mmu_4KB.py \
	tests/gupsgen_mmu.py \
	tests/gupsgen_mmu_three_levels.py \
	tests/stencil3dbench_mmu.py \
	tests/streambench_mmu.py 

#noinst_PROGRAMS = infogather
#infogather_SOURCES = infogather.cc
#infogather_CPPFLAGS = -I/usr/local/systemc-2.3/include \
#											-std=c++11 -fPIC -Wall \
#											$(MPI_CPPFLAGS) 
#infogather_LDFLAGS =  -L/usr/local/systemc-2.3/lib-linux64/ -lsystemc \
#											-lhmcsim \
#										 	-avoid-version
#infogather_LDADD = libhmcsim.so

