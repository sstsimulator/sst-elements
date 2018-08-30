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


#ifndef _H_EMBER_LQCD
#define _H_EMBER_LQCD

#define EVEN 0x02
#define ODD 0x01
#define EVENANDODD 0x03

#define XUP 0
#define YUP 1
#define ZUP 2
#define TUP 3
#define TDOWN 4
#define ZDOWN 5
#define YDOWN 6
#define XDOWN 7

#define NODIR -1  /* not a direction */

#define OPP_DIR(dir)	(7-(dir))	/* Opposite direction */
#define NDIRS 8				/* number of directions */

/* defines for 3rd nearest neighbor (NAIK) stuff */
#define X3UP 8
#define Y3UP 9
#define Z3UP 10
#define T3UP 11
#define T3DOWN 12
#define Z3DOWN 13
#define Y3DOWN 14
#define X3DOWN 15

#define OPP_3_DIR(dir) (23-(dir))
#define DIR3(dir) ((dir)+8)
#define FORALL3UPDIR(dir) for(dir=X3UP; dir<=T3UP; dir++)

#include "mpi/embermpigen.h"

namespace SST {
namespace Ember {

class EmberLQCDGenerator : public EmberMessagePassingGenerator {

public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        EmberLQCDGenerator,
        "ember",
        "LQCDMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "CG portion of Lattice QCD (MILC) with Naik Gathers.",
        "SST::Ember::EmberGenerator"
    )

    SST_ELI_DOCUMENT_PARAMS(
        { "arg.iterations",	"Sets the number of ping pong operations to perform", 	"1"},
        { "arg.nx", 	    "Sets the size of a lattice site in X", "8" },
        { "arg.ny",         "Sets the size of a lattice site in Y", "8" },
        { "arg.nz",         "Sets the size of a lattice site in Z", "8" },
        { "arg.nt",         "Sets the size of a lattice site in T", "8" },
        { "arg.peflops", "Processor element flops per second", "1000000000000"},
        { "arg.computetime", "", ""},
        {       "arg.verbose",                  "Sets the verbosity of the output", "0" },
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
	EmberLQCDGenerator(SST::Component* owner, Params& params);
	~EmberLQCDGenerator() {}
	void configure();
	bool generate( std::queue<EmberEvent*>& evQ );

private:
    int get_node_index(int x, int y, int z, int t);
    void neighbor_coords(const int coords[], int n_coords[], const int dim, const int offset);
    void lex_coords(int coords[], const int dim, const int size[], const uint32_t rank);
    int coord_parity(int r[]);
    int lex_rank(const int coords[], int dim, int size[]);
    void setup_hyper_prime();
    int node_number_from_lex(int x, int y, int z, int t);
    int get_transfer_size(int dimension);
    int node_number(int x, int y, int z, int t);
	uint32_t m_loopIndex;

	uint32_t iterations;
    // timers to understand comm, comp and overlap
    SimTime_t coll_start;
    SimTime_t coll_time;
    SimTime_t comp_start;
    SimTime_t comp_time;
    SimTime_t gather_pos_start;
    SimTime_t gather_pos_time;
    SimTime_t gather_neg_start;
    SimTime_t gather_neg_time;

	// Share these over all instances of the motif
    uint32_t num_nodes;
    uint32_t sites_on_node;
    uint32_t even_sites_on_node;
    uint32_t odd_sites_on_node;
    uint32_t sizeof_su3matrix;
    uint32_t sizeof_su3vector;

	uint64_t nsCompute;
    uint64_t compute_nseconds_mmvs4d;
    uint64_t compute_nseconds_resid;

	uint32_t nx;
	uint32_t ny;
	uint32_t nz;
    uint32_t nt;

    uint32_t n_ranks[8];
    
    // logical breakup of sites onto compute nodes
    int squaresize[4];	   /* dimensions of hypercubes */
    int nsquares[4];	           /* number of hypercubes in each direction */
    int machine_coordinates[4]; /* logical machine coordinates */ 
};

}
}

#endif
