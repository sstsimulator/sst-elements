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

#include <map>
#include <tuple>
#include <cmath>

namespace SST {
namespace Ember {

struct PolyModelND : public SST::Core::Serialization::serializable
{
    // coeff is a map from powers to coefficients, e.g.
    // [0,2,2]:0.4 means that the coefficient of x0^0 * x1^2 * x2^2 is 0.4
    // We will not worry about making sure all powers are there. We need only
    // evaluate what we find in this list
    std::map<std::vector<int>, double> coeff;
    double scale; // scale the result of eval
    double min; // minimum value returned by the model

    PolyModelND() : coeff(), scale(1), min(0) {}

    PolyModelND(std::map<std::vector<int>, double> coeff, double scale, double min)
        : coeff(coeff)
        , scale(scale)
        , min(min)
    {}

    // Evaluate the model at x
    double eval(std::vector<int> x)
    {
        double val = 0.0;
        for (const auto &co : coeff) {         // loop over our map of powers->coefficients
            double term = co.second;             // value in map is the coefficient
            for (int i = 0; i < x.size(); i++) {       // for each power, evaluate that power and multiple
                term *= pow(x[i], co.first[i]);
            }
            val += term;                        // sum terms
        }
        val *= scale;                           // scale value (used for time conversion)
        val = std::nearbyint(val);              // round so we don't get fractional message sizes
        return val > min ? val : min;           // don't return less than min, useful to remove negatives
    }

    double eval(std::vector<int> x, int verbose)
    {

        std::cout << "[" << x[0] << "," << x[1] << "]" << std::endl;

        double val = 0.0;
        for (const auto &co : coeff) {         // loop over our map of powers->coefficients
            std::cout << "  [" << co.first[0] << "," << co.first[1] << "]: " << co.second << std::endl;
            double term = co.second;             // value in map is the coefficient
            for (int i = 0; i < x.size(); i++) {       // for each power, evaluate that power and multiple
                term *= pow(x[i], co.first[i]);
            }
            val += term;                        // sum terms
        }
        val *= scale;                           // scale value (used for time conversion)
        val = std::nearbyint(val);              // round so we don't get fractional message sizes
        val = val > min ? val : min;           // don't return less than min, useful to remove negatives
        std::cout << "  -> " << val << std::endl;;
        return val;
    }

    bool operator==(const PolyModelND& lhs) const
    {
        return (coeff == lhs.coeff);
    }

    bool operator!=(const PolyModelND& lhs) const
    {
        return (coeff != lhs.coeff);
    }

    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        ser& coeff;
    }

    ImplementSerializable(SST::Ember::PolyModelND);

};

struct PolyModel : public SST::Core::Serialization::serializable
{
    std::vector<double> coeff; // polynomial coefficients
    double scale; // scale the result of eval
    double min; // minimum value returned by the model

    PolyModel() : coeff(0), scale(1), min(0) {}

    PolyModel(std::vector<double> coeff, double scale, double min)
        : coeff(coeff)
        , scale(scale)
        , min(min)
    {}

    // Evaluate the model at x
    // x should be between 0 and 1
    double eval(double x)
    {
        double xt = 1.0;  // holds powers of x
        double res = 0.0; // accumulator
        for (auto c : coeff) {
            res += c * xt;
            xt  *= x;
        }
        res *= scale;
        res = std::nearbyint(res);
        return res > min ? res : min;
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
        {   "arg.msg_model",   "Communication volume model file",  "bfs-comm.model"},
        {   "arg.exec_model",   "Computation timing model file",    "bfs-comp.model"},
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
    bool generate( std::queue<EmberEvent*>& evQ);

private:
    Output out;

    int32_t m_sz; // graph size
    int32_t m_threads; // threads per rank
    int32_t m_nodes;
    int32_t m_ranks_per_node;

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

    // all-gather message sizes for site 31
    int *recvCounts_31;

    // all-to-allV message sizessite 35
    int *sendCounts_35;
    int *recvCounts_35;

    // partner iterators for size 48/49;
    int s_partner_48;
    int r_partner_48;

    std::map<std::tuple<int,int,int>,PolyModel> msg_model;
    std::map<std::tuple<int,int,int,int>,PolyModel> exec_model;

    std::map<std::tuple<int,int>,PolyModelND> msg_model_nd;
    std::map<std::tuple<int,int,int>,PolyModelND> exec_model_nd;

    // Random number generator
    SST::RNG::MersenneRNG* rng;
};

}
}


#endif
