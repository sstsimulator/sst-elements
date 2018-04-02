// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include "sst_config.h"

#include <assert.h>

#include "emberengine.h"

#define SST_ELI_COMPILE_OLD_ELI_WITHOUT_DEPRECATION_WARNINGS

#include "sst/core/element.h"
#include "sst/core/params.h"
#include "sst/core/subcomponent.h"

#include "embermap.h"
#include "emberlinearmap.h"
#include "embercustommap.h" //NetworkSim: added custom rank map
#include "mpi/motifs/emberhalo2d.h"
#include "mpi/motifs/emberhalo2dNBR.h"
#include "mpi/motifs/emberhalo3d.h"
#include "mpi/motifs/emberhalo3dsv.h"
#include "mpi/motifs/embersweep2d.h"
#include "mpi/motifs/embersweep3d.h"
#include "mpi/motifs/embernaslu.h"
#include "mpi/motifs/emberpingpong.h"
#include "mpi/motifs/emberbipingpong.h"
#include "mpi/motifs/emberTrafficGen.h"
#include "mpi/motifs/emberring.h"
#include "mpi/motifs/emberdetailedring.h"
#include "mpi/motifs/emberdetailedstream.h"
#include "mpi/motifs/emberinit.h"
#include "mpi/motifs/emberfini.h"
#include "mpi/motifs/emberbarrier.h"
#include "mpi/motifs/emberallreduce.h"
#include "mpi/motifs/emberalltoall.h"
#include "mpi/motifs/emberalltoallv.h"
#include "mpi/motifs/emberreduce.h"
#include "mpi/motifs/emberbcast.h"
#include "mpi/motifs/emberallpingpong.h"
#include "mpi/motifs/embernull.h"
#include "mpi/motifs/embermsgrate.h"
#include "mpi/motifs/emberhalo3d26.h"
#include "mpi/motifs/ember3dcommdbl.h"
#include "mpi/motifs/embercomm.h"
#include "mpi/motifs/ember3damr.h"
#include "mpi/motifs/emberfft3d.h"
#include "mpi/motifs/embercmt1d.h"
#include "mpi/motifs/embercmt2d.h"
#include "mpi/motifs/embercmt3d.h"
#include "mpi/motifs/embercmtcr.h"
#include "mpi/motifs/emberstop.h"
#include "mpi/motifs/emberunstructured.h" //NetworkSim: added unstructured communication motif
#include "mpi/motifs/embersiriustrace.h"
#include "mpi/motifs/emberrandomgen.h"

#include "shmem/motifs/emberShmemTest.h"
#include "shmem/motifs/emberShmemWait.h"
#include "shmem/motifs/emberShmemWaitUntil.h"
#include "shmem/motifs/emberShmemPut.h"
#include "shmem/motifs/emberShmemGet.h"
#include "shmem/motifs/emberShmemPutv.h"
#include "shmem/motifs/emberShmemGetv.h"
#include "shmem/motifs/emberShmemFadd.h"
#include "shmem/motifs/emberShmemAdd.h"
#include "shmem/motifs/emberShmemCswap.h"
#include "shmem/motifs/emberShmemSwap.h"
#include "shmem/motifs/emberShmemBarrierAll.h"
#include "shmem/motifs/emberShmemBarrier.h"
#include "shmem/motifs/emberShmemBroadcast.h"
#include "shmem/motifs/emberShmemCollect.h"
#include "shmem/motifs/emberShmemFcollect.h"
#include "shmem/motifs/emberShmemAlltoall.h"
#include "shmem/motifs/emberShmemAlltoalls.h"
#include "shmem/motifs/emberShmemReduction.h"
#include "shmem/motifs/emberShmemRing.h"
#include "shmem/motifs/emberShmemRing2.h"
#include "shmem/motifs/emberShmemAtomicInc.h"

#include "emberconstdistrib.h"
#include "embergaussdistrib.h"

using namespace SST;
using namespace SST::Ember;

