// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include "sst_config.h"
#include "sst/core/serialization.h"

#include <assert.h>

#include "emberengine.h"

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
#include "mpi/motifs/embersiriustrace.h"
#include "mpi/motifs/emberrandomgen.h"
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

//NetworkSim: loader for the stop motif
static SubComponent*
load_Stop( Component* comp, Params& params ) {
	return new EmberStopGenerator(comp, params);
}
//end->NetworkSim

static const ElementInfoParam component_params[] = {
    { "module", "Sets the OS module", ""},
    { "verbose", "Sets the output verbosity of the component", "0" },
    { "jobId", "Sets the job id", "-1"},
    { "netMapName", "used internally", "-1"},
    { "_apiName", "used internally", "-1"},
    { "_jobId", "used internally", "-1"},
    { "_motifNum", "used internally", "-1"},

    { "motif_count", "Sets the number of motifs which will be run in this simulation, default is 1", "1"},

    { "distribModule", "Sets the distribution SST module for compute modeling, default is a constant distribution of mean 1", "1.0"},

    { "rankmapper", "Sets the rank mapping SST module to load to rank translations, default is linear mapping", "ember.LinearMap" },

    { "motif%(motif_count)d", "Sets the event generator or motif for the engine", "ember.EmberPingPongGenerator" },

    { "name", "Sets the event generator or motif for the engine", "ember.EmberPingPongGenerator" },
    { "api", "Sets the api used by a motif", "hermesParams" },

    { "spyplotmode", "Sets the spyplot generation mode, 0 = none, 1 = spy on sends", "0" },

    { "motifLog", "Sets a file path to a file where motif execution details are written, empty = no log", "" },

    { "Send_bin_width", "Bin width of the send time histogram", "5" },
    { "Compute_bin_width", "Bin width of the compute time histogram", "5" },
    { "Init_bin_width", "Bin width of the init time histogram", "5" },
    { "Finalize_bin_width", "Bin width of the finalize time histogram", "5"},
    { "Recv_bin_width", "Bin width of the recv time histogram", "5" },
    { "Rank_bin_width", "Bin width of the rank time histogram", "5" },
    { "Size_bin_width", "Bin width of the size time histogram", "5" },
    { "Recv_bin_width", "Bin width of the recv time histogram", "5" },
    { "Irecv_bin_width", "Bin width of the irecv time histogram", "5" },
    { "Isend_bin_width", "Bin width of the isend time histogram", "5" },
    { "Wait_bin_width", "Bin width of the wait time histogram", "5" },
    { "Waitall_bin_width", "Bin width of the waitall time histogram", "5" },
    { "Waitany_bin_width", "Bin width of the waitany time histogram", "5" },
    { "Barrier_bin_width", "Bin width of the barrier time histogram", "5" },
    { "Recvsize_bin_width", "Bin width of the recv sizes (bytes) histogram", "64" },
    { "Sendsize_bin_width", "Bin width of the send sizes (bytes) histogram", "64" },
    { "Allreduce_bin_width", "Bin width of the allreduce time histogram", "5" },
    { "Alltoall_bin_width", "Bin width of the alltoall time histogram", "5" },
    { "Alltoallv_bin_width", "Bin width of the alltoallv time histogram", "5" },
    { "Reduce_bin_width", "Bin width of the reduce time histogram", "5" },
    { "Bcast_bin_width", "Bin width of the bcast time histogram", "5" },
    { "Commsplit_bin_width", "Bin width of the comm_split time histogram", "5" },
    { "Commcreate_bin_width", "Bin width of the comm_create time histogram", "5" },
    { "Commdestroy_bin_width", "Bin width of the comm_destroy time histogram", "5" },
    { "Gettime_bin_width", "Bin width of the gettime time histogram", "5" },


    { "noisegen", "Sets the noise generator for the system", "constant" },
    { "noisemean", "Sets the mean of a Gaussian noise generator", "1.0" },
    { "noisestddev", "Sets the standard deviation of a noise generator", "0.1" },
    { NULL, NULL, NULL }
};

static const ElementInfoPort component_ports[] = {
    {"nic", "Port connected to the nic", NULL},
    {"loop", "Port connected to the loopBack", NULL},
    {NULL, NULL, NULL}
};

