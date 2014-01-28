# -*- Makefile -*-
#
#

EXTRA_DIST =

compdir = $(pkglibdir)
comp_LTLIBRARIES = libportals4.la
libportals4_la_SOURCES = \
    portals4lib.cc\
    ptlNic/recvEntry.cpp\
    ptlNic/recvEntry.h\
    ptlNic/dmaEngine.cpp \
    ptlNic/dmaEngine.h \
    ptlNic/context.h \
    ptlNic/context.cpp \
    ptlNic/ptlNic.cpp \
    ptlNic/ptlNicVCInfo.cpp \
    ptlNic/ptlNic.h \
    ptlNic/ptlNicEvent.h \
    ptlNic/cmdQueueEntry.h \
    ptlNic/portals4.h \
    ptlNic/callback.h \
    ptlNic/ptlNicTypes.h \
    ptlNic/dmaEvent.h \
    ptlNic/ptlEvent.h \
    ptlNic/ptlHdr.h \
    ptlNic/cmdQueue.h \
    ptlNic/debug.h \
    ptlNic/nicMmu.h

libportals4_la_CPPFLAGS = $(BOOST_CPPFLAGS) $(MPI_CPPFLAGS) \
            -I$(top_srcdir)/sst/elements/include \
            -I$(top_srcdir)/sst/elements

libportals4_la_LDFLAGS = -module -avoid-version
libportals4_la_LIBADD = $(SST_RT_LIB)
