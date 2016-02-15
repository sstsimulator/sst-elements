
AM_CPPFLAGS = \
	$(BOOST_CPPFLAGS) \
	$(MPI_CPPFLAGS) \
	-I$(top_srcdir)/sst/elements/ember/sirius/include

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
	embercustommap.h \
	emberdistrib.h \
	emberdistrib.cc \
	emberconstdistrib.h \
	emberconstdistrib.cc \
	embergaussdistrib.h \
	embergaussdistrib.cc \
	embercomputeev.h \
	embercomputeev.cc \
	embermotiflog.h \
	embermotiflog.cc \
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
	mpi/emberalltoallvev.h \
	mpi/emberalltoallev.h \
	mpi/emberredev.h \
	mpi/emberbcastev.h \
	mpi/emberMPIEvent.h \
	mpi/emberrankev.h \
	mpi/embersizeev.h \
	mpi/emberCommCreateEv.h \
	mpi/emberCommDestroyEv.h \
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
	mpi/motifs/emberbipingpong.cc \
	mpi/motifs/emberbipingpong.h \
	mpi/motifs/emberTrafficGen.cc \
	mpi/motifs/emberTrafficGen.h \
	mpi/motifs/emberbarrier.cc \
	mpi/motifs/emberbarrier.h \
	mpi/motifs/emberallpingpong.cc \
	mpi/motifs/emberallpingpong.h \
	mpi/motifs/emberallreduce.cc \
	mpi/motifs/emberallreduce.h \
	mpi/motifs/emberalltoall.cc \
	mpi/motifs/emberalltoall.h \
	mpi/motifs/emberalltoallv.cc \
	mpi/motifs/emberalltoallv.h \
	mpi/motifs/emberreduce.cc \
	mpi/motifs/emberbcast.h \
	mpi/motifs/emberbcast.cc \
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
	mpi/motifs/ember3damrbinaryfile.h \
	mpi/motifs/ember3damrtextfile.h \
	mpi/motifs/ember3damrblock.h \
	mpi/motifs/ember3damrfile.h \
	mpi/motifs/emberfft3d.h \
	mpi/motifs/emberfft3d.cc \
	mpi/motifs/embercmt1d.h \
	mpi/motifs/embercmt1d.cc \
	mpi/motifs/embercmt2d.h \
	mpi/motifs/embercmt2d.cc \
	mpi/motifs/embercmt3d.h \
	mpi/motifs/embercmt3d.cc \
	mpi/motifs/embercmtcr.h \
	mpi/motifs/embercmtcr.cc \
	mpi/motifs/emberstop.h \
	mpi/motifs/emberstop.cc \
	mpi/motifs/embersiriustrace.h \
	mpi/motifs/embersiriustrace.cc \
	mpi/motifs/emberrandomgen.h \
	mpi/motifs/emberrandomgen.cc \
	sirius/include/sirius/siriusglobals.h \
	shmem/emberShmemGen.cc \
	shmem/emberShmemGen.h

bin_PROGRAMS = sst-spygen sst-meshconvert

sst_spygen_SOURCES = tools/spygen/spygen.cc
sst_meshconvert_SOURCES = tools/meshconverter/meshconverter.cc

libember_la_LDFLAGS = -module -avoid-version

EXTRA_DIST = \
	test/emberLoad.py \
	test/exaParams.py \
	test/loadInfo.py \
	test/CrossProduct.py \
	test/networkConfig.py \
	test/Tester.py \
	test/defaultSim.py \
	test/defaultParams.py \
	test/chamaOpenIBParams.py \
	test/chamaPSMParams.py \
	test/bgqParams.py

if USE_EMBER_CONTEXTS
libember_la_SOURCES += \
	embermpicxtgen.h \
	embermpicxtgen.cc
endif
