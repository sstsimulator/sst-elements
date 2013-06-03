
AM_CPPFLAGS = \
	$(BOOST_CPPFLAGS) \
	$(MPI_CPPFLAGS) 

compdir = $(pkglibdir)
comp_LTLIBRARIES = libvanadis.la
libvanadis_la_SOURCES = \
	vanadis.cc

libvanadis_la_LDFLAGS = -module -avoid-version