static const ElementInfoParam pingpong_params[] = {
    	{	"arg.messageSize",		"Sets the message size of the ping pong operation",	"1024"},
	{	"arg.iterations",		"Sets the number of ping pong operations to perform", 	"1"},
	{	"arg.rank2",		"Sets the 2nd rank to pingpong with (0 is the 1st)", 	"1"},
	{	"arg.blockingSend",		"Sets the send mode", 	"1"},
	{	"arg.blockingRecv",		"Sets the recv mode", 	"1"},
	{	"arg.waitall",		    "Sets the wait mode", 	"1"},
	{	NULL,	NULL,	NULL	}
};

static const ElementInfoParam bipingpong_params[] = {
    	{	"arg.messageSize",		"Sets the message size of the operation",	"1024"},
	{	"arg.iterations",		"Sets the number of operations to perform", 	"1"},
	{	NULL,	NULL,	NULL	}
};

static const ElementInfoParam randomgen_params[] = {
    	{	"arg.messagesize",		"Sets the message size of the communications (in count of DOUBLE)",	"1"},
	{	"arg.iterations",		"Sets the number of iterations to perform", 	"1"},
	{	NULL,	NULL,	NULL	}
};

static const ElementInfoParam siriustrace_params[] = {
	{       "arg.traceprefix",              "Sets the trace prefix for loading SIRIUS files", "" },
	{	NULL,	NULL,	NULL	}
};

static const ElementInfoParam amr3d_params[] = {
	{	"arg.iterations",		"Sets the number of ping pong operations to perform", 	"1"},
	{	"arg.printmap",			"Prints a map of blocks to ranks (=\"no\" or \"yes\")", "no" },
	{	"arg.blockfile",		"File containing the 3D AMR blocks (from MiniAMR)", 	"blocks.amr"},
	{       "arg.nx", 		        "Sets the size of a block in X", "8" },
        {       "arg.ny",                       "Sets the size of a block in Y", "8" },
        {       "arg.nz",                       "Sets the size of a block in Z", "8" },
        {       "arg.filetype",                 "Mesh file type, set to \'binary\' or \'text\'", "text" },
        {       "arg.fieldspercell",            "Sets the number of fields per mesh cell", "8" },
        {       "arg.verbose",                  "Sets the verbosity of the output", "0" },
	{	NULL,	NULL,	NULL	}
};
static const ElementInfoParam fft3d_params[] = {
	{ "arg.iterations",	"Sets the number of ping pong operations to perform", 	"1"},
	{ "arg.nx", 	    "Sets the size of a block in X", "8" },
    { "arg.ny",         "Sets the size of a block in Y", "8" },
    { "arg.nz",         "Sets the size of a block in Z", "8" },
    { "arg.npRow",      "Sets the number of rows in the PE decomposition", "0" },
    { "arg.nsPerElement",  "", "" },
    { "arg.fwd_fft1",  "", "" },
    { "arg.fwd_fft2",  "", "" },
    { "arg.fwd_fft3",  "", "" },
    { "arg.bwd_fft1",  "", "" },
    { "arg.bwd_fft2",  "", "" },
    { "arg.bwd_fft3",  "", "" },
	{	NULL,	NULL,	NULL	}
};

static const ElementInfoParam trafficGen_params[] = {
	{ "arg.messageSize",	"Sets the size of exchange", 	"1"},
	{ "arg.mean",	"Sets the mean time between exchange", 	"1"},
	{ "arg.stddev",	"Sets the stddev of time between exchange", 	"1"},
	{ "arg.startDelay",	"Sets the stddev of time between exchange", 	"1"},
	{	NULL,	NULL,	NULL	}
};


static const ElementInfoParam allpingpong_params[] = {
    	{	"arg.messageSize",		"Sets the message size of the ping pong operation",	"128"},
	{	"arg.iterations",		"Sets the number of ping pong operations to perform", 	"1024"},
	{	"arg.computetime",		"Sets the time spent computing some values",	 	"1000"},
	{	NULL,	NULL,	NULL	}
};

static const ElementInfoParam barrier_params[] = {
	{	"arg.iterations",		"Sets the number of barrier operations to perform", 	"1024"},
	{	"arg.compute",		"Sets the time spent computing",	 	"1"},
	{	NULL,	NULL,	NULL	}
};

//NetworkSim: stop motif params
static const ElementInfoParam stop_params[] = {
	{	"arg.iterations",		"Sets the number of barrier operations to perform", 	"1024"},
	{	"arg.compute",		"Sets the time spent computing",	 	"1"},
	{	NULL,	NULL,	NULL	}
};
//end->NetworkSim

