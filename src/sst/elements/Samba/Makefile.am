AM_CPPFLAGS = \
	$(BOOST_CPPFLAGS) \
	$(MPIT_CPPFLAGS) 

AM_LDFLAGS = -lm 
compdir = $(pkglibdir)
comp_LTLIBRARIES = libSamba.la 

libSamba_la_SOURCES = \
	libSamba.cc \
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
	$(BOOST_CPPFLAGS) \
	$(MPI_CPPFLAGS)

libSamba_la_LDFLAGS = \
	-avoid-version

libSamba_la_LIBADD = \
	$(SST_SYSTEMC_LIB)

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

