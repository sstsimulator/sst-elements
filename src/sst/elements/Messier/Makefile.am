AM_CPPFLAGS = \
	$(BOOST_CPPFLAGS) \
	$(MPIT_CPPFLAGS) 

AM_LDFLAGS = -lm 
compdir = $(pkglibdir)
comp_LTLIBRARIES = libMessier.la 

libMessier_la_SOURCES = \
	libMessier.cpp \
	memReqEvent.h \
	Messier.cc \
	Messier.h \
	Bank.h \
	Rank.h \
	WriteBuffer.h \
	WriteBuffer.cc \
	NVM_Request.h \
	NVM_Parser.h \
	NVM_DIMM.h \
	NVM_DIMM.cc \
	NVM_Params.h 
	


libMessier_la_CPPFLAGS = \
	-I$(top_srcdir)/src \
	$(SST_CXX0X_FLAGS) -fPIC -Wall \
	$(BOOST_CPPFLAGS) \
	$(MPI_CPPFLAGS)

libMessier_la_LDFLAGS = \
	-avoid-version

libMessier_la_LIBADD = \
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

