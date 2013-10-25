
AM_CPPFLAGS = \
        $(BOOST_CPPFLAGS) \
        $(MPI_CPPFLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libariel.la
libariel_la_SOURCES = \
        ariel.h \
        ariel.cc \
	arielcore.cc \
	arielcore.h \
	arielevent.cc \
	arielevent.h \
	arielmemmgr.cc \
	arielmemmgr.h \
	arielreadev.cc \
	arielreadev.h \
	arielwriteev.cc \
	arielwriteev.h

libariel_la_LDFLAGS = -module -avoid-version
libariel_la_CPPFLAGS = -DARIEL_TRACE_LIB=$(libdir)/sst \
	$(CPPFLAGS) $(AM_CPPFLAGS)
