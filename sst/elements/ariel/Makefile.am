
AM_CPPFLAGS = \
        $(BOOST_CPPFLAGS) \
        $(MPI_CPPFLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libariel.la
libariel_la_SOURCES = \
	libariel.cc \
	arielcpu.cc \
	arielcpu.h \
	arielcore.cc \
	arielcore.h \
	arielmemmgr.cc \
	arielmemmgr.h \
	arielreadev.cc \
	arielreadev.h \
	arielexitev.h \
	arielexitev.cc \
	arielwriteev.cc \
	arielwriteev.h \
	arielevent.cc \
	arielevent.h \
	arielnoop.h \
	arielnoop.cc \
	arielallocev.h \
	arielallocev.cc \
	arielfreeev.h \
	arielfreeev.cc \
	arielswitchpool.cc \
	arielswitchpool.h \
	ariel_shmem.h

libariel_la_LDFLAGS = -module -avoid-version
libariel_la_CPPFLAGS = -DARIEL_TRACE_LIB=$(libdir)/sst \
	$(CPPFLAGS) $(AM_CPPFLAGS)


sstdir = $(includedir)/sst/elements/ariel
nobase_sst_HEADERS = \
	ariel_shmem.h 
