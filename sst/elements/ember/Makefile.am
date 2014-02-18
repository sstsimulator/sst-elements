
AM_CPPFLAGS = \
	$(BOOST_CPPFLAGS) \
	$(MPI_CPPFLAGS) 

compdir = $(pkglibdir)
comp_LTLIBRARIES = libember.la
libember_la_SOURCES = \
	ember.cc \
	emberengine.h  \
	emberfinalizeev.cc  \
	embergen.h     \
	emberinitev.cc  \
	emberrecvev.cc  \
	embersendev.cc \
	emberengine.cc  \
	emberevent.h \
	emberfinalizeev.h \
	emberhalo1d.h  \
	emberinitev.h  \
	emberrecvev.h \
	embersendev.h

libember_la_LDFLAGS = -module -avoid-version
