// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_EMBER_CMT_1D
#define _H_EMBER_CMT_1D

#include <sst/core/rng/gaussian.h>
#include "mpi/embermpigen.h"

namespace SST {
namespace Ember {

class EmberCMT1DGenerator : public EmberMessagePassingGenerator {

public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        EmberCMT1DGenerator,
        "ember",
        "CMT1DMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Performs nearest neighbor exchange over a linear/1D decomposition",
        "SST::Ember::EmberGenerator"
    )

    SST_ELI_DOCUMENT_PARAMS(
        {   "arg.iterations",   "Sets the number of data exchanges to perform", "1"},
        {   "arg.elementsize",  "Sets the number of gridpoints per element", "10"},
        {   "arg.variables",    "Sets the number of physical quantities for which derivatives are calculated", "1"},
        {   "arg.threads",      "Sets the number of MPI threads per processor", "1"},
        {   "arg.nelt",     "Sets the number of elements per processor",    "1000"},
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
	EmberCMT1DGenerator(SST::Component* owner, Params& params);
//	~EmberCMT1DGenerator();
    void configure();
	bool generate( std::queue<EmberEvent*>& evQ);

private:

// User parameters - application
	uint32_t iterations;	// total no. of timesteps required
	uint32_t eltSize;		// size of 3d element or stencil (5 - 20)
    uint32_t variables;     // No. of physical quantities	

// User parameters - machine
	uint32_t threads;			
	
// User parameters - mpi rank	
	uint32_t nelt;			// no. of elements per process (100 - 10000)

// User parameters - processor
	uint64_t procFlops;		// no. of FLOPS/cycle for the processor
	uint64_t procFreq;		// operating frequency of the processor							
	double m_mean;
	double m_stddev;

// Model parameters
	uint32_t m_loopIndex;   // Loop over 'iterations'
    uint32_t m_phyIndex;    // Loop over 'variables'
	SSTGaussianDistribution* m_random;
    uint64_t myID;
	uint64_t xferSize;		// Amount of data transferred 

};

}
}

#endif
