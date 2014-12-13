
AM_CPPFLAGS = \
	$(BOOST_CPPFLAGS) \
	$(MPI_CPPFLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libember.la

libember_la_SOURCES = \
	embergen.h \
	embergen.cc	\
	embermap.h \
	ember.cc \
	emberengine.h  \
	emberengine.cc  \
	emberevent.h \
	emberevent.cc \
	embergettimeev.h \
	embergettimeev.cc \
	emberlinearmap.h \
	emberlinearmap.h \
	emberdistrib.h \
	emberdistrib.cc \
	emberconstdistrib.h \
	emberconstdistrib.cc \
	embergaussdistrib.h \
	embergaussdistrib.cc \
	embercomputeev.h \
	embercomputeev.cc \
	mpi/embermpigen.h \
	mpi/embermpigen.cc \
	mpi/emberrecvev.h \
	mpi/embersendev.h \
	mpi/emberfinalizeev.h \
	mpi/emberinitev.h  \
	mpi/emberwaitev.h \
	mpi/emberwaitanyev.h \
	mpi/emberwaitallev.h \
	mpi/emberirecvev.h \
	mpi/emberisendev.h \
	mpi/emberbarrierev.h \
	mpi/emberallredev.h \
	mpi/emberredev.h \
	mpi/emberMPIEvent.h \
	mpi/emberrankev.h \
	mpi/embersizeev.h \
	mpi/emberCommSplitEv.h \
	mpi/motifs/emberinit.h  \
	mpi/motifs/emberfini.h  \
	mpi/motifs/ember3dcommdbl.h \
	mpi/motifs/ember3dcommdbl.cc \
	mpi/motifs/embernull.h  \
	mpi/motifs/emberhalo1d.h  \
	mpi/motifs/emberhalo1d.cc  \
	mpi/motifs/emberhalo2d.h  \
	mpi/motifs/emberhalo2d.cc  \
	mpi/motifs/emberhalo2dNBR.h  \
	mpi/motifs/emberhalo2dNBR.cc  \
	mpi/motifs/embersweep2d.h  \
	mpi/motifs/embersweep2d.cc  \
	mpi/motifs/embernull.h \
	mpi/motifs/emberring.h  \
	mpi/motifs/emberring.cc  \
	mpi/motifs/emberpingpong.cc \
	mpi/motifs/emberpingpong.h \
	mpi/motifs/emberbarrier.cc \
	mpi/motifs/emberbarrier.h \
	mpi/motifs/emberallpingpong.cc \
	mpi/motifs/emberallpingpong.h \
	mpi/motifs/emberallreduce.cc \
	mpi/motifs/emberallreduce.h \
	mpi/motifs/emberreduce.cc \
	mpi/motifs/emberreduce.h \
	mpi/motifs/emberhalo3d.h \
	mpi/motifs/emberhalo3d.cc \
	mpi/motifs/emberhalo3dsv.h \
	mpi/motifs/emberhalo3dsv.cc \
	mpi/motifs/emberhalo3d26.h \
	mpi/motifs/emberhalo3d26.cc \
	mpi/motifs/embersweep3d.h \
	mpi/motifs/embersweep3d.cc \
	mpi/motifs/embernaslu.h \
	mpi/motifs/embernaslu.cc \
	mpi/motifs/embermsgrate.h \
	mpi/motifs/embermsgrate.cc \
	mpi/motifs/embercomm.h \
	mpi/motifs/embercomm.cc \
	mpi/motifs/ember3damr.cc \
	mpi/motifs/ember3damr.h \
	shmem/emberShmemGen.cc \
	shmem/emberShmemGen.h

bin_PROGRAMS = sstspygen

sstspygen_SOURCES = tools/spygen/spygen.cc

libember_la_LDFLAGS = -module -avoid-version

EXTRA_DIST = \
	test/emberLoad.py \
	test/loadInfo.py \
	test/CrossProduct.py \
	test/networkConfig.py \
	test/Tester.py 

if USE_EMBER_CONTEXTS
libember_la_SOURCES += \
	embermpicxtgen.h \
	embermpicxtgen.cc
endif
