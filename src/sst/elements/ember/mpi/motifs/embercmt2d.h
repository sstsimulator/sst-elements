// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_EMBER_CMT_2D
#define _H_EMBER_CMT_2D

#include <sst/core/rng/gaussian.h>
#include "mpi/embermpigen.h"

namespace SST {
namespace Ember {

class EmberCMT2DGenerator : public EmberMessagePassingGenerator {

public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        EmberCMT2DGenerator,
        "ember",
        "CMT2DMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Performs nearest neighbor exchange over a 2D mesh decomposition",
        SST::Ember::EmberGenerator
    )

    SST_ELI_DOCUMENT_PARAMS(
        {   "arg.iterations",   "Sets the number of data exchanges to perform", "1"},
        {   "arg.elementsize",  "Sets the number of gridpoints per element", "10"},
        {   "arg.variables",    "Sets the number of physical quantities for which derivatives are calculated", "1"},
        {   "arg.px",           "Sets the size of the processors in the machine in x_dim", "4"},
        {   "arg.py",           "Sets the size of the processors in the machine in y_dim", "4"},
        {   "arg.threads",          "Sets the number of MPI threads per processor", "1"},
        {   "arg.mx",           "Sets the number of elements per processor in x dim",   "10"},
        {   "arg.my",           "Sets the number of elements per processor in y dim",   "10"},
        {   "arg.mz",           "Sets the number of elements per processor in z dim",   "10"},
        {   "arg.processorflops",   "Sets the processor flops for compute time estimation", "4"},
        {   "arg.processorfreq",    "Sets the processor frequency for compute time estimation", "2.5"},
        {   "arg.nsComputeMean",    "Sets the mean compute time per processor", "1000"},
        {   "arg.nsComputeStddev",  "Sets the stddev in compute time per processor", "50"},
    )

    SST_ELI_DOCUMENT_STATISTICS(
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
    )


public:
	EmberCMT2DGenerator(SST::ComponentId_t, Params& params);
//	~EmberCMT2DGenerator();
	void configure();
	bool generate( std::queue<EmberEvent*>& evQ);

private:
// User parameters - application
	uint32_t iterations;	// total no. of timesteps required
	uint32_t eltSize;		// size of element or stencil in 3-dim (5-20)
    uint32_t variables;     // No. of physical quantities

// User parameters - machine
	int32_t px;				// Machine size (no. of nodes in mesh)
	int32_t py;
	uint32_t threads;

// User parameters - mpi rank
	uint32_t mx;			// local distribution of the elements on a proc
	uint32_t my;
	uint32_t mz;
	uint32_t nelt;			// Total no. of elements per process (100-10,000)

// User parameters - processor
	uint64_t procFlops;		// no. of FLOPS/cycle for the processor
	uint64_t procFreq;		// operating frequency of the processor
	double m_mean;
	double m_stddev;

// Model parameters
	uint32_t m_loopIndex;   // Loop over 'iterations'
    uint32_t m_phyIndex;    // Loop over 'variables'
	SSTGaussianDistribution* m_random;
	int32_t myX;			// Local (x,y,z) coordinates
	int32_t myY;
    uint64_t myID;
	uint64_t x_xferSize;	// Amount of data transferred in x,y direction
	uint64_t y_xferSize;
	int32_t x_pos;			// x & y neighbors
	int32_t x_neg;
	int32_t y_pos;
	int32_t y_neg;
	bool sendx_pos;			// Set to true if a node transfers data in +/-x, +/-y
	bool sendx_neg;
	bool sendy_pos;
	bool sendy_neg;


};

}
}

#endif
