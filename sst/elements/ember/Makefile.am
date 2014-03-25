
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
	embergen.cc	\
	emberinitev.cc  \
	emberrecvev.cc  \
	embersendev.cc \
	emberengine.cc  \
	emberevent.h \
	emberevent.cc \
	emberfinalizeev.h \
	emberinitev.h  \
	emberrecvev.h \
	embersendev.h \
	emberstartev.h \
	emberstartev.cc \
	embercomputeev.h \
	embercomputeev.cc \
	motifs/emberhalo1d.h  \
	motifs/emberhalo1d.cc  \
	motifs/emberhalo2d.h  \
	motifs/emberhalo2d.cc  \
	motifs/emberfini.h  \
	motifs/emberfini.cc  \
	motifs/emberring.h  \
	motifs/emberring.cc  \
	motifs/emberpingpong.cc \
	motifs/emberpingpong.h


libember_la_LDFLAGS = -module -avoid-version