static const ElementInfoParam alltoall_params[] = {
	{	"arg.iterations",		"Sets the number of alltoall operations to perform", 	"1"},
	{	"arg.bytes",		"Sets the number of bytes per rank",	 	"1"},
	{	"arg.compute",		"Sets the time spent computing",	 	"1"},
	{	NULL,	NULL,	NULL	}
};

//static const ElementInfoParam alltoallv_params[] = {
//	{	"arg.iterations",		"Sets the number of alltoallv operations to perform", 	"1"},
//	{	"arg.bytes",		"Sets the number of bytes per rank",	 	"1"},
//	{	"arg.compute",		"Sets the time spent computing",	 	"1"},
//	{	NULL,	NULL,	NULL	}
//};

static const ElementInfoParam allreduce_params[] = {
	{	"arg.iterations",		"Sets the number of allreduce operations to perform", 	"1"},
	{	"arg.count",		"Sets the number of elements to reduce",	 	"1"},
	{	"arg.compute",		"Sets the time spent computing",	 	"1"},
	{	NULL,	NULL,	NULL	}
};

static const ElementInfoParam reduce_params[] = {
	{	"arg.iterations",		"Sets the number of reduce operations to perform", 	"1"},
	{	"arg.count",		"Sets the number of elements to reduce",	 	"1"},
	{	"arg.compute",		"Sets the time spent computing",	 	"1"},
	{	"arg.root",			"Sets the root of the reduction",		 	"0"},
	{	NULL,	NULL,	NULL	}
};

static const ElementInfoParam bcast_params[] = {
	{	"arg.iterations",		"Sets the number of bcast operations to perform", 	"1"},
	{	"arg.count",		"Sets the number of elements to bcast",	 	"1"},
	{	"arg.compute",		"Sets the time spent computing",	 	"1"},
	{	"arg.root",			"Sets the root of the reduction",		 	"0"},
	{	NULL,	NULL,	NULL	}
};


//static const ElementInfoParam halo1d_params[] = {
//	{	"iterations",		"Sets the number of halo1d operations to perform", 	"10"},
//	{	"computenano",		"Sets the number of nanoseconds to compute for", 	"1000"},
//	{	"messagesize",		"Sets the message size (in bytes)",		 	"128"},
//	{	NULL,	NULL,	NULL	}
//};

static const ElementInfoParam halo2d_params[] = {
	{	"arg.iterations",		"Sets the number of halo2d operations to perform", 	"10"},
	{	"arg.computenano",		"Sets the number of nanoseconds to compute for", 	"10"},
	{	"arg.messagesizex",		"Sets the message size in X-dimension (in bytes)",	"128"},
	{	"arg.messagesizey",		"Sets the message size in Y-dimension (in bytes)",	"128"},
	{	"arg.computecopy",		"Sets the time spent copying data between messages",	"5"},
	{	"arg.sizex",		"Sets the processor decomposition in Y", "0"},
	{	"arg.sizey",		"Sets the processor decomposition in X", "0"},
	{	NULL,	NULL,	NULL	}
};

static const ElementInfoParam halo3d_params[] = {
	{	"arg.iterations",		"Sets the number of halo3d operations to perform", 	"10"},
	{	"arg.computetime",		"Sets the number of nanoseconds to compute for", 	"10"},
	{	"arg.flopspercell",		"Sets the number of number of floating point operations per cell, default is 26 (27 point stencil)", 	"26"},
	{	"arg.peflops",		"Sets the FLOP/s rate of the processor (used to calculate compute time if not supplied, default is 10000000000 FLOP/s)", "10000000000"},
	{	"arg.nx",			"Sets the problem size in X-dimension",			"100"},
	{	"arg.ny",			"Sets the problem size in Y-dimension",			"100"},
	{	"arg.nz",			"Sets the problem size in Z-dimension",			"100"},
	{	"arg.pex",			"Sets the processors in X-dimension (0=auto)",		"0"},
	{	"arg.pey",			"Sets the processors in Y-dimension (0=auto)",		"0"},
	{	"arg.pez",			"Sets the processors in Z-dimension (0=auto)",		"0"},
	{	"arg.copytime",		"Sets the time spent copying data between messages",	"5"},
	{	"arg.doreduce",		"How often to do reduces, 1 = each iteration",		"1"},
	{	"arg.fields_per_cell",	"Specify how many variables are being computed per cell (this is one of the dimensions in message size. Default is 1", "1"},
	{	"arg.field_chunk",	        "Specify how many variables are being computed per cell (this is one of the dimensions in message size. Default is 1", "1"},
	{	"arg.datatype_width",	"Specify the size of a single variable, single grid point, typically 8 for double, 4 for float, default is 8 (double). This scales message size to ensure byte count is correct.", "8"},
	{	NULL,	NULL,	NULL	}
};

