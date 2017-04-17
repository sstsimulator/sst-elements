AM_CPPFLAGS = -Wall \
	$(MPIT_CPPFLAGS) 

AM_LDFLAGS = -lm 
compdir = $(pkglibdir)
comp_LTLIBRARIES = libHMC.la

libHMC_la_SOURCES = HMC.cc \
	HMCadapter.h \
	HMCadapter.cc \
	HMCsimplemem.h \
	HMCsimplemem.cc

libHMC_la_CPPFLAGS = \
	$(MICRON_HMCSIM_CPPFLAGS) \
	$(SST_CXX0X_FLAGS) -fPIC -Wall \
	$(MPI_CPPFLAGS)

libHMC_la_LDFLAGS = \
	$(MICRON_HMCSIM_LDFLAGS) \
	-avoid-version

libHMC_la_LIBADD = \
	$(MICRON_HMCSIM_LIBS) \
	$(SST_SYSTEMC_LIB)

#noinst_PROGRAMS = infogather
#infogather_SOURCES = infogather.cc
#infogather_CPPFLAGS = -I/usr/local/systemc-2.3/include \
#											-std=c++11 -fPIC -Wall \
#											$(MPI_CPPFLAGS) 
#infogather_LDFLAGS =  -L/usr/local/systemc-2.3/lib-linux64/ -lsystemc \
#											-lhmcsim \
#										 	-avoid-version
#infogather_LDADD = libhmcsim.so

