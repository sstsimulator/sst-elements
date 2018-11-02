
AM_CPPFLAGS = \
	$(MPI_CPPFLAGS) \
	-I$(top_srcdir)/src/sst/elements/ember/sirius/include \
	-I$(top_srcdir)/src \
	$(OTF2_CPPFLAGS)

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
	emberdetailedcomputeev.h \
	embermotiflog.h \
	embermotiflog.cc \
	embermemoryev.h \
	libs/emberLib.h \
	libs/misc.h \
	libs/miscEvents/emberGetNodeNumEvent.h \
	libs/miscEvents/emberGetNumNodesEvent.h \
	libs/miscEvents/emberMiscEvent.h \
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
	mpi/embermakeprogressev.h \
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
	mpi/motifs/emberdetailedring.h  \
	mpi/motifs/emberdetailedring.cc  \
	mpi/motifs/emberdetailedstream.h  \
	mpi/motifs/emberdetailedstream.cc  \
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
	mpi/motifs/emberlqcd.h \
	mpi/motifs/emberlqcd.cc \
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
	shmem/emberShmemAddEv.h \
	shmem/emberShmemAlltoallEv.h \
	shmem/emberShmemAlltoallsEv.h \
	shmem/emberShmemBarrierAllEv.h \
	shmem/emberShmemBarrierEv.h \
	shmem/emberShmemBroadcastEv.h \
	shmem/emberShmemCollectEv.h \
	shmem/emberShmemCswapEv.h \
	shmem/emberShmemEvent.h \
	shmem/emberShmemFaddEv.h \
	shmem/emberShmemFcollectEv.h \
	shmem/emberShmemFenceEv.h \
	shmem/emberShmemFiniEv.h \
	shmem/emberShmemFreeEv.h \
	shmem/emberShmemGen.cc \
	shmem/emberShmemGen.h \
	shmem/emberShmemGetEv.h \
	shmem/emberShmemGetVEv.h \
	shmem/emberShmemInitEv.h \
	shmem/emberShmemMallocEv.h \
	shmem/emberShmemMyPeEv.h \
	shmem/emberShmemNPesEv.h \
	shmem/emberShmemPutEv.h \
	shmem/emberShmemPutVEv.h \
	shmem/emberShmemQuietEv.h \
	shmem/emberShmemReductionEv.h \
	shmem/emberShmemSwapEv.h \
	shmem/emberShmemWaitEv.h \
	shmem/motifs/emberShmemAdd.h \
	shmem/motifs/emberShmemAlltoall.h \
	shmem/motifs/emberShmemAlltoalls.h \
	shmem/motifs/emberShmemBarrier.h \
	shmem/motifs/emberShmemBarrierAll.h \
	shmem/motifs/emberShmemBroadcast.h \
	shmem/motifs/emberShmemCollect.h \
	shmem/motifs/emberShmemCswap.h \
	shmem/motifs/emberShmemFadd.h \
	shmem/motifs/emberShmemFcollect.h \
	shmem/motifs/emberShmemGet.h \
	shmem/motifs/emberShmemGetNBI.h \
	shmem/motifs/emberShmemGetv.h \
	shmem/motifs/emberShmemPut.h \
	shmem/motifs/emberShmemPutv.h \
	shmem/motifs/emberShmemReduction.h \
	shmem/motifs/emberShmemRing.h \
	shmem/motifs/emberShmemRing2.h \
	shmem/motifs/emberShmemSwap.h \
	shmem/motifs/emberShmemTest.h \
	shmem/motifs/emberShmemWait.h \
	shmem/motifs/emberShmemWaitUntil.h \
	shmem/motifs/emberShmemAtomicInc.h \
	shmem/motifs/emberShmemFAM_Get.h \
	shmem/motifs/emberShmemFAM_Get2.h \
	mpi/motifs/emberunstructured.h \
	mpi/motifs/emberunstructured.cc

bin_PROGRAMS = sst-spygen sst-meshconvert

sst_spygen_SOURCES = tools/spygen/spygen.cc
sst_meshconvert_SOURCES = tools/meshconverter/meshconverter.cc

libember_la_LDFLAGS = -module -avoid-version

EXTRA_DIST = \
	test/emberLoad.py \
	test/exaParams.py \
	test/loadInfo.py \
	test/EmberEP.py \
	test/loadFileParse.py \
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

if EMBER_HAVE_OTF2
libember_la_SOURCES += \
	mpi/motifs/emberotf2.h \
	mpi/motifs/emberotf2.cc

libember_la_LIBADD = \
	$(OTF2_LDFLAGS) \
	$(OTF2_LIBS)

endif