static Component*
create_EmberComponent(SST::ComponentId_t id,
                  SST::Params& params)
{
    return new EmberEngine( id, params );
}

static SubComponent*
load_RandomGen( Component* comp, Params& params ) {
	return new EmberRandomTrafficGenerator(comp, params);
}

static SubComponent*
load_SIRIUSTrace( Component* comp, Params& params ) {
	return new EmberSIRIUSTraceGenerator(comp, params);
}

static SubComponent*
load_PingPong( Component* comp, Params& params ) {
	return new EmberPingPongGenerator(comp, params);
}

static SubComponent*
load_BiPingPong( Component* comp, Params& params ) {
	return new EmberBiPingPongGenerator(comp, params);
}

static SubComponent*
load_3DAMR( Component* comp, Params& params) {
	return new Ember3DAMRGenerator(comp, params);
}

static Module*
load_ConstDistrib( Component* comp, Params& params) {
	return new EmberConstDistribution(comp, params);
}

static Module*
load_GaussDistrib( Component* comp, Params& params) {
	return new EmberGaussianDistribution(comp, params);
}

static SubComponent*
load_AllPingPong( Component* comp, Params& params ) {
	return new EmberAllPingPongGenerator(comp, params);
}

static SubComponent*
load_Barrier( Component* comp, Params& params ) {
	return new EmberBarrierGenerator(comp, params);
}

static SubComponent*
load_Ring( Component* comp, Params& params ) {
	return new EmberRingGenerator(comp, params);
}

static SubComponent*
load_DetailedRing( Component* comp, Params& params ) {
	return new EmberDetailedRingGenerator(comp, params);
}

static SubComponent*
load_DetailedStream( Component* comp, Params& params ) {
	return new EmberDetailedStreamGenerator(comp, params);
}

static Module*
load_LinearNodeMap( Component* comp, Params& params ) {
	return new EmberLinearRankMap(comp, params);
}

//NetworkSim: added custom map loader
static Module*
load_CustomNodeMap( Component* comp, Params& params ) {
	return new EmberCustomRankMap(comp, params);
}
//end->NetworkSim

static SubComponent*
load_CommDoubling( Component* comp, Params& params ) {
	return new Ember3DCommDoublingGenerator(comp, params);
}

static SubComponent*
load_Init( Component* comp, Params& params ) {
	return new EmberInitGenerator(comp, params);
}

static SubComponent*
load_Fini( Component* comp, Params& params ) {
	return new EmberFiniGenerator(comp, params);
}

static SubComponent*
load_Halo2D( Component* comp, Params& params ) {
	return new EmberHalo2DGenerator(comp, params);
}

static SubComponent*
load_Halo2DNBR( Component* comp, Params& params ) {
	return new EmberHalo2DNBRGenerator(comp, params);
}

static SubComponent*
load_Halo3D26( Component* comp, Params& params ) {
	return new EmberHalo3D26Generator(comp, params);
}

static SubComponent*
load_Halo3DSV( Component* comp, Params& params ) {
        return new EmberHalo3DSVGenerator(comp, params);
}

static SubComponent*
load_Halo3D( Component* comp, Params& params ) {
	return new EmberHalo3DGenerator(comp, params);
}

static SubComponent*
load_Sweep2D( Component* comp, Params& params ) {
	return new EmberSweep2DGenerator(comp, params);
}

static SubComponent*
load_Sweep3D( Component* comp, Params& params ) {
	return new EmberSweep3DGenerator(comp, params);
}

static SubComponent*
load_NASLU( Component* comp, Params& params ) {
	return new EmberNASLUGenerator(comp, params);
}

static SubComponent*
load_Allreduce( Component* comp, Params& params ) {
	return new EmberAllreduceGenerator(comp, params);
}

static SubComponent*
load_Alltoall( Component* comp, Params& params ) {
	return new EmberAlltoallGenerator(comp, params);
}

static SubComponent*
load_Alltoallv( Component* comp, Params& params ) {
	return new EmberAlltoallvGenerator(comp, params);
}

