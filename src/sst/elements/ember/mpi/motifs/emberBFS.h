// Copyright 2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2022, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_EMBER_BFS_MOTIF
#define _H_EMBER_BFS_MOTIF

#include "mpi/embermpigen.h"
#include <sst/core/rng/mersenne.h>
#include <sst/core/shared/sharedMap.h>
#include <sst/core/output.h>

namespace SST {
namespace Ember {

struct PolyModel : public SST::Core::Serialization::serializable
{
    std::vector<float> coeff;

    PolyModel() : coeff(0) {}

    PolyModel(std::vector<float> coeff) : coeff(coeff) {}

    // Evaluate the model at x
    // x should be between 0 and 1
    float eval(float x)
    {
        float xt = 1.0;  // holds powers of x
        float res = 0.0; // accumulator
        for (auto c : coeff) {
            res += c * xt;
            xt  *= x;
        }
        return res;
    }

    bool operator==(const PolyModel& lhs) const
    {
        return (coeff == lhs.coeff);
    }

    bool operator!=(const PolyModel& lhs) const
    {
        return (coeff != lhs.coeff);
    }

    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        ser& coeff;
    }

    ImplementSerializable(SST::Ember::PolyModel);

};

class EmberBFSGenerator : public EmberMessagePassingGenerator {

public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        EmberBFSGenerator,
        "ember",
        "BFSMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "motif for the ComBLAS BFS code.",
        SST::Ember::EmberGenerator
    )

    SST_ELI_DOCUMENT_PARAMS(
        {   "arg.sz",           "Sets the graph size",              "14"},
        {   "arg.seed",         "Sets the RNG seed",                "1"},
        {   "arg.comm_model",   "Communication volume model file",  "bfs-comm.model"},
        {   "arg.comp_model",   "Computation timing model file",    "bfs-comp.model"},
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
                                { "time-Commcreate", "Time spent in Commcreate event", "ns", 0}
                                )

public:
    EmberBFSGenerator(SST::ComponentId_t, Params& params);
    ~EmberBFSGenerator();
    void init(unsigned int phase) override;
    bool generate( std::queue<EmberEvent*>& evQ);

private:    
    Output out;

    int32_t m_sz; // graph size

    uint64_t s_time;
    
    uint64_t state; uint64_t prevState;  // for debugging
    uint64_t iter;
    uint64_t maxIter;
    uint64_t idx_39; // state for node 39
    uint64_t idx_49; // state for node 49
    uint64_t idx_50; // state for node 50
    uint64_t square; // sqrt(size())

    uint64_t opposite; // our 'opposite' rank on the other side of the
                     // matrix
    Communicator comm0;  // 'row'
    Communicator comm1;  // comm world
    Communicator comm2;  // 'col'
    MessageResponse m_resp;
    std::vector<int>    comm0_ranks;
    std::vector<int>    comm1_ranks;
    std::vector<int>    comm2_ranks;
    uint32_t comm0_rank; // rank within comm0
    uint32_t comm2_rank; // rank within comm2    

    // since we are not sending data, we use null buffers /
    // displacement maps
    void* nullBuf;
    int* nullDispMap;
    
    // start a new iteration
    void initIter();

    // compute the send/receive data size pattern for site 29
    int sendElem_29;
    int recvElem_29;
    int msgSize_29(double meanSize);
    void computeSR_29(int &sendElem, int &recvElem);

    // compute the all-gather pattern for site 31
    int sendCount_31;
    int *recvCounts_31;
    int msgSize_31(double meanSize, double initRoll);
    void computeAG_31();

    // compute the all-to-allV pattern for site 35
    int *sendCounts_35;
    int *recvCounts_35;
    int msgSize_35(double meanSize, double initRoll);
    void computeAA_35();

    // partner iterators for size 48/49;
    int s_partner_48;
    int r_partner_48;

    // compute the send/receive data size pattern for site 49
    void computeSR_49(int &msgElem);

    // add randomness from uniform dist
    double adjustUniformRand(double input, double range,
                             SST::RNG::MersenneRNG *_rng) {
        double roll = _rng->nextUniform() * range * 2.0;
        return input * (1.0-range+roll);
    }

    std::string comm_filename;
    std::string comp_filename;

    // Communication models
    Shared::SharedMap<int, PolyModel> comm_model;
    // Computation models
    Shared::SharedMap<std::pair<int,int>, PolyModel> comp_model;
    
    // Random number generator    
    SST::RNG::MersenneRNG* rng;
    // 'per rank' rng generator
    SST::RNG::MersenneRNG* rng_rank;
    // 'per opposite pair' rng generator
    SST::RNG::MersenneRNG* rng_opposite;   
    // 'per comm0'
    SST::RNG::MersenneRNG* rng_comm0;
    // 'per comm2'
    SST::RNG::MersenneRNG* rng_comm2;
};

}
}


#endif