static const ElementInfoParam halo3dsv_params[] = {
	{	"arg.iterations",		"Sets the number of ping pong operations to perform", 	"10"},
	{	"arg.computetime",		"Sets the number of nanoseconds to compute for", 	"10"},
	{	"arg.flopspercell",		"Sets the number of number of floating point operations per cell, default is 26 (27 point stencil)", 	"26"},
	{	"arg.peflops",		"Sets the FLOP/s rate of the processor (used to calculate compute time if not supplied, default is 10000000000 FLOP/s)", "10000000000"},
	{	"arg.nx",			"Sets the problem size in X-dimension",			"100"},
	{	"arg.ny",			"Sets the problem size in Y-dimension",			"100"},
	{	"arg.nz",			"Sets the problem size in Z-dimension",			"100"},
	{	"arg.pex",			"Sets the processors in X-dimension (0=auto)",		"0"},
	{	"arg.pey",			"Sets the processors in Y-dimension (0=auto)",		"0"},
	{	"arg.pez",			"Sets the processors in Z-dimension (0=auto)",		"0"},
	{	"arg.copytime",		"Sets the time spent copying data between messages",	"5"},
	{	"arg.doreduce",		"How often to do reduces, 1 = each iteration",		"1"},
	{	"arg.fields_per_cell",	"Specify how many variables are being computed per cell (this is one of the dimensions in message size. Default is 1", "1"},
	{	"arg.field_chunk",	        "Specify how many variables are being computed per field chunk. Default is 1", "1"},
	{	"arg.datatype_width",	"Specify the size of a single variable, single grid point, typically 8 for double, 4 for float, default is 8 (double). This scales message size to ensure byte count is correct.", "8"},
	{	NULL,	NULL,	NULL	}
};

static const ElementInfoParam halo3d26_params[] = {
	{	"arg.iterations",		"Sets the number of ping pong operations to perform", 	"10"},
	{	"arg.computetime",		"Sets the number of nanoseconds to compute for", 	"10"},
	{	"arg.flopspercell",		"Sets the number of number of floating point operations per cell, default is 26 (27 point stencil)", 	"26"},
	{	"arg.peflops",		"Sets the FLOP/s rate of the processor (used to calculate compute time if not supplied, default is 10000000000 FLOP/s)", "10000000000"},
	{	"arg.nx",			"Sets the problem size in X-dimension",			"100"},
	{	"arg.ny",			"Sets the problem size in Y-dimension",			"100"},
	{	"arg.nz",			"Sets the problem size in Z-dimension",			"100"},
	{	"arg.pex",			"Sets the processors in X-dimension (0=auto)",		"0"},
	{	"arg.pey",			"Sets the processors in Y-dimension (0=auto)",		"0"},
	{	"arg.pez",			"Sets the processors in Z-dimension (0=auto)",		"0"},
	{	"arg.copytime",		"Sets the time spent copying data between messages",	"5"},
	{	"arg.doreduce",		"How often to do reduces, 1 = each iteration",		"1"},
	{	"arg.fields_per_cell",	"Specify how many variables are being computed per cell (this is one of the dimensions in message size. Default is 1", "1"},
	{	"arg.datatype_width",	"Specify the size of a single variable, single grid point, typically 8 for double, 4 for float, default is 8 (double). This scales message size to ensure byte count is correct.", "8"},
	{	NULL,	NULL,	NULL	}
};

static const ElementInfoParam ring_params[] = {
	{	"arg.iterations",		"Sets the number of ping pong operations to perform", 	"1"},
	{	"arg.messagesize",		"Sets the size of the message in bytes",	 	"1024"},
	{	NULL,	NULL,	NULL	}
};