static SubComponent*
load_Reduce( Component* comp, Params& params ) {
	return new EmberReduceGenerator(comp, params);
}

static SubComponent*
load_Bcast( Component* comp, Params& params ) {
	return new EmberBcastGenerator(comp, params);
}

static SubComponent*
load_Null( Component* comp, Params& params ) {
	return new EmberNullGenerator(comp, params);
}

static SubComponent*
load_MsgRate( Component* comp, Params& params ) {
	return new EmberMsgRateGenerator(comp, params);
}

static SubComponent*
load_Comm( Component* comp, Params& params ) {
	return new EmberCommGenerator(comp, params);
}

static SubComponent*
load_FFT3D( Component* comp, Params& params ) {
	return new EmberFFT3DGenerator(comp, params);
}

static SubComponent*
load_CMT1D( Component* comp, Params& params ) {
	return new EmberCMT1DGenerator(comp, params);
}

static SubComponent*
load_CMT2D( Component* comp, Params& params ) {
	return new EmberCMT2DGenerator(comp, params);
}

static SubComponent*
load_CMT3D( Component* comp, Params& params ) {
	return new EmberCMT3DGenerator(comp, params);
}

static SubComponent*
load_CMTCR( Component* comp, Params& params ) {
	return new EmberCMTCRGenerator(comp, params);
}

static SubComponent*
load_TrafficGen( Component* comp, Params& params ) {
	return new EmberTrafficGenGenerator(comp, params);
}

static SubComponent*
load_ShmemTest( Component* comp, Params& params ) {
	return new EmberShmemTestGenerator(comp, params);
}

static SubComponent*
load_ShmemPutInt( Component* comp, Params& params ) {
	return new EmberShmemPutGenerator<int>(comp, params);
}

static SubComponent*
load_ShmemPutLong( Component* comp, Params& params ) {
	return new EmberShmemPutGenerator<long>(comp, params);
}

static SubComponent*
load_ShmemPutFloat( Component* comp, Params& params ) {
	return new EmberShmemPutGenerator<float>(comp, params);
}

static SubComponent*
load_ShmemPutDouble( Component* comp, Params& params ) {
	return new EmberShmemPutGenerator<double>(comp, params);
}

static SubComponent*
load_ShmemPutvInt( Component* comp, Params& params ) {
	return new EmberShmemPutvGenerator<int>(comp, params);
}

static SubComponent*
load_ShmemPutvLong( Component* comp, Params& params ) {
	return new EmberShmemPutvGenerator<long>(comp, params);
}

static SubComponent*
load_ShmemPutvFloat( Component* comp, Params& params ) {
	return new EmberShmemPutvGenerator<float>(comp, params);
}

static SubComponent*
load_ShmemPutvDouble( Component* comp, Params& params ) {
	return new EmberShmemPutvGenerator<double>(comp, params);
}

static SubComponent*
load_ShmemGetInt( Component* comp, Params& params ) {
	return new EmberShmemGetGenerator<int>(comp, params);
}

static SubComponent*
load_ShmemGetLong( Component* comp, Params& params ) {
	return new EmberShmemGetGenerator<long>(comp, params);
}

static SubComponent*
load_ShmemGetFloat( Component* comp, Params& params ) {
	return new EmberShmemGetGenerator<float>(comp, params);
}

static SubComponent*
load_ShmemGetDouble( Component* comp, Params& params ) {
	return new EmberShmemGetGenerator<double>(comp, params);
}

static SubComponent*
load_ShmemGetvInt( Component* comp, Params& params ) {
	return new EmberShmemGetvGenerator<int>(comp, params);
}

static SubComponent*
load_ShmemGetvLong( Component* comp, Params& params ) {
	return new EmberShmemGetvGenerator<long>(comp, params);
}

static SubComponent*
load_ShmemGetvFloat( Component* comp, Params& params ) {
	return new EmberShmemGetvGenerator<float>(comp, params);
}

