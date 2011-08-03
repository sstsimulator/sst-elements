# -*- Makefile -*-
#
#
AM_CPPFLAGS = $(BOOST_CPPFLAGS) $(MPI_CPPFLAGS)

EXTRA_DIST = sstdisksim.xml

compdir = $(pkglibdir)
comp_LTLIBRARIES = libsstdisksim.la libsstdisksim_luascriptreader.la libsstdisksim_tracereader.la \
	libsstdisksim_straightdisk.la libsstdisksim_raid0.la libsstdisksim_raid1.la

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
	sstdisksim_tau_parser.cpp \
	sstdisksim_tau_parser.h \
	sstdisksim_tracereader.cpp \
	sstdisksim_tracereader.h \
	sstdisksim_diskmodel.h \
	sstdisksim_event.cpp \
	sstdisksim_event.h \
	sstdisksim_posix_call.cpp \
	sstdisksim_posix_call.h \
	sstdisksim_straightdisk.h \
	sstdisksim_straightdisk.cpp \
	sstdisksim_raid0.h \
	sstdisksim_raid0.cpp \
	sstdisksim_raid1.h \
	sstdisksim_raid1.cpp \
	sstdisksim.cpp \
	sstdisksim.h	

libsstdisksim_tracereader_la_CPPFLAGS = $(BOOST_CPPFLAGS) $(SSTDISKSIM_CPPFLAGS) \
	-I$(top_srcdir)/elements/include
libsstdisksim_tracereader_la_LDFLAGS = -module -avoid-version $(SSTDISKSIM_LDFLAGS)
libsstdisksim_tracereader_la_LIBADD = $(SSTDISKSIM_LIB)

#libsstdisksim_straightdisk
libsstdisksim_straightdisk_la_SOURCES = \
	sstdisksim_diskmodel.h \
	sstdisksim_event.cpp \
	sstdisksim_event.h \
	sstdisksim_posix_call.cpp \
	sstdisksim_posix_call.h \
	sstdisksim_straightdisk.h \
	sstdisksim_straightdisk.cpp \
	sstdisksim.cpp \
	sstdisksim.h	

libsstdisksim_straightdisk_la_CPPFLAGS = $(BOOST_CPPFLAGS) $(SSTDISKSIM_CPPFLAGS) \
	-I$(top_srcdir)/elements/include
libsstdisksim_straightdisk_la_LDFLAGS = -module -avoid-version $(SSTDISKSIM_LDFLAGS)
libsstdisksim_straightdisk_la_LIBADD = $(SSTDISKSIM_LIB)

#libsstdisksim_raid0
libsstdisksim_raid0_la_SOURCES = \
	sstdisksim_diskmodel.h \
	sstdisksim_event.cpp \
	sstdisksim_event.h \
	sstdisksim_posix_call.cpp \
	sstdisksim_posix_call.h \
	sstdisksim_raid0.h \
	sstdisksim_raid0.cpp \
	sstdisksim.cpp \
	sstdisksim.h	

libsstdisksim_raid0_la_CPPFLAGS = $(BOOST_CPPFLAGS) $(SSTDISKSIM_CPPFLAGS) \
	-I$(top_srcdir)/elements/include
libsstdisksim_raid0_la_LDFLAGS = -module -avoid-version $(SSTDISKSIM_LDFLAGS)
libsstdisksim_raid0_la_LIBADD = $(SSTDISKSIM_LIB)

#libsstdisksim_raid1
libsstdisksim_raid1_la_SOURCES = \
	sstdisksim_diskmodel.h \
	sstdisksim_event.cpp \
	sstdisksim_event.h \
	sstdisksim_posix_call.cpp \
	sstdisksim_posix_call.h \
	sstdisksim_raid1.h \
	sstdisksim_raid1.cpp \
	sstdisksim.cpp \
	sstdisksim.h	

libsstdisksim_raid0_la_CPPFLAGS = $(BOOST_CPPFLAGS) $(SSTDISKSIM_CPPFLAGS) \
	-I$(top_srcdir)/elements/include
libsstdisksim_raid0_la_LDFLAGS = -module -avoid-version $(SSTDISKSIM_LDFLAGS)
libsstdisksim_raid0_la_LIBADD = $(SSTDISKSIM_LIB)

#libsstdisksim_luascriptreader
libsstdisksim_luascriptreader_la_SOURCES = \
	sstdisksim_luascriptreader.cpp \
	sstdisksim_luascriptreader.h \
	sstdisksim_diskmodel.h \
	sstdisksim_event.cpp \
	sstdisksim_event.h \
	sstdisksim_straightdisk.h \
	sstdisksim_straightdisk.cpp \
	sstdisksim_raid0.h \
	sstdisksim_raid0.cpp \
	sstdisksim_posix_call.cpp \
	sstdisksim_posix_call.h 
	sstdisksim.cpp \
	sstdisksim.h	

libsstdisksim_luascriptreader_la_CPPFLAGS = $(BOOST_CPPFLAGS) $(SSTDISKSIM_CPPFLAGS) \
	-I$(top_srcdir)/elements/include
libsstdisksim_luascriptreader_la_LDFLAGS = -module -avoid-version $(SSTDISKSIM_LDFLAGS)
libsstdisksim_luascriptreader_la_LIBADD = $(SSTDISKSIM_LIB)