static const ElementInfoParam sweep3d_params[] = {
	{	"arg.iterations",		"Sets the number of ping pong operations to perform", 	"1"},
	{	"arg.pex",			"Sets the processor array size in X-dimension, 0 means auto-calculate",	"0"},
	{	"arg.pey",			"Sets the processor array size in Y-dimension, 0 means auto-calculate",	"0"},
	{	"arg.nx",			"Sets the problem size in the X-dimension",	"50"},
	{	"arg.ny",			"Sets the problem size in the Y-dimension",	"50"},
	{	"arg.nz",			"Sets the problem size in the Z-dimension",	"50"},
	{	"arg.kba",			"Sets the KBA (Nz-K blocking factor, default is 1 (no blocking))", "1"},
	{	"arg.computetime",		"Sets the compute time per nx * ny block in picoseconds", "1000"},
	{	"arg.fields_per_cell",	        "Sets the number of fields at each cell point", "8"},
	{	"arg.datatype_width",	        "Sets the width of the datatype used at the cell", "8"},
	{       "arg.nodeflops",                "Sets the FLOP/s count for the MPI rank", "1000000000"},
	{       "arg.flops_per_cell",           "Sets the number of FLOPs per cell", "275" },
	{	NULL,	NULL,	NULL	}
};

static const ElementInfoParam sweep2d_params[] = {
	{	"arg.iterations",		"Sets the number of ping pong operations to perform", 	"1"},
	{	"arg.nx",			"Sets the problem size in the X-dimension",	"50"},
	{	"arg.ny",			"Sets the problem size in the Y-dimension",	"50"},
	{	"arg.y_block",		"Sets the Y-blocking factor (must be Ny % y_block == 0, default is 1 (no blocking))", "1"},
	{	"arg.computetime",		"Sets the compute time per KBA-data block in nanoseconds", "1000"},
	{	NULL,	NULL,	NULL	}
};

static const ElementInfoParam naslu_params[] = {
	{	"arg.iterations",		"Sets the number of ping pong operations to perform", 	"1"},
	{	"arg.pex",			"Sets the processor array size in X-dimension, 0 means auto-calculate",	"0"},
	{	"arg.pey",			"Sets the processor array size in Y-dimension, 0 means auto-calculate",	"0"},
	{	"arg.nx",			"Sets the problem size in the X-dimension",	"50"},
	{	"arg.ny",			"Sets the problem size in the Y-dimension",	"50"},
	{	"arg.nz",			"Sets the problem size in the Z-dimension",	"50"},
	{	"arg.nzblock",		"Sets the Z-dimensional block size (Nz % Nzblock == 0, default is 1)", "1"},
	{	"arg.computetime",		"Sets the compute time per KBA-data block in nanoseconds", "1000"},
	{	NULL,	NULL,	NULL	}
};

static const ElementInfoParam cmt1d_params[] = {
	{	"arg.iterations",	"Sets the number of data exchanges to perform", "1"},
	{	"arg.elementsize",	"Sets the number of gridpoints per element", "10"},
	{   "arg.variables",    "Sets the number of physical quantities for which derivatives are calculated", "1"},
    {   "arg.threads",      "Sets the number of MPI threads per processor", "1"},
	{	"arg.nelt",		"Sets the number of elements per processor",	"1000"},
	{	"arg.processorflops",	"Sets the processor flops for compute time estimation",	"4"},
	{	"arg.processorfreq",	"Sets the processor frequency for compute time estimation", "2.5"},
	{	"arg.nsComputeMean",	"Sets the mean compute time per processor", "1000"},
	{	"arg.nsComputeStddev",	"Sets the stddev in compute time per processor", "50"},
	{	NULL,	NULL,	NULL	}
};

static const ElementInfoParam cmt2d_params[] = {
	{	"arg.iterations",	"Sets the number of data exchanges to perform", "1"},
	{	"arg.elementsize",	"Sets the number of gridpoints per element", "10"},
	{   "arg.variables",    "Sets the number of physical quantities for which derivatives are calculated", "1"},
	{	"arg.px",		    "Sets the size of the processors in the machine in x_dim", "4"},
	{	"arg.py",		    "Sets the size of the processors in the machine in y_dim", "4"},
	{	"arg.threads",          "Sets the number of MPI threads per processor", "1"},
	{	"arg.mx",           "Sets the number of elements per processor in x dim",	"10"},
	{	"arg.my",           "Sets the number of elements per processor in y dim",	"10"},
	{	"arg.mz",           "Sets the number of elements per processor in z dim",	"10"},
	{	"arg.processorflops",	"Sets the processor flops for compute time estimation",	"4"},
	{	"arg.processorfreq",	"Sets the processor frequency for compute time estimation", "2.5"},
	{	"arg.nsComputeMean",	"Sets the mean compute time per processor", "1000"},
	{	"arg.nsComputeStddev",	"Sets the stddev in compute time per processor", "50"},
	{	NULL,	NULL,	NULL	}
};

