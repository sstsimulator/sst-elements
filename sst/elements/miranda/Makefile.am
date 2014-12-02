
AM_CPPFLAGS = \
	$(BOOST_CPPFLAGS) \
	$(MPI_CPPFLAGS) 

compdir = $(pkglibdir)
comp_LTLIBRARIES = libmiranda.la

libmiranda_la_SOURCES = \
	reqGenModule.h \
	requestGenCPU.cc \
	requestGenCPU.h	\
	generators/singlestream.h \
	generators/singlestream.cc

libmiranda_la_LDFLAGS = -module -avoid-version
