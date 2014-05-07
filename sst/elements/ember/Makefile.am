
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
	embermpigen.h \
	embermpigen.cc \
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
	emberstopev.h \
	embercomputeev.h \
	embercomputeev.cc \
	emberwaitev.cc \
	emberwaitev.h \
	emberirecvev.cc \
	emberirecvev.h \
	emberbarrierev.h \
	emberbarrierev.cc \
	emberallredev.h \
	emberallredev.cc \
	motifs/embernull.h  \
	motifs/emberhalo1d.h  \
	motifs/emberhalo1d.cc  \
	motifs/emberhalo2d.h  \
	motifs/emberhalo2d.cc  \
	motifs/emberhalo2dNBR.h  \
	motifs/emberhalo2dNBR.cc  \
	motifs/embersweep2d.h  \
	motifs/embersweep2d.cc  \
	motifs/emberfini.h  \
	motifs/emberfini.cc  \
	motifs/embernull.h \
	motifs/emberring.h  \
	motifs/emberring.cc  \
	motifs/emberpingpong.cc \
	motifs/emberpingpong.h \
	motifs/emberbarrier.cc \
	motifs/emberbarrier.h \
	motifs/emberallpingpong.cc \
	motifs/emberallpingpong.h


libember_la_LDFLAGS = -module -avoid-version

EXTRA_DIST = \
	test/embernightly.py