static const ElementInfoParam cmt3d_params[] = {
	{	"arg.iterations",	"Sets the number of data exchanges to perform", "1"},
	{	"arg.elementsize",	"Sets the number of gridpoints per element", "10"},
	{   "arg.variables",    "Sets the number of physical quantities for which derivatives are calculated", "1"},
	{	"arg.px",		"Sets the size of the processors in the machine in x_dim", "8"},
	{	"arg.py",		"Sets the size of the processors in the machine in y_dim", "8"},
	{	"arg.pz",		"Sets the size of the processors in the machine in z_dim", "8"},
	{	"arg.threads",          "Sets the number of MPI threads per processor", "1"},
	{	"arg.mx",	        "Sets the number of elements per MPI rank in x dim",	"10"},
	{	"arg.my",	        "Sets the number of elements per MPI rank in y dim",	"10"},
	{	"arg.mz",	        "Sets the number of elements per MPI rank in z dim",	"10"},
	{	"arg.processorflops",	"Sets the processor flops for compute time estimation",	"4"},
	{	"arg.processorfreq",	"Sets the processor frequency for compute time estimation", "2.5"},
	{	"arg.nsComputeMean",	"Sets the mean compute time per processor", "1000"},
	{	"arg.nsComputeStddev",	"Sets the stddev in compute time per processor", "50"},
	{	NULL,	NULL,	NULL	}
};

static const ElementInfoParam cmtcr_params[] = {
	{	"arg.iterations",	"Sets the number of data exchanges to perform", "1"},
	{	"arg.elementsize",	"Sets the number of gridpoints per element", "10"},
	{   "arg.variables",    "Sets the number of physical quantities for which derivatives are calculated", "1"},
	{	"arg.mx",           "Sets the number of elements per processor in x dim",	"10"},
	{	"arg.my",           "Sets the number of elements per processor in y dim",	"10"},
	{	"arg.mz",           "Sets the number of elements per processor in z dim",	"10"},
	{	"arg.px",		    "Sets the size of the processors in the machine in x_dim", "4"},
	{	"arg.py",		    "Sets the size of the processors in the machine in y_dim", "4"},
	{	"arg.pz",		    "Sets the size of the processors in the machine in z_dim", "4"},
	{	"arg.threads",          "Sets the number of MPI threads per processor", "1"},
	{	"arg.processorflops",	"Sets the processor flops for compute time estimation",	"4"},
	{	"arg.processorfreq",	"Sets the processor frequency for compute time estimation", "2.5"},
	{	"arg.nsComputeMean",	"Sets the mean compute time per processor", "1000"},
	{	"arg.nsComputeStddev",	"Sets the stddev in compute time per processor", "50"},
	{	NULL,	NULL,	NULL	}
};


static const ElementInfoParam init_params[] = {
	{	NULL,	NULL,	NULL	}
};

static const ElementInfoParam fini_params[] = {
	{	NULL,	NULL,	NULL	}
};

static const ElementInfoParam nullmotif_params[] = {
	{	NULL,	NULL,	NULL	}
};

static const ElementInfoParam msgrate_params[] = {
	{	"arg.iterations",		"Sets the number of ping pong operations to perform", 	"1"},
	{	"arg.msgSize",		"Sets the size of the message in bytes",	 	"0"},
	{	"arg.numMsgs",		"Sets the size of the message in bytes",	 	"128"},
	{	NULL,	NULL,	NULL	}
};

static const ElementInfoParam commdbl_params[] = {
	{	"pex",		"Sets the processors in the X-dimension", "1"	},
	{	"pey",		"Sets the processors in the Y-dimension", "1" 	},
	{	"pez",		"Sets the processors in the Z-dimension", "1"	},
        {       "basephase",    "Starts the phase at offset.", "0" },
	{	"compute_at_step", 	"Sets the computation time in between each communication phase in nanoseconds", "1000" 	},
	{	"items_per_node",	"Sets the number of items to exchange between nodes per phase", "100" },
	{	NULL,	NULL,	NULL	}
};

static const ElementInfoParam comm_params[] = {
	{	"arg.iterations",		"Sets the number of ping pong operations to perform", 	"1"},
	{	"arg.messagesize",		"Sets the size of the message in bytes",	 	"0"},
	{	NULL,	NULL,	NULL	}
};

static const ElementInfoParam constDistrib_params[] = {
	{ 	"constant",		"Sets the constant value to return in the distribution.", "1.0" },
	{	NULL, NULL, NULL 	}
};

