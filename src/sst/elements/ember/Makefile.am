
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
	libs/miscEvents/emberMallocEvent.h \
	libs/miscEvents/emberMiscEvent.h \
	libs/emberMpiLib.h \
	libs/emberMpiLib.cc \
	libs/mpi/emberscatterev.h \
	libs/mpi/emberscattervev.h \
	libs/mpi/embercancelev.h \
	libs/mpi/emberrecvev.h \
	libs/mpi/embersendev.h \
	libs/mpi/emberfinalizeev.h \
	libs/mpi/emberinitev.h \
	libs/mpi/embertestanyev.h \
	libs/mpi/embertestev.h \
	libs/mpi/emberwaitev.h \
	libs/mpi/emberwaitanyev.h \
	libs/mpi/emberwaitallev.h \
	libs/mpi/emberirecvev.h \
	libs/mpi/emberisendev.h \
	libs/mpi/emberbarrierev.h \
	libs/mpi/emberallredev.h \
	libs/mpi/emberalltoallvev.h \
	libs/mpi/emberalltoallev.h \
	libs/mpi/emberallgathervev.h \
	libs/mpi/emberallgatherev.h \
	libs/mpi/emberredev.h \
	libs/mpi/emberbcastev.h \
	libs/mpi/emberMPIEvent.h \
	libs/mpi/emberrankev.h \
	libs/mpi/embersizeev.h \
	libs/mpi/embermakeprogressev.h \
	libs/mpi/emberCommCreateEv.h \
	libs/mpi/emberCommDestroyEv.h \
	libs/mpi/emberCommSplitEv.h \
	mpi/embermpigen.h \
	mpi/embermpigen.cc \
	mpi/motifs/emberinit.h  \
	mpi/motifs/emberinit.cc \
	mpi/motifs/emberfini.h  \
	mpi/motifs/emberfini.cc \
	mpi/motifs/ember3dcommdbl.h \
	mpi/motifs/ember3dcommdbl.cc \
	mpi/motifs/embersendrecv.h  \
	mpi/motifs/embersendrecv.cc  \
	mpi/motifs/emberwaitany.h  \
	mpi/motifs/emberwaitany.cc  \
	mpi/motifs/embertest.h  \
	mpi/motifs/embertest.cc  \
	mpi/motifs/embertestany.h  \
	mpi/motifs/embertestany.cc  \
	mpi/motifs/embernull.h  \
	mpi/motifs/embernull.cc  \
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
	mpi/motifs/emberallgatherv.cc \
	mpi/motifs/emberallgatherv.h \
	mpi/motifs/emberallgather.cc \
	mpi/motifs/emberallgather.h \
	mpi/motifs/emberalltoall.cc \
	mpi/motifs/emberalltoall.h \
	mpi/motifs/emberalltoallv.cc \
	mpi/motifs/emberalltoallv.h \
	mpi/motifs/emberreduce.cc \
	mpi/motifs/emberbcast.h \
	mpi/motifs/emberbcast.cc \
	mpi/motifs/emberscatter.h \
	mpi/motifs/emberscatter.cc \
	mpi/motifs/emberscatterv.h \
	mpi/motifs/emberscatterv.cc \
	mpi/motifs/emberreduce.h \
	mpi/motifs/emberhalo3d.h \
	mpi/motifs/emberhalo3d.cc \
	mpi/motifs/emberlqcd.h \
	mpi/motifs/emberlqcd.cc \
	mpi/motifs/emberhalo3dsv.h \
	mpi/motifs/emberhalo3dsv.cc \
	mpi/motifs/emberhalo3d26.h \
	mpi/motifs/emberhalo3d26.cc \
	mpi/motifs/emberincast.h \
	mpi/motifs/emberincast.cc \
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
	mpi/motifs/emberunstructured.h \
	mpi/motifs/emberunstructured.cc \
	mpi/motifs/emberNtoM.h \
	mpi/motifs/emberNtoM.cc \
	libs/emberShmemLib.h \
	libs/shmem/emberFamEvent.h \
	libs/shmem/emberFamAddEv.h \
	libs/shmem/emberFamCswapEv.h \
	libs/shmem/emberFamPut_Ev.h \
	libs/shmem/emberFamGet_Ev.h \
	libs/shmem/emberFamScatterv_Ev.h \
	libs/shmem/emberFamScatter_Ev.h \
	libs/shmem/emberFamGatherv_Ev.h \
	libs/shmem/emberFamGather_Ev.h \
	libs/shmem/emberShmemAddEv.h \
	libs/shmem/emberShmemAlltoallEv.h \
	libs/shmem/emberShmemAlltoallsEv.h \
	libs/shmem/emberShmemBarrierAllEv.h \
	libs/shmem/emberShmemBarrierEv.h \
	libs/shmem/emberShmemBroadcastEv.h \
	libs/shmem/emberShmemCollectEv.h \
	libs/shmem/emberShmemCswapEv.h \
	libs/shmem/emberShmemEvent.h \
	libs/shmem/emberShmemFaddEv.h \
	libs/shmem/emberShmemFcollectEv.h \
	libs/shmem/emberShmemFenceEv.h \
	libs/shmem/emberShmemFiniEv.h \
	libs/shmem/emberShmemFreeEv.h \
	libs/shmem/emberShmemGetEv.h \
	libs/shmem/emberShmemGetVEv.h \
	libs/shmem/emberShmemInitEv.h \
	libs/shmem/emberShmemMallocEv.h \
	libs/shmem/emberShmemMyPeEv.h \
	libs/shmem/emberShmemNPesEv.h \
	libs/shmem/emberShmemPutEv.h \
	libs/shmem/emberShmemPutVEv.h \
	libs/shmem/emberShmemQuietEv.h \
	libs/shmem/emberShmemReductionEv.h \
	libs/shmem/emberShmemSwapEv.h \
	libs/shmem/emberShmemWaitEv.h \
	shmem/emberShmemGen.cc \
	shmem/emberShmemGen.h \
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
	shmem/motifs/emberShmemAtomicIncV2.h \
	shmem/motifs/emberShmemFAM_Get2.h \
	shmem/motifs/emberShmemFAM_Put.h \
	shmem/motifs/emberShmemFAM_Scatterv.h \
	shmem/motifs/emberShmemFAM_Gatherv.h \
	shmem/motifs/emberShmemFAM_AtomicInc.h \
	shmem/motifs/emberShmemFAM_Cswap.h \
	sirius/include/sirius/siriusglobals.h \
	pyember.py


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
	test/statModule.py \
	test/generateNidListInterval.py \
	test/generateNidListRange.py \
	test/generateNidListRandom.py \
	test/generateNidListGroup.py \
	test/loadFAM \
	test/loadFAMvars \
	test/loadFAM200 \
	test/loadFAM200rand \
	test/loadFAM2048 \
	test/loadFileParse.py \
	test/loadUtils.py \
	test/paramUtils.py \
	test/Tester.py \
	test/defaultSim.py \
	test/defaultParams.py \
	test/chamaOpenIBParams.py \
	test/chamaPSMParams.py \
	test/bgqParams.py \
	test/runFAMloadfile \
	test/generateNidListQOS.py \
	test/generateNidListRange.py \
	test/generateNidListGroup.py \
	test/generateNidListInterval.py \
	test/generateNidListRandom.py \
	tests/testsuite_default_ember_nightly.py \
	tests/testsuite_default_ember_sweep.py \
	tests/testsuite_default_ember_qos.py \
	tests/testsuite_default_ember_ESshmem.py \
	tests/ESshmem_List-of-Tests \
	tests/qos-dragonfly.sh \
	tests/qos-fattree.sh \
	tests/qos-hyperx.sh \
	tests/qos.load \
    tests/refFiles/ESshmem_cumulative.out \
    tests/refFiles/test_EmberSweep.out \
    tests/refFiles/test_embernightly.out \
    tests/refFiles/test_qos-dragonfly.out \
    tests/refFiles/test_qos-fattree.out \
    tests/refFiles/test_qos-hyperx.out

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

install-exec-hook:
	$(SST_REGISTER_TOOL) SST_ELEMENT_SOURCE     ember=$(abs_srcdir)
	$(SST_REGISTER_TOOL) SST_ELEMENT_TESTS      ember=$(abs_srcdir)/tests

BUILT_SOURCES = \
	pyember.inc

# This sed script converts 'od' output to a comma-separated list of byte-
# values, suitable for #include'ing into an array definition.
# This can be done much more simply with xxd or hexdump, but those tools
# are not installed by default on all supported platforms.
#
# od:	-v:		Print all data
#		-t x1:	Print as byte-values, in hex
# sed:	Script 1:  Remove base-address column from od output
# 		Script 2:  Remove trailing blank line resulting from script 1
# 		Script 3:  Add '0x' prefix, and ',' suffix to each value
%.inc: %.py
	od -v -t x1 < $< | sed -e 's/^[^ ]*[ ]*//g' -e '/^\s*$$/d' -e 's/\([0-9a-f]*\)[ $$]*/0x\1,/g' > $@