static SubComponent*
load_ShmemGetvDouble( Component* comp, Params& params ) {
	return new EmberShmemGetvGenerator<double>(comp, params);
}

static SubComponent*
load_ShmemAddInt( Component* comp, Params& params ) {
	return new EmberShmemAddGenerator<int>(comp, params);
}

static SubComponent*
load_ShmemAddLong( Component* comp, Params& params ) {
	return new EmberShmemAddGenerator<long>(comp, params);
}

static SubComponent*
load_ShmemAddFloat( Component* comp, Params& params ) {
	return new EmberShmemAddGenerator<float>(comp, params);
}

static SubComponent*
load_ShmemAddDouble( Component* comp, Params& params ) {
	return new EmberShmemAddGenerator<double>(comp, params);
}

static SubComponent*
load_ShmemFaddInt( Component* comp, Params& params ) {
	return new EmberShmemFaddGenerator<int>(comp, params);
}

static SubComponent*
load_ShmemFaddLong( Component* comp, Params& params ) {
	return new EmberShmemFaddGenerator<long>(comp, params);
}

static SubComponent*
load_ShmemFaddFloat( Component* comp, Params& params ) {
	return new EmberShmemFaddGenerator<float>(comp, params);
}

static SubComponent*
load_ShmemFaddDouble( Component* comp, Params& params ) {
	return new EmberShmemFaddGenerator<double>(comp, params);
}

static SubComponent*
load_ShmemCswapInt( Component* comp, Params& params ) {
	return new EmberShmemCswapGenerator<int>(comp, params);
}

static SubComponent*
load_ShmemCswapLong( Component* comp, Params& params ) {
	return new EmberShmemCswapGenerator<long>(comp, params);
}

static SubComponent*
load_ShmemCswapFloat( Component* comp, Params& params ) {
	return new EmberShmemCswapGenerator<float>(comp, params);
}

static SubComponent*
load_ShmemCswapDouble( Component* comp, Params& params ) {
	return new EmberShmemCswapGenerator<double>(comp, params);
}

static SubComponent*
load_ShmemSwapInt( Component* comp, Params& params ) {
	return new EmberShmemSwapGenerator<int>(comp, params);
}

static SubComponent*
load_ShmemSwapLong( Component* comp, Params& params ) {
	return new EmberShmemSwapGenerator<long>(comp, params);
}

static SubComponent*
load_ShmemWaitInt( Component* comp, Params& params ) {
	return new EmberShmemWaitGenerator<int>(comp, params);
}

static SubComponent*
load_ShmemWaitLong( Component* comp, Params& params ) {
	return new EmberShmemWaitGenerator<long>(comp, params);
}

static SubComponent*
load_ShmemWaitUntilInt( Component* comp, Params& params ) {
	return new EmberShmemWaitUntilGenerator<int>(comp, params);
}

static SubComponent*
load_ShmemWaitUntilLong( Component* comp, Params& params ) {
	return new EmberShmemWaitUntilGenerator<long>(comp, params);
}

static SubComponent*
load_ShmemBarrierAll( Component* comp, Params& params ) {
	return new EmberShmemBarrierAllGenerator(comp, params);
}

static SubComponent*
load_ShmemBarrier( Component* comp, Params& params ) {
	return new EmberShmemBarrierGenerator(comp, params);
}

static SubComponent*
load_ShmemBroadcast32( Component* comp, Params& params ) {
	return new EmberShmemBroadcastGenerator<uint32_t>(comp, params);
}

static SubComponent*
load_ShmemBroadcast64( Component* comp, Params& params ) {
	return new EmberShmemBroadcastGenerator<uint64_t>(comp, params);
}

static SubComponent*
load_ShmemFcollect32( Component* comp, Params& params ) {
	return new EmberShmemFcollectGenerator<uint32_t>(comp, params);
}

static SubComponent*
load_ShmemFcollect64( Component* comp, Params& params ) {
	return new EmberShmemFcollectGenerator<uint64_t>(comp, params);
}