static const ElementInfoParam gaussDistrib_params[] = {
	{	"mean",			"Sets the mean value of the Gaussian distribution", "1.0" },
	{	"stddev",		"Sets the standard deviation of the Gaussian distribution", "0.25" },
	{	NULL, NULL, NULL 	}
};

static const ElementInfoParam linearMapper_params[] = {
	{	NULL, NULL, NULL 	}
};

//NetworkSim: added custom mapper params
static const ElementInfoParam customMapper_params[] = {
	{	"mapFile",			"Sets the name of the input file for custom map", "customMap.txt" },
	{	"_mapjobId",		"Sets the jobId for custom map", "-1" },
	{	NULL, NULL, NULL 	}
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
    { 	"PingPongMotif",
	"Performs a Ping-Pong Motif",
	NULL,
	load_PingPong,
	pingpong_params,
	emberMotifTime_statistics,
    	"SST::Ember::EmberGenerator"
    },
    {
	"SIRIUSTraceMotif",
	"Performs a SIRIUS trace-based execution",
	NULL,
	load_SIRIUSTrace,
	siriustrace_params,
	emberMotifTime_statistics,
	"SST::Ember::EmberGenerator"
    },
    {
	"RandomMotif",
	"Performs a random traffic pattern communication",
	NULL,
	load_RandomGen,
	randomgen_params,
	emberMotifTime_statistics,
	"SST::Ember::EmberGenerator"
    },
    { 	"BiPingPongMotif",
	"Performs a InOut Motif",
	NULL,
	load_BiPingPong,
	bipingpong_params,
	emberMotifTime_statistics,
    	"SST::Ember::EmberGenerator"
    },
    {
	"CommDoubleMotif",
	"Performs a communication doubling pattern based on a research scientific analytics problem",
	NULL,
	load_CommDoubling,
	commdbl_params,
	emberMotifTime_statistics,
	"SST::Ember::EmberGenerator"
    },
    {
	"FFT3DMotif",
	"Models an FFT",
	NULL,
	load_FFT3D,
	fft3d_params,
	emberMotifTime_statistics,
	"SST::Ember::EmberGenerator"
    },
    {
	"TrafficGenMotif",
	"Models network traffic",
	NULL,
	load_TrafficGen,
	trafficGen_params,
	emberMotifTime_statistics,
	"SST::Ember::EmberGenerator"
    },
    {
	"3DAMRMotif",
	"Models an adaptive refinement step from MiniAMR",
	NULL,
	load_3DAMR,
	amr3d_params,
	emberMotifTime_statistics,
	"SST::Ember::EmberGenerator"
    },
    { 	"RingMotif",
	"Performs a Ring Motif",
	NULL,
	load_Ring,
	ring_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	"BarrierMotif",
	"Performs a Barrier Motif",
	NULL,
	load_Barrier,
	barrier_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	"StopMotif",
	"NetworkSim: Performs a Barrier Motif and gives a fatal",
	NULL,
	load_Stop,
	stop_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	"AllPingPongMotif",
	"Performs a All Ping Pong Motif",
	NULL,
	load_AllPingPong,
	allpingpong_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	"Halo2DMotif",
	"Performs a 2D halo exchange Motif",
	NULL,
	load_Halo2D,
	halo2d_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	"Halo2DNBRMotif",
	"Performs a 2D halo exchange Motif with non-blocking receives",
	NULL,
	load_Halo2DNBR,
	halo2d_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	"Halo3DMotif",
	"Performs a 3D blocking motif",
	NULL,
	load_Halo3D,
	halo3d_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	"Halo3DSVMotif",
	"Performs a 3D blocking motif with single variable commmunication",
	NULL,
	load_Halo3DSV,
	halo3dsv_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	"Halo3D26Motif",
	"Performs a 3D 26-non-blocking motif",
	NULL,
	load_Halo3D26,
	halo3d26_params,
	emberMotifTime_statistics,
        "SST::Ember::EmberGenerator"
    },
    { 	"Sweep2DMotif",
	"Performs a 2D sweep exchange Motif with multiple vertex communication ordering",
	NULL,
	load_Sweep2D,
	sweep2d_params,
	emberMotifTime_statistics,
        "SST::Ember::EmberGenerator"
    },
    { 	"Sweep3DMotif",
	"Performs a 3D sweep communication motif from all 8 vertices",
	NULL,
	load_Sweep3D,
	sweep3d_params,
	emberMotifTime_statistics,
        "SST::Ember::EmberGenerator"
    },
    { 	"NASLUMotif",
	"Performs a NAS-LU communication motif from 2 (opposite) vertices",
	NULL,
	load_NASLU,
	naslu_params,
	emberMotifTime_statistics,
        "SST::Ember::EmberGenerator"
    },
    { 	"CMT1DMotif",
	"Performs nearest neighbor exchange over a linear/1D decomposition",
	NULL,
	load_CMT1D,
	cmt1d_params,
	emberMotifTime_statistics,
        "SST::Ember::EmberGenerator"
    },
    { 	"CMT2DMotif",
	"Performs nearest neighbor exchange over a 2D mesh decomposition",
	NULL,
	load_CMT2D,
	cmt2d_params,
	emberMotifTime_statistics,
        "SST::Ember::EmberGenerator"
    },
    { 	"CMT3DMotif",
	"Performs nearest neighbor exchange over a 3D machine decomposition",
	NULL,
	load_CMT3D,
	cmt3d_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	"CMTCRMotif",
	"Performs all-to-all communication using Crystal Router",
	NULL,
	load_CMTCR,
	cmtcr_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	"AlltoallMotif",
	"Performs a Alltoall operation with type set to float64 and operation SUM",
	NULL,
	load_Alltoall,
	alltoall_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	"AlltoallvMotif",
	"Performs a Alltoallv operation with type set to float64 and operation SUM",
	NULL,
	load_Alltoallv,
	alltoall_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	"AllreduceMotif",
	"Performs a Allreduce operation with type set to float64 and operation SUM",
	NULL,
	load_Allreduce,
	allreduce_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	"ReduceMotif",
	"Performs a reduction operation with type set to float64 and operation SUM from a user-specified reduction-tree root",
	NULL,
	load_Reduce,
	reduce_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	"BcastMotif",
	"Performs a broadcast operation with type set to float64 from a user-specified reduction-tree root",
	NULL,
	load_Bcast,
	bcast_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	"InitMotif",
	"Performs a communication Initialization Motif",
	NULL,
	load_Init,
	init_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	"FiniMotif",
	"Performs a communication finalize Motif",
	NULL,
	load_Fini,
	fini_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	"NullMotif",
	"Performs an idle on the node, no traffic can be generated.",
	NULL,
	load_Null,
	nullmotif_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	"MsgRateMotif",
	"Performs a message rate test.",
	NULL,
	load_MsgRate,
	msgrate_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    { 	"CommMotif",
	"Performs a comm_split test.",
	NULL,
	load_Comm,
	comm_params,
	emberMotifTime_statistics,
    "SST::Ember::EmberGenerator"
    },
    {   NULL, NULL, NULL, NULL, NULL, NULL, NULL  }
};

static const ElementInfoModule modules[] = {
    {
	"ConstDistrib",
	"Constant compute distribution model",
	NULL,
	NULL,
	load_ConstDistrib,
	constDistrib_params,
	"SST::Ember::EmberComputeDistribution"
    },
    {
	"GaussianDistrib",
	"Gaussian distributed compute noise model",
	NULL,
	NULL,
	load_GaussDistrib,
	gaussDistrib_params,
	"SST::Ember::EmberComputeDistribution"
    },
    {
	"LinearMap",
	"Performs a linear mapping of MPI ranks",
	NULL,
	NULL,
	load_LinearNodeMap,
	linearMapper_params,
	"SST::Ember::EmberRankMap"
    },
    {
	"CustomMap",
	"NetworkSim: Performs a custom mapping of MPI ranks based on an input file",
	NULL,
	NULL,
	load_CustomNodeMap,
	customMapper_params,
	"SST::Ember::EmberRankMap"
    },
    {   NULL, NULL, NULL, NULL, NULL, NULL, NULL  }
};

static const ElementInfoComponent components[] = {
    {
        "EmberEngine",
        "Base communicator motif engine.",
        NULL,
        create_EmberComponent,
        component_params,
        component_ports,
        COMPONENT_CATEGORY_UNCATEGORIZED,
        NULL
    },
    { NULL, NULL, NULL, NULL, NULL, NULL, 0, NULL}
};

extern "C" {
    ElementLibraryInfo ember_eli = {
        "Ember",
        "Message Pattern Generation Library",
        components,
	NULL, 		// Events
	NULL,		// Introspector
	modules,
	subcomponents,
	NULL,
	NULL
    };
}
