# -*- Makefile -*-
#
#
AM_CPPFLAGS = $(BOOST_CPPFLAGS) $(MPI_CPPFLAGS)

EXTRA_DIST = sstdisksim.xml

compdir = $(pkglibdir)
comp_LTLIBRARIES = libsstdisksim.la libsstdisksim_tracereader.la libsstdisksim_luascriptreader.la

#libsstdisksim
libsstdisksim_la_SOURCES = \
	sstdisksim.cpp \
	sstdisksim.h 

libsstdisksim_la_CPPFLAGS = $(BOOST_CPPFLAGS) $(SSTDISKSIM_CPPFLAGS) \
	-I$(top_srcdir)/elements/include
libsstdisksim_la_LDFLAGS = -module -avoid-version $(SSTDISKSIM_LDFLAGS)
libsstdisksim_la_LIBADD = $(SSTDISKSIM_LIB)

#libsstdisksim_tracereader
libsstdisksim_tracereader_la_SOURCES = \
	sstdisksim_otf_parser.cpp \
	sstdisksim_otf_parser.h \
	sstdisksim_tracereader.cpp \
	sstdisksim_tracereader.h \
	sstdisksim.cpp \
	sstdisksim.h	

libsstdisksim_tracereader_la_CPPFLAGS = $(BOOST_CPPFLAGS) $(SSTDISKSIM_CPPFLAGS) \
	-I$(top_srcdir)/elements/include
libsstdisksim_tracereader_la_LDFLAGS = -module -avoid-version $(SSTDISKSIM_LDFLAGS)
libsstdisksim_tracereader_la_LIBADD = $(SSTDISKSIM_LIB)

#libsstdisksim_luascriptreader
libsstdisksim_luascriptreader_la_SOURCES = \
	sstdisksim_luascriptreader.cpp \
	sstdisksim_luascriptreader.h \
	sstdisksim.cpp \
	sstdisksim.h	

libsstdisksim_luascriptreader_la_CPPFLAGS = $(BOOST_CPPFLAGS) $(SSTDISKSIM_CPPFLAGS) \
	-I$(top_srcdir)/elements/include
libsstdisksim_luascriptreader_la_LDFLAGS = -module -avoid-version $(SSTDISKSIM_LDFLAGS)
libsstdisksim_luascriptreader_la_LIBADD = $(SSTDISKSIM_LIB)