static SubComponent*
load_ShmemCollect32( Component* comp, Params& params ) {
	return new EmberShmemCollectGenerator<uint32_t>(comp, params);
}

static SubComponent*
load_ShmemCollect64( Component* comp, Params& params ) {
	return new EmberShmemCollectGenerator<uint64_t>(comp, params);
}

static SubComponent*
load_ShmemAlltoall32( Component* comp, Params& params ) {
	return new EmberShmemAlltoallGenerator<uint32_t>(comp, params);
}

static SubComponent*
load_ShmemAlltoall64( Component* comp, Params& params ) {
	return new EmberShmemAlltoallGenerator<uint64_t>(comp, params);
}

static SubComponent*
load_ShmemAlltoalls32( Component* comp, Params& params ) {
	return new EmberShmemAlltoallsGenerator<uint32_t>(comp, params);
}

static SubComponent*
load_ShmemAlltoalls64( Component* comp, Params& params ) {
	return new EmberShmemAlltoallsGenerator<uint64_t>(comp, params);
}

static SubComponent*
load_ShmemReductionFloat( Component* comp, Params& params ) {
	return new EmberShmemReductionGenerator<float>(comp, params);
}

static SubComponent*
load_ShmemReductionDouble( Component* comp, Params& params ) {
	return new EmberShmemReductionGenerator<double>(comp, params);
}

static SubComponent*
load_ShmemReductionInt( Component* comp, Params& params ) {
	return new EmberShmemReductionGenerator<int>(comp, params);
}

static SubComponent*
load_ShmemReductionLong( Component* comp, Params& params ) {
	return new EmberShmemReductionGenerator<long>(comp, params);
}

static SubComponent*
load_ShmemReductionLongLong( Component* comp, Params& params ) {
	return new EmberShmemReductionGenerator<long long>(comp, params);
}

static SubComponent*
load_ShmemRing( Component* comp, Params& params ) {
	return new EmberShmemRingGenerator<int>(comp, params);
}

static SubComponent*
load_ShmemRing2( Component* comp, Params& params ) {
	return new EmberShmemRing2Generator<int>(comp, params);
}

static SubComponent*
load_ShmemAtomicInc( Component* comp, Params& params ) {
	return new EmberShmemAtomicIncGenerator(comp, params);
}


//NetworkSim: loader for the stop motif
static SubComponent*
load_Stop( Component* comp, Params& params ) {
	return new EmberStopGenerator(comp, params);
}
//end->NetworkSim

//NetworkSim: loader for the unstructured communication motif
static SubComponent*
load_Unstructured( Component* comp, Params& params ) {
	return new EmberUnstructuredGenerator(comp, params);
}
//end->NetworkSim

static const ElementInfoParam component_params[] = {
};

static const ElementInfoPort component_ports[] = {
};


static const ElementInfoParam shmemTest_params[] = {
	{	NULL,	NULL,	NULL	}
};

//end->NetworkSim

static const ElementInfoStatistic emberMotifTime_statistics[] = {
    { "time-Init", "Time spent in Init event",          "ns",  0},
    { "time-Finalize", "Time spent in Finalize event",  "ns", 0},
    { "time-Rank", "Time spent in Rank event",          "ns", 0},
    { "time-Size", "Time spent in Size event",          "ns", 0},
    { "time-Send", "Time spent in Recv event",          "ns", 0},
    { "time-Recv", "Time spent in Recv event",          "ns", 0},
    { "time-Irecv", "Time spent in Irecv event",        "ns", 0},
    { "time-Isend", "Time spent in Isend event",        "ns", 0},
    { "time-Wait", "Time spent in Wait event",          "ns", 0},
    { "time-Waitall", "Time spent in Waitall event",    "ns", 0},
    { "time-Waitany", "Time spent in Waitany event",    "ns", 0},
    { "time-Compute", "Time spent in Compute event",    "ns", 0},
    { "time-Barrier", "Time spent in Barrier event",    "ns", 0},
    { "time-Alltoallv", "Time spent in Alltoallv event", "ns", 0},
    { "time-Alltoall", "Time spent in Alltoall event",  "ns", 0},
    { "time-Allreduce", "Time spent in Allreduce event", "ns", 0},
    { "time-Reduce", "Time spent in Reduce event",      "ns", 0},
    { "time-Bcast", "Time spent in Bcast event",        "ns", 0},
    { "time-Gettime", "Time spent in Gettime event",    "ns", 0},
    { "time-Commsplit", "Time spent in Commsplit event", "ns", 0},
    { "time-Commcreate", "Time spent in Commcreate event", "ns", 0},
    { NULL, NULL, NULL, 0 }
};

