
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
	embermap.h \
	emberlinearmap.h \
	emberstartev.h \
	emberstartev.cc \
	emberstopev.h \
	embercomputeev.h \
	embercomputeev.cc \
	emberwaitev.cc \
	emberwaitev.h \
	emberwaitanyev.cc \
	emberwaitanyev.h \
	emberwaitallev.cc \
	emberwaitallev.h \
	emberirecvev.cc \
	emberirecvev.h \
	emberisendev.cc \
	emberisendev.h \
	emberbarrierev.h \
	emberbarrierev.cc \
	emberallredev.h \
	emberallredev.cc \
	emberredev.h \
	emberredev.cc \
	embergettimeev.h \
	embergettimeev.cc \
	embermap.h \
	emberlinearmap.h \
	emberdistrib.h \
	emberdistrib.cc \
	emberconstdistrib.h \
	emberconstdistrib.cc \
	embergaussdistrib.h \
	embergaussdistrib.cc \
	motifs/ember3dcommdbl.h \
	motifs/ember3dcommdbl.cc \
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
	motifs/emberallpingpong.h \
	motifs/emberallreduce.cc \
	motifs/emberallreduce.h \
	motifs/emberreduce.cc \
	motifs/emberreduce.h \
	motifs/emberhalo3d.h \
	motifs/emberhalo3d.cc \
	motifs/emberhalo3dsv.h \
	motifs/emberhalo3dsv.cc \
	motifs/emberhalo3d26.h \
	motifs/emberhalo3d26.cc \
	motifs/embersweep3d.h \
	motifs/embersweep3d.cc \
	motifs/embernaslu.h \
	motifs/embernaslu.cc \
	motifs/embermsgrate.h \
	motifs/embermsgrate.cc

#	motifs/emberhalo3dnb.h \
#	motifs/emberhalo3dnb.cc

bin_PROGRAMS = sstspygen

sstspygen_SOURCES = tools/spygen/spygen.cc

libember_la_LDFLAGS = -module -avoid-version

EXTRA_DIST = \
	test/emberLoad.py \
	test/loadInfo.py \
	test/CrossProduct.py \
	test/networkConfig.py \
	test/Tester.py 