static const ElementInfoSubComponent subcomponents[] = {

    { 	"ShmemPutIntMotif",
	"SHMEM put int",
	NULL,
	load_ShmemPutInt,
    shmemTest_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	"ShmemPutLongMotif",
	"SHMEM put long",
	NULL,
	load_ShmemPutLong,
    shmemTest_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	"ShmemPutFloatMotif",
	"SHMEM put float",
	NULL,
	load_ShmemPutFloat,
    shmemTest_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	"ShmemPutDoubleMotif",
	"SHMEM put double",
	NULL,
	load_ShmemPutDouble,
    shmemTest_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	"ShmemPutvIntMotif",
	"SHMEM putv int",
	NULL,
	load_ShmemPutvInt,
    shmemTest_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	"ShmemPutvLongMotif",
	"SHMEM putv long",
	NULL,
	load_ShmemPutvLong,
    shmemTest_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	"ShmemPutvFloatMotif",
	"SHMEM putv float",
	NULL,
	load_ShmemPutvFloat,
    shmemTest_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	"ShmemPutvDoubleMotif",
	"SHMEM putv",
	NULL,
	load_ShmemPutvDouble,
    shmemTest_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	"ShmemGetIntMotif",
	"SHMEM get int",
	NULL,
	load_ShmemGetInt,
    shmemTest_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	"ShmemGetLongMotif",
	"SHMEM get long",
	NULL,
	load_ShmemGetLong,
    shmemTest_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	"ShmemGetFloatMotif",
	"SHMEM get float",
	NULL,
	load_ShmemGetFloat,
    shmemTest_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	"ShmemGetDoubleMotif",
	"SHMEM get double",
	NULL,
	load_ShmemGetDouble,
    shmemTest_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	"ShmemGetvIntMotif",
	"SHMEM getv int",
	NULL,
	load_ShmemGetvInt,
    shmemTest_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	"ShmemGetvLongMotif",
	"SHMEM getv long",
	NULL,
	load_ShmemGetvLong,
    shmemTest_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	"ShmemGetvFloatMotif",
	"SHMEM getv float",
	NULL,
	load_ShmemGetvFloat,
    shmemTest_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	"ShmemGetvDoubleMotif",
	"SHMEM getv double",
	NULL,
	load_ShmemGetvDouble,
    shmemTest_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	"ShmemWaitIntMotif",
	"SHMEM wait int test",
	NULL,
	load_ShmemWaitInt,
    shmemTest_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	"ShmemWaitLongMotif",
	"SHMEM wait long test",
	NULL,
	load_ShmemWaitLong,
    shmemTest_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	"ShmemWaitUntilIntMotif",
	"SHMEM wait_until int test",
	NULL,
	load_ShmemWaitUntilInt,
    shmemTest_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	"ShmemWaitUntilLongMotif",
	"SHMEM wait_until long test",
	NULL,
	load_ShmemWaitUntilLong,
    shmemTest_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	"ShmemAddIntMotif",
	"SHMEM add int",
	NULL,
	load_ShmemAddInt,
    shmemTest_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	"ShmemAddLongMotif",
	"SHMEM add long",
	NULL,
	load_ShmemAddLong,
    shmemTest_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	"ShmemAddFloatMotif",
	"SHMEM add float",
	NULL,
	load_ShmemAddFloat,
    shmemTest_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	"ShmemAddDoubleMotif",
	"SHMEM add double",
	NULL,
	load_ShmemAddDouble,
    shmemTest_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	"ShmemFaddIntMotif",
	"SHMEM add int",
	NULL,
	load_ShmemFaddInt,
    shmemTest_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	"ShmemFaddLongMotif",
	"SHMEM fadd long",
	NULL,
	load_ShmemFaddLong,
    shmemTest_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	"ShmemFaddFloatMotif",
	"SHMEM fadd float",
	NULL,
	load_ShmemFaddFloat,
    shmemTest_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	"ShmemFaddDoubleMotif",
	"SHMEM fadd double",
	NULL,
	load_ShmemFaddDouble,
    shmemTest_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	"ShmemCswapIntMotif",
	"SHMEM cswap int",
	NULL,
	load_ShmemCswapInt,
    shmemTest_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	"ShmemCswapLongMotif",
	"SHMEM cswap ong",
	NULL,
	load_ShmemCswapLong,
    shmemTest_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	"ShmemCswapFloatMotif",
	"SHMEM cswap float",
	NULL,
	load_ShmemCswapFloat,
    shmemTest_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	"ShmemCswapDoubleMotif",
	"SHMEM cswap double",
	NULL,
	load_ShmemCswapDouble,
    shmemTest_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	"ShmemSwapIntMotif",
	"SHMEM swap int",
	NULL,
	load_ShmemSwapInt,
    shmemTest_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	"ShmemSwapLongMotif",
	"SHMEM swap long",
	NULL,
	load_ShmemSwapLong,
    shmemTest_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	"ShmemBarrierAllMotif",
	"SHMEM barrier_all",
	NULL,
	load_ShmemBarrierAll,
    shmemTest_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	"ShmemBarrierMotif",
	"SHMEM barrier",
	NULL,
	load_ShmemBarrier,
    shmemTest_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	"ShmemBroadcast32Motif",
	"SHMEM broadcast32",
	NULL,
	load_ShmemBroadcast32,
    shmemTest_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	"ShmemBroadcast64Motif",
	"SHMEM broadcast64",
	NULL,
	load_ShmemBroadcast64,
    shmemTest_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	"ShmemFcollect32Motif",
	"SHMEM fcollect32",
	NULL,
	load_ShmemFcollect32,
    shmemTest_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	"ShmemFcollect64Motif",
	"SHMEM fcollect64",
	NULL,
	load_ShmemFcollect64,
    shmemTest_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	"ShmemCollect32Motif",
	"SHMEM collect32",
	NULL,
	load_ShmemCollect32,
    shmemTest_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	"ShmemCollect64Motif",
	"SHMEM collect64",
	NULL,
	load_ShmemCollect64,
    shmemTest_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	"ShmemAlltoall32Motif",
	"SHMEM alltoall32",
	NULL,
	load_ShmemAlltoall32,
    shmemTest_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	"ShmemAlltoall64Motif",
	"SHMEM alltoall64",
	NULL,
	load_ShmemAlltoall64,
    shmemTest_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	
        "ShmemAlltoalls32Motif",
	"SHMEM alltoalls32",
	NULL,
	load_ShmemAlltoalls32,
    shmemTest_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	
        "ShmemAlltoalls64Motif",
	"SHMEM alltoalls64",
	NULL,
	load_ShmemAlltoalls64,
    shmemTest_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	
        "ShmemReductionIntMotif",
	"SHMEM reduction int",
	NULL,
	load_ShmemReductionInt,
    shmemTest_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	
        "ShmemReductionLongMotif",
	"SHMEM reduction long",
	NULL,
	load_ShmemReductionLong,
    shmemTest_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	
        "ShmemReductionLongLongMotif",
	"SHMEM reduction longlong",
	NULL,
	load_ShmemReductionLongLong,
    shmemTest_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	
        "ShmemReductionFloatMotif",
	"SHMEM reduction float",
	NULL,
	load_ShmemReductionFloat,
    shmemTest_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
};
